/* Example #3: options plus positional arguments.
 *
 * The program takes one required ARG1 followed by any number of
 * additional STRINGs. It uses the same option set as example #2.
 *
 * Positionals arrive one at a time through TINY_ARGP_KEY_ARG. The
 * parser looks at state->arg_num to figure out which slot the current
 * argument belongs in: index 0 is ARG1, later indices go into the
 * STRINGs array. Once parsing has run to completion, TINY_ARGP_KEY_END
 * fires; if ARG1 was never set the parser prints an error with
 * tiny_argp_error() and returns TINY_ARGP_ERR_PARSE to stop the parse.
 *
 * The `args_doc` field ("ARG1 [STRING...]") appears on the "Usage:"
 * line so that --help shows the expected shape of the invocation. The
 * longer description in `doc` is split by '\v': text before the vertical
 * tab prints above the option list, text after prints below.
 *
 * The --silent alias sets OPTION_HIDDEN, which keeps it functional on
 * the command line but hides it from --help. tiny_argp does not
 * support option groups, so hiding aliases and structuring the doc
 * string are the tools available for a tidy help layout.
 */

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

#include "tiny_argp.h"

#define MAX_STRINGS 32

struct arguments {
  const char *arg1;
  const char *strings[MAX_STRINGS];
  size_t string_count;
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
    {"quiet", 'q', 0, 0, "Don't produce any output (also -s / --silent)"},
    {"silent", 's', 0, OPTION_HIDDEN, NULL},
    {"output", 'o', "FILE", 0, "Output to FILE instead of standard output"},
    {0}};

static int parse_opt(int key, char *arg, struct tiny_argp_state *state) {
  struct arguments *args = state->input;
  switch (key) {
  case TINY_ARGP_KEY_INIT:
    args->arg1 = NULL;
    args->string_count = 0;
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
    if (state->arg_num == 0) {
      args->arg1 = arg;
    } else if (state->arg_num - 1 < MAX_STRINGS) {
      args->strings[state->arg_num - 1] = arg;
      args->string_count++;
    } else {
      tiny_argp_error(state, "too many string arguments");
      return TINY_ARGP_ERR_PARSE;
    }
    break;
  case TINY_ARGP_KEY_END:
    if (state->arg_num < 1) {
      tiny_argp_error(state, "too few arguments");
      return TINY_ARGP_ERR_PARSE;
    }
    break;
  case TINY_ARGP_KEY_NO_ARGS:
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
    .args_doc = "ARG1 [STRING...]",
    .doc = "tiny_argp example #3 -- a program with several arguments.\n"
           "\n"
           "This example takes one required ARG1 and zero or more STRINGs. "
           "It shares the option set of example #2 but additionally reads "
           "positional arguments and validates them from the parser callback.\v"
           "Trailing STRINGs are collected in order.  Use `--` to force "
           "subsequent tokens to be treated as positionals even if they look "
           "like options, e.g.  `ex3_positionals -v -- --literal-arg`.\n"
           "\n"
           "Report bugs at <https://github.com/zkwinkle/tiny_argp/issues>.",
    .printer = printf,
    .err_printer = fprintf_stderr,
};

int main(int argc, char **argv) {
  struct arguments args;
  int status = tiny_argp_parse(&argp, argc, argv, 0, NULL, &args);
  if (status != TINY_ARGP_SUCCESS)
    return status;

  printf("ARG1 = %s\nSTRINGS = ", args.arg1);
  for (size_t i = 0; i < args.string_count; i++)
    printf("%s%s", args.strings[i], i + 1 < args.string_count ? ", " : "");
  printf("\nOUTPUT_FILE = %s\nVERBOSE = %s\nSILENT = %s\n", args.output_file,
         args.verbose ? "yes" : "no", args.silent ? "yes" : "no");
  return 0;
}
