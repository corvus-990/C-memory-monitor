# C-memory-monitor
Lightweight simple memory leak detector for Linux programmed in C. It wraps malloc,free,realloc,calloc via LD_PRELOAD to trace memory allocations and reports leaks at exit of the program.

Compile the program via:
`gcc -fPIC -shared -o mem_leak.so memory_monitor.c`

**IGNORE GCC WARNINGS**

Inject it to a program via:
`LD_PRELOAD="./mem_leak.so" ./<program>`

Also added a wrapper around the _exit(), _Exit() and syscall() functions to show leaks reports when program force quits. It does not show it for normal linux commands like ls or cat.. because of LD_PRELOAD limitations.


Example:
<img width="2006" height="509" alt="image" src="https://github.com/user-attachments/assets/c4ef7e9f-d4fd-45c7-ab7f-d686dc7e2207" />

If you have reallocated a NULL pointer, the program will detect it as MALLOC, simply because reallocating a NULL pointer is like using malloc to allocate.

**Note that the program tracks all memory using a hash table that is allocated in the BSS segment. If you are running a heavy program that allocates simultaneously a bunch of memory, increase the macro at line 12 `HASH_SIZE`, it is set by default to 51200 entries (approximately 1600KB).**
