#include "list.h"

#ifdef __KERNEL__
#include <linux/slab.h>
#define list_alloc(size) kmalloc(size, GFP_KERNEL)
#define list_dealloc(ptr) kfree(ptr)
#else
#include <stdlib.h>
#define list_alloc(size) malloc(size)
#define list_dealloc(ptr) free(ptr)
#endif


static ListNode *new_node(void *value) {
    ListNode *node = list_alloc(sizeof(ListNode));
    if (node == NULL) {
        return NULL;
    }
    node->value = value;
    node->next = NULL;
    return node;
}


List *list_create(void) {
    List *list = list_alloc(sizeof(List));
    if (list != NULL) {
        list_init(list);
    }
    return list;
}


void list_free(List *list) {
    list_clear(list, NULL);
    list_dealloc(list);
}


void list_init(List *list) {
    list->length = 0;
    list->first = NULL;
    list->last = NULL;
}


void list_clear(List *list, void (*free_value)(void *value)) {
    ListNode *node = list->first;
    while (node != NULL) {
        ListNode *next_node = node->next;
        if (free_value != NULL) {
            free_value(node->value);
        }
        list_dealloc(node);
        node = next_node;
    }
    list_init(list);
}


int list_append(List *list, void *value) {
    ListNode *node = new_node(value);
    if (node == NULL) {
        return 1;
    }

    if (list->length == 0) {
        list->first = node;
    }
    else {
        list->last->next = node;
    }
    list->last = node;

    list->length++;
    return 0;
}


int list_prepend(List *list, void *value) {
    ListNode *node = new_node(value);
    if (node == NULL) {
        return 1;
    }

    node->next = list->first;
    list->first = node;
    if (list->length == 0) {
        list->last = node;
    }

    list->length++;
    return 0;
}


int list_insert(List *list, size_t index, void *value) {
    ListNode *node;
    ListNode *inserted_node;
    size_t i;

    if (index > list->length) {
        return 1;
    }

    if (index == 0) {
        return list_prepend(list, value);
    }

    if (index == list->length) {
        return list_append(list, value);
    }

    inserted_node = new_node(value);
    if (inserted_node == NULL) {
        return 1;
    }

    node = list->first;
    for (i = 0; i < index-1; i++) {
        node = node->next;
    }

    inserted_node->next = node->next;
    node->next = inserted_node;

    list->length++;
    return 0;
}


void *list_peekfront(const List *list) {
    return list->first != NULL ? list->first->value : NULL;
}


void *list_popfront(List *list) {
    return list_remove_next(list, NULL);
}


void *list_remove_next(List *list, ListNode *prev) {
    ListNode *removed = prev != NULL ? prev->next : list->first;
    void *value;

    if (removed == NULL) {
        return NULL;
    }

    if (prev != NULL) {
        prev->next = removed->next;
    }
    else {
        list->first = removed->next;
    }
    if (list->last == removed) {
        list->last = prev;
    }

    value = removed->value;
    list_dealloc(removed);
    list->length--;
    return value;
}


void list_transfer_front(List *dst, List *src) {
    ListNode *node = src->first;

    if (node == NULL) {
        return;
    }

    src->first = node->next;
    if (src->last == node) {
        src->last = NULL;
    }
    src->length--;

    node->next = NULL;
    if (dst->length == 0) {
        dst->first = node;
    }
    else {
        dst->last->next = node;
    }
    dst->last = node;
    dst->length++;
}
