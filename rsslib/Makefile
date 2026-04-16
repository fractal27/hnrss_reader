CC = gcc
CFLAGS = -Wall -Wextra

.PHONY: all clean test

all: test_rsslib

test_rsslib: test_rsslib.c rsslib.c rsslib.h
	$(CC) $(CFLAGS) -o test_rsslib test_rsslib.c rsslib.c

test: test_rsslib
	./test_rsslib

clean:
	rm -f test_rsslib