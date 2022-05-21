CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -pedantic -O2

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

all: bfc

bfc: bfc.o
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -f bfc bfc.o
