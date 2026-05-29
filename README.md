# C-memory-monitor
Lightweight memory leak detector for Linux. It wraps malloc,free,realloc,calloc via LD_PRELOAD to trace memory allocations and reports leaks at exit of the program.

Compile the program via:
`gcc -fPIC -shared -o mem_leak.so memory_monitor.c`

Inject it to a program via:
`LD_PRELOAD="./mem_leak.so" ./<program>`
