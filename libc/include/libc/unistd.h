#ifndef UNISTD_H
#define UNISTD_H
#include <stddef.h>
#include <stdint.h>

typedef int ssize_t;
typedef uint32_t useconds_t;

int usleep(useconds_t usec);
unsigned int sleep(unsigned int seconds);

ssize_t write(int fd, const void *buf, size_t count);

#endif //UNISTD_H