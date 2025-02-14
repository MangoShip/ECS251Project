# Compiler
CC = gcc

# Compiler Flags

CFLAGS = -Wall -Wextra -O3 -march=native -pthread -lm

TARGET = cholesky

SRCS = cholesky.c

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS) -lm


run:
	@if [ -z "$(N)" ] || [ -z "$(T)"]; then \
		echo "Usage: make run N=<matrix_size> T=<num_threads>"; \
	else \
		./$(TARGET) $(N) $(T); \
	fi 
 
clean: 
	rm -f $(TARGET)


