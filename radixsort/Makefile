# Directories
SRC_DIR     = .
INC_DIR     = ../tholder
LIB_DIR     = ../tholder/lib
OBJ_DIR     = obj
TARGET_DIR  = target

# List of targets (each target should have a corresponding .c file in SRC_DIR)
TARGETS = radixsort_parallel radixsort_seq radixsort_tholder

# Compiler settings 
#
# NOTE: Linking against -fopenmp may result in slightly larger binary sizes, but this is fine
CC      = gcc
CFLAGS  = -Wall -Wextra -Wpedantic -I$(INC_DIR) -fopenmp 
LDFLAGS  = -L$(LIB_DIR) -ltholder -fopenmp

ifdef DEBUG
	CFLAGS += -O0 -g -DDEBUG
else
	CFLAGS += -O3 -DNDEBUG
endif

# Object files for each target (for main1, the corresponding object file is obj/main1.o, etc.)
OBJECTS = $(addprefix $(OBJ_DIR)/, $(addsuffix .o, $(TARGETS)))
# Executable paths in TARGET_DIR.
EXECUTABLES = $(addprefix $(TARGET_DIR)/, $(TARGETS))

# Default target: build all executables. To build a specific target, run `make $(TARGET_DIR)/my_executable`
all: $(EXECUTABLES)

# Pattern rule to compile each source file into its corresponding object.
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Pattern rule to link the object file into the executable.
$(TARGET_DIR)/%: $(OBJ_DIR)/%.o | $(TARGET_DIR)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

# Create the object and target directories if they don't exist.
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(TARGET_DIR):
	mkdir -p $(TARGET_DIR)

clean:
	rm -rf $(OBJ_DIR) $(TARGET_DIR)

.PHONY: all clean

