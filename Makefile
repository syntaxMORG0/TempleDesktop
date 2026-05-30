
CC := gcc
CFLAGS := -Wall -Wextra -O2
PKG_CONFIG ?= pkg-config
BUILD_DIR := build
SRC_DIR := src

X11_CFLAGS := $(shell $(PKG_CONFIG) --cflags x11 2>/dev/null)
X11_LIBS := $(shell $(PKG_CONFIG) --libs x11 2>/dev/null)

ifeq ($(strip $(X11_LIBS)),)
X11_LIBS := -lX11
endif

.PHONY: compile run start install clean

compile:
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(X11_CFLAGS) $(SRC_DIR)/*.c -o $(BUILD_DIR)/main $(X11_LIBS)

run:
	$(BUILD_DIR)/main

start:
	xinit $(PWD)/$(BUILD_DIR)/main -- :0

install:
	printf 'exec %s/%s/main\n' '$(PWD)' '$(BUILD_DIR)' > ~/.xinitrc

clean:
	rm -rf $(BUILD_DIR)/*