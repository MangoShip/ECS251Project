FILE_NAME1 := thread
FILE_NAME2 := thread_gtod
FILE_NAME3 := thread_clock_gettime

all: thread thread_gtod thread_clock_gettime

thread: $(FILE_NAME1).c
	gcc $(DEBUG) $(FILE_NAME1).c -o $(FILE_NAME1)

thread_gtod: $(FILE_NAME2).c
	gcc $(DEBUG) $(FILE_NAME2).c -o $(FILE_NAME2)

thread_clock_gettime: $(FILE_NAME3).c
	gcc $(DEBUG) $(FILE_NAME3).c -o $(FILE_NAME3)

debug: DEBUG = -DDEBUG

debug: thread thread_gtod thread_clock_gettime

clean:
	rm -f $(FILE_NAME1) $(FILE_NAME2) $(FILE_NAME3)