CC := gcc
CFLAGS := -Wall -Wextra -g -std=c11 -Iinclude
SRC_DIR := src
BUILD_DIR := build
TARGET := dnsrelay

SOURCES := $(wildcard $(SRC_DIR)/*.c)
OBJECTS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SOURCES))

.PHONY: all clean report report-sync report-async

all: $(TARGET)

REPORT_DIR := docs/report
TYPST_ROOT := ../..

report-sync:
	cd $(REPORT_DIR) && typst compile --root $(TYPST_ROOT) 实验报告-同步.typ 实验报告-同步.pdf
	@echo "Built $(REPORT_DIR)/实验报告-同步.pdf"

report-async:
	cd $(REPORT_DIR) && typst compile --root $(TYPST_ROOT) 实验报告-异步.typ 实验报告-异步.pdf
	@echo "Built $(REPORT_DIR)/实验报告-异步.pdf"

report: report-sync report-async
	cp $(REPORT_DIR)/实验报告-同步.pdf 实验报告.pdf
	@echo "Both PDFs: $(REPORT_DIR)/实验报告-同步.pdf, $(REPORT_DIR)/实验报告-异步.pdf"
	@echo "Submit copy: ./实验报告.pdf (sync)"

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(TARGET)
