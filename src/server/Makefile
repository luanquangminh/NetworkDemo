# Server module Makefile - Optimized with dependency tracking
CC = gcc
CFLAGS = -Wall -Wextra -pthread -I. -I../common -I../database -I../../lib/cJSON -I/opt/homebrew/opt/openssl@3/include
# Enable automatic dependency generation for incremental builds
DEPFLAGS = -MMD -MP
LDFLAGS = -L../common -L../database -L/opt/homebrew/opt/openssl@3/lib
LIBS = -lcommon -ldatabase -lsqlite3 -lpthread -lcrypto

# Source files
SRCS = main.c server.c socket_mgr.c thread_pool.c commands.c storage.c permissions.c
OBJS = $(SRCS:.c=.o)
DEPS = $(OBJS:.o=.d)

# Target binary
TARGET = ../../build/server

all: $(TARGET)

$(TARGET): $(OBJS) ../common/libcommon.a ../database/libdatabase.a
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
