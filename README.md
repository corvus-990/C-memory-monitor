# C-memory-monitor

A lightweight memory leak detector for Linux, written in C. It is loaded into a program with `LD_PRELOAD`, traces every `malloc`, `calloc`, `realloc` and `free` call as it happens, and reports the blocks that were never freed when the program exits.

The target program does not need to be recompiled or relinked.

## Features

- Live trace of every allocation and deallocation, with address and size
- Leak report at exit, listing each unfreed block and the total bytes leaked
- Report is still printed when the program exits through `_exit()`, `_Exit()` or a raw `exit` / `exit_group` syscall
- Optional `free` mode that releases the leaked blocks before the program ends
- A single C file with no dependencies beyond glibc

## Requirements

- Linux with glibc
- GCC or Clang

## Build

```sh
gcc -fPIC -shared -o mem_leak.so memory_monitor.c
```

On glibc older than 2.34, add `-ldl` to the end of the command.

## Usage

Preload the library when starting the program you want to check:

```sh
LD_PRELOAD=./mem_leak.so ./your_program
```

To also free the leaked blocks at exit, pass `free` as the only argument:

```sh
LD_PRELOAD=./mem_leak.so ./your_program free
```

All output is written to stderr, so it can be kept apart from the program's own output:

```sh
LD_PRELOAD=./mem_leak.so ./your_program 2> trace.log
```

## Example

[`examples/leaky.c`](examples/leaky.c) makes a few allocations and deliberately leaks one 20-byte block.

```sh
gcc -o leaky examples/leaky.c
LD_PRELOAD=./mem_leak.so ./leaky
```

```text
[MALLOC ]       0x5633b2ae02a0  16 Bytes
[MALLOC ]       0x5633b2ae02c0  8 Bytes
[MALLOC ]       0x5633b2ae02e0  32 Bytes
[FREE   ]       0x5633b2ae02a0
[REALLOC]       0x5633b2ae02c0 -> 0x5633b2ae0310        (64 Bytes)
[FREE   ]       0x5633b2ae02e0
[MALLOC ]       0x5633b2ae0360  64 Bytes
[FREE   ]       0x5633b2ae0310
[FREE   ]       0x5633b2ae0360
[CALLOC ]       0x5633b2ae03b0  20 Bytes


-------MEMORY LEAKS-------
[LEAK   ]               0x5633b2ae03b0  (20 Bytes)
Total leaked bytes: 20B
```

With the `free` argument, the same report is followed by the cleanup:

```sh
LD_PRELOAD=./mem_leak.so ./leaky free
```

```text
-------MEMORY LEAKS-------
[LEAK   ]               0x5602279b73b0  (20 Bytes)
Total leaked bytes: 20B

--ALL LEAKS FREED--
[FREE   ]       0x5602279b73b0
Total freed bytes: 20B
```

Addresses will differ from run to run.

### Reading the trace

| Line | Meaning |
| --- | --- |
| `[MALLOC ] addr size` | `malloc(size)` returned `addr` |
| `[CALLOC ] addr size` | `calloc(n, m)` returned `addr`; the size shown is `n * m` |
| `[REALLOC] old -> new (size)` | the block at `old` was resized to `size` bytes and now lives at `new` |
| `[FREE   ] addr` | `free(addr)` was called |
| `[LEAK   ] addr (size)` | the block was still allocated when the program exited |

`realloc(NULL, n)` is logged as `MALLOC` and `realloc(p, 0)` as `FREE`, because that is how those calls behave. `free(NULL)` is logged with the address `(nil)`.

## How it works

- **Interposition.** Because the library is preloaded, the dynamic linker resolves the program's calls to `malloc`, `calloc`, `realloc` and `free` to the versions defined here. Each wrapper looks up the real function with `dlsym(RTLD_NEXT, ...)`, calls it, and records the result.
- **Bootstrap.** `dlsym` may allocate memory while it is looking up the real functions. During that window, requests are served from a small static bump allocator so the wrappers do not recurse into themselves.
- **Tracking.** Every live block is stored in a fixed-size, open-addressing hash table keyed by its address. The table is a static array in the BSS segment, so the monitor never allocates heap memory of its own.
- **Reporting.** A library destructor walks the table when the program exits; anything still in it is a leak. `_exit()`, `_Exit()` and `syscall()` are wrapped as well, so the report is not skipped when a program bypasses the normal exit path.
- **Logging.** Messages are formatted into fixed buffers and written straight to stderr with `write()`.

## Configuration

`HASH_SIZE` in `memory_monitor.c` sets the capacity of the tracking table. The default is 51,200 entries, which takes about 1.6 MB.

The table does not grow, so `HASH_SIZE` must be larger than the highest number of blocks the program has allocated at the same time. Increase it and rebuild before monitoring allocation-heavy programs.

## Limitations

- **Programs that take arguments.** The monitor reads its `free` option from the target program's command line. A program started with any other argument, or with more than one, stops with a usage message, so programs that need their own arguments cannot be monitored.
- **Programs that close stderr at exit.** GNU coreutils such as `ls` and `cat` close stdout and stderr before library destructors run. Their allocations are traced, but the final leak report is lost.
- **Abnormal termination.** No report is printed if the program is killed by a signal or calls `abort()`.
- **Threads.** The table and the log buffers are shared without locking. Use it on single-threaded programs only.
- **Allocator coverage.** Only `malloc`, `calloc`, `realloc` and `free` are intercepted. Memory obtained through `aligned_alloc`, `posix_memalign` or `mmap` is not tracked.
- **Dynamic linking.** `LD_PRELOAD` has no effect on statically linked binaries and is ignored for setuid programs.
