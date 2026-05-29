# C-memory-monitor
Lightweight simple memory leak detector for Linux programmed in C. It wraps malloc,free,realloc,calloc via LD_PRELOAD to trace memory allocations and reports leaks at exit of the program.

Compile the program via:
`gcc -fPIC -shared -o mem_leak.so memory_monitor.c`

Inject it to a program via:
`LD_PRELOAD="./mem_leak.so" ./<program>`

Also added a wrapper around the _exit(), _Exit() and syscall() functions to show leaks reports when program force quits. It does not show it for normal linux commands like ls or cat.. because of LD_PRELOAD limitations.
