# Tests Makefile - Optimized with dependency tracking
CC = gcc
CFLAGS = -Wall -Wextra -pthread -I../src/common -I../src/database -I../lib/cJSON -I/opt/homebrew/opt/openssl@3/include
# Enable automatic dependency generation for incremental builds
DEPFLAGS = -MMD -MP
LDFLAGS = -L../src/common -L../src/database -L/opt/homebrew/opt/openssl@3/lib
LIBS = -lcommon -ldatabase -lsqlite3 -lpthread -lcrypto

# Test binaries
TEST_PROTOCOL = test_protocol
TEST_DB = test_db
OBJS = test_protocol.o test_db.o
DEPS = $(OBJS:.o=.d)

all: $(TEST_PROTOCOL) $(TEST_DB)

$(TEST_PROTOCOL): test_protocol.o ../src/common/libcommon.a
	$(CC) $< $(LDFLAGS) $(LIBS) -o $@

$(TEST_DB): test_db.o ../src/common/libcommon.a ../src/database/libdatabase.a
	$(CC) $< $(LDFLAGS) $(LIBS) -o $@

# Compile with automatic dependency generation
%.o: %.c
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

# Include automatically generated dependencies (if they exist)
-include $(DEPS)

clean:
	rm -f *.o $(TEST_PROTOCOL) $(TEST_DB) $(DEPS)

.PHONY: all clean
