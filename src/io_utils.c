#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "io_utils.h"

#include "jcontext.h"

uint8_t *read_file(JContext *jctx, char *filename, int *size)
{
    uint8_t *buffer = NULL;
    FILE *file = NULL;

    file = fopen(filename, "rb");
    if (file == NULL)
    {
        printf("Error: fopen failed\n");
        return NULL;
    }
    fseek(file, 0, SEEK_END);
    *size = ftell(file);
    fseek(file, 0, SEEK_SET);
    buffer = (uint8_t *)JContext_alloc(jctx, *size);
    if (buffer == NULL)
    {
        printf("Error: malloc failed\n");
        return NULL;
    }
    fread(buffer, *size, 1, file);
    fclose(file);
    return buffer;
}

int write_file(char *filename, uint8_t *buffer, int size)
{
    FILE *file = NULL;
    file = fopen(filename, "wb");
    if (file == NULL)
    {
        printf("Error: fopen failed\n");
        return 1;
    }
    fwrite(buffer, size, 1, file);
    fclose(file);
    return 0;
}