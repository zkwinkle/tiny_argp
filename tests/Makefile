CC       ?= cc
CFLAGS   ?= -std=c11 -Wall -Wextra -Wpedantic -g -O0
CPPFLAGS ?= -I.. -Iunity -DUNITY_OUTPUT_COLOR
BUILD    := build

TESTS := \
	test_short_opts \
	test_long_opts \
	test_non_option \
	test_key_sequence \
	test_flags \
	test_option_flags \
	test_help \
	test_errors \
	test_state

SRCS := $(addsuffix .c,$(TESTS)) test_main.c test_utils.c ../tiny_argp.c unity/unity.c
OBJS := $(patsubst %.c,$(BUILD)/%.o,$(notdir $(SRCS)))

BIN := $(BUILD)/run_tests

# Optimization levels for the matrix target. Add/remove here as needed.
OPT_LEVELS := -O0 -Os -O2 -O3

.PHONY: all clean run matrix

all: $(BIN)

run: $(BIN)
	$(BIN)

# Rebuild + run the suite at each opt level. Aborts on first failure.
matrix:
	@set -e; for opt in $(OPT_LEVELS); do \
		echo "==== building/testing at $$opt ===="; \
		$(MAKE) --no-print-directory clean; \
		$(MAKE) --no-print-directory run \
		  CFLAGS="-std=c11 -Wall -Wextra -Wpedantic $$opt"; \
	done
	@echo "==== all opt levels green ===="

$(BIN): $(OBJS) | $(BUILD)
	$(CC) $(CFLAGS) $(OBJS) -o $@

$(BUILD)/%.o: %.c | $(BUILD)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

$(BUILD)/%.o: ../%.c | $(BUILD)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

$(BUILD)/%.o: unity/%.c | $(BUILD)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

$(BUILD):
	mkdir -p $(BUILD)

clean:
	rm -rf $(BUILD)
