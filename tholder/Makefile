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

ifdef DEBUG
	CFLAGS += -O0 -g -DDEBUG
else
	CFLAGS += -O3 -DNDEBUG
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
