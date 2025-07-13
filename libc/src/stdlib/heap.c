#include <libc/stdlib.h>


#include <libc/string.h>
#include <stdbool.h>

#include "uart/uart.h"

//==============================================================================
// TYPES AND CONSTANTS
//==============================================================================

extern char __heap_start__[];
extern char __heap_end__[];

#define ALIGN_4BYTE(x) (((size_t)x + (4 - 1)) & -4)
#define __heap_size__ (__heap_end__ - (char*)ALIGN_4BYTE(__heap_start__))

typedef struct {
    void *prev;
    void *next;
    size_t size;
    bool isAllocated;
} memory_space_t;

// doubly linked list
memory_space_t *head;

//==============================================================================
// HELPER FUNCTIONS
//==============================================================================

memory_space_t *find_free_space(size_t size) {
    memory_space_t *curr = head;

    while(curr->next && (curr->isAllocated || curr->size < size)) {
        curr = curr->next;
    }

    if(curr->size < size || curr->isAllocated) {
        return (memory_space_t*)0;
    }

    return curr;
}

bool mergeNextMemorySpace(memory_space_t* memory_space) {
    memory_space_t* next_memory_space = memory_space->next;
    if(next_memory_space && !next_memory_space->isAllocated) {
        // the one after the next one
        memory_space_t* next_er_memory_space = next_memory_space->next;
        if(next_er_memory_space) { // only set prev if it even exists
            next_er_memory_space->prev = memory_space;
        }
        memory_space->next = next_er_memory_space;
        memory_space->size += sizeof(memory_space_t) + next_memory_space->size;
        return true;
    }
    return false;
}

bool splitMemorySpace(memory_space_t* memory_space, size_t size) {
    // check if we can split this memory space, and also leave some room
    if(memory_space->size > size + sizeof(memory_space_t) + 4) {
        memory_space_t* next = (void*)memory_space + sizeof(memory_space_t) + size;
        next->prev = memory_space;
        next->next = memory_space->next;
        next->size = memory_space->size - size - sizeof(memory_space_t);
        next->isAllocated = false;
        memory_space->next = next;
        memory_space->size = size;
        // try to merge the leftover
        mergeNextMemorySpace(next);

        return true;
    }
    return false;
}

//==============================================================================
// HEAP FUNCTION IMPLEMENTATIONS
//==============================================================================

void init_malloc() {
    head = (memory_space_t*)ALIGN_4BYTE(__heap_start__);
    head->prev = (void*)0;
    head->next = (void*)0;
    head->size = __heap_size__ - sizeof(memory_space_t);
    head->isAllocated = false;
}

void *malloc(size_t size) {
    size = ALIGN_4BYTE(size);

    memory_space_t* free_space = find_free_space(size);
    if(free_space == (void*)0) return (void*)0;

    splitMemorySpace(free_space, size);
    
    free_space->isAllocated = true;
    return (void*)free_space + sizeof(memory_space_t);
}

void *realloc(void *ptr, size_t size) {
    // TODO: reimplement this smarter
    memory_space_t* memory_space = ptr - sizeof(memory_space_t);
    size_t oldSize = memory_space->size;
    void *new = malloc(size);
    if(new == (void*)0) return (void*)0;

    size_t toCopy;
    if(oldSize < size) {
        toCopy = oldSize;
    } else {
        toCopy = size;
    }

    memcpy(new, ptr, toCopy);

    free(ptr);

    return new;
}

void free(void *ptr) {
    if (ptr == (void*)0) return;

    memory_space_t* memory_space = ptr - sizeof(memory_space_t);
    memory_space->isAllocated = false;

    mergeNextMemorySpace(memory_space);

    memory_space_t* prev_memory_space = memory_space->prev;
    if(prev_memory_space && !prev_memory_space->isAllocated)
        // mergeNextMemorySpace(memory_space->prev);
        mergeNextMemorySpace(prev_memory_space);
}

void dumpHeap() {
    // puts("[");
    uartTx('[');
    memory_space_t *curr = head;

    //printf("\tstart: %p, size: %ld, isalloc: %d\n", curr, curr->size, curr->isAllocated);
    uartTxStr("start: "); uartTxHex((uint32_t)curr); uartTxStr(",size: "); uartTxDec(curr->size); uartTxStr(",lloc"); uartTxDec(curr->isAllocated); uartTxStr("\r\n");
    uartRx();

    while(curr->next) {
        curr = curr->next;
        //printf("\tstart: %p, size: %ld, isalloc: %d\n", curr, curr->size, curr->isAllocated);
        uartTxStr("start: "); uartTxHex((uint32_t)curr); uartTxStr(",size: "); uartTxDec(curr->size); uartTxStr(",lloc"); uartTxDec(curr->isAllocated); uartTxStr("\r\n");
        uartRx();
    }
    // puts("]");
    uartTx(']');
}