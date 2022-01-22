# Zeptux

__NOTE:__ Zeptux is on hold for the time being as I am busy improving my
algorithm skills and playing with __memtux__. I plan to return to it at a later
date.

Zeptux is an in-development, opinionated, UNIX-like kernel which may or may not
target POSIX conformity.

It is, rather unfashionably, implemented in C and assembly.

It is released under an MIT license.

## Roadmap

### Next steps

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

## Usage

Make sure you have __gcc__, __binutils__, __python 3__, __go__ and __qemu__ installed.

### Building

```
$ ./build
```

### Emulation

To run under qemu:

```
$ ./build qemu
```

To run under qemu using a graphical output:

```
$ ./build qemu-vga
```

### Tests

To execute all tests, run:

```
$ ./build test
```

This will generate a test kernel image which will then be executed under qemu.

## Docs

* [Design concepts](docs/concepts.md) - a rough draft document on zeptux design concepts.

* [Memory map](docs/memmap.md) - a description of the physical and virtual
  memory maps within zeptux.
