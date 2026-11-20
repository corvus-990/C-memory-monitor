COMPILE = gcc
FLAGS = -fPIC -shared

mm: memory_monitor.c
	$(COMPILE) $(FLAGS) $< -o mm.so