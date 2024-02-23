gcc -o server.o server.c tuple_space.c ts_list.c tuple.c -I. -std=c99 -Wall -lc
./server.o
rm -rf server.o