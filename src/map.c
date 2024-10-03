#include "map.h"
#include "allocator.h"
#include "string.h"

bool ptr_comparator(void *a, void *b) { return a == b; }

Map *map_new(Allocator *allocator)
{
    Map *m = allocator->allocate(allocator->handle, sizeof(Map));
    m->allocator = allocator;
    m->compare = &ptr_comparator;
    m->node = NULL;
    m->destroy = NULL;
    return m;
}

void map_setComparator(Map *m, bool (*comparator)(void *, void *))
{
    m->compare = comparator;
}

void *map_get(Map *m, void *k, bool *ok)
{
    MapNode *node = m->node;

    *ok = true;

    while (node != NULL)
    {
        if (m->compare(node->entry.key, k))
        {
            return node->entry.value;
        }
        node = node->next;
    }

    *ok = false;

    return NULL;
}
#include <stdio.h>
void map_put(Map *m, void *k, void *v)
{
    MapNode *node = m->node;
    MapNode *newNode =
        m->allocator->allocate(m->allocator->handle, sizeof(MapNode));
    newNode->next = NULL;
    newNode->entry.key = k;
    newNode->entry.value = v;

    if (node == NULL)
        m->node = newNode;
    else
    {
        while (true)
        {
            MapNode *next = node->next;
            if (next == NULL)
            {
                node->next = newNode;
                break;
            }
            else
                node = next;
        }
    }
}

// https://www.geeksforgeeks.org/c-program-for-deleting-a-node-in-a-linked-list/
MapEntry map_remove(Map *m, void *k, bool *ok)
{
    struct MapNode *temp = m->node, *prev;

    *ok = true;
    MapEntry me = {0};

    if (temp != NULL && m->compare(temp->entry.key, k))
    {
        m->node = temp->next; // Changed head
        me = temp->entry;
        m->allocator->deallocate(m->allocator->handle, temp);
        return me;
    }

    // Search for the key to be deleted, keep track of the
    // previous node as we need to change 'prev->next'
    while (temp != NULL && !m->compare(temp->entry.key, k))
    {
        prev = temp;
        temp = temp->next;
    }

    // If key was not present in linked list
    if (temp == NULL)
    {
        *ok = false;
        return me;
    }

    // Unlink the node from linked list
    prev->next = temp->next;

    me = temp->entry;
    m->allocator->deallocate(m->allocator->handle, temp);
    return me;
}

void map_clear(Map *m)
{
    MapNode *node = m->node;

    void (*deallocate)(void *, void *) = m->allocator->deallocate;
    void *handle = m->allocator->handle;
    while (node != NULL)
    {
        MapNode *next = node->next;
        if (m->destroy != NULL)
        {
            m->destroy(node->entry.key, node->entry.value);
        }
        deallocate(handle, node);
        node = next;
    }

    m->node = NULL;
}

void map_destroy(Map *m)
{
    map_clear(m);
    m->allocator->deallocate(m->allocator->handle, m);
}