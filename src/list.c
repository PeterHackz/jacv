#include "list.h"
#include <stdio.h>

int list_push(ListT *list, void *_item, Allocator *allocator)
{
    if (list->size == list->capacity)
    {
        list->capacity *= 2;
        void **items = allocator->allocate(allocator->handle,
                                           sizeof(void *) * list->capacity);
        LIST_FOREACH(list, void *, { items[idx] = item; })
        allocator->deallocate(allocator->handle, list->items);
        list->items = items;
    }
    list->items[list->size++] = _item;
    return list->size;
}

bool list_remove(ListT *list, int index)
{
    if (index < 0 || index >= list->size)
        return false;
    list->items[index] = NULL;
    for (int i = index; i < list->size - 1; i++)
        list->items[i] = list->items[i + 1];
    list->size--;
    return true;
}

int list_index_of(ListT *list, void *_item)
{
    LIST_FOREACH(list, void *, {
        if (item == _item)
            return idx;
    });
    return -1;
}

bool list_removeItem(ListT *list, void *item)
{
    int index = list_index_of(list, item);
    if (index == -1)
        return false;
    return list_remove(list, index);
}

void list_free(ListT *list, Allocator *allocator)
{
    allocator->deallocate(allocator->handle, list->items);
    allocator->deallocate(allocator->handle, list);
}