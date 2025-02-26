# Files
LIB = libtholder.a
SOURCES = $(wildcard $(SRC_DIR)/*.c)
SRC_DIR = .
OBJECTS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SOURCES))

# Directories
LIB_DIR = lib
INCLUDE_DIR = include
OBJ_DIR = obj

# Compiler & linker flags
CC = gcc
CFLAGS = -Wall -Wpedantic -I$(INCLUDE_DIR)
DEBUG_FLAGS=-g -DDEBUG=true -O0
RELEASE_FLAGS=-O3

ifdef DEBUG
	CFLAGS += $(DEBUG_FLAGS)
else
	CFLAGS += $(RELEASE_FLAGS)
endif



all: $(LIB)


$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(LIB): $(LIB_DIR) $(OBJECTS)
	ar rcs $(LIB_DIR)/$(LIB) $(wildcard $(OBJECTS))

$(LIB_DIR):
	mkdir -p $(LIB_DIR)

clean:
	rm -rf $(OBJ_DIR) $(LIB_DIR)
