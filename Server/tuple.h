#ifndef _TUPLE_H
#define _TUPLE_H

#include <stdbool.h>
#include <stdint.h>

#define TUPLE_MAX_SIZE   4
#define TUPLE_NAME_SIZE  32
#define _TUPLE_BUF_SIZE  128

//* Tuple types */
#define TUPLE_TYPE_UNDEF 0b000
#define TUPLE_TYPE_UNDEF_STR "undefined"
#define TUPLE_TYPE_INT   0b001
#define TUPLE_TYPE_INT_STR "int"
#define TUPLE_TYPE_FLOAT 0b010
#define TUPLE_TYPE_FLOAT_STR "float"

#define TUPLE_OCCUPIED_YES    0b1
#define TUPLE_OCCUPIED_NO     0b0

// typedef unsigned int uint32_t;

typedef union {
    int32_t _int;
    float _float;
} _tuple_data_t;

typedef struct {
    uint32_t occupied:   1;
    uint32_t tuple_type: 3;
    uint32_t padding:    28;
    _tuple_data_t data;
} tuple_field_t;

typedef struct {
    char name[TUPLE_NAME_SIZE];
    uint32_t size;
    tuple_field_t fields[TUPLE_MAX_SIZE];
} tuple_t;

//* Make a new tuple with [size] amount of fields. */
tuple_t tuple_new(const char* name, uint32_t size);

//* Insert a tuple field of type int at a given position. */
//* Returns 0 on success or error code on failure (e.g. out of bonds error) */
uint32_t tuple_insert_int(tuple_t* tuple, uint32_t position, uint32_t value);

//* Insert a tuple field of type float at a given position. */
//* Returns 0 on success or error code on failure (e.g. out of bonds error) */
uint32_t tuple_insert_float(tuple_t* tuple, uint32_t position, float value);

//* Get the value from the tuple at [position]. */
tuple_field_t tuple_get(tuple_t* tuple, uint32_t position);

//* Return a tuple_template created from a given tuple_string. */
//* tuple_string: a string with format: `([name], [type] [value]/?, ...)` */
//* Example: `("test", int 123, float ?)` */
tuple_t tuple_from_string(const char* tuple_string);

//* Check if the provided 2 tuples match. */
//* (match - tuples are identical (`?` matches anything)). */
bool tuple_is_match(tuple_t* t1, tuple_t* t2);

//* Print the provided tuple to stdout. */
void tuple_print(tuple_t* tuple);

#endif
