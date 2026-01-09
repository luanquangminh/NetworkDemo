# Makefile for GTK GUI Client - Optimized with dependency tracking
CC = gcc
CFLAGS = -Wall -Wextra -pthread -I.. -I../../common -I../../../lib/cJSON
CFLAGS += -I/opt/homebrew/include/gtk-3.0
CFLAGS += -I/opt/homebrew/include/glib-2.0
CFLAGS += -I/opt/homebrew/lib/glib-2.0/include
CFLAGS += -I/opt/homebrew/include/pango-1.0
CFLAGS += -I/opt/homebrew/include/harfbuzz
CFLAGS += -I/opt/homebrew/include/cairo
CFLAGS += -I/opt/homebrew/include/gdk-pixbuf-2.0
CFLAGS += -I/opt/homebrew/include/atk-1.0
# Enable automatic dependency generation for incremental builds
DEPFLAGS = -MMD -MP

LDFLAGS = -L../../common -L/opt/homebrew/lib
LDFLAGS += -lcommon -lpthread
LDFLAGS += -lgtk-3 -lgdk-3 -lpangocairo-1.0 -lpango-1.0
LDFLAGS += -lharfbuzz -latk-1.0 -lcairo-gobject -lcairo
LDFLAGS += -lgdk_pixbuf-2.0 -lgio-2.0 -lgobject-2.0 -lglib-2.0

OBJS = main.o login_dialog.o main_window.o file_operations.o dialogs.o admin_dashboard.o
DEPS = $(OBJS:.o=.d)
TARGET = ../../../build/gui_client

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS) ../client.o ../net_handler.o
	mkdir -p ../../../build
	$(CC) $(OBJS) ../client.o ../net_handler.o $(LDFLAGS) -o $(TARGET)

# Compile with automatic dependency generation
# This replaces the hardcoded gui.h dependency with automatic tracking
%.o: %.c
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

# Include automatically generated dependencies (if they exist)
-include $(DEPS)

clean:
	rm -f $(OBJS) $(TARGET) $(DEPS)
