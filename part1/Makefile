CC = gcc

all: empty part1

empty: empty.c
	$(CC) -o empty empty.c

part1: part1.c
	$(CC) -o part1 part1.c

traces: empty part1
	strace -o empty.trace ./empty
	strace -o part1.trace ./part1

clean:
	rm -f empty part1

.PHONY: all traces clean
