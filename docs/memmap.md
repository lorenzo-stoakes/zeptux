# Zeptux memory map layout

This describes both the physical and virtual memory layout used by
zeptux. Initially only x86-64 is supported so everything here assumes this
architecture.

## PHYSICAL memory map

Physical memory is divided into ranges of usable and unusable memory _roughly_
as follows (though E820 is the final adjudicator):

```
                                                                       Usable?
                                       0 -> |--------------| ^
                                            |   Real mode  | | 1 KiB
                                            |      IVT*    | |
                                     400 -> |--------------| x
                                            |   BIOS Data  | | 256 bytes
                                            |   Area (BDA) | |
                                     500 -> |--------------| x            *
                                            | conventional | | ~30 KiB    *
                                            |    memory    | |            *
                                    7c00 -> |--------------| x            *
                                            |     boot     | | 512 bytes  *
                                            |    sector    | |            *
                                    7e00 -> |--------------| x            *
                                            | conventional | | ~480 KiB   *
                                            |    memory    | |            *
                                   80000 -> |--------------| x            *
                                            |  Ext. BIOS   | | 128 KiB
                                            |  Data Area   | |
                                   a0000 -> |--------------| x
                                            |  Video disp. | | 128 KiB
                                            |    memory    | |
                                   c0000 -> |--------------| x
                                            |    Video     | | 32 KiB
                                            |     BIOS     | |
                                   c8000 -> |--------------| x
                                            |     BIOS     | | 160 KiB
                                            |  Expansions  | |
                                   f0000 -> |--------------| x
                                            | Motherboard  | | 64 KiB
                                            |     BIOS     | |
                                  100000 -> |--------------| x            *
                                            |   Extended   | | 14 MiB     *
                                            |    memory    | |            *
                                  f00000 -> |--------------| x            *
                                            |  ISA Memory  | |  1 MiB
                                            |     Hole     | |
                                 1000000 -> |--------------| x            *
                                            |   Extended   | |  Up to     *
                                            |    memory    | |  3 GiB     *
                      [ depends on RAM ] -> |--------------| v            *
                                            /    maybe     /
                                            \    unused    \
                                            /     hole     /
                                c0000000 -> |--------------| ^
                                            |    Memory    | | 1 GiB
                                            |  mapped dev  | |
                               100000000 -> |--------------| x            *
                                            |   Extended   | | depends    *
                                            |    memory    | | on RAM     *
                      [ depends on RAM ] -> |--------------| v            *


* IVT = Interrupt Vector Table
```

ref: https://wiki.osdev.org/Memory_Map

## VIRTUAL memory map

```
Userland (128 TiB)
                         0000000000000000 -> |---------------| ^
                                             |    Process    | |
                                             |    address    | | 128 TiB
                                             |     space     | |
                         0000800000000000 -> |---------------| v
                     .        ` .     -                 `-       ./   _
                              _    .`   -   The netherworld of  `/   `
                    -     `  _        |  /      unavailable sign-extended -/ .
                     ` -        .   `  48-bit address space  -     \  /    -
                   \-                - . . . .             \      /       -
Kernel (128 TiB)
KERNEL_DIRECT_MAP_BASE = ffff800000000000 -> |----------------| ^
                                             | Direct mapping | |
                                             |  of all phys.  | | 64 TiB
                                             |     memory     | |
    KERNEL_ELF_ADDRESS = ffffc00000000000 -> |----------------| v
                                             |     Kernel     | |
                                             |      text      | | 1 GiB
                                             |     mapping    | |
KERNEL_MEM_MAP_ADDRESS = ffffc00040000000 -> |----------------| x
                                             |    Array of    | |
                                             |    physblock   | | 1 TiB
                                             |     entries    | |
     APIC_BASE_ADDRESS = ffffc10040000000 -> |----------------| x
                                             |    Array of    | |
                                             |    physblock   | | 4 KiB
                                             |     entries    | |
                         ffffc10040001000 -> |----------------| v
                                             /                /
                                             \     unused     \
                                             /      hole      /
                                             \                \
KERNEL_VMALLOC_ADDRESS = ffffd00000000000 -> ------------------ ^
                                             |     vmalloc    | |
                                             |     mapping    | | 32 TiB
                                             |      space     | |
                         fffff00000000000 -> ------------------ v
                                             /                /
                                             \     unused     \
                                             /      hole      /
                                             \                \
                                             ------------------
```


### Early boot loader

* The boot sector (stage 1) and early stage 2 boot loader are loaded in the boot
  sector/conventional memory range [0x7c00, 0x8600).
* Early PGD and PUD (with the 1 GiB 'gigantic page' bit set) page tables are
  loaded in conventional memory in the range [0x1000, 0x5000).
* Initial kernel stack is set to the top of conventional memory at 0x80000.
* The kernel ELF image is loaded at the beginning of the post-ISA memory hole
  extended memory at 0x1000000.
* Early boot information (including kernel ELF size, e820 entries) is stored at
  0x6000 in lower conventional memory.
