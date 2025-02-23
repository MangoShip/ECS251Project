# -----------------------------------------------------------------------------
# Detect OS and define variables accordingly
# -----------------------------------------------------------------------------
ifeq ($(OS),Windows_NT)
  # Windows environment
  RM = del /Q
  EXE = .exe
  RUN_PARALLEL = cholesky_parallel$(EXE)
  RUN_SERIAL   = cholesky_serial$(EXE)
else
  # Unix-like environment (Linux, macOS, etc.)
  RM = rm -f
  EXE =
  RUN_PARALLEL = ./cholesky_parallel$(EXE)
  RUN_SERIAL   = ./cholesky_serial$(EXE)
endif

# -----------------------------------------------------------------------------
# Compiler and Flags
# -----------------------------------------------------------------------------
CC = gcc
CFLAGS = -Wall -Wextra -O3 -march=native
LDFLAGS = -lm
PARALLEL_FLAGS = -pthread

# -----------------------------------------------------------------------------
# Target Names
# -----------------------------------------------------------------------------
TARGET_PARALLEL = cholesky_parallel
TARGET_SERIAL   = cholesky_serial

# -----------------------------------------------------------------------------
# Source Files
# -----------------------------------------------------------------------------
SRCS_PARALLEL = cholesky_parallel.c
SRCS_SERIAL   = cholesky_serial.c

# -----------------------------------------------------------------------------
# Default rule: build both
# -----------------------------------------------------------------------------
all: $(TARGET_PARALLEL)$(EXE) $(TARGET_SERIAL)$(EXE)

# -----------------------------------------------------------------------------
# Build Parallel Implementation
# -----------------------------------------------------------------------------
$(TARGET_PARALLEL)$(EXE): $(SRCS_PARALLEL)
	$(CC) $(CFLAGS) $(PARALLEL_FLAGS) -o $(TARGET_PARALLEL)$(EXE) $(SRCS_PARALLEL) $(LDFLAGS)

# -----------------------------------------------------------------------------
# Build Serial Implementation
# -----------------------------------------------------------------------------
$(TARGET_SERIAL)$(EXE): $(SRCS_SERIAL)
	$(CC) $(CFLAGS) -o $(TARGET_SERIAL)$(EXE) $(SRCS_SERIAL) $(LDFLAGS)

# -----------------------------------------------------------------------------
# Run Targets
#   Usage:
#       make run_parallel ARGS="1000 8"
#       make run_serial ARGS="1000"
# -----------------------------------------------------------------------------

run_parallel:
ifeq ($(strip $(ARGS)),)
	@echo "Usage: make run_parallel ARGS=\"<matrix_size> <num_threads>\""
else
	@echo "Running: $(RUN_PARALLEL) $(ARGS)"
	$(RUN_PARALLEL) $(ARGS)
endif

run_serial:
ifeq ($(strip $(ARGS)),)
	@echo "Usage: make run_serial ARGS=\"<matrix_size>\""
else
	@echo "Running: $(RUN_SERIAL) $(ARGS)"
	$(RUN_SERIAL) $(ARGS)
endif

# -----------------------------------------------------------------------------
# Clean up
# -----------------------------------------------------------------------------
clean:
	$(RM) $(TARGET_PARALLEL)$(EXE) $(TARGET_SERIAL)$(EXE)
