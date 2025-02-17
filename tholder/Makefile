CC := gcc
CFLAGS := -g -Wall -Wpedantic
OBJECTS := tholder.o

all: tholder_main pthread_main

tholder_main: $(OBJECTS) tholder_main.c
	$(CC) $(CFLAGS) $^ -o $@

pthread_main: pthread_main.c
	$(CC) $(CFLAGS) $^ -o $@


$(OBJECTS): %.o: %.c
	$(CC) $(CFLAGS) -c $^ -o $@

clean:
	rm -f *.o tholder_main pthread_main