
CC = gcc
CCFLAGS = -Wall -g

all: 4_2 clear

4_2: 4_1_coro.o 4_2_mythreads.o
	$(CC) $(CCFLAGS) 4_1_coro.o 4_2_mythreads.o -o 4_2
4_2_mythreads.o: 4_2_mythreads.c 4_1_coro.h 4_2_mythreads.h
	$(CC) $(CCFLAGS) -c 4_2_mythreads.c
4_1_coro.o: 4_1_coro.c 4_1_coro.h
	$(CC) $(CCFLAGS) -c 4_1_coro.c 

clear:
	rm *.o

.PHONY: clean
clean:
	rm 4_2
