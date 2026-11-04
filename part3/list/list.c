#include <stdlib.h>
#include "list.h"


ListNode *new_node(void *value) {
    ListNode *node = malloc(sizeof(ListNode));
    node->value = value;
    node->next = NULL;
}


List* list_create() {
    List *list = malloc(sizeof(List));

    list->length = 0;
    list->first = NULL;
    list->last = NULL;

    return list;
}


void list_free(List *list) {
    ListNode *node = list->first;
    while (node != NULL) {
        ListNode *next_node = node->next;
        free(node);
        node = next_node; 
    }
    free(list);
}


void list_append(List *list, void *value) {
    if (list->length == 0) {
        list->first = new_node(value);
        list->last = list->first;
    }
    else {
        list->last->next = new_node(value);
    }

    list->length++;
}


void list_prepend(List *list, void *value) {
    ListNode *node = new_node(value);
    node->next = list->first;

    if (list->length == 0) {
        list->first = node;
        list->last = list->first;
    }
    else {
        list->first = node;
    }

    list->length++;
}


int list_insert(List *list, size_t index, void *value) {
    if (index > list->length) {
        return 1;
    }

    if (index == 0) {
        list_prepend(list, value);
        return 0;
    }

    if (index == list->length) {
        list_append(list, value);
        return 0;
    }
    
    ListNode *node = list->first;
    for (size_t i = 0; i < index-1; i++) {
        node = node->next;
    }

    ListNode *inserted_node = new_node(value);
    inserted_node->next = node->next;
    node->next = inserted_node;

    return 0;
}

void list_popfront(List *list) {
    ListNode *new_first = list->first->next;
    free(list->first);
    list->first = new_first;
    list->length--;
}
