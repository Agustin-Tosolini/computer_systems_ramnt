CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -pedantic -O0 -g -fno-omit-frame-pointer
BUILD_DIR = build
TARGET = $(BUILD_DIR)/gini_client
SRC = main.c
LDLIBS = -lssl -lcrypto

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(SRC)
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LDLIBS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
