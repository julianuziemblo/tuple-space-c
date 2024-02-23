#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/time.h>
#include <unistd.h>
#include <arpa/inet.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>

#include "tuple.h"
#include "tuple_space.h"

int main(void) {
    srand(time(NULL));

    tuple_t template = tuple_from_string("(`check_is_prime`, float 23)");
    ts_out(template);
    printf("Received tuple: ");
    tuple_print(&template);

    sleep(2);

    template = tuple_from_string("(`check_is_prime`, int 23)");
    ts_out(template);
    printf("Received tuple: ");
    tuple_print(&template);

    sleep(3);

    template = tuple_from_string("(`check_is_prime`, int 456)");
    ts_out(template);
    printf("Received tuple: ");
    tuple_print(&template);
    
    sleep(9);

    tuple_t tuple = tuple_from_string("(`check_is_prime`, int ?)");
    uint32_t in_res = ts_in(&tuple);
    printf("out status code: %d\n", in_res);
    printf("sent tuple: ");
    tuple_print(&tuple);



    return 0;
}