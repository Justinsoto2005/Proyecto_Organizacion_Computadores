CC = gcc
CFLAGS = -Wall -Wextra -g

all: simulador

simulador: main.o pipeline.o tests.o
	$(CC) $(CFLAGS) -o simulador main.o pipeline.o tests.o

main.o: main.c
	$(CC) $(CFLAGS) -c main.c

pipeline.o: pipeline.c pipeline.h
	$(CC) $(CFLAGS) -c pipeline.c

tests.o: tests.c pipeline.h
	$(CC) $(CFLAGS) -c tests.c

clean:
	rm -f *.o simulador