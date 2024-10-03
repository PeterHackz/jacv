#include "jcontext.h"
#include "jruntime.h"

#ifdef MONITOR_MEMORY
#include "jutils/jutils.h"
#endif

JContext *JContext_new(JRuntime *jr)
{
    JContext *jctx = jr->allocator.allocate(jr, sizeof(JContext));

    jctx->jr = jr;

    return jctx;
}

void JContext_destroy(JContext *jctx)
{
    JRuntime *jr = jctx->jr;
    JContext_free(jctx, jctx);
#ifdef MONITOR_MEMORY
    ASSERT(jr->memory_allocated == 0,
           "runtime memory was not deallocated infully?!");
#endif
}

void *JContext_alloc(JContext *jctx, size_t size)
{
    return jctx->jr->allocator.allocate(jctx->jr, size);
}

void JContext_free(JContext *jctx, void *ptr)
{
    JRuntime *jr = jctx->jr;
    jr->allocator.deallocate(jr, ptr);
}