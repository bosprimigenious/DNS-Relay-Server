CC := gcc
CFLAGS := -Wall -Wextra -g -std=c11 -Iinclude
SRC_DIR := src
BUILD_DIR := build
TARGET := dnsrelay

SOURCES := $(wildcard $(SRC_DIR)/*.c)
OBJECTS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SOURCES))

.PHONY: all clean report

all: $(TARGET)

report:
	cd docs/report && typst compile 实验报告.typ 实验报告.pdf
	cp docs/report/实验报告.pdf 实验报告.pdf
	@echo "Report: docs/report/实验报告.pdf (copied to ./实验报告.pdf)"

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(TARGET)
