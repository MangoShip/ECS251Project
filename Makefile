FILE_NAME1 := thread_gtod
FILE_NAME2 := thread_clock_gettime

all: thread_gtod thread_clock_gettime

thread_gtod: $(FILE_NAME1).c
	gcc $(DEBUG) $(FILE_NAME1).c -o $(FILE_NAME1)

thread_clock_gettime: $(FILE_NAME2).c
	gcc $(DEBUG) $(FILE_NAME2).c -o $(FILE_NAME2)

debug: DEBUG = -DDEBUG

debug: thread_gtod thread_clock_gettime

clean:
	rm -f $(FILE_NAME1) $(FILE_NAME2)