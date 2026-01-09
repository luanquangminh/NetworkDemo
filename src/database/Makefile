# Database module Makefile - Optimized with dependency tracking
CC = gcc
CFLAGS = -Wall -Wextra -pthread -I. -I../common -I../../lib/cJSON
# Enable automatic dependency generation for incremental builds
DEPFLAGS = -MMD -MP
AR = ar
ARFLAGS = rcs

# Source files
SRCS = db_manager.c
OBJS = $(SRCS:.c=.o)
DEPS = $(OBJS:.o=.d)

# Target library
LIB = libdatabase.a

all: $(LIB)

$(LIB): $(OBJS)
	$(AR) $(ARFLAGS) $@ $^

# Compile with automatic dependency generation
%.o: %.c
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

# Include automatically generated dependencies (if they exist)
-include $(DEPS)

clean:
	rm -f $(OBJS) $(LIB) $(DEPS)

.PHONY: all clean
