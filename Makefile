CC=gcc
LDFLAGS=
BUILD_DIR=./build
SRC_DIR=./src

compile:
	$(CC) $(LDFLAGS) -o $(BUILD_DIR)/main $(SRC_DIR)/*.c

run:
	$(BUILD_DIR)/main

clean:
	rm -rf $(BUILD_DIR)/*