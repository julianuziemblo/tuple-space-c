#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "tuple.h"
#include "ts_list.h"

//* Return new, empty list */
tslist_t tslist_new() {
    return (tslist_t) {
        .head = NULL,
        .size = 0
    };
}

//* Clear the list, freeing all the elements. */
//! ALWAYS CALL AT THE END!
void tslist_free(tslist_t* self) {
    tslist_node_t* current = self->head;
    while(current != NULL) {
        tslist_node_t* c = current;

        current = current->next;
        free(c);
    }
    self->size = 0;
}

//* Add the tuple to the front of the list */
void tslist_add(tslist_t* self, tuple_t tuple) {
    tslist_node_t* node = (tslist_node_t*)malloc(sizeof (tslist_node_t));
    node->tuple = tuple;
    node->next  = self->head;
    self->head = node;
    self->size++;
    // printf("HEAD is now: (%p): ", self->head);
    // tuple_print(&self->head->tuple);
}

//* Remove node at provided [index]. */
tuple_t tslist_remove(tslist_t* self, uint32_t index) {
    assert(index >= 0 && index < self->size);

    if(index == 0) {
        tuple_t tuple;
        tslist_node_t* current = self->head;

        tuple = current->tuple;
        self->head = self->head->next;

        free(current);
        self->size--;
        return tuple;
    }

    tslist_node_t* current = self->head;
    for(int i=0; i < (index-1); i++) {
        current = current->next;
    }

    tslist_node_t* previous = current;
    tslist_node_t* to_remove = current->next;

    previous->next = to_remove->next;

    tuple_t removed = to_remove->tuple;

    free(to_remove);

    self->size--;
    return removed;
}

//* Remove node that matches provided node. */
//* Return: index of the removed node on success */
//* or -1 on failure */
uint32_t tslist_remove_matching(tslist_t* self, tuple_t* tuple) {
    if(self->head == NULL) {
        return -1;
    }

    uint32_t counter = 0;
    tslist_node_t* current = self->head;

    // if(tuple_is_match(&current->tuple, tuple)) {
    //     if(tuple_is_match(&current->tuple, tuple)) {
    //         self->head = self->head->next;
    //         free(current);
    //         self->size--;
    //         return 0;
    //     }
    // }
    
    tslist_node_t* previous = current;

    while(current != NULL) {
        current = current->next;
        if(tuple_is_match(&previous->tuple, tuple)) {
            if(current != NULL)
                previous->next = current->next;
            else
                previous->next = NULL;
            
            self->size--;
            return counter;
        }
        counter++;
        previous = previous->next;
    }

    return -1;
}

//* Find provided tuple with tuple template. */
//* Return: index of the tuple in the list on success */
//* or [TSLIST_NOTFND] (0) otherwise. */
//? The equivalent of `ts_rd()` and `ts_rdp()` functions.
uint32_t tslist_find(tslist_t* self, tuple_t* tuple) {
    tslist_node_t* current = self->head;
    int counter = 0;

    while(current != NULL) {
        if(tuple_is_match(&current->tuple, tuple)) {
            *tuple = current->tuple;
            return counter;
        }

        counter++;
        current = current->next;
    }

    return TSLIST_NOTFND;
}

//* Find and withdraw provided tuple with tuple template. */
//* Return: index of the tuple in the list on success */
//* or [TSLIST_NOTFND] (0) otherwise. */
//? The equivalent of `ts_in()` and `ts_inp()` functions.
uint32_t tslist_withdraw(tslist_t* self, tuple_t* tuple) {
    tslist_node_t* current  = self->head;
    int counter = 0;

    while(current != NULL) {
        if(tuple_is_match(&current->tuple, tuple)) {
            *tuple = tslist_remove(self, counter);
            return counter;
        }
        counter++;
        current = current->next;
    }

    return TSLIST_NOTFND;
}

//* Prints the list. */
void tslist_print(tslist_t* self) {
    printf("Tuple Space List {\n");
    printf("\tHEAD at %p\n", self->head);
    printf("\tSize: %ld\n", self->size);
    if (self->head == NULL) {
        printf("\t[]\n}\n");
        return;
    }
    tslist_node_t* current = self->head;
    uint32_t counter = 0;

    while(current != NULL) {
        printf("\ttslist[%d]: ", counter);
        tuple_print(&current->tuple);

        current = current->next;
        counter++;
    }
    printf("}\n");
}