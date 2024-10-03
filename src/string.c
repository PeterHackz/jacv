#include "jutils/string.h"
#include "vadefs.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <stdarg.h>

INLINE String *String_new(Allocator *allocator, char *str)
{
    String *s = allocator->allocate(allocator->handle, sizeof(String));
    s->allocator = allocator;

    String_init(s, str);
    return s;
}

INLINE void String_init(String *s, char *str)
{
    if (s->length > 0)
    {
        String_clear(s);
    }
    s->length = strlen(str);
    if (s->length < 8)
    {
        memcpy(s->content.bytes, str, s->length);
        s->content.bytes[s->length] = 0;
    }
    else
    {
        s->content.ptr =
            s->allocator->allocate(s->allocator->handle, s->length + 1);
        memcpy(s->content.ptr, str, s->length);
        s->content.ptr[s->length] = 0;
    }
}

INLINE char *String_getContents(String *s)
{
    if (s->length < 8)
        return s->content.bytes;

    return s->content.ptr;
}

INLINE void String_clear(String *s)
{
    if (s->length > 0)
    {
        if (s->length >= 8)
            s->allocator->deallocate(s->allocator->handle, s->content.ptr);
        else
            memset(s->content.bytes, 0, sizeof(s->content.bytes));
        s->length = 0;
    }
}

INLINE void String_destroy(String *s)
{
    String_clear(s);
    s->allocator->deallocate(s->allocator->handle, s);
}

int String_indexOf(String *s, char *lookup)
{
    int p = 0;
    int len = strlen(lookup);

    if (len > s->length)
        return -1;

    char *content = String_getContents(s);

    for (int i = 0; i < s->length; i++)
    {
        if (!(content[i] == lookup[p++]))
            p = 0;

        if (p == len)
            return i - len + 1;
    }

    return -1;
}

bool String_replace(String *s, char *toReplace, char *replacement)
{
    int index = String_indexOf(s, toReplace);
    if (index == -1)
        return false;

    char *content = String_getContents(s);
    int slen = s->length;

    int len = strlen(toReplace);
    int rlen = strlen(replacement);

    if (len >= rlen)
    {
        // this means we can fit replacement within the same string without
        // having to resize it
        memcpy(content + index, replacement, rlen);
        if (len > rlen)
        {
            // we need to move contents after the index
            memcpy(content + index + rlen, content + index + len,
                   slen - len - index);
        }

        content[slen - len + rlen] = 0;
    }
    else
    {
        int newSize = slen - len + rlen;
        char *newContent =
            s->allocator->allocate(s->allocator->handle, newSize + 1);
        newContent[newSize] = 0;

        memcpy(newContent, content, index);
        memcpy(newContent + index, replacement, rlen);
        memcpy(newContent + index + rlen, content + index + len,
               slen - index - len);

        String_init(s, newContent);

        s->allocator->deallocate(s->allocator->handle, newContent);

        return true;
    }

    s->length = slen - len + rlen;

    return true;
}

// TODO: rewrite this entirely and not vsnprintf to have same
// behavior/results on all systems
void String_format(String *s, ...)
{
    va_list args;
    va_start(args, s);

    char *fmt = String_getContents(s);

    int size = vsnprintf(NULL, 0, fmt, args);

    char *newContent = s->allocator->allocate(s->allocator->handle, size + 1);
    vsnprintf(newContent, size + 1, fmt, args);

    String_init(s, newContent);

    s->allocator->deallocate(s->allocator->handle, newContent);

    va_end(args);
}