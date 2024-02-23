#include <stdlib.h>
#include <stdio.h>

#include "tuple.h"


//! TEST 
void tuple_test(void) {
    printf("\nTUPLE FUNCTIONS:\n");
    printf("Size of a tuple: %ld bytes (%ld bits)\n", sizeof(tuple_t), sizeof(tuple_t) * 8);
    printf("Size of a tuple field: %ld bytes (%ld bits)\n", sizeof(tuple_field_t), sizeof(tuple_field_t) * 8);
    tuple_t t1 = tuple_new("test1", 3);
    tuple_insert_int(&t1, 0, 1);
    tuple_insert_float(&t1, 1, 1.0f);

    tuple_print(&t1);

    tuple_insert_int(&t1, 2, 2137);

    tuple_print(&t1);

    printf("t1[0] = %d\n", tuple_get(&t1, 0).data._int);
    printf("t1[1] = %f\n", tuple_get(&t1, 1).data._float);
    printf("t1[2] = %d\n", tuple_get(&t1, 2).data._int);

    printf("\nTUPLE FROM STRING:\n");
    printf("Testing tuple: (\"test2\", float 3.1415, int 69, float ?) (HARDCODED STRING)\n");
    tuple_t t2 = tuple_from_string("(\"test2\", float 3.1415, int 69, float ?)");
    printf("Parsed tuple:  ");
    tuple_print(&t2);
}

int main(void) {
    tuple_test();       
    return 0;
}