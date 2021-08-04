#include "elf.h"
#include "io_asm.h"
#include "macros.h"
#include "mem.h"

#define SECTOR_SIZE_BYTES (512)

// We are struggling for code size to fit into the bootsector (512 bytes!) so
// we are having to be succinct here.

// Use ATA PIO commands to perform early disk reading.
// See https://wiki.osdev.org/ATA_PIO_Mode#48_bit_PIO

static void ata_pio_wait_disk_ready(void)
{
	while ((inb(0x1f7) & 0xc0) != 0x40)
		;
}

// Use 48-bit PIO to read sectors into memory.
static void ata_pio_read_sectors(void *ptr, uint64_t offset, uint16_t count)
{
	ata_pio_wait_disk_ready();

	// Set MASTER.
	outb(0x1f6, 0x40);

	// Sector count high byte.
	outb(0x1f2, count >> 8);
	// Offset high bytes.
	outb(0x1f3, offset >> 24);
	outb(0x1f4, offset >> 32);
	outb(0x1f5, offset >> 40);

	// Sector count low byte.
	outb(0x1f2, count);
	// Offset low bytes.
	outb(0x1f3, offset);
	outb(0x1f4, offset >> 8);
	outb(0x1f5, offset >> 16);

	outb(0x1f7, 0x24); // READ SECTORS EXT

	ata_pio_wait_disk_ready();
	for (uint16_t i = 0; i < count; i++) {
		// Read 1 sector at a time.
		insl(ptr, 0x1f0, SECTOR_SIZE_BYTES / 4);
		ata_pio_wait_disk_ready();
		ptr += SECTOR_SIZE_BYTES;
	}
}

void load(void)
{
	// In order to save on generated code size we use initial direct mapping
	// from 0 - 1 GiB rather than the ultimately mapping kernel address.
	uint8_t *buf = (uint8_t *)(KERNEL_ELF_ADDRESS & ((1UL << 32) - 1));

	// Read ELF header.
	ata_pio_read_sectors(buf, 1, 1);
	struct elf_header* header = (struct elf_header *)buf;

	// ASSUME: Section header is located at end of ELF file.
	uint32_t size = ALIGN_UP(header->shoff, SECTOR_SIZE_BYTES);
	// ASSUME: We avoid needing to read the number of sections and simply
	// assume we won't have more than 24.
	ata_pio_read_sectors(&buf[SECTOR_SIZE_BYTES], 2, size + 2);

	uint16_t count = size / SECTOR_SIZE_BYTES;
	ata_pio_read_sectors(&buf[SECTOR_SIZE_BYTES], 2, count);

	// Directly load the kernel.
	void (*entry)(void) = (void(*)(void))(header->entry);
	entry();
}
