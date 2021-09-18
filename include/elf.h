#pragma once

#include "compiler.h"
#include "macros.h"
#include "types.h"

// See https://en.wikipedia.org/wiki/Executable_and_Linkable_Format

// 0x7f followed by 'ELF' in ASCII. Little endian so in reverse.
#define ELF_MAGIC_NUMBER (0x464c457fUL)

// Represents the 'program' type.
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

// Represents the section header type.
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

// Represents a section header's flags.
enum elf_sh_flags {
	ELF_SHF_WRITE = BIT_MASK(0),
	ELF_SHF_ALLOC = BIT_MASK(1),
	ELF_SHF_EXECINSTR = BIT_MASK(2),
	ELF_SHF_MERGE = BIT_MASK(4),
	ELF_SHF_STRINGS = BIT_MASK(5),
	ELF_SHF_INFO_LINK = BIT_MASK(6),
	ELF_SHF_LINK_ORDER = BIT_MASK(7),
	ELF_SHF_OS_NONCONFORMING = BIT_MASK(8),
	ELF_SHF_GROUP = BIT_MASK(9),
	ELF_SHF_TLS = BIT_MASK(10),
	ELF_SHF_MASKOS = 0xff00000UL,
	ELF_SHF_MASKPROC = 0xf0000000UL,

};

// Directly maps to initial ELF header.
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

// Directly maps to ELF 'program' headers.
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

// Directly maps to ELF section headers.
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
