#include "process.h"

pcb_t process_table[MAX_PROCESSES];
uint8_t num_processes = 0;
uint8_t next_available_pid = 200;
int16_t current_running_pid = -1;

void init_scheduler() {
    for (uint8_t i = 0; i < MAX_PROCESSES; i++) {
        process_table[i].pid = -1;
        process_table[i].state = PROC_NEW;
        process_table[i].pc = 0;

        for (uint8_t r = 0; r < NUM_REGISTERS; r++) {
            process_table[i].registers[r] = 0;
        }

        process_table[i].burst_time = 0;
        process_table[i].time_remaining = 0;
    }
}

int create_process(uint8_t pid, unsigned int burst_time) {
    if (num_processes >= MAX_PROCESSES) {
        return;
    }

    int16_t idx = -1;

    for (uint8_t i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].pid == -1) {
            idx = i;
            break;
        }
    }
    
    if (idx == -1) {
        return -1;
    }

    process_table[idx].pid = next_available_pid++;
    process_table[idx].state = PROC_READY;
    process_table[idx].pc = 0;
    for (uint8_t r = 0; r < NUM_REGISTERS; r++) {
        process_table[idx].registers[r] = 0;
    }
    process_table[idx].burst_time = burst_time;
    process_table[idx].time_remaining = burst_time;

    num_processes++;
    return process_table[idx].pid;
}

// void context_switch

// void schedule() {
//     int8_t next_pid = -1;
//     bool found_ready = false;

//     if (current_running_pid != -1) {
//         for (uint8_t i = 0; i < num_processes; i++) {
//             if (process_table[i].pid == current_running_pid && process_table[i].state == PROC_RUNNING) {

//             }
//         }
//     }
// }

void run_process(uint8_t pid) {
    current_running_pid = pid;

    for (uint8_t = 0; i < num_processes; i++) {
        if (process_table[i].pid == pid) {

        }
    }
}