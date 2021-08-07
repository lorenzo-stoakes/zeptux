## Physical memory map

### x86-64

Physical memory is divided into ranges of usable and unusable memory as follows:

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
