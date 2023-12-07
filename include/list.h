#ifndef LIST_H
#define LIST_H

#include "stdbool.h"
#include "stdlib.h"
#include <string.h>

#define LIST_FIELDS                                                            \
    int size;                                                                  \
    int capacity

typedef struct
{
    LIST_FIELDS;
    void **items;
} ListT;

#define INT2VOIDP(i) (void *)(uintptr_t)(i)
#define INTP2INT(i) (int)(uintptr_t)(i)

#define LIST_TYPEOF(type) type##_list_t

#define LIST_INIT_TYPE(type)                                                   \
    typedef struct                                                             \
    {                                                                          \
        LIST_FIELDS;                                                           \
        type **items;                                                          \
    } LIST_TYPEOF(type)

#define LIST_FOREACH(list, type, BLOCK)                                        \
    for (int idx = 0; idx < list->size; idx++)                                 \
    {                                                                          \
        type item = list->items[idx];                                          \
        BLOCK                                                                  \
    }

#define LIST_NEW(size) LIST_NEW_WITH_SIZE(10)
#define LIST_NEW_WITH_SIZE(lsize)                                              \
    ({                                                                         \
        ListT *list = (ListT *)MALLOC(sizeof(ListT));                          \
        list->size = 0;                                                        \
        list->capacity = lsize;                                                \
        list->items = (void **)MALLOC(sizeof(void *) * lsize);                 \
        list;                                                                  \
    })

#define LIST_CREATE(type, size) (LIST_TYPEOF(type) *)LIST_NEW(size)
#define LIST_CREATE_EMPTY(type) (LIST_TYPEOF(type) *)LIST_NEW(10)

#define LIST_DECLARE(var, type, size)                                          \
    LIST_TYPEOF(type) *var = LIST_CREATE(type, size)
#define LIST_DECLARE_EMPTY(var, type)                                          \
    LIST_TYPEOF(type) *var = LIST_CREATE(type, 10)

int list_push(ListT *list, void *item);
bool list_remove(ListT *list, int index);
int list_index_of(ListT *list, void *item);
bool list_removeItem(ListT *list, void *item);
void list_free(ListT *list);
void test();

#define LIST_PUSH(list, item) list_push((ListT *)list, (void *)item)
#define LIST_REMOVE(list, index) list_remove((ListT *)list, index)
#define LIST_INDEX_OF(list, item) list_index_of((ListT *)list, (void *)item)
#define LIST_REMOVE_ITEM(list, item)                                           \
    list_removeItem((ListT *)list, (void *)item)
#define LIST_FREE(list) list_free((ListT *)list)

#endif // LIST_H