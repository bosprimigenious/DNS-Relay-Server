CC := gcc
CFLAGS := -Wall -Wextra -g -std=c11 -Iinclude
SRC_DIR := src
BUILD_DIR := build
TARGET := dnsrelay

SOURCES := $(wildcard $(SRC_DIR)/*.c)
OBJECTS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SOURCES))

.PHONY: all clean report report-sync report-async

all: $(TARGET)

report-sync:
	cd docs/report && typst compile 实验报告-同步.typ 实验报告-同步.pdf
	@echo "Built docs/report/实验报告-同步.pdf"

report-async:
	cd docs/report && typst compile 实验报告-异步.typ 实验报告-异步.pdf
	@echo "Built docs/report/实验报告-异步.pdf"

report: report-sync report-async
	cp docs/report/实验报告-同步.pdf 实验报告.pdf
	@echo "Both reports built; ./实验报告.pdf <- sync (课设提交)"

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(TARGET)
