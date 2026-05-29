# C-memory-monitor
Lightweight simple memory leak detector for Linux programmed in C. It wraps malloc,free,realloc,calloc via LD_PRELOAD to trace memory allocations and reports leaks at exit of the program.

Compile the program via:
`gcc -fPIC -shared -o mem_leak.so memory_monitor.c`

**IGNORE GCC WARNINGS**

Inject it to a program via:
`LD_PRELOAD="./mem_leak.so" ./<program> <optional argument: free>`

Also added a wrapper around the _exit(), _Exit() and syscall() functions to show leaks reports when program force quits. It does not show it for normal linux commands like ls or cat.. because of LD_PRELOAD limitations.

**----------------------------------**

Added an optional argument. If you add the argument free, it will free the memory. If no argument is added, it only shows the memory leaks and exits without freeing them.

Output without argument:
<img width="1948" height="738" alt="image" src="https://github.com/user-attachments/assets/6dd77d0a-da6c-4a0b-9cf7-78d288498bff" />

Output with `free` argument:
<img width="1944" height="936" alt="image" src="https://github.com/user-attachments/assets/6a3d8af6-c5b3-446a-8f37-a2340db019c2" />


If you have reallocated a NULL pointer, the program will detect it as MALLOC, simply because reallocating a NULL pointer is like using malloc to allocate.

**Note that the program tracks all memory using a hash table that is allocated in the BSS segment. If you are running a heavy program that allocates simultaneously a bunch of memory, increase the macro at line 12 `HASH_SIZE`, it is set by default to 51200 entries (approximately 1600KB).**
