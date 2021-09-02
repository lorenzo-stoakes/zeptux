# Zeptux

Zeptux is an in-development, opinionated, UNIX-like kernel which may or may not
target POSIX conformity.

It is, rather unfashionably, implemented in C and assembly.

It is released under an MIT license.

## Roadmap

### Next steps

* Add initial physical memory allocator.
* Move boot sector allocated physical memory into physical allocator.
* Map boot sector allocated memory into VA correctly (including the kernel ELF),
  correcting ELF page flags (e.g. ro for .rodata).
* Implement early slab allocator.
* Implement interrupt handler and enable interrupts.
* Page fault handling.
* Implement interrupt-driven serial port interface for early kernel I/O.
* Implement kernel ring buffer (outputting to serial interface to start with).
* Implement virtual file system support.
* Implement correct, possibly trivial disk I/O driver.
* Implement processes both userland and kernel.
* Implement scheduler.
* Load init userland process.
* Add basic system calls.
* Implement trivial shell.

### Usage

Make sure you have gcc, binutils, python 3, go and qemu installed. Then run:

```
$ ./build
```

#### Tests

To execute all tests, run:

```
$ ./build test
```

This will generate a test kernel image which will then be executed under qemu.

## Docs

* [Design concepts](docs/concepts.md) - a rough draft document on zeptux design concepts.

* [Memory map](docs/memmap.md) - a description of the physical and virtual
  memory maps within zeptux.
