#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include "stdint.h"

typedef struct Allocator
{
    void *(*allocate)(void *, size_t);
    void (*deallocate)(void *, void *);
    void *handle;
} Allocator;

#endif