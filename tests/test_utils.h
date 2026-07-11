#ifndef TINY_ARGP_TEST_UTILS_H
#define TINY_ARGP_TEST_UTILS_H

/*
 * test_utils — shared fixtures for the tiny_argp test suite.
 *
 * Provides:
 *   1. A recording sink for the parser callback (parser_log[]) so tests
 *      can assert on the exact sequence of (key, arg) pairs the library
 *      delivered.
 *   2. Mock printer functions (mock_printer / mock_err_printer) that
 *      capture what would otherwise go to stdout/stderr, into buffers
 *      (mock_stdout / mock_stderr) tests can inspect via strstr, strcmp,
 *      etc.
 *   3. build_argv(): copies string literals into a mutable pool and
 *      returns argv-style pointers. tiny_argp_parse mutates argv (it
 *      reorders under default mode), so passing an array of string
 *      literals directly is technically UB.
 *   4. Debug formatters (format_key, format_parser_log) that produce
 *      human-readable descriptions of what was recorded, used in test
 *      failure messages.
 *
 * ## Why global state, not a per-test context
 *
 * The parser callback signature is fixed by tiny_argp
 * (int, char *, tiny_argp_state *). state->input is already dedicated to
 * the user's own struct in some tests (test_state.c inspects
 * state->argc/arg_num/etc.), so sharing an additional per-test log
 * through state->input isn't clean. A file-scope global keeps every test
 * parser trivially short.
 *
 * ## Why concurrent access isn't a concern
 *
 * Unity runs tests serially in a single thread. Each test's setUp() calls
 * test_utils_reset() which zeroes every global, so tests are independent
 * regardless of order. No test spawns threads. If that ever changed,
 * these globals would become unsafe and we should use per-test context.
 *
 * ## Hard limits vs. panics
 *
 * The constants below are sized for the current suite with generous
 * headroom. If a test ever exceeds one, the corresponding function prints
 * a message and calls exit(1) rather than silently truncating. A
 * truncated log or over-run capture buffer would corrupt assertion
 * results in ways that are painful to debug. If you hit a panic, either
 * raise the limit or investigate why the test is doing so much.
 */

#include <stdbool.h>
#include <stddef.h>

#include "../tiny_argp.h"

/* Max recorded parser calls per test. Real usage stays well under 20 (INIT
 * + a handful of user keys + END + SUCCESS + FINI). */
#define PARSER_LOG_MAX 128

/* Max copied arg length per recorded call. Tests use short arg strings;
 * 64 covers any realistic value. */
#define PARSER_LOG_ARG_MAX 64

/* Bytes captured per mock printer (stdout and stderr each get their own
 * buffer of this size). tiny_argp_help output for the tests' small option
 * lists fits in <1KB. 4KB has generous margin. */
#define MOCK_BUF_SIZE 4096

/* Max argv strings in a single build_argv call. Tests pass at most a
 * handful. */
#define ARGV_MAX_COUNT 32

/* Max length per argv string. */
#define ARGV_MAX_STR_LEN 128

/* ==============================================================
 * Parser call recording
 * ============================================================== */

typedef struct {
  /* Key the parser was invoked with. Either a user option key (e.g. 'a')
   * or one of the special TINY_ARGP_KEY_* codes. */
  int key;
  /* True if `arg` was non-NULL. Could be NULL for option keys that don't have
  * an associated arg. */
  bool has_arg;
  /* Copy of the arg string. Empty when has_arg is false. */
  char arg[PARSER_LOG_ARG_MAX];
} parser_call_t;

extern parser_call_t parser_log[PARSER_LOG_MAX];
extern size_t parser_log_count;

/* Append a call to the log. Test parsers should call this as their first
 * statement. */
void log_parser_call(int key, const char *arg);

/* ==============================================================
 * Mock printers
 * ============================================================== */

extern char mock_stdout[MOCK_BUF_SIZE];
extern size_t mock_stdout_len;
extern char mock_stderr[MOCK_BUF_SIZE];
extern size_t mock_stderr_len;

/* printf-signature functions that vsnprintf into the capture buffers
 * instead of stdout/stderr. Pass to tiny_argp.printer / .err_printer. */
int mock_printer(const char *fmt, ...);
int mock_err_printer(const char *fmt, ...);

/* ==============================================================
 * argv builder
 * ============================================================== */

/* Copy `count` string arguments into a static mutable pool and return
 * an argv-style pointer array (NULL-terminated). Only have one active argv
 * array per call, a subsequent call overwrites the pool. */
char **build_argv(size_t count, ...);

/* ==============================================================
 * Setup
 * ============================================================== */

/* Zero every global buffer and counter. Call in Unity's setUp(). */
void test_utils_reset(void);

/* ==============================================================
 * Debug formatters
 * ============================================================== */

/* Human-readable label for a key: "'a'" for printable user keys,
 * "INIT"/"END"/… for special keys, decimal integer otherwise. */
const char *format_key(int key);

/* Formatted log ("INIT -> 'a' -> END -> SUCCESS -> FINI"). Used in
 * failure messages so a wrong sequence is immediately visible. */
const char *format_parser_log(void);

#endif
