Assignments from the systems programming class I took in '25 Spring.  

Assignments: 

Chip8 - A (mostly) working Chip8 emulator

malloc_v3.c - A slab memory allocator that includes optimizations for small chunks. Works with "cat" and "ls". Still some seg faults with larger allocations due to hardcoded virtual address space...(best I could do in under a week).

stack_unwinder - A stack undwinder written in x86 Assembly. Reads the saved RBPs on the stack and writes them to stdout. Loops in assembly are quite fun. 
