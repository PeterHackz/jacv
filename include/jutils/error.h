#ifndef ERROR_H
#define ERROR_H

#include "jutils/string.h"

typedef struct Error
{
    String val;
    bool state;
} Error;

#endif // ERROR_H