#ifndef RUNTIME_H
#define RUNTIME_H

#include "allocator.h"

#include "defines.h"
#include "map.h"

typedef struct JRuntime
{
    Allocator allocator;
#ifdef MONITOR_MEMORY
    int memory_allocated;
    Map *allocations;
#endif
} JRuntime;

JRuntime *JRuntime_new();
void JRuntime_destroy(JRuntime *);

#endif // RUNTIME_H