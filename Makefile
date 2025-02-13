# NOTE: This version of Makefile only creates an executable
#       with a single source file (that has the same name)
#       If you want to have multiple source files for one
#       executable, you need to modify this Makefile.

# Name of binary file
PRODUCT := thread thread_gtod thread_clock_gettime

# Flags for debug mode
DEBUG_FLAGS := -g -DDEBUG -O0

# Flags for release mode
RELEASE_FLAGS := -O3 -DNDEBUG

# In default, compile in release mode
# To enable debug mode, call the following: make DEBUG=1
ifeq ($(DEBUG),1)
	CFLAGS := $(DEBUG_FLAGS)
else 
	CFLAGS := $(RELEASE_FLAGS)
endif

all: $(PRODUCT)

$(PRODUCT): %: %.c
	gcc ${CFLAGS} -o $@ $<

clean:
	rm -f $(PRODUCT)