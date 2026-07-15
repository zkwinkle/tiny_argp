/* Example #2: options only, no positional arguments.
 *
 * The program understands three boolean flags (--verbose, --quiet, and
 * --silent) plus one option that takes an argument (--output=FILE).
 * --quiet and --silent are aliases for the same behavior, so they share
 * a key ('q' and 's'); the parser callback handles both cases identically.
 *
 * In this example, `main()` uses a `struct arguments` to communicate with the
 * `parse_opt` function. It passes this struct through the `input` pointer,
 * and the parser reaches it via state->input on each call.
 * Defaults are set once at TINY_ARGP_KEY_INIT before any option has been seen.
 */

#include <stdarg.h>
#include <stdio.h>

#include "tiny_argp.h"

struct arguments {
  int silent, verbose;
  const char *output_file;
};

static int fprintf_stderr(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int r = vfprintf(stderr, fmt, ap);
  va_end(ap);
  return r;
}

static const struct tiny_argp_option options[] = {
    {"verbose", 'v', 0, 0, "Produce verbose output"},
    {"quiet", 'q', 0, 0, "Don't produce any output"},
    {"silent", 's', 0, 0, "Alias for --quiet"},
    {"output", 'o', "FILE", 0, "Output to FILE instead of standard output"},
    {0}};

static int parse_opt(int key, char *arg, struct tiny_argp_state *state) {
  struct arguments *args = state->input;
  switch (key) {
  case TINY_ARGP_KEY_INIT:
    args->silent = 0;
    args->verbose = 0;
    args->output_file = "-";
    break;
  case 'q':
  case 's':
    args->silent = 1;
    break;
  case 'v':
    args->verbose = 1;
    break;
  case 'o':
    args->output_file = arg;
    break;
  case TINY_ARGP_KEY_ARG:
  case TINY_ARGP_KEY_NO_ARGS:
  case TINY_ARGP_KEY_END:
  case TINY_ARGP_KEY_SUCCESS:
  case TINY_ARGP_KEY_FINI:
    break;
  default:
    return TINY_ARGP_ERR_UNKNOWN;
  }
  return TINY_ARGP_SUCCESS;
}

static const struct tiny_argp argp = {
    .options = options,
    .parser = parse_opt,
    .args_doc = NULL,
    .doc = "tiny_argp example #2 -- options only, no positional arguments.",
    .printer = printf,
    .err_printer = fprintf_stderr,
};

int main(int argc, char **argv) {
  struct arguments args;
  int status = tiny_argp_parse(&argp, argc, argv, 0, NULL, &args);
  if (status != TINY_ARGP_SUCCESS)
    return status;

  printf("OUTPUT_FILE = %s\nVERBOSE = %s\nSILENT = %s\n", args.output_file,
         args.verbose ? "yes" : "no", args.silent ? "yes" : "no");
  return 0;
}
