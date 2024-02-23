gcc -o test_tuple.o tuple_test.c tuple.c -I. -std=c99 -Wall
./test_tuple.o
rm -rf test_tuple.o