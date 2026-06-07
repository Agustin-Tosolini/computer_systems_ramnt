CC ?= gcc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -pedantic
LDFLAGS ?=
BUILD_DIR ?= build
TARGET ?= $(BUILD_DIR)/gpio_jsonl_reader
SRC := src/gpio_jsonl_reader.c

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRC)
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -rf $(BUILD_DIR)
