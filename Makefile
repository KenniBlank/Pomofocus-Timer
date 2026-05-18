.PHONY: clean crun build/app all
SHELL=/bin/bash

CC = gcc
# CPPFLAGS = -std=c99 -g -Wextra -Wall -fsanitize=address -O0
# CPPFLAGS += -DDEBUG
# CPPFLAGS += -Werror
CFLAGS = -I./include
LDFLAGS = -lGL -lm -lpthread -ldl -lrt -lX11 -lraylib -L./src/lib/

CPPFLAGS = -std=c99 -O3 -march=native -flto=auto -ffunction-sections -fdata-sections -DAPP_TITLE="\"PomoFocus Timer"\"
LDFLAGS += -flto=auto -Wl,--gc-sections

SRC_DIRS := ./src
FILES = $(wildcard $(SRC_DIRS)/*.c)

BUILD_DIR := ./build
RESOURCES_DIR := resources
TARGET_EXEC := app

all: crun

debug: build/app
	gdb $(BUILD_DIR)/$(TARGET_EXEC)

crun: build/app
	$(BUILD_DIR)/$(TARGET_EXEC)

build/app: $(FILES)
	@mkdir -p $(BUILD_DIR)
	@cp -r $(RESOURCES_DIR) $(BUILD_DIR)
	@cp $(BUILD_DIR)/data.json $(BUILD_DIR)/data-bk.json
	$(CC) $(CPPFLAGS) $^ -o $@ $(CFLAGS) $(LDFLAGS)

clean:
	rm -rf $(BUILD_DIR)
