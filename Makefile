CC=gcc
CFLAGS=-Wall -Wextra -O2
LDFLAGS=-lX11 -lXcursor
BUILD_DIR=./build
SRC_DIR=./src

# build targeted towards Linux

compile:
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(SRC_DIR)/*.c -o $(BUILD_DIR)/main $(LDFLAGS)

run:
	$(BUILD_DIR)/main

clean:
	rm -rf $(BUILD_DIR)/*