#include <stdint.h>
#include <stddef.h> // For size_t
#include "syscall.h" // Include your new syscall header
#include <libc/unistd.h>

#include "uart/uart.h"
static size_t sys_write(int fd, const void *buf, size_t count) {
    uart_puts((const char *)buf, count);
    return count;
}

// static int sys_execv(const char *file, char *const argv[]) {

// }

int do_syscall(uint32_t *regs, uint32_t syscall_num) {
    int ret_val = -1;

    switch (syscall_num) {
        case SYS_WRITE:
        // Args: fd R0, buf R1, count R2
            ret_val = sys_write((int)regs[0], (const void *)regs[1], (size_t)regs[2]);
            break;
        default:
            uartTxStr("Unknown syscall number: ");
            uartTxDec(syscall_num);
            uartTxStr("\r\n");
            ret_val = -2;
            break;
    }

    return ret_val;
}

__attribute__((naked)) void svCallHandler(void) {
    // When an SVC exception occurs:
    // R0-R3, R12, LR, PC, xPSR are pushed onto the stack automatically by the hardware.
    // The stack pointer (either MSP or PSP depending on EXC_RETURN) points to these.

    asm volatile (
    // When an SVC exception occurs, the hardware automatically pushes:
    // R0, R1, R2, R3, R12, LR, PC, xPSR onto the current stack.
    // The stack pointer (SP) points to R0.

    // Determine which stack (MSP or PSP) was used by the interrupted code.
    // We do this by examining bit 2 of the EXC_RETURN value in LR.
    // If bit 2 is 0, MSP was used. If 1, PSP was used.
    "movs r3, lr \n"           // Move LR (high register) to R3 (low register). (This instruction should be fine).
    "movs r2, #4 \n"           // Load immediate 4 into R2. (This should be fine).
    "and r3, r2 \n"            // R3 = R3 AND R2. (This is the non-flag-setting AND, which should be 16-bit Thumb-1).
    "cmp r3, #0 \n"            // Compare R3 with 0. If R3 is 0, Z flag is set.

    "beq use_msp \n"           // If Z flag is set (meaning bit 2 of original LR was 0), branch to use MSP.
    "mrs r0, psp \n"           // If Z flag not set (meaning bit 2 of original LR was 1), PSP was used. Copy PSP to R0.
    "b continue_handler \n"    // Jump to the common handler entry point.

    "use_msp: \n"
        "mrs r0, msp \n"           // MSP was used. Copy MSP to R0.

    "continue_handler: \n"
        // R0 now points to the stacked registers: [R0_old, R1_old, R2_old, R3_old, R12_old, LR_old, PC_old, PSR_old]
        // The stacked PC (Program Counter) is at offset 6*4 = 24 bytes from stacked R0.
        // It points to the instruction *after* the SVC.
        // To get the SVC instruction itself, subtract 2 bytes (for 16-bit Thumb).
        "push {lr} \n"
        "ldr r2, [r0, #24] \n"     // Load stacked PC into R1 (R1 now has address of instruction AFTER SVC)
        "sub r2, #2 \n"           // Subtract 2 to point R1 *to* the SVC instruction address
        "ldrh r1, [r2, #0] \n"     // Load the 16-bit SVC instruction itself into R3 from address in R1. Offset #0 is explicit.
        //"movs r1, r3 \n"           // Move the instruction from R3 back to R1. (Use MOVS for 16-bit encoding if possible)
        "movs r3, #0xFF \n"        // Load immediate #0xFF into R3
        "and r1, r3 \n"            // Mask out the immediate value (lower 8 bits) into R1 (R1 = R1 AND R3).
                                   // R1 now holds the syscall number.

        "bl do_syscall \n"         // Call the C dispatcher: int do_syscall(uint32_t syscall_num, uint32_t *regs)

        "str r0, [r0, #0]\n"
        // Return value from do_syscall (in R0) is now the syscall result.
        // It will overwrite the stacked R0 when returning from exception.
        "pop {r3} \n"
        //"mov lr, r3\n"
        "bx r3 \n"                 // Return from exception
        :
        :
        : "r0", "r1", "r2", "r3", "r12", "lr" // Clobber list for registers
    );
}