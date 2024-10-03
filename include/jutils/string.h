#ifndef STRING_H
#define STRING_H

#include "allocator.h"
#include "defines.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct String
{
    uint16_t length;
    union
    {
        char bytes[8];
        char *ptr;
    } content;
    Allocator *allocator;
} String;

typedef enum
{
    VARG_INT,
    VARG_DOUBLE,
    VARG_CHAR,
    VARG_STR,
} VargType;

typedef struct VArg
{
    char type;
    union
    {
        int i;
        double f;
        char c;
        char *str;
    } value;
} VArg;

String *String_new(Allocator *, char *);
void String_init(String *, char *);

void String_clear(String *);
void String_destroy(String *);

char *String_getContents(String *);

int String_indexOf(String *, char *);

bool String_replace(String *, char *, char *);

void String_format(String *s, ...);

#define STRING_NEW(var, content, _allocator)                                    \
    String var = {                                                             \
        .allocator = _allocator,                                                \
    };                                                                         \
    String_init(&var, content)

#endif // STRING_H