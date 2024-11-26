CC=gcc
CFLAGS=-I.
PTHREAD=-pthread

all : parallel_min_max zombie process_memory parallel_sum

parallel_min_max : utils.o find_min_max.o utils.h find_min_max.h
	$(CC) -o parallel_min_max utils.o find_min_max.o parallel_min_max.c $(CFLAGS)

utils.o : utils.c utils.h
	$(CC) -c utils.c -o utils.o $(CFLAGS)

find_min_max.o : find_min_max.c utils.h find_min_max.h
	$(CC) -c find_min_max.c -o find_min_max.o $(CFLAGS)

zombie : zombie.c
	$(CC) -o zombie zombie.c $(CFLAGS)

process_memory : process_memory.c
	$(CC) -o process_memory process_memory.c $(CFLAGS)

parallel_sum : parallel_sum.c utils.o
	$(CC) -o parallel_sum parallel_sum.c utils.o $(CFLAGS) $(PTHREAD)

clean :
	rm -f utils.o find_min_max.o parallel_min_max zombie process_memory parallel_sum