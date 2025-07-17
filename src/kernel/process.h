#ifndef PROCESS_H
#define PROCESS_H
#include <stdint.h>

typedef enum {
    PROC_NEW = 0,
    PROC_READY,
    PROC_RUNNING,
    PROC_WAITING,
    PROC_TERM
} process_state_t;

#define NUM_REGISTERS 14

typedef struct {
    uint8_t pid; // Process id
    process_state_t state;
    //uint8_t ppid; // Parent process id
    uint32_t pc; // Program counter
    int registers[NUM_REGISTERS];
    unsigned int burst_time;
    unsigned int time_remaining;

    // Add registers and scheduling info
} pcb_t; // Program control block

#define MAX_PROCESSES 8
#define TIME_QUANTUM 5

void init_scheduler();
void create_process(uint8_t pid);
void schedule();
void run_process(uint8_t pid);

#endif //PROCESS_H