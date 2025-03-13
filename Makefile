CC = gcc
CFLAGS = -Wall -g
LIB = -lm

SRC = allocate.c queues.c

OBJ = $(SRC:.c=.o)

allocate: allocate.o queues.o
	$(CC) $(CFLAGS) -o allocate $(OBJ) $(LIB)

%.o: %.c %.h
	$(CC) -c -o $@ $< $(CFLAGS)

clean:
	rm -f $(OBJ) allocate queues