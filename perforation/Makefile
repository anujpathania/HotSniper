CC = gcc
CFLAGS = -Wall

all: test

test: test.c
	$(CC) $(CFLAGS) -o test -lm test.c perforation.c

clean:
	rm -f test