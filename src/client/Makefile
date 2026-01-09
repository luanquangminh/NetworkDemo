# Client module Makefile - Optimized with dependency tracking
CC = gcc
CFLAGS = -Wall -Wextra -pthread -I. -I../common -I../../lib/cJSON
# Enable automatic dependency generation for incremental builds
DEPFLAGS = -MMD -MP
LDFLAGS = -L../common
LIBS = -lcommon -lpthread

# Source files
SRCS = main.c client.c net_handler.c
OBJS = $(SRCS:.c=.o)
DEPS = $(OBJS:.o=.d)

# Target binary
TARGET = ../../build/client

all: $(TARGET)

$(TARGET): $(OBJS) ../common/libcommon.a
	mkdir -p ../../build
	$(CC) $(OBJS) $(LDFLAGS) $(LIBS) -o $@

# Compile with automatic dependency generation
%.o: %.c
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

# Include automatically generated dependencies (if they exist)
-include $(DEPS)

clean:
	rm -f $(OBJS) $(TARGET) $(DEPS)

.PHONY: all clean
