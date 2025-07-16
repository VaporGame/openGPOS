#include <stddef.h>
#include <libc/stdlib.h>

extern int main(void);

// // Data section (initialized data)
// extern unsigned char __data_start__;
// extern unsigned char __data_end__;

// // BSS section (uninitialized data, zeroed at startup)
// extern unsigned char __bss_start__;
// extern unsigned char __bss_end__;

// // Heap section (dynamic memory allocation)
// extern unsigned char _heap_start;
// extern unsigned char _heap_end;

// // Stack section (used by the initial assembly setup)
// Note: _stack_top is for the initial SP, used by the naked _start function.
extern unsigned char _stack_top;

void _start_c(void) {
    init_malloc();

    (void)main();
    
    for(;;);
}

void _start(void) __attribute__((naked));
void _start(void) {
    __asm__ volatile (
        "ldr r0, =_stack_top\n"
        "mov sp, r0\n"
        "b _start_c\n"
    );
}