#ifndef __LIST_H
#define __LIST_H

#ifdef __KERNEL__
#include <linux/types.h>
#else
#include <stddef.h>
#endif

typedef struct ListNode {
    void *value;
    struct ListNode *next;
} ListNode;

typedef struct {
    size_t length;
    ListNode *first;
    ListNode *last;
} List;

// Iterate over every node in the list
#define list_foreach(node, list) \
    for ((node) = (list)->first; (node) != NULL; (node) = (node)->next)

// Create a new heap-allocated list (NULL on allocation failure)
List *list_create(void);

// Free a list created with list_create (does not free the values)
void list_free(List *list);

// Initialize a list that is embedded in another struct or statically allocated
void list_init(List *list);

// Remove every node; if free_value is not NULL it is called on each value
void list_clear(List *list, void (*free_value)(void *value));

// Add a value to the end of the list (0 on success, nonzero on failure)
int list_append(List *list, void *value);

// Add a value to the beginning of the list (0 on success, nonzero on failure)
int list_prepend(List *list, void *value);

// Insert a value at a given index (0 on success, nonzero on failure)
int list_insert(List *list, size_t index, void *value);

// Get the value at the start of the list without removing it (NULL if empty)
void *list_peekfront(const List *list);

// Remove a value from the start of the list and return it (NULL if empty)
void *list_popfront(List *list);

// Remove the node after prev (or the first node if prev is NULL) and return
// its value. Lets callers delete nodes while iterating with a trailing pointer.
void *list_remove_next(List *list, ListNode *prev);

// Move the first node of src to the end of dst without allocating
void list_transfer_front(List *dst, List *src);

#endif
