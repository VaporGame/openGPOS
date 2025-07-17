#include <libc/unistd.h>

int main(void) {
    write(1, "Hello from userland!", 20);
    return 0;
}