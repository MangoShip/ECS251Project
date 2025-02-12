FILE_NAME1 := radixsort_seq

all: radixsort_seq

radixsort_seq: $(FILE_NAME1).c
	gcc $(DEBUG) $(FILE_NAME1).c -o $(FILE_NAME1)

debug: DEBUG = -DDEBUG

debug: radixsort_seq

clean:
	rm -f $(FILE_NAME1)