#ifndef GC_H
#define GC_H

#include "stdlib.h"

#define GC_HEADER                                                              \
    int refs_count;                                                            \
    void (*destructor)(void *);                                                \
    void *ptr;

typedef struct gc_header
{
    GC_HEADER;
} gc_header_t;

#define GC_NEW(type) GC_CREATE_NEW(type, 1, NULL)

#define GC_NEW_ARRAY(type, size) GC_CREATE_NEW(type, size, NULL)

#define GC_NEW_WITH_DESTRUCTOR(type, size)                                     \
    GC_CREATE_NEW(type, size, type##_destruct)

#define GC_CREATE_NEW(type, size, destructor_func)                             \
    ({                                                                         \
        type *ptr = (type *)malloc(sizeof(type) * size);                       \
        ((gc_header_t *)(ptr))->refs_count = 1;                                \
        ((gc_header_t *)(ptr))->destructor = destructor_func;                  \
        ((gc_header_t *)(ptr))->ptr = ptr;                                     \
        ptr;                                                                   \
    })

#define GC_REF(ptr)                                                            \
    ({                                                                         \
        if (ptr != NULL)                                                       \
        {                                                                      \
            ((gc_header_t *)(ptr))->refs_count++;                              \
        }                                                                      \
        ptr;                                                                   \
    })

#define GC_UNREF(ptr)                                                          \
    ({                                                                         \
        if (ptr != NULL)                                                       \
        {                                                                      \
            ((gc_header_t *)(ptr))->refs_count--;                              \
            if (((gc_header_t *)(ptr))->refs_count == 0)                       \
            {                                                                  \
                if (((gc_header_t *)(ptr))->destructor != NULL)                \
                {                                                              \
                    ((gc_header_t *)(ptr))->destructor(ptr);                   \
                }                                                              \
                free(((gc_header_t *)(ptr))->ptr);                             \
            }                                                                  \
        }                                                                      \
        ptr;                                                                   \
    })

#endif // GC_H