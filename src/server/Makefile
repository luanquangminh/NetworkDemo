# Server module Makefile
CC = gcc
CFLAGS = -Wall -Wextra -pthread -I. -I../common -I../database -I../../lib/cJSON -I/opt/homebrew/opt/openssl@3/include
LDFLAGS = -L../common -L../database -L/opt/homebrew/opt/openssl@3/lib
LIBS = -lcommon -ldatabase -lsqlite3 -lpthread -lcrypto

# Source files
SRCS = main.c server.c socket_mgr.c thread_pool.c commands.c storage.c permissions.c
OBJS = $(SRCS:.c=.o)

# Target binary
TARGET = ../../build/server

all: $(TARGET)

$(TARGET): $(OBJS) ../common/libcommon.a ../database/libdatabase.a
	mkdir -p ../../build
	$(CC) $(OBJS) $(LDFLAGS) $(LIBS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
