#ifndef IO_UTILS_C
#define IO_UTILS_C

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

uint8_t *read_file(char *filename, int *size);

int write_file(char *filename, uint8_t *buffer, int size);

#endif // IO_UTILS_C