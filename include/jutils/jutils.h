#ifndef JUTILS_H
#define JUTILS_H

#include "defines.h"
#include "stdint.h"
#include "string.h"

#include <assert.h>

#define MAKE_CTOR_DEC(type, fn_name) type *fn_name##_new(JContext *jctx)
#define MAKE_DTOR_DEC(type, fn_name)                                           \
    void fn_name##_destroy(type *fn_name, JContext *jctx)

#define MAKE_CTOR(type, fn_name, BODY)                                         \
    type *fn_name##_new(JContext *jctx)                                        \
    {                                                                          \
        type *fn_name = JContext_alloc(jctx, sizeof(type));                    \
        BODY;                                                                  \
        return fn_name;                                                        \
    }
#ifndef DUMP_EXPR
#define MAKE_EXPR_CTOR(type, fn_name, BODY, DUMP)                              \
    type *fn_name##_new(JContext *jctx)                                        \
    {                                                                          \
        type *fn_name = JContext_alloc(jctx, sizeof(type));                    \
        BODY;                                                                  \
        return fn_name;                                                        \
    }
#else
#define MAKE_EXPR_CTOR(type, fn_name, BODY, DUMP)                              \
    void fn_name##_dump(type *expr, JContext *jctx) { DUMP; }                  \
    type *fn_name##_new(JContext *jctx)                                        \
    {                                                                          \
        type *fn_name = JContext_alloc(jctx, sizeof(type));                    \
        fn_name->dump = &fn_name##_dump;                                       \
        fn_name->destroy = (void *)&_default_destructor;                       \
        BODY;                                                                  \
        return fn_name;                                                        \
    }
#endif

#define MAKE_CTOR_QUICK(type, fn_name) MAKE_CTOR(type, fn_name, )

#define MAKE_DTOR(type, fn_name, BODY)                                         \
    void fn_name##_destroy(type *fn_name, JContext *jctx) { BODY; }

#define STREQ(value, keyword) strcmp(value, keyword) == 0

typedef enum
{
    STATE_FAIL,
    STATE_SUCCESS_NUM,
    STATE_SUCCESS_FLOAT,
} ParseNumberResultState;

typedef struct
{
    char state;
    union
    {
        uint64_t uint64;
        double float64;
    } num;
} ParseNumberResult;

ParseNumberResult parse_number(const char *str);

#define ASSERT(cond, msg) assert(((void)msg, cond))

#endif // JUTILS_H