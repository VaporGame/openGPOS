#ifndef UNISTD_H
#define UNISTD_H
#include <stddef.h>
#include <stdint.h>

typedef uint32_t useconds_t;

int usleep(useconds_t usec);
unsigned int sleep(unsigned int seconds);

#endif //UNISTD_H