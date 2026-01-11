#include <stdlib.h>
#include <stdio.h>
#include "queue.h"

linked_list_t* ll_new() {
    return (linked_list_t*) malloc(sizeof(linked_list_t));
}

linked_list_item_t* ll_new_item(void* value) {
    linked_list_item_t* item = malloc(sizeof(linked_list_item_t));
    item->value = value;
    item->prev = NULL;
    item->next = NULL;

    return item;
}

void ll_insert(linked_list_t* list, linked_list_item_t* item) {
    if (list->first == NULL) {
        list->first = item;
    }

    if (list->last == NULL) {
        list->last = item;
    }

    list->last->next = item;
    list->last = item;
    item->next = NULL;
}

void ll_remove(linked_list_t* list, linked_list_item_t* item) {
    linked_list_item_t* prev = item->prev;
    linked_list_item_t* next = item->next;

    if (prev != NULL) {
        prev->next = next;
    }

    if (next != NULL) {
        next->prev = prev;
    }

    if (list->first == item) {
        list->first = NULL;
    }

    if (list->last == item) {
        list->last = NULL;
    }

    free(item);
}