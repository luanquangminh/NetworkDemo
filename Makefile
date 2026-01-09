# ============================================================================
# File Sharing System - Root Makefile (Optimized for Parallel Builds)
# ============================================================================
# Optimizations:
# - Parallel compilation enabled (automatically uses all CPU cores)
# - Automatic dependency tracking (.d files)
# - Improved incremental builds
# - Eliminated redundant recursive make calls
# - Build directory order-only prerequisites
# ============================================================================

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -pthread -Isrc/common -Isrc/database -Ilib/cJSON
LIBS = -lsqlite3 -lpthread -lcrypto

# Parallel build configuration
# Automatically detect number of CPU cores for parallel builds
NPROCS := $(shell sysctl -n hw.ncpu 2>/dev/null || echo 1)
MAKEFLAGS += --jobs=$(NPROCS) --output-sync=target

# Directories
SRC_COMMON = src/common
SRC_DATABASE = src/database
SRC_SERVER = src/server
SRC_CLIENT = src/client
TESTS = tests
BUILD = build

# Runtime configuration
DEFAULT_PORT = 8080
DEFAULT_HOST = localhost

# Build targets
SERVER_BIN = $(BUILD)/server
CLIENT_BIN = $(BUILD)/client
GUI_CLIENT_BIN = $(BUILD)/gui_client

# Libraries (for dependency tracking)
COMMON_LIB = $(SRC_COMMON)/libcommon.a
DATABASE_LIB = $(SRC_DATABASE)/libdatabase.a

# ============================================================================
# Phony targets
# ============================================================================
.PHONY: all server client gui tests clean \
        run-server run-client run-gui run-both \
        stop-server stop-client stop-gui stop-all \
        restart-server restart-gui restart-all \
        status check-deps help \
        docker-build docker-run docker-stop docker-restart \
        docker-logs docker-clean docker-shell docker-status

# ============================================================================
# Default target
# ============================================================================
all: server client gui

# ============================================================================
# Build targets (optimized for parallel execution)
# ============================================================================

# Build server - now uses explicit dependency on libraries for parallel builds
# This allows make to build common and database libraries in parallel
server: $(COMMON_LIB) $(DATABASE_LIB) | $(BUILD)
	@echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
	@echo "Building Server"
	@echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
	@$(MAKE) -C $(SRC_SERVER)
	@echo "✓ Server built: $(SERVER_BIN)"
	@echo ""

# Build client - explicit dependency on common library
client: $(COMMON_LIB) | $(BUILD)
	@echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
	@echo "Building CLI Client"
	@echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
	@$(MAKE) -C $(SRC_CLIENT)
	@echo "✓ Client built: $(CLIENT_BIN)"
	@echo ""

# Build GUI client - explicit dependencies
gui: $(COMMON_LIB) | $(BUILD)
	@echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
	@echo "Building GTK GUI Client"
	@echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
	@$(MAKE) -C $(SRC_CLIENT) client.o net_handler.o
	@$(MAKE) -C $(SRC_CLIENT)/gui
	@echo "✓ GUI client built: $(GUI_CLIENT_BIN)"
	@echo ""

# Build tests - explicit dependencies on both libraries
tests: $(COMMON_LIB) $(DATABASE_LIB) | $(BUILD)
	@echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
	@echo "Building Tests"
	@echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
	@$(MAKE) -C $(TESTS)
	@echo "✓ Tests built"
	@echo ""

# Library build rules - these can be built in parallel
$(COMMON_LIB):
	@$(MAKE) -C $(SRC_COMMON)

$(DATABASE_LIB):
	@$(MAKE) -C $(SRC_DATABASE)

# Create build directory
$(BUILD):
	@mkdir -p $(BUILD)

# ============================================================================
# Run targets
# ============================================================================

# Run server
run-server: server
	@echo "Starting server on port $(DEFAULT_PORT)..."
	@./$(SERVER_BIN) $(DEFAULT_PORT)

# Run CLI client
run-client: client
	@echo "Starting CLI client..."
	@./$(CLIENT_BIN) $(DEFAULT_HOST) $(DEFAULT_PORT)

# Run GUI client
run-gui: gui
	@echo "Starting GUI client..."
	@./$(GUI_CLIENT_BIN)

# Run server and GUI client together (in background)
run-both: server gui
	@echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
	@echo "Starting Server + GUI Client"
	@echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
	@echo "Starting server on port $(DEFAULT_PORT)..."
	@./$(SERVER_BIN) $(DEFAULT_PORT) > /tmp/fileshare-server.log 2>&1 & echo $$! > /tmp/fileshare-server.pid
	@sleep 1
	@echo "Starting GUI client..."
	@./$(GUI_CLIENT_BIN) > /tmp/fileshare-gui.log 2>&1 & echo $$! > /tmp/fileshare-gui.pid
	@echo "✓ Server PID: $$(cat /tmp/fileshare-server.pid)"
	@echo "✓ GUI PID: $$(cat /tmp/fileshare-gui.pid)"
	@echo "✓ Server log: /tmp/fileshare-server.log"
	@echo "✓ GUI log: /tmp/fileshare-gui.log"
	@echo ""
	@echo "Use 'make stop-all' to stop both processes"
	@echo "Use 'make status' to check process status"

# ============================================================================
# Stop targets
# ============================================================================

stop-server:
	@echo "Stopping server..."
	@pkill -f "./$(SERVER_BIN)" || echo "No server process found"
	@rm -f /tmp/fileshare-server.pid /tmp/fileshare-server.log

stop-client:
	@echo "Stopping CLI client..."
	@pkill -f "./$(CLIENT_BIN)" || echo "No client process found"

stop-gui:
	@echo "Stopping GUI client..."
	@pkill -f "./$(GUI_CLIENT_BIN)" || echo "No GUI client process found"
	@rm -f /tmp/fileshare-gui.pid /tmp/fileshare-gui.log

stop-all: stop-server stop-client stop-gui
	@echo "✓ All processes stopped"

# ============================================================================
# Restart targets
# ============================================================================

restart-server: stop-server
	@$(MAKE) run-server

restart-gui: stop-gui
	@$(MAKE) run-gui

restart-all: stop-all
	@$(MAKE) run-both

# ============================================================================
# Status and diagnostic targets
# ============================================================================

status:
	@echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
	@echo "Process Status"
	@echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
	@if pgrep -f "./$(SERVER_BIN)" > /dev/null; then \
		echo "✓ Server: RUNNING (PID: $$(pgrep -f './$(SERVER_BIN)'))"; \
	else \
		echo "✗ Server: NOT RUNNING"; \
	fi
	@if pgrep -f "./$(CLIENT_BIN)" > /dev/null; then \
		echo "✓ CLI Client: RUNNING (PID: $$(pgrep -f './$(CLIENT_BIN)'))"; \
	else \
		echo "✗ CLI Client: NOT RUNNING"; \
	fi
	@if pgrep -f "./$(GUI_CLIENT_BIN)" > /dev/null; then \
		echo "✓ GUI Client: RUNNING (PID: $$(pgrep -f './$(GUI_CLIENT_BIN)'))"; \
	else \
		echo "✗ GUI Client: NOT RUNNING"; \
	fi
	@echo ""

check-deps:
	@echo "Checking dependencies..."
	@command -v gcc > /dev/null || { echo "✗ gcc not found"; exit 1; }
	@command -v sqlite3 > /dev/null || { echo "✗ sqlite3 not found"; exit 1; }
	@command -v pkg-config > /dev/null || { echo "✗ pkg-config not found"; exit 1; }
	@echo "✓ All dependencies found"

check-gtk:
	@command -v pkg-config > /dev/null || { echo "✗ pkg-config not found (required for GTK)"; exit 1; }
	@pkg-config --exists gtk4 || { echo "✗ GTK4 not found. Install with: brew install gtk4"; exit 1; }

# ============================================================================
# Clean targets (optimized for parallel execution)
# ============================================================================

# Clean all build artifacts
# Note: We use subshells with & to enable parallel cleaning across directories
clean:
	@echo "Cleaning build artifacts..."
	@$(MAKE) -C $(SRC_COMMON) clean & \
	$(MAKE) -C $(SRC_DATABASE) clean & \
	$(MAKE) -C $(SRC_SERVER) clean & \
	$(MAKE) -C $(SRC_CLIENT) clean & \
	$(MAKE) -C $(SRC_CLIENT)/gui clean 2>/dev/null & \
	$(MAKE) -C $(TESTS) clean & \
	wait
	@rm -rf $(BUILD)
	@rm -f /tmp/fileshare-*.pid /tmp/fileshare-*.log
	@echo "✓ Clean complete"

# ============================================================================
# Help target
# ============================================================================

help:
	@echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
	@echo "File Sharing System - Build Targets (Parallel Build Optimized)"
	@echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
	@echo ""
	@echo "BUILD TARGETS:"
	@echo "  make all             - Build server, CLI client, and GUI client (default)"
	@echo "                         Uses all $(NPROCS) CPU cores for parallel compilation"
	@echo "  make server          - Build server only"
	@echo "  make client          - Build CLI client only"
	@echo "  make gui             - Build GUI client only"
	@echo "  make tests           - Build test suite"
	@echo ""
	@echo "RUN TARGETS:"
	@echo "  make run-server      - Build and run server (port $(DEFAULT_PORT))"
	@echo "  make run-client      - Build and run CLI client"
	@echo "  make run-gui         - Build and run GUI client"
	@echo "  make run-both        - Run server + GUI client in background"
	@echo ""
	@echo "CONTROL TARGETS:"
	@echo "  make stop-server     - Stop running server"
	@echo "  make stop-gui        - Stop running GUI client"
	@echo "  make stop-all        - Stop all running processes"
	@echo "  make restart-server  - Restart server"
	@echo "  make restart-gui     - Restart GUI client"
	@echo "  make restart-all     - Restart all processes"
	@echo "  make status          - Show running process status"
	@echo ""
	@echo "DOCKER TARGETS:"
	@echo "  make docker-build    - Build Docker image"
	@echo "  make docker-run      - Run server in Docker container"
	@echo "  make docker-stop     - Stop Docker container"
	@echo "  make docker-restart  - Restart Docker container"
	@echo "  make docker-logs     - View Docker container logs"
	@echo "  make docker-clean    - Remove Docker container and image"
	@echo "  make docker-shell    - Open shell in running container"
	@echo "  make docker-status   - Show Docker container status"
	@echo ""
	@echo "MAINTENANCE TARGETS:"
	@echo "  make clean           - Remove all build artifacts"
	@echo "  make check-deps      - Verify required dependencies"
	@echo "  make help            - Show this help message"
	@echo ""
	@echo "CONFIGURATION:"
	@echo "  DEFAULT_PORT=$(DEFAULT_PORT)"
	@echo "  DEFAULT_HOST=$(DEFAULT_HOST)"
	@echo ""
	@echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

# ============================================================================
# Docker targets
# ============================================================================

# Docker configuration
DOCKER_IMAGE = fileshare-server
DOCKER_TAG = latest
DOCKER_CONTAINER = fileshare-server

docker-build:
	@echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
	@echo "Building Docker Image"
	@echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
	@docker build -t $(DOCKER_IMAGE):$(DOCKER_TAG) .
	@echo "✓ Docker image built: $(DOCKER_IMAGE):$(DOCKER_TAG)"
	@echo ""

docker-run:
	@echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
	@echo "Starting Docker Container"
	@echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
	@mkdir -p data storage logs
	@docker-compose up -d
	@echo "✓ Container started: $(DOCKER_CONTAINER)"
	@echo "✓ Server accessible at localhost:3000"
	@echo ""
	@echo "Use 'make docker-logs' to view logs"
	@echo "Use 'make docker-stop' to stop the container"

docker-stop:
	@echo "Stopping Docker container..."
	@docker-compose down
	@echo "✓ Container stopped"

docker-restart:
	@echo "Restarting Docker container..."
	@docker-compose restart
	@echo "✓ Container restarted"

docker-logs:
	@echo "Showing Docker container logs (Ctrl+C to exit)..."
	@docker-compose logs -f

docker-clean:
	@echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
	@echo "Cleaning Docker Resources"
	@echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
	@docker-compose down -v
	@docker rmi $(DOCKER_IMAGE):$(DOCKER_TAG) 2>/dev/null || true
	@echo "✓ Docker resources cleaned"

docker-shell:
	@echo "Opening shell in running container..."
	@docker exec -it $(DOCKER_CONTAINER) /bin/bash

docker-status:
	@echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
	@echo "Docker Container Status"
	@echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
	@docker ps -a --filter "name=$(DOCKER_CONTAINER)" --format "table {{.Names}}\t{{.Status}}\t{{.Ports}}"
	@echo ""
