#include "ts_list.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//! TEST 
void test_tslist(void) {
    printf("###########################################\n");
    printf("#           TS_LIST TEST START            #\n");
    printf("###########################################\n\n");

    printf("Size of tuple: %lu bytes\nSize of tuple field: %lu bytes\n", sizeof(tuple_t), sizeof(tuple_field_t));
    
    tslist_t list = tslist_new();
    tuple_t t1 = tuple_from_string("(`456`, int 123, float 1.23)");
    tslist_add(&list, t1);
    printf("First element entered and pushed. Element: ");
    tuple_print(&t1);

    tuple_t t2 = tuple_from_string("(`456`, int 456, float 4.56)");
    tslist_add(&list, t2);
    printf("Second element entered and pushed. Element: ");
    tuple_print(&t2);

    tuple_t t3 = tuple_from_string("(`123`, int 789, float 7.89, int 2)");
    tslist_add(&list, t3);
    printf("Third element entered and pushed. Element: ");
    tuple_print(&t3);

    tuple_t template = tuple_from_string("(`456`, int ?, float 1.23)");
    printf("Template created: ");
    tuple_print(&template);

    printf("List: \n");
    tslist_print(&list);

    printf("FINDING:\n");
    uint32_t did_find = tslist_find(&list, &template);
    if(did_find) {
        printf("Found tuple: ");
    } else {
        printf("Couldn't find provided tuple: \n");
    }
    tuple_print(&template);

    printf("WITHDRAWING:\n");
    did_find = tslist_withdraw(&list, &template);
    if(did_find) {
        printf("Withdrawn tuple: ");
    } else {
        printf("Couldn't withdraw provided tuple: ");
    }
    tuple_print(&template);

    printf("List: \n");
    tslist_print(&list);

    printf("WITHDRAWING:\n");
    did_find = tslist_withdraw(&list, &template);
    if(did_find) {
        printf("Withdrawn tuple: ");
    } else {
        printf("Couldn't withdraw provided tuple: ");
    }
    tuple_print(&template);

    printf("List: \n");
    tslist_print(&list);

    tslist_remove(&list, 0);
    tslist_print(&list);
    tslist_remove(&list, 0);
    tslist_print(&list);


    tslist_free(&list);

    printf("\n###########################################\n");
    printf("#            TS_LIST TEST END             #\n");
    printf("###########################################\n");
}

int main(void) {
    test_tslist();

    return 0;
}