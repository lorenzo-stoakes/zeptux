#include "asm.h"
#include "bootsector.h"
#include "config.h"
#include "early_mem.h"
#include "elf.h"
#include "macros.h"
#include "mm.h"
#include "string.h"

// Helpers for iterating through ELF program and section headers.

#define for_each_prog_header(_buf, _header, _prog_header)           \
	struct elf_program_header *_prog_header =                   \
		(struct elf_program_header *)&_buf[_header->phoff]; \
	for (int i = 0; i < _header->phnum; i++, _prog_header++)

#define for_each_sect_header(_buf, _header, _sect_header)           \
	struct elf_section_header *_sect_header =                   \
		(struct elf_section_header *)&_buf[_header->shoff]; \
	for (int i = 0; i < _header->shnum; i++, _sect_header++)

// Zero the .bss section.
static bool elf_zero_bss(struct elf_header *header)
{
	uint8_t *buf = (uint8_t *)KERNEL_ELF_ADDRESS;

	// Locate the shstr - the section header string buffer so we can lookup
	// section names.
	struct elf_section_header *section_headers =
		(struct elf_section_header *)&buf[header->shoff];
	struct elf_section_header *shstr_sect_header =
		&section_headers[header->shstrndx];
	const char *shstr = (const char *)&buf[shstr_sect_header->offset];

	// We only keep the .text section for this loader so we put this name on
	// the stack.
	const char bss_name[] = ".bss";

	// Find the .bss section.
	bool found = false;
	for_each_sect_header (buf, header, sect_header) {
		const char *name = &shstr[sect_header->name];

		if (strcmp(name, bss_name) != 0)
			continue;

		// Once found, clear it!
		found = true;
		memset((void *)sect_header->addr, 0, sect_header->size);
		break;
	}

	return found;
}

// Check that virtual addresses referenced by ELF elements match where they have
// actually been loaded in memory.
static bool elf_check_addrs(struct elf_header *header)
{
	uint8_t *buf = (uint8_t *)KERNEL_ELF_ADDRESS;

	// Check program header addresses.
	for_each_prog_header (buf, header, prog_header) {
		// Unloaded programs (e.g. .bss) do not need to be checked.
		if (prog_header->filesz == 0)
			continue;

		uint64_t addr = prog_header->vaddr;
		if (addr != 0 && prog_header->offset != addr - KERNEL_ELF_ADDRESS)
			return false;
	}

	// Check section header addresses.
	for_each_sect_header (buf, header, sect_header) {
		if (sect_header->type != ELF_SHT_PROGBITS)
			continue;

		uint64_t addr = sect_header->addr;
		if (addr != 0 && sect_header->offset != addr - KERNEL_ELF_ADDRESS)
			return false;
	}

	return true;
}

#ifdef CONFIG_BOOTLOADER_ATA_PIO

// Represents current ELF image load state while loading via ATA PIO.
struct elf_load_state {
	struct elf_header *header;
	uint64_t bytes_loaded, sectors_loaded;
};

// This file performs early kernel ELF file loading by loading the image at
// KERNEL_ELF_ADDRESS, performing some checks and initialising .bss, before
// finally jumping to the kernel entrypoint. This leaves readonly sections
// mapped as read/write which we leave to the kernel proper to resolve once
// memory management is in a saner place.

// Note that we are trying to keep a 1-to-1 match between the on-disk ELF file
// and the in-memory ELF data (other than .bss which needs separate special
// attention).

// Use ATA PIO commands to perform early disk reading.
// See https://wiki.osdev.org/ATA_PIO_Mode#48_bit_PIO

static void wait_disk_ready(void)
{
	while ((inb(0x1f7) & 0xc0) != 0x40)
		;
}

// Use 48-bit PIO to read sectors into memory. We assume that we have this
// available as the standard originated in 2003 (!)
//
//   offset: number of sectors to offset by on disk.
//    count: number of sectors to read.
static void read_sectors_lba48(uint8_t *ptr, uint64_t offset, uint16_t count)
{
	wait_disk_ready();

	outb(0x1f6, 0x40); // Set master disk.

	outb(0x1f2, count >> 8);   // Sector count high byte.
	outb(0x1f3, offset >> 24); // Offset high bytes.
	outb(0x1f4, offset >> 32);
	outb(0x1f5, offset >> 40);

	outb(0x1f2, count);  // Sector count low byte.
	outb(0x1f3, offset); // Offset low bytes.
	outb(0x1f4, offset >> 8);
	outb(0x1f5, offset >> 16);

	outb(0x1f7, 0x24); // READ SECTORS EXT

	wait_disk_ready();
	for (uint16_t i = 0; i < count; i++) {
		// Read 1 sector. This command reads 4 bytes at a time so we
		// read 1/4 sector size times.
		insl(ptr, 0x1f0, SECTOR_SIZE_BYTES / 4);
		wait_disk_ready();
		ptr += SECTOR_SIZE_BYTES;
	}
}

// Load ELF file header and all sectors up to and including sections headers.
static struct elf_load_state load1(uint8_t *buf)
{
	struct elf_load_state ret = {0};

	// Read the first sector of the data immediately after the boot
	// data. This should contain the ELF header.
	read_sectors_lba48(buf, BOOT_SECTORS, 1);

	struct elf_header *header = (struct elf_header *)buf;
	if (header->magic != ELF_MAGIC_NUMBER)
		return ret;

	// We are assuming program headers come after section headers. If we're
	// in some bizarro world where that's not the case, just quit out.
	if (header->phoff > header->shoff)
		return ret;

	// Calculate the number of sectors we need to load in order to also load
	// the section headers.
	uint32_t section_header_end_bytes =
		header->shoff + header->shnum * sizeof(struct elf_section_header);
	section_header_end_bytes =
		ALIGN_UP(section_header_end_bytes, SECTOR_SIZE_BYTES);
	uint16_t sectors = section_header_end_bytes / SECTOR_SIZE_BYTES;

	// Read these sectors. We are now in a state of having loaded the ELF
	// file up to and including all section table entries (and possibly more
	// as we aligned up to sector size).
	read_sectors_lba48(&buf[SECTOR_SIZE_BYTES], BOOT_SECTORS + 1,
			   sectors - 1);

	ret.header = header;
	ret.bytes_loaded = section_header_end_bytes;
	ret.sectors_loaded = sectors;

	return ret;
}

// Determine the number of bytes contained within the ELF file (for some reason
// this is not encoded...!) - this assumes that we have the header, program
// headers and section headers loaded in memory.
static uint64_t elf_get_size_bytes(uint8_t *buf, struct elf_header *header)
{
	uint64_t ret = 0;

	// First, determine the highest offset amongst programs.
	for_each_prog_header (buf, header, prog_header) {
		uint64_t end = prog_header->offset + prog_header->memsz;
		if (end > ret)
			ret = end;
	}

	// Next, determine the highest offset amongst sections.
	for_each_sect_header (buf, header, sect_header) {
		// Ignore sections that don't require data loaded.
		if (sect_header->type == ELF_SHT_NOBITS)
			continue;

		uint64_t end = sect_header->offset + sect_header->size;
		if (end > ret)
			ret = end;
	}

	return ret;
}

// Perform some additional tasks - most likely we have loaded everything we
// need, however we have to make sure that memory locations match expected VA
// locations
static bool load2(uint8_t *buf, struct elf_load_state *state)
{
	struct elf_header *header = state->header;

	// Step 1: Determine whether we need to load any more sectors, if so -
	// load them!
	uint64_t bytes = elf_get_size_bytes(buf, header);
	if (bytes > state->bytes_loaded) {
		uint64_t old_bytes_aligned =
			state->sectors_loaded * SECTOR_SIZE_BYTES;
		uint64_t bytes_aligned = ALIGN_UP(bytes, SECTOR_SIZE_BYTES);
		uint16_t sectors = bytes_aligned / SECTOR_SIZE_BYTES;
		uint16_t delta = sectors - state->sectors_loaded;

		read_sectors_lba48(&buf[old_bytes_aligned], state->sectors_loaded,
				   delta);
	}

	return true;
}

// Load the kernel from disk using direct ATA PIO calls and polling. Not all
// hardware supports, though qemu does. We assume LBA 48 mode is supported.
static struct elf_header *load_ata_pio(void)
{
	// We have mapped KERNEL_ELF_ADDRESS at 0x1000000 PA (16 MiB offset)
	// which is a safely addressable area of extended (assuming the system
	// has more than 16 MiB available!) See docs/memmap.md for more details
	// on x86-64 physical memory layout.
	uint8_t *buf = (uint8_t *)KERNEL_ELF_ADDRESS;

	// Load ELF header and all sectors up to and including section header
	// table.
	struct elf_load_state state = load1(buf);
	if (state.header == NULL)
		return NULL;

	// Load anything not already present, check things are where we expect
	// in memory, and initialise .bss section.
	if (!load2(buf, &state))
		return NULL;

	early_get_boot_info()->kernel_elf_size_bytes = state.bytes_loaded;

	return state.header;
}
#endif // CONFIG_BOOTLOADER_ATA_PIO

#ifdef CONFIG_BOOTLOADER_BIOS
// Very simply copy the entire kernel loaded into conventional memory for us by
// the bootloader to its expected place in memory. As with an ATA-loaded kernel
// we expect that everything will work with the file image simply copied into
// place (modulo the steps in check_and_finalise()). This may need revisiting.
static struct elf_header *copy_kernel_elf_image(void)
{
	memcpy((void *)KERNEL_ELF_ADDRESS,
	       (void *)BIOS_KERNEL_ELF_LOAD_PHYS_ADDRESS,
	       early_get_boot_info()->kernel_elf_size_bytes);

	return (struct elf_header *)KERNEL_ELF_ADDRESS;
}
#endif

// Perform final integrity checks on kernel ELF image and perform any tasks not
// achieved by loading the image.
static bool check_and_finalise(struct elf_header *header)
{
	// Check that any VAs specified within the ELF headers match the
	// actually loaded locations.
	if (!elf_check_addrs(header))
		return false;

	// Zero the .bss section of the ELF image.
	return elf_zero_bss(header);
}

// Load the kernel ELF image into memory at KERNEL_ELF_ADDRESS.
static struct elf_header *load_elf_image(void)
{
	struct elf_header *header;

#ifdef CONFIG_BOOTLOADER_ATA_PIO
	header = load_ata_pio();
#elif defined(CONFIG_BOOTLOADER_BIOS)
	header = copy_kernel_elf_image();
#else
#error misconfigured
#endif

	return header;
}

// Execute the kernel. This should not return.
static void exec(struct elf_header *header)
{
	// Directly call into the kernel.
	void (*entry)(void) = (void (*)(void))(header->entry);
	entry();
}

void load(void)
{
	struct elf_header *header;

	header = load_elf_image();
	if (!check_and_finalise(header))
		return; // System halt.
	exec(header);
}
