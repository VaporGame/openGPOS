#ifndef KERNEL_SYSCALL_H
#define KERNEL_SYSCALL_H

#include <stdint.h>
#include <stddef.h>



typedef enum {
    SYS_RESERVED = 0,
    SYS_WRITE,
    SYS_READ,
    SYS_EXIT,
    SYS_USLEEP,
    NUM_SYSCALLS
} syscall_t;

void svCallHandler(void);
int do_syscall(uint32_t *regs, uint32_t syscall_num);

#endif //KERNEL_SYSCALL_H