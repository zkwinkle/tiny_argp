#include "test_utils.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Unity assertions are available transitively via the test file that
 * includes test_utils.h.  We include unity.h here so the assert helpers
 * defined below can call TEST_FAIL_MESSAGE directly. */
#include "unity.h"

parser_call_t parser_log[PARSER_LOG_MAX];
size_t parser_log_count;

char mock_stdout[MOCK_BUF_SIZE];
size_t mock_stdout_len;
char mock_stderr[MOCK_BUF_SIZE];
size_t mock_stderr_len;

static char argv_pool[ARGV_MAX_COUNT][ARGV_MAX_STR_LEN];
static char *argv_ptrs[ARGV_MAX_COUNT + 1];

/* Sized to fit PARSER_LOG_MAX entries. Longest per-entry payload is
 * format_key() output (~11 chars in the worst case. -2147483648)
 * plus the " -> " separator (4 chars) = 15. *16 rounds up with headroom. */
static char parser_log_str[PARSER_LOG_MAX * 16];
static char key_label_buf[16];

/* Hard-abort if we exceed any buffer limits */
static void overflow_panic(const char *what) {
  fprintf(stderr,
          "test_utils panic: %s exceeded. Raise the constant in "
          "test_utils.h or investigate why the test generates so many items.\n",
          what);
  exit(1);
}

/* Append one call record. Copies the arg string (bounded); NULL arg means
 * "no arg was supplied for this key". */
void log_parser_call(int key, const char *arg) {
  if (parser_log_count >= PARSER_LOG_MAX) overflow_panic("PARSER_LOG_MAX");
  parser_call_t *c = &parser_log[parser_log_count++];
  c->key = key;
  c->has_arg = (arg != NULL);
  if (arg) {
    if (strlen(arg) >= PARSER_LOG_ARG_MAX) overflow_panic("PARSER_LOG_ARG_MAX");
    strncpy(c->arg, arg, PARSER_LOG_ARG_MAX - 1);
    c->arg[PARSER_LOG_ARG_MAX - 1] = '\0';
  } else {
    c->arg[0] = '\0';
  }
}

/* vsnprintf-append into mock_stdout, tracking length so successive calls
 * concatenate. Truncation is a hard error. */
int mock_printer(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  size_t room = MOCK_BUF_SIZE - mock_stdout_len;
  int n = vsnprintf(mock_stdout + mock_stdout_len, room, fmt, ap);
  va_end(ap);
  if (n < 0) return n;
  if ((size_t)n >= room) overflow_panic("MOCK_BUF_SIZE (stdout)");
  mock_stdout_len += (size_t)n;
  return n;
}

/* Same as mock_printer but into mock_stderr. */
int mock_err_printer(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  size_t room = MOCK_BUF_SIZE - mock_stderr_len;
  int n = vsnprintf(mock_stderr + mock_stderr_len, room, fmt, ap);
  va_end(ap);
  if (n < 0) return n;
  if ((size_t)n >= room) overflow_panic("MOCK_BUF_SIZE (stderr)");
  mock_stderr_len += (size_t)n;
  return n;
}

/* Copy each variadic string into the mutable pool, then return the
 * pointer array (NULL-terminated so it looks like a real argv). tiny_argp
 * may reorder these entries in place, which is safe because the pool is
 * writable memory. */
char **build_argv(size_t count, ...) {
  if (count > ARGV_MAX_COUNT) overflow_panic("ARGV_MAX_COUNT");
  va_list ap;
  va_start(ap, count);
  for (size_t i = 0; i < count; i++) {
    const char *s = va_arg(ap, const char *);
    if (strlen(s) >= ARGV_MAX_STR_LEN) overflow_panic("ARGV_MAX_STR_LEN");
    strncpy(argv_pool[i], s, ARGV_MAX_STR_LEN - 1);
    argv_pool[i][ARGV_MAX_STR_LEN - 1] = '\0';
    argv_ptrs[i] = argv_pool[i];
  }
  argv_ptrs[count] = NULL;
  va_end(ap);
  return argv_ptrs;
}

/* Wipe the log and both capture buffers. The argv pool is intentionally
 * left alone: it's overwritten on the next build_argv() call. */
void test_utils_reset(void) {
  parser_log_count = 0;
  memset(parser_log, 0, sizeof(parser_log));
  mock_stdout_len = 0;
  mock_stdout[0] = '\0';
  mock_stderr_len = 0;
  mock_stderr[0] = '\0';
}

/* Map a key int to a stable, human-readable label. Special keys get their
 * TINY_ARGP_KEY_ name; printable user keys render as 'x'; anything else
 * falls back to the raw integer. User-key/integer cases share a static
 * buffer, so the pointer returned is only valid until the next call. */
const char *format_key(int key) {
  switch (key) {
    case TINY_ARGP_KEY_ARG: return "ARG";
    case TINY_ARGP_KEY_ARGS: return "ARGS";
    case TINY_ARGP_KEY_END: return "END";
    case TINY_ARGP_KEY_NO_ARGS: return "NO_ARGS";
    case TINY_ARGP_KEY_INIT: return "INIT";
    case TINY_ARGP_KEY_FINI: return "FINI";
    case TINY_ARGP_KEY_SUCCESS: return "SUCCESS";
    case TINY_ARGP_KEY_ERROR: return "ERROR";
    default:
      if (key > 0 && key < 128) {
        snprintf(key_label_buf, sizeof(key_label_buf), "'%c'", key);
      } else {
        snprintf(key_label_buf, sizeof(key_label_buf), "%d", key);
      }
      return key_label_buf;
  }
}

/* Assert the full ordered key sequence is exactly as expected.
 * Checks count first, then each individual key.  On any mismatch the
 * failure message includes the formatted log for easy diagnosis. */
void assert_key_sequence_exact(const int *expected_keys, size_t n) {
  if (parser_log_count != n) {
    char msg[640];
    snprintf(msg, sizeof(msg),
             "expected %zu keys but got %zu. Log: %s",
             n, parser_log_count, format_parser_log());
    TEST_FAIL_MESSAGE(msg);
    return;
  }
  for (size_t i = 0; i < n; i++) {
    if (parser_log[i].key != expected_keys[i]) {
      char msg[640];
      snprintf(msg, sizeof(msg),
               "key mismatch at index %zu: expected %s got %s. Log: %s",
               i, format_key(expected_keys[i]), format_key(parser_log[i].key),
               format_parser_log());
      TEST_FAIL_MESSAGE(msg);
      return;
    }
  }
}

/* Canonical recording parser: logs the call and returns SUCCESS for all
 * standard special keys; UNKNOWN for everything else. */
int default_rec(int key, char *arg, struct tiny_argp_state *state) {
  (void)state;
  log_parser_call(key, arg);
  switch (key) {
    case TINY_ARGP_KEY_ARG:
    case TINY_ARGP_KEY_ARGS:
    case TINY_ARGP_KEY_INIT:
    case TINY_ARGP_KEY_END:
    case TINY_ARGP_KEY_NO_ARGS:
    case TINY_ARGP_KEY_SUCCESS:
    case TINY_ARGP_KEY_ERROR:
    case TINY_ARGP_KEY_FINI:
      return TINY_ARGP_SUCCESS;
    default:
      return TINY_ARGP_ERR_UNKNOWN;
  }
}

/* Join the current log as "K1 -> K2 -> K3" into a static buffer. */
const char *format_parser_log(void) {
  size_t pos = 0;
  parser_log_str[0] = '\0';
  for (size_t i = 0; i < parser_log_count; i++) {
    size_t room = sizeof(parser_log_str) - pos;
    int n = snprintf(parser_log_str + pos, room, "%s%s",
                     i == 0 ? "" : " -> ", format_key(parser_log[i].key));
    if (n < 0) return parser_log_str;
    /* Unreachable: parser_log_str is dimensioned to hold PARSER_LOG_MAX
     * entries with headroom. Reaching this means format_key() started
     * returning longer strings or PARSER_LOG_MAX outgrew the buffer. */
    if ((size_t)n >= room) overflow_panic("parser_log_str");
    pos += (size_t)n;
  }
  return parser_log_str;
}
