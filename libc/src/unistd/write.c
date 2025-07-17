#include <libc/unistd.h>
#include "stdint.h"

#include "uart/uart.h"

#include "syscall.h"

ssize_t write(int fd, const void *buf, size_t count) {
    long ret;

    asm volatile (
        // Prepare arguments in R0-R3 for the syscall
        "mov r0, %2 \n"      
        "mov r1, %3 \n"      
        "mov r2, %4 \n"      
        //"mov r7, %1 \n"      
        "svc %1     \n"      // Trigger SVC exception. The immediate #0 is typically ignored.
        "mov %0, r0 \n"      // After SVC returns, R0 holds the syscall result. Move it to 'ret'.
        : "=r" (ret)                                            // Output: 'ret' comes from R0
        : "i" (SYS_WRITE), "r" (fd), "r" (buf), "r" (count)     // Inputs: immediate SYS_WRITE, fd, buf, count in registers
        : "r0", "r1", "r2", "r3", "memory"                      // Clobbered registers: R0-R3 are explicitly used/modified.
                                                                // "memory" clobber might be needed if syscall writes to memory
                                                                // that compiler might optimize away. For write, it's safer.
    );
    return (ssize_t)ret;
}