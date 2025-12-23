# Common module Makefile
CC = gcc
CFLAGS = -Wall -Wextra -pthread -I. -I../../lib/cJSON -I/opt/homebrew/opt/openssl@3/include
AR = ar
ARFLAGS = rcs

# Source files
SRCS = protocol.c utils.c crypto.c ../../lib/cJSON/cJSON.c
OBJS = $(SRCS:.c=.o)

# Target library
LIB = libcommon.a

all: $(LIB)

$(LIB): $(OBJS)
	$(AR) $(ARFLAGS) $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(LIB)

.PHONY: all clean
