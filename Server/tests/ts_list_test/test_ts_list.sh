gcc -o test_ts_list.o ts_list_test.c ../../ts_list.c ../../tuple.c -I. -std=c99 -Wall
./test_ts_list.o
rm -rf test_ts_list.o