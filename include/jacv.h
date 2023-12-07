#ifndef JACV_H
#define JACV_H

#include "gc.h"

static enum {
    JTYPE_INT,
    JTYPE_FLOAT,
    JTYPE_STRING,
    JTYPE_ARRAY,
    JTYPE_OBJECT,
    JTYPE_NULL,
    JTYPE_BOOL,
} JType;

typedef struct
{
    GC_HEADER;
    char type;
} JValue;

#endif // JACV_H