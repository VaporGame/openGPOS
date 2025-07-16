#include <libc/unistd.h>
#include "hardware_structs/clocks.h"

static uint64_t readTime() {
    uint32_t timeLR = timer_hw->LR;
    uint32_t timeHR = timer_hw->HR;
    return (((uint64_t)timeHR << 32) | timeLR);
}

int usleep(useconds_t usec) {
    uint64_t timeOld = readTime(); // Get current timer value
    while ((readTime() - timeOld) < usec); // Wait till desired time is passed
    return 0; // Don't know what errors it can throw
}