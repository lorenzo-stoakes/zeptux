# Design concepts

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
