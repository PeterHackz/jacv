#ifndef CONTEXT_H
#define CONTEXT_H

#include "jruntime.h"
#include <stdint.h>

#include "defines.h"

typedef struct JContext
{
    JRuntime *jr;
} JContext;

JContext *JContext_new(JRuntime *);
void JContext_destroy(JContext *);

void *JContext_alloc(JContext *, size_t);
void JContext_free(JContext *, void *);

#endif