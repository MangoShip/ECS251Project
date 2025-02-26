# Directories
SRC_DIR     = .
INC_DIR     = ../tholder
LIB_DIR     = ../tholder/lib
OBJ_DIR     = obj
TARGET_DIR  = target
OPENMP_FLAG ?= 

# List of targets (each target should have a corresponding .c file in SRC_DIR)
TARGETS = cholesky_openmp cholesky_parallel cholesky_serial

# Compiler settings
CC      = gcc
CFLAGS  = -Wall -Wextra -g -I$(INC_DIR)
LFLAGS  = -L$(LIB_DIR) -ltholder -fopenmp -lm

# Derived lists:
# Object files for each target (for main1, the corresponding object file is obj/main1.o, etc.)
OBJECTS = $(addprefix $(OBJ_DIR)/, $(addsuffix .o, $(TARGETS)))
# Executable paths in TARGET_DIR.
EXECUTABLES = $(addprefix $(TARGET_DIR)/, $(TARGETS))

# Default target: build all executables.
all: $(EXECUTABLES)

# Pattern rule to compile each source file into its corresponding object.
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Pattern rule to link the object file into the executable.
$(TARGET_DIR)/%: $(OBJ_DIR)/%.o | $(TARGET_DIR)
	$(CC) $(CFLAGS) -o $@ $< $(LFLAGS) $(OPENMP_FLAG)

# Create the object and target directories if they don't exist.
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(TARGET_DIR):
	mkdir -p $(TARGET_DIR)

clean:
	rm -rf $(OBJ_DIR) $(TARGET_DIR)

.PHONY: all clean
