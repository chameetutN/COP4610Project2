#ifndef __LIST_H
#define __LIST_H

#include <stddef.h>

typedef struct {
    void *value;
    ListNode *next;
} ListNode;

typedef struct {
    size_t length;
    ListNode *first;
    ListNode *last;
} List;


// Create a new list
List* list_create();

// Free an existing list
void list_free(List *list);

// Add a value to the end of the list
void list_append(List *list, void *value);

// Add a value to the beginning of the list
void list_prepend(List *list, void *value);

// Insert a value at a given index
void list_insert(List *list, size_t index, void *value);

// Remove a value from the start of the list
void list_popfront(List *list);

#endif
