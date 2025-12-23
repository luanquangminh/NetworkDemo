# Database module Makefile
CC = gcc
CFLAGS = -Wall -Wextra -pthread -I. -I../common -I../../lib/cJSON
AR = ar
ARFLAGS = rcs

# Source files
SRCS = db_manager.c
OBJS = $(SRCS:.c=.o)

# Target library
LIB = libdatabase.a

all: $(LIB)

$(LIB): $(OBJS)
	$(AR) $(ARFLAGS) $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(LIB)

.PHONY: all clean
