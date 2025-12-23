# Root Makefile for File Sharing System
# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -pthread -Isrc/common -Isrc/database -Ilib/cJSON
LIBS = -lsqlite3 -lpthread -lcrypto

# Directories
SRC_COMMON = src/common
SRC_DATABASE = src/database
SRC_SERVER = src/server
SRC_CLIENT = src/client
TESTS = tests
BUILD = build

# Targets
SERVER_BIN = $(BUILD)/server
CLIENT_BIN = $(BUILD)/client

.PHONY: all server client gui tests clean run-server run-client run-gui help

# Default target
all: server client gui

# Build server
server:
	@echo "Building common library..."
	@$(MAKE) -C $(SRC_COMMON)
	@echo "Building database library..."
	@$(MAKE) -C $(SRC_DATABASE)
	@echo "Building server..."
	@$(MAKE) -C $(SRC_SERVER)
	@echo "Server built successfully: $(SERVER_BIN)"

# Build client
client:
	@echo "Building common library..."
	@$(MAKE) -C $(SRC_COMMON)
	@echo "Building client..."
	@$(MAKE) -C $(SRC_CLIENT)
	@echo "Client built successfully: $(CLIENT_BIN)"

# Build GUI client
gui:
	@echo "Building common library..."
	@$(MAKE) -C $(SRC_COMMON)
	@echo "Building client library..."
	@$(MAKE) -C $(SRC_CLIENT) client.o net_handler.o
	@echo "Building GTK GUI client..."
	@$(MAKE) -C $(SRC_CLIENT)/gui
	@echo "GUI client built successfully: $(BUILD)/gui_client"

# Build tests
tests:
	@echo "Building common library..."
	@$(MAKE) -C $(SRC_COMMON)
	@echo "Building database library..."
	@$(MAKE) -C $(SRC_DATABASE)
	@echo "Building tests..."
	@$(MAKE) -C $(TESTS)
	@echo "Tests built successfully"

# Clean all build artifacts
clean:
	@echo "Cleaning build artifacts..."
	@$(MAKE) -C $(SRC_COMMON) clean
	@$(MAKE) -C $(SRC_DATABASE) clean
	@$(MAKE) -C $(SRC_SERVER) clean
	@$(MAKE) -C $(SRC_CLIENT) clean
	@$(MAKE) -C $(SRC_CLIENT)/gui clean 2>/dev/null || true
	@$(MAKE) -C $(TESTS) clean
	@rm -rf $(BUILD)
	@echo "Clean complete"

# Run server (default port 8080)
run-server: server
	@echo "Starting server on port 8080..."
	@./$(SERVER_BIN) 8080

# Run client (default localhost:8080)
run-client: client
	@echo "Starting client..."
	@./$(CLIENT_BIN) localhost 8080

# Run GUI client
run-gui: gui
	@echo "Starting GUI client..."
	@./$(BUILD)/gui_client

# Help target
help:
	@echo "File Sharing System - Build Targets"
	@echo "===================================="
	@echo "  make all         - Build both server and client (default)"
	@echo "  make server      - Build server only"
	@echo "  make client      - Build client only"
	@echo "  make tests       - Build test suite"
	@echo "  make clean       - Remove all build artifacts"
	@echo "  make run-server  - Build and run server on port 8080"
	@echo "  make run-client  - Build and run client (connect to localhost:8080)"
	@echo "  make help        - Show this help message"
