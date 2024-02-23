#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "tuple.h"

char buf[_TUPLE_BUF_SIZE];

//* Make a new tuple with [size] amount of fields. */
tuple_t tuple_new(const char* name, uint32_t size) {
    assert(size >= 0 && size <= TUPLE_MAX_SIZE);
    assert(strlen(name) >=0 && strlen(name) < TUPLE_NAME_SIZE);

    tuple_t tuple = {
        .size = size
    };

    strncpy(tuple.name, name, strlen(name));
    memset(tuple.fields, 0, sizeof(tuple.fields));

    for(int i=0; i<size; i++) {
        tuple_field_t field = {
            .occupied = TUPLE_OCCUPIED_NO,
            .tuple_type = TUPLE_TYPE_UNDEF
        };
        tuple.fields[i] = field;
    }

    return tuple;
}

//* Insert a tuple field of type int at a given position. */
//* Returns 0 on success or error code on failure (e.g. out of bonds error) */
uint32_t tuple_insert_int(tuple_t* tuple, uint32_t position, uint32_t value) {
    if (position < 0 || position >= tuple->size) {
        return -1;
    }

    tuple->fields[position].data._int = value;
    tuple->fields[position].tuple_type = TUPLE_TYPE_INT;
    tuple->fields[position].occupied = TUPLE_OCCUPIED_YES;

    return 0;
}

//* Insert a tuple field of type float at a given position. */
//* Returns 0 on success or error code on failure (e.g. out of bonds error) */
uint32_t tuple_insert_float(tuple_t* tuple, uint32_t position, float value) {
    if (position < 0 || position >= tuple->size) {
        return -1;
    }

    tuple->fields[position].data._float = value;
    tuple->fields[position].tuple_type = TUPLE_TYPE_FLOAT;
    tuple->fields[position].occupied = TUPLE_OCCUPIED_YES;

    return 0;
}

//* Get the value from the tuple at [position] */
tuple_field_t tuple_get(tuple_t* tuple, uint32_t position) {
    return tuple->fields[position];
}

//* Return a tuple_template created from a given tuple_string. */
//* tuple_string: a string with format: `( [type] [value],* )` */
//* Example: `("test", int ?, undefined)` */
tuple_t tuple_from_string(const char* tuple_string) {
    char token_buf[_TUPLE_BUF_SIZE];
    const char* sep = ", ";

    tuple_t tuple = {
        .size = 0
    };

    // remove 1st and last elements (the parentesis)
    strncpy(token_buf, tuple_string + 1, strlen(tuple_string) - 2);

    char* token;
    token = strtok(token_buf, sep);
    // printf("First token: %s\n", token);

    while(token != NULL) {
        size_t token_len = strlen(token);

        if(token[0] == '"' || token[0] == '\'' || token[0] == '`') {
            token[token_len-1] = '\0';
            strncpy(tuple.name, token + 1, token_len - 2);
            // tuple_print(&tuple);
        } else if(!strncmp(token, TUPLE_TYPE_INT_STR, strlen(TUPLE_TYPE_INT_STR))) {
            // printf("token: `%s`\n", token);
            tuple.fields[tuple.size].tuple_type = TUPLE_TYPE_INT;
            token = strtok(NULL, sep);
            // printf("token: `%s`\n", token);

            if(!strncmp(token, "?", 1)) {
                tuple.fields[tuple.size].occupied = TUPLE_OCCUPIED_NO;
            } else {
                tuple.fields[tuple.size].data._int = (uint32_t)atoi(token);
                tuple.fields[tuple.size].occupied  = TUPLE_OCCUPIED_YES;
            }

            tuple.size++;
            // printf("field %d: occupied: %d, type %d, value %d\n", tuple.size-1, tuple.fields[tuple.size-1].occupied, tuple.fields[tuple.size-1].tuple_type, tuple.fields[tuple.size-1].data._int);
            // tuple_print(&tuple);
        } else if(!strncmp(token, TUPLE_TYPE_FLOAT_STR, strlen(TUPLE_TYPE_FLOAT_STR))) {
            // printf("token: `%s`\n", token);
            tuple.fields[tuple.size].tuple_type = TUPLE_TYPE_FLOAT;
            token = strtok(NULL, sep);
            // printf("token: `%s`\n", token);

            if(!strncmp(token, "?", 1)) {
                tuple.fields[tuple.size].occupied  = TUPLE_OCCUPIED_NO;
            } else {
                tuple.fields[tuple.size].data._float = (float)atof(token);
                tuple.fields[tuple.size].occupied  = TUPLE_OCCUPIED_YES;
            }

            tuple.size++;
            // printf("field %d: occupied: %d, type %d, value %f\n", tuple.size-1, tuple.fields[tuple.size-1].occupied, tuple.fields[tuple.size-1].tuple_type, tuple.fields[tuple.size-1].data._float);
            // tuple_print(&tuple);
        } else if(!strncmp(token, TUPLE_TYPE_UNDEF_STR, strlen(TUPLE_TYPE_UNDEF_STR))) {
            tuple.fields[tuple.size].tuple_type = TUPLE_TYPE_UNDEF;
            tuple.fields[tuple.size].occupied   = TUPLE_OCCUPIED_NO;
            tuple.size++;
        }

        token = strtok(NULL, sep);
        // printf("Next token: `%s`\n", token);
    }

    return tuple;
}

const char* _tuple_field_to_string(tuple_field_t* tuple_field) {
    switch(tuple_field->tuple_type) {
        case TUPLE_TYPE_INT:
            if(tuple_field->occupied == TUPLE_OCCUPIED_NO) 
                return "int ?";
            snprintf(buf, sizeof(buf), "int %d", tuple_field->data._int);
            return buf;
        case TUPLE_TYPE_FLOAT:
            if(tuple_field->occupied == TUPLE_OCCUPIED_NO) 
                return "float ?";
            snprintf(buf, sizeof(buf), "float %f", tuple_field->data._float);
            return buf;
        case TUPLE_TYPE_UNDEF:
            return "undefined";
    }

    fprintf(stderr, "TUPLE UNSUPPORTED TYPE %d!", tuple_field->tuple_type);
    exit(0);
}

//* Check if the provided 2 tuples match. */
//* (match - tuples are identical (`?` matches anything)). */
bool tuple_is_match(tuple_t* t1, tuple_t* t2) {
    if(t1->size != t2->size) return false;
    if(strcmp(t1->name, t2->name)) return false;

    for(int i=0; i<t1->size; i++) {
        tuple_field_t t1f = t1->fields[i];
        tuple_field_t t2f = t2->fields[i];

        //* do types match? */
        if(t1f.tuple_type != t2f.tuple_type) return false;
        
        if(t1f.occupied == TUPLE_OCCUPIED_NO && t2f.occupied == TUPLE_OCCUPIED_NO) return false;
        
        //* do values match? (only if occupied) */
        if(t1f.occupied == TUPLE_OCCUPIED_YES && t2f.occupied == TUPLE_OCCUPIED_YES) {
            if(t1f.data._int != t2f.data._int) return false;
        }
        
    }
    return true;
}

//* Print the provided tuple to stdout. */
void tuple_print(tuple_t* tuple) {
    if(tuple->size == 0) {
        printf("(`%s`)\n", tuple->name);
        return;
    }

    printf("(\"%s\", ", tuple->name);
    for(int i=0; i<tuple->size-1; i++) {
        tuple_field_t field = tuple->fields[i];
        printf("%s, ", _tuple_field_to_string(&field));
    }
    tuple_field_t field = tuple->fields[tuple->size-1];
    printf("%s", _tuple_field_to_string(&field));
    printf(")\n");
}
