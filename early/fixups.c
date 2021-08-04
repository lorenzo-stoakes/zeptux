#include "zeptux_early.h"

// Lookup entries in the kernel elf image.
#define ELF_LOOKUP(_index) (void *)((uint8_t *)KERNEL_ELF_ADDRESS + (_index))

// Because we don't load the kernel ELF in a conventional way the .bss section
// is NOT zeroed. This obviously breaks things so we need to do this manually
// ourselves.
void fixup_bss(void)
{
	struct elf_header *header = ELF_LOOKUP(0);
	if (header->magic != ELF_MAGIC_NUMBER) {
		panic("no ELF magic?");
	}

	struct elf_section_header *section_headers = ELF_LOOKUP(header->shoff);
	struct elf_section_header *shstr_header = &section_headers[header->shstrndx];
	const char *shstr = ELF_LOOKUP(shstr_header->offset);

	struct elf_section_header *bss_header = NULL;
	for (uint16_t i = 0; i < header->shnum; i++) {
		struct elf_section_header *section_header = &section_headers[i];
		const char* name = &shstr[section_header->name];

		if (strcmp(name, ".bss") == 0) {
			bss_header = section_header;
			break;
		}
	}

	if (bss_header == NULL)
		panic("cannot find .bss section");

	memset((void *)bss_header->addr, 0, bss_header->size);
}
