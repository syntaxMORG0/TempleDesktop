
CC=gcc
CFLAGS=-Wall -Wextra -O2
PKG_CONFIG ?= pkg-config

# If pkg-config is available and knows x11+xcursor, prefer it (Linux/Arch).
ifneq (, $(shell command -v $(PKG_CONFIG) 2>/dev/null))
PKG_CHECK := $(shell $(PKG_CONFIG) --exists x11 xcursor 2>/dev/null; echo $$?)
else
PKG_CHECK := 1
endif

ifeq ($(PKG_CHECK),0)
PKG_CFLAGS := $(shell $(PKG_CONFIG) --cflags x11 xcursor)
PKG_LIBS := $(shell $(PKG_CONFIG) --libs x11 xcursor)
CPPFLAGS += $(PKG_CFLAGS)
LDFLAGS += $(PKG_LIBS)
else
# Default X11 prefix (macOS XQuartz puts headers/libs in /opt/X11)
X11_PREFIX ?= /opt/X11
# Add X11 include path
CPPFLAGS += -I$(X11_PREFIX)/include
# Linker flags: look in X11 lib dir and link X11 and Xcursor
LDFLAGS += -L$(X11_PREFIX)/lib -lX11 -lXcursor
endif
BUILD_DIR=./build
SRC_DIR=./src

# build targeted towards Linux

compile:
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(SRC_DIR)/*.c -o $(BUILD_DIR)/main $(LDFLAGS)

run:
	$(BUILD_DIR)/main

clean:
	rm -rf $(BUILD_DIR)/*