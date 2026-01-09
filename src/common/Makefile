# Common module Makefile - Optimized with dependency tracking
CC = gcc
CFLAGS = -Wall -Wextra -pthread -I. -I../../lib/cJSON -I/opt/homebrew/opt/openssl@3/include
# Enable automatic dependency generation for incremental builds
DEPFLAGS = -MMD -MP
AR = ar
ARFLAGS = rcs

# Source files
SRCS = protocol.c utils.c crypto.c ../../lib/cJSON/cJSON.c
OBJS = $(SRCS:.c=.o)
DEPS = $(OBJS:.o=.d)

# Target library
LIB = libcommon.a

all: $(LIB)

$(LIB): $(OBJS)
	$(AR) $(ARFLAGS) $@ $^

# Compile with automatic dependency generation
# -MMD generates dependency files, -MP adds phony targets for headers
%.o: %.c
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

# Include automatically generated dependencies (if they exist)
-include $(DEPS)

clean:
	rm -f $(OBJS) $(LIB) $(DEPS)

.PHONY: all clean
