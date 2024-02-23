gcc -o client_test.o client_test.c tuple_space.c ts_list.c tuple.c -I. -std=c99 -Wall
./client_test.o
rm -rf client_test.o