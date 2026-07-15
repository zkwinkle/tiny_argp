/* Example #1: a minimal tiny_argp program.
 *
 * The program declares no options and does nothing with positional
 * arguments; they are silently accepted. --help and --usage still work
 * because tiny_argp installs them by default.
 */

#include <stdarg.h>
#include <stdio.h>

#include "tiny_argp.h"

static int fprintf_stderr(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int r = vfprintf(stderr, fmt, ap);
  va_end(ap);
  return r;
}

static int parse_opt(int key, char *arg, struct tiny_argp_state *state) {
  (void)arg;
  (void)state;
  switch (key) {
  case TINY_ARGP_KEY_INIT:
  case TINY_ARGP_KEY_ARG:
  case TINY_ARGP_KEY_NO_ARGS:
  case TINY_ARGP_KEY_END:
  case TINY_ARGP_KEY_SUCCESS:
  case TINY_ARGP_KEY_FINI:
    return TINY_ARGP_SUCCESS;
  default:
    return TINY_ARGP_ERR_UNKNOWN;
  }
}

static const struct tiny_argp_option options[] = {{0}};

static const struct tiny_argp argp = {
    .options = options,
    .parser = parse_opt,
    .args_doc = NULL,
    .doc = NULL,
    .printer = printf,
    .err_printer = fprintf_stderr,
};

int main(int argc, char **argv) {
  return tiny_argp_parse(&argp, argc, argv, 0, NULL, NULL);
}
