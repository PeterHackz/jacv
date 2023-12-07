#ifndef JUTILS_H
#define JUTILS_H

#include "string.h"

#define MAKE_CTOR_DEC(type, fn_name) type *fn_name##_new()
#define MAKE_DTOR_DEC(type, fn_name) void fn_name##_destruct(type *fn_name)

#define MAKE_CTOR(type, fn_name, BODY)                                         \
    type *fn_name##_new()                                                      \
    {                                                                          \
        type *fn_name = (type *)MALLOC(sizeof(type));                          \
        BODY;                                                                  \
        return fn_name;                                                        \
    }

#define MAKE_CTOR_QUICK(type, fn_name) MAKE_CTOR(type, fn_name, )

#define MAKE_DTOR(type, fn_name, BODY)                                         \
    void fn_name##_destruct(type *fn_name) { BODY; }

#define STREQ(value, keyword) strcmp(value, keyword) == 0

#endif // JUTILS_H