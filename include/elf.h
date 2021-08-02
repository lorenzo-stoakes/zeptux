#pragma once

#include "compiler.h"
#include "types.h"

// See https://en.wikipedia.org/wiki/Executable_and_Linkable_Format

// 0x7f followed by 'ELF' in ASCII. Little endian so in reverse.
#define ELF_MAGIC_NUMBER (0x464c457fUL)

struct elf_header {
	uint32_t magic;         // Must = ELF_MAGIC_NUMBER.
	uint8_t  ident_class;   // 1 = 32-bit, 2 = 64-bit.
	uint8_t  ident_data;    // 1 = little endian, 2 = big endian.
	uint8_t  ident_version; // = 1.
	uint8_t  ident_abi;     // Target ABI (often set to 0 by default).
	uint8_t  ident_abi_ver; // ABI 'version'.
	uint8_t  padding[7];
	uint16_t type;          // Executable file type.
	uint16_t machine;       // Target arch.
	uint32_t version;       // = 1.
	uint64_t entry;         // Entry point address.
	uint64_t phoff;         // Offset to program headers (usually right after).
	uint64_t shoff;         // Offset to section header table.
	uint32_t flags;         // Arch-specific flags.
	uint16_t ehsize;        // Size of header (= 64 bytes).
	uint16_t phentsize;     // Size of program header table entry.
	uint16_t phnum;         // Number of entries in program header table.
	uint16_t shentsize;     // Size of section header table entry.
	uint16_t shnum;         // Number of entries in section header table.
	uint16_t shstrndx;      // Index of section header table entry containing
	                        // section names.
} PACKED;
static_assert(sizeof(struct elf_header) == 64);

struct elf_program_header {
	uint32_t type;    // Program header type.
	uint32_t flags;   // Segment-dependent flags.
	uint64_t offset;  // Offset of segment in file image.
	uint64_t vaddr;   // VA of segment in memory.
	uint64_t paddr;   // PA of segment in memory.
	uint64_t filesz;  // Size of segment in file (bytes).
	uint64_t memsz;   // Size of segment in memory (bytes).
	uint64_t align;   // 0, 1 = no alignment. Otherwise is a power of 2.
	                  // vaddr = offset % align.
} PACKED;
static_assert(sizeof(struct elf_program_header) == 56);

struct elf_section_header {
	uint32_t name;      // Offset in .shstrtab to name of section.
	uint32_t type;      // Type of header.
	uint64_t flags;     // Section attributes.
	uint64_t addr;      // VA of section in memory (when loaded).
	uint64_t offset;    // Offset of section in file (bytes).
	uint64_t size;      // Size of section in file (bytes).
	uint32_t link;      // Section index of an associated section.
	uint32_t info;      // Extra information about section.
	uint64_t addralign; // Required alignment of section. Must be power of 2.
	uint64_t entsize;   // Size of each entry (bytes) if contains fixed size
	                    // entries, otherwise 0.
} PACKED;
static_assert(sizeof(struct elf_section_header) == 64);
