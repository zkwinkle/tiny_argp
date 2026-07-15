CC       ?= cc
CFLAGS   ?= -std=c11 -Wall -Wextra -Wpedantic -Os
CPPFLAGS ?= -I..
BUILD    := build

EXAMPLES := ex1_minimal ex2_options ex3_positionals ex4_subcommands
BINS     := $(addprefix $(BUILD)/,$(EXAMPLES))

.PHONY: all clean verify

all: $(BINS)

clean:
	rm -rf $(BUILD)

$(BUILD)/%: %.c ../tiny_argp.c ../tiny_argp.h | $(BUILD)
	$(CC) $(CFLAGS) $(CPPFLAGS) $< ../tiny_argp.c -o $@

$(BUILD):
	mkdir -p $(BUILD)

verify: $(BINS)
	@./verify.sh
