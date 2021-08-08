#pragma once

#include "compiler.h"
#include "types.h"

// See https://en.wikipedia.org/wiki/Executable_and_Linkable_Format

// 0x7f followed by 'ELF' in ASCII. Little endian so in reverse.
#define ELF_MAGIC_NUMBER (0x464c457fUL)

enum elf_pt_type {
	ELF_PT_NULL = 0,
	ELF_PT_LOAD = 1,
	ELF_PT_DYMAMIC = 2,
	ELF_PT_INTERP = 3,
	ELF_PT_NOTE = 4,
	ELF_PT_SHLIB = 5,
	ELF_PT_PHDR = 6,
	ELF_PT_TLS = 7,
};

enum elf_sh_type {
	ELF_SHT_NULL = 0,
	ELF_SHT_PROGBITS = 1,
	ELF_SHT_SYMTAB = 2,
	ELF_SHT_STRTAB = 3,
	ELF_SHT_RELA = 4,
	ELF_SHT_HASH = 5,
	ELF_SHT_DYNAMIC = 6,
	ELF_SHT_NOTE = 7,
	ELF_SHT_NOBITS = 8,
	ELF_SHT_REL = 9,
	ELF_SHT_SHLIB = 10,
	ELF_SHT_DYNSYM = 11,
	ELF_SHT_INIT_ARRAY = 14,
	ELF_SHT_FINI_ARRAY = 15,
	ELF_SHT_PREINIT_ARRAY = 16,
	ELF_SHT_GROUP = 17,
	ELF_SHT_SYMTAB_SHNDX = 18,
	ELF_SHT_NUM = 19,
};

struct elf_header {
	uint32_t magic;	       // Must = ELF_MAGIC_NUMBER.
	uint8_t ident_class;   // 1 = 32-bit, 2 = 64-bit.
	uint8_t ident_data;    // 1 = little endian, 2 = big endian.
	uint8_t ident_version; // = 1.
	uint8_t ident_abi;     // Target ABI (often set to 0 by default).
	uint8_t ident_abi_ver; // ABI 'version'.
	uint8_t padding[7];
	uint16_t type;	    // Executable file type.
	uint16_t machine;   // Target arch.
	uint32_t version;   // = 1.
	uint64_t entry;	    // Entry point address.
	uint64_t phoff;	    // Offset to program headers (usually right after).
	uint64_t shoff;	    // Offset to section header table.
	uint32_t flags;	    // Arch-specific flags.
	uint16_t ehsize;    // Size of header (= 64 bytes).
	uint16_t phentsize; // Size of program header table entry.
	uint16_t phnum;	    // Number of entries in program header table.
	uint16_t shentsize; // Size of section header table entry.
	uint16_t shnum;	    // Number of entries in section header table.
	uint16_t shstrndx;  // Index of section header table entry containing
			    // section names.
} PACKED;
static_assert(sizeof(struct elf_header) == 64);

struct elf_program_header {
	uint32_t type;	 // Program header type.
	uint32_t flags;	 // Segment-dependent flags.
	uint64_t offset; // Offset of segment in file image.
	uint64_t vaddr;	 // VA of segment in memory.
	uint64_t paddr;	 // PA of segment in memory.
	uint64_t filesz; // Size of segment in file (bytes).
	uint64_t memsz;	 // Size of segment in memory (bytes).
	uint64_t align;	 // 0, 1 = no alignment. Otherwise is a power of 2.
			 // vaddr = offset % align.
} PACKED;
static_assert(sizeof(struct elf_program_header) == 56);

struct elf_section_header {
	uint32_t name;	    // Offset in .shstrtab to name of section.
	uint32_t type;	    // Type of header.
	uint64_t flags;	    // Section attributes.
	uint64_t addr;	    // VA of section in memory (when loaded).
	uint64_t offset;    // Offset of section in file (bytes).
	uint64_t size;	    // Size of section in file (bytes).
	uint32_t link;	    // Section index of an associated section.
	uint32_t info;	    // Extra information about section.
	uint64_t addralign; // Required alignment of section. Must be power of 2.
	uint64_t entsize;   // Size of each entry (bytes) if contains fixed size
			    // entries, otherwise 0.
} PACKED;
static_assert(sizeof(struct elf_section_header) == 64);
