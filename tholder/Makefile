# Source and object files
SRC_DIR = src
OBJ_DIR = obj
INC_DIR = include
SOURCES = $(wildcard $(SRC_DIR)/*.c)
OBJECTS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SOURCES))

# Executable name and location
EXEC = main
EXEC_DIR = target

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -g -I$(INC_DIR)

# Targets
all: $(EXEC)

$(EXEC): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(EXEC_DIR)/$(EXEC) $(OBJECTS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

clean:
	rm -rf $(OBJ_DIR) $(EXEC_DIR)/$(EXEC)

.PHONY: all clean
