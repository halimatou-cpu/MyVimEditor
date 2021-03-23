CC=gcc
CFLAGS=-W -Werror -Wall -g -std=gnu99
EXEC=miniVim
OBJECTS=terminal.o main.o

.PHONY: default clean
default: $(EXEC)

terminal.o: terminal.c terminal.h
	$(CC) -o terminal.o -c terminal.c $(CFLAGS)

main.o: main.c
	$(CC) -o main.o -c main.c $(CFLAGS)

$(EXEC): $(OBJECTS)
	$(CC) -o $(EXEC) $(OBJECTS)

clean:
	rm $(OBJECTS)
