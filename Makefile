.PHONY: clean crun build/app all
SHELL=/bin/bash

CC = clang
CPPFLAGS = -std=c99 -g -Wextra -Wall -fsanitize=address -O0
# CPPFLAGS += -Werror
CFLAGS = -I./include
LDFLAGS = -lGL -lm -lpthread -ldl -lrt -lX11 -lraylib -L./src/lib/

SRC_DIRS := ./src
FILES = $(wildcard $(SRC_DIRS)/*.c)

BUILD_DIR := ./build
RESOURCES_DIR := resources
TARGET_EXEC := app

all: crun

# windows: # I want exe to be available so that no such bullshit as compiling
# 	echo "TODO"

# debian: # I want .deb to be available so that no such bullshit as compiling
# 	echo "TODO"

# flatpak:
# 	echo "TODO"

debug: build/app
	gdb $(BUILD_DIR)/$(TARGET_EXEC)

crun: build/app
	$(BUILD_DIR)/$(TARGET_EXEC)

build/app: $(FILES)
	@mkdir -p $(BUILD_DIR)
	@cp -r $(RESOURCES_DIR) $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $^ -o $@ $(CFLAGS) $(LDFLAGS)

clean:
	rm -rf $(BUILD_DIR)
