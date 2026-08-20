# Compilador y flags:
# -Wall -Wextra: activan advertencias extra del compilador (buena práctica)
# -g: incluye información de depuración (para usar gdb, por ejemplo)
CC = gcc
CFLAGS = -Wall -Wextra -g

# Objetivo por defecto: compilar el ejecutable "simulador"
all: simulador

# Enlaza los tres archivos objeto (.o) en el ejecutable final
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
