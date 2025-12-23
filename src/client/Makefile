# Client module Makefile
CC = gcc
CFLAGS = -Wall -Wextra -pthread -I. -I../common -I../../lib/cJSON
LDFLAGS = -L../common
LIBS = -lcommon -lpthread

# Source files
SRCS = main.c client.c net_handler.c
OBJS = $(SRCS:.c=.o)

# Target binary
TARGET = ../../build/client

all: $(TARGET)

$(TARGET): $(OBJS) ../common/libcommon.a
	mkdir -p ../../build
	$(CC) $(OBJS) $(LDFLAGS) $(LIBS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
