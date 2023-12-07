CC      ?= cc
CFLAGS  ?= -std=c11 -Wall -Wextra -Wpedantic -Os
BUILD   ?= build

SRC := tiny_argp.c
OBJ := $(BUILD)/tiny_argp.o

.PHONY: all clean size

all: $(OBJ)

$(OBJ): $(SRC) tiny_argp.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD):
	mkdir -p $(BUILD)

size: $(OBJ)
	size $(OBJ)

clean:
	rm -rf $(BUILD)
