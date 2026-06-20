CC := gcc
CFLAGS := -Wall -Wextra -g -std=c11 -Iinclude
SRC_DIR := src
BUILD_DIR := build

ifeq ($(OS),Windows_NT)
    DETECTED_OS := Windows
    TARGET := dnsrelay.exe
    LDFLAGS += -lws2_32
    RM_RF := cmd /c "if exist $(BUILD_DIR) rmdir /s /q $(BUILD_DIR) & if exist $(TARGET) del /q $(TARGET)"
    MKDIR_P := cmd /c "if not exist $(BUILD_DIR) mkdir $(BUILD_DIR)"
else
    DETECTED_OS := Unix
    TARGET := dnsrelay
    RM_RF := rm -rf $(BUILD_DIR) $(TARGET)
    MKDIR_P := mkdir -p $(BUILD_DIR)
endif

SOURCES := $(wildcard $(SRC_DIR)/*.c)
OBJECTS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SOURCES))

.PHONY: all clean report report-sync report-async info

all: info $(TARGET)

info:
	@echo Building for $(DETECTED_OS) -\> $(TARGET)

REPORT_DIR := docs/report
TYPST_ROOT := ../..

report-sync:
	cd $(REPORT_DIR) && typst compile --root $(TYPST_ROOT) 实验报告-同步.typ 实验报告-同步.pdf
	@echo "Built $(REPORT_DIR)/实验报告-同步.pdf"

report-async:
	cd $(REPORT_DIR) && typst compile --root $(TYPST_ROOT) 实验报告-异步.typ 实验报告-异步.pdf
	cp $(REPORT_DIR)/实验报告-异步.pdf 实验报告.pdf
	@echo "Built $(REPORT_DIR)/实验报告-异步.pdf"
	@echo "Submit copy: ./实验报告.pdf (async)"

report: report-sync report-async
	cp $(REPORT_DIR)/实验报告-同步.pdf 实验报告.pdf
	@echo "Both PDFs: $(REPORT_DIR)/实验报告-同步.pdf, $(REPORT_DIR)/实验报告-异步.pdf"
	@echo "Submit copy: ./实验报告.pdf (sync)"

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR):
	$(MKDIR_P)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM_RF)
