# Tests Makefile
CC = gcc
CFLAGS = -Wall -Wextra -pthread -I../src/common -I../src/database -I../lib/cJSON -I/opt/homebrew/opt/openssl@3/include
LDFLAGS = -L../src/common -L../src/database -L/opt/homebrew/opt/openssl@3/lib
LIBS = -lcommon -ldatabase -lsqlite3 -lpthread -lcrypto

# Test binaries
TEST_PROTOCOL = test_protocol
TEST_DB = test_db

all: $(TEST_PROTOCOL) $(TEST_DB)

$(TEST_PROTOCOL): test_protocol.o ../src/common/libcommon.a
	$(CC) $< $(LDFLAGS) $(LIBS) -o $@

$(TEST_DB): test_db.o ../src/common/libcommon.a ../src/database/libdatabase.a
	$(CC) $< $(LDFLAGS) $(LIBS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o $(TEST_PROTOCOL) $(TEST_DB)

.PHONY: all clean
