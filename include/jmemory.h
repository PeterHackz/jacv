#ifndef JMEMORY_H
#define JMEMORY_H

extern int memory_allocated;

#include "stdlib.h"

#define MONITOR_MEMORY

#ifdef MONITOR_MEMORY
#define MALLOC(size)                                                           \
    ({                                                                         \
        void *ptr = (void *)malloc(size + sizeof(int));                        \
        ((int *)ptr)[0] = size;                                                \
        memory_allocated += size;                                              \
        (void *)((uintptr_t)ptr + sizeof(int));                                \
    })

#define FREE(ptr)                                                              \
    ({                                                                         \
        memory_allocated -= ((int *)((uintptr_t)ptr - sizeof(int)))[0];        \
        free((void *)((uintptr_t)ptr - sizeof(int)));                          \
    })

#else
#define MALLOC(size) malloc(size)
#define FREE(ptr) free(ptr)
#endif // MONITOR_MEMORY

#endif // JMEMORY_H