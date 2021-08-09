# Zeptux

Zeptux is an in-development, opinionated, UNIX-like kernel which may or may not
target POSIX conformity.

It is, rather unfashionably, implemented in C and assembly.

It is released under an MIT license.

## Status
_"Almost non-existent."_

- Self-loading 2-stage boot sector (via ATA).
- Early PIO serial port output support.
- Fairly functional [v]snprintf.
- Reads e820 records.
- Early kernel unit test framework.

## Roadmap

### Next steps

* Add initial physical memory allocator.
* Drop direct 1 GiB mapping e.g. from VA 0 - 1 GiB -> 0 1 GiB PA.
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

Make sure you have gcc, binutils and qemu installed. Then run:

```
$ make qemu
```

#### Tests

To execute tests, run:

```
$ make test
```

This will generate a test kernel image which will then be executed under qemu.

## Design concepts

As the kernel is entirely incomplete at this stage nearly all aspects of the
design are up for grabs. I have several ideas, chaotically described below:

* Monolithic kernel, probably - Linux has shown this to be a sane and efficient
  kernel design paradigm.
* JIT eBPF-type kernel-side code - perhaps implementing large parts of the
  kernel in something like this.
* Kernel-bypass as a default paradigm where it makes sense - highly efficient
  io_uring-like async I/O, networking, etc.
* Deterministic memory allocation - avoid overcommit, or provide at least a more
  controlled version of it to avoid the complexities and indeterminate nature of
  overcommit.
* Heavy thought put into memory fragmentation, reclaim and memory pressure
  mechanisms.
* Very significant levels of memory usage observability. Something similar to
  amazon's DAMON efforts in the kernel.
* Generally providing extreme levels of observability, like dtrace/eBPF/etc.
* cgroup/namespace support but very fine-grained and heavily built in to design
  patterns from the beginning.
* Some means of help with highly concurrent processes, an interface that
  generally encourages concurrency rather than making it difficult.
* 'Foreign' processes - Ability to run processes using a different kernel,
  _somehow_.
* More built-in userland stuff, more 'batteries included' - specifically highly
  efficient (perhaps shared memory implemented) - specifically - standardised
  IPC, standardised systemd-like init.
* Heavy testing from the off - Unit tests where feasible, qemu-implemented
  end-to-end tests, fuzzing, stress tests. Ideally having an infrastructure to
  make testing kernel work straightforward.
