#include "jruntime.h"
#include "allocator.h"
#include "jutils/jutils.h"

#include <stdlib.h>
#include <string.h>

#ifdef MONITOR_MEMORY
#include "map.h"

#include <stdio.h>

void *_alloc2(void *_, size_t size) { return malloc(size); }

void _dealloc2(void *_, void *ptr) { free(ptr); }
#endif

void *_alloc(void *handle, size_t size)
{
    void *ptr = (void *)malloc(size + sizeof(int));
    memset(ptr, 0, size + sizeof(int));
    ((int *)ptr)[0] = size;
#ifdef MONITOR_MEMORY
    JRuntime *jr = (JRuntime *)handle;
    jr->memory_allocated += size;

    map_put(jr->allocations, ptr, NULL);
#endif
    return (void *)((uintptr_t)ptr + sizeof(int));
}

void _dealloc(void *handle, void *ptr)
{
    int size = ((int *)((uintptr_t)ptr - sizeof(int)))[0];
    void *realPtr = (void *)((uintptr_t)ptr - sizeof(int));
    memset(realPtr, 0, size + sizeof(int));

#ifdef MONITOR_MEMORY
    JRuntime *jr = (JRuntime *)handle;
    jr->memory_allocated -= size;

    bool ok;
    map_remove(jr->allocations, realPtr, &ok);
    ASSERT(ok, "_dealloc called twice?");
#endif
    free(realPtr);
}

JRuntime *JRuntime_new()
{
    JRuntime *jr = (JRuntime *)malloc(sizeof(JRuntime));
    memset(jr, 0, sizeof(JRuntime));

    jr->allocator.allocate = &_alloc;
    jr->allocator.deallocate = &_dealloc;
    jr->allocator.handle = (void *)jr;

#ifdef MONITOR_MEMORY
    jr->memory_allocated = 0;
    Allocator *allocator = malloc(sizeof(Allocator));
    allocator->handle = NULL;
    allocator->allocate = &_alloc2;
    allocator->deallocate = &_dealloc2;
    jr->allocations = map_new(allocator);
#endif

    return jr;
}

void JRuntime_destroy(JRuntime *jr)
{

#ifdef MONITOR_MEMORY
    map_clear(jr->allocations);
    Allocator *allocator = jr->allocations->allocator;
    allocator->deallocate(allocator->handle, jr->allocations);
    free(allocator);
#endif

    memset(jr, 0, sizeof(JRuntime));
    free(jr);
}