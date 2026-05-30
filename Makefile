
CC := gcc
CFLAGS := -Wall -Wextra -O2
PKG_CONFIG ?= pkg-config

PKG_EXISTS := $(shell command -v $(PKG_CONFIG) >/dev/null 2>&1 && $(PKG_CONFIG) --exists x11 xcursor >/dev/null 2>&1 && echo yes || echo no)

ifeq ($(PKG_EXISTS),yes)
PKG_CFLAGS := $(shell $(PKG_CONFIG) --cflags x11 xcursor)
PKG_LIBS := $(shell $(PKG_CONFIG) --libs x11 xcursor)
else
PKG_CFLAGS :=
PKG_LIBS := -lX11 -lXcursor
endif

BUILD_DIR := build
SRC_DIR := src

.PHONY: compile run startx install-xinit clean

compile:
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(PKG_CFLAGS) $(SRC_DIR)/*.c -o $(BUILD_DIR)/main $(PKG_LIBS)

run:
	$(BUILD_DIR)/main

# Start X and run the binary as the X client (X exits when program exits).
start:
	xinit $(PWD)/$(BUILD_DIR)/main -- :0

install:
	printf '%s
'exec $(PWD)/$(BUILD_DIR)/main'' > ~/.xinitrc

clean:
	rm -rf $(BUILD_DIR)/*