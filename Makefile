OBJMODULES = flag.o
CFLAGS = -g -Wall
CC = gcc

example: example.c $(OBJMODULES)
	$(CC) $(CFLAGS) $^ -o $@

flag.o: flag.c flag.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o example
