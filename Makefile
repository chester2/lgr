CC = gcc
CFLAGS = -Wall -Werror -Werror=vla -Wextra -pedantic -std=c99 -Isrc -g3 -ggdb

CFILES = date.c entry.c
OBJS = $(CFILES:%.c=obj/%.o)


test: test/date.c $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $< -o bin/$@

date: obj/date.o
obj/date.o: src/date.c src/date.h
	$(CC) $(CFLAGS) -c $< -o $@

entry: obj/entry.o
obj/entry.o: src/entry.c src/entry.h src/date.h
	$(CC) $(CFLAGS) -c $< -o $@



.PHONY: clean

clean:
	rm -f obj/*.o