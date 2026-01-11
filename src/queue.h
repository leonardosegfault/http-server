typedef struct LinkedList {
    struct LinkedListItem* first;
    struct LinkedListItem* last;
} linked_list_t;

typedef struct LinkedListItem {
    void* value;
    struct LinkedListItem* prev;
    struct LinkedListItem* next;
} linked_list_item_t;

linked_list_t* ll_new();

linked_list_item_t* ll_new_item(void* value);

void ll_insert(linked_list_t* list, linked_list_item_t* item);

void ll_remove(linked_list_t* list, linked_list_item_t* item);