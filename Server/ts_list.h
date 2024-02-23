#ifndef _TSLIST_H
#define _TSLIST_H

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "tuple.h"

#define TSLIST_NOTFND -1

typedef struct tslist_node {
    tuple_t tuple;
    struct tslist_node* next;
} tslist_node_t;

typedef struct {
    tslist_node_t* head;
    size_t size;
} tslist_t;

//* Return new, empty list */
tslist_t tslist_new();

//* Clear the list, freeing all the elements. */
//! ALWAYS CALL AT THE END!
void tslist_free(tslist_t* self);

//* Add the tuple to the front of the list. */
//? The equivalent of `ts_out()` function.
void tslist_add(tslist_t* self, tuple_t tuple);

//* Remove node at provided [index]. */
tuple_t tslist_remove(tslist_t* self, uint32_t index);

//* Remove node that matches provided node. */
//* Return: index of the removed node on success */
//* or -1 on failure */
uint32_t tslist_remove_matching(tslist_t* self, tuple_t* tuple);

//* Find provided tuple with tuple template. */
//* Return: index of the tuple in the list on success */
//* or [TSLIST_NOTFND] (0) otherwise. */
//? The equivalent of `ts_rd()` and `ts_rdp()` functions.
uint32_t tslist_find(tslist_t* self, tuple_t* tuple);

//* Find and withdraw provided tuple with tuple template. */
//* Return: index of the tuple in the list on success */
//* or [TSLIST_NOTFND] (0) otherwise. */
//? The equivalent of `ts_in()` and `ts_inp()` functions.
uint32_t tslist_withdraw(tslist_t* self, tuple_t* tuple);

//* Print the list. */
void tslist_print(tslist_t* self);

#endif