#ifndef MAP_H
#define MAP_H

#include "allocator.h"

#include <stdbool.h>

typedef struct MapEntry
{
    void *key;
    void *value;
} MapEntry;

typedef struct MapNode
{
    MapEntry entry;
    struct MapNode *next;
} MapNode;

typedef struct Map
{
    MapNode *node;
    bool (*compare)(void *, void *);
    void (*destroy)(void *, void *);
    Allocator *allocator;
} Map;

Map *map_new(Allocator *);

void map_setComparator(Map *, bool (*)(void *, void *));

void *map_get(Map *, void *, bool *);
void map_put(Map *, void *, void *);
MapEntry map_remove(Map *, void *, bool *);

void map_clear(Map *);

void map_destroy(Map *);

#endif // MAP_H