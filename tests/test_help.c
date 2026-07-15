#include <errno.h>

#include "test_utils.h"
#include "unity.h"

static int rec(int key, char *arg, struct tiny_argp_state *state) {
  (void)state;
  log_parser_call(key, arg);
  switch (key) {
  case 'a':
  case TINY_ARGP_KEY_ARG:
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

/* Parser that calls tiny_argp_state_help() when it sees 'a'. */
static int rec_calls_state_help(int key, char *arg,
                                struct tiny_argp_state *state) {
  log_parser_call(key, arg);
  if (key == 'a') {
    tiny_argp_state_help(state, mock_printer, TINY_ARGP_HELP_LONG);
  }
  switch (key) {
  case 'a':
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

/* Parser that calls tiny_argp_usage() when it sees 'a'. */
static int rec_calls_usage(int key, char *arg, struct tiny_argp_state *state) {
  log_parser_call(key, arg);
  if (key == 'a')
    tiny_argp_usage(state);
  switch (key) {
  case 'a':
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

/* Parser that calls tiny_argp_error() when it sees 'a', then returns EINVAL. */
static int rec_calls_error(int key, char *arg, struct tiny_argp_state *state) {
  log_parser_call(key, arg);
  if (key == 'a') {
    tiny_argp_error(state, "bad input");
    return EINVAL;
  }
  switch (key) {
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

/* Parser that calls tiny_argp_exit() when it sees 'a'. */
static int rec_calls_exit(int key, char *arg, struct tiny_argp_state *state) {
  log_parser_call(key, arg);
  if (key == 'a')
    tiny_argp_exit(state, 42);
  switch (key) {
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

/* Parser that calls tiny_argp_state_help(HELP_STD_HELP) when it sees 'a'. */
static int rec_calls_state_help_std(int key, char *arg,
                                    struct tiny_argp_state *state) {
  log_parser_call(key, arg);
  if (key == 'a') {
    tiny_argp_state_help(state, mock_printer, TINY_ARGP_HELP_STD_HELP);
  }
  switch (key) {
  case 'a':
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

static const struct tiny_argp_option opts[] = {
    {"alpha", 'a', 0, 0, "the alpha flag"},
    {"beta", 'b', "VAL", 0, "beta takes val"},
    {0}};

static const struct tiny_argp_option opts_hidden[] = {
    {"visible", 'a', 0, 0, "shown flag"},
    {"secret", 'h', 0, OPTION_HIDDEN, "hidden flag"},
    {0}};

static struct tiny_argp argp_basic = {
    opts,          rec,
    "ARG1 [ARG2]", "pre-doc body\vpost-doc body",
    mock_printer,  mock_err_printer};

static struct tiny_argp argp_state_help = {
    opts, rec_calls_state_help, "", "", mock_printer, mock_err_printer};
static struct tiny_argp argp_state_help_std = {
    opts, rec_calls_state_help_std, "", "", mock_printer, mock_err_printer};
static struct tiny_argp argp_usage = {opts, rec_calls_usage, "",
                                      "",   mock_printer,    mock_err_printer};
static struct tiny_argp argp_error = {opts, rec_calls_error, "",
                                      "",   mock_printer,    mock_err_printer};
static struct tiny_argp argp_exit = {opts, rec_calls_exit, "",
                                     "",   mock_printer,   mock_err_printer};
static struct tiny_argp argp_hidden = {
    opts_hidden, rec, "", "", mock_printer, mock_err_printer};

/* `--help` with default flags returns TINY_ARGP_HELP_OPT (not SUCCESS, not
 * an error code) and emits the full STD_HELP composite to the printer
 * sink (stdout), including pre-doc, options, and post-doc. */
static void test_help_long_option(void) {
  char **argv = build_argv(2, "prog", "--help");
  int r = tiny_argp_parse(&argp_basic, 2, argv, 0, NULL, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_HELP_OPT, r);
  TEST_ASSERT_EQUAL_STRING(
      "Usage: prog [OPTION...] ARG1 [ARG2]\r\n"
      "pre-doc body\r\n"
      "\r\n"
      "  -a, --alpha                the alpha flag\r\n"
      "  -b, --beta=VAL             beta takes val\r\n"
      "  -?, --help                 Give this help list\r\n"
      "      --usage                Give a short usage message\r\n"
      "\r\n"
      "Mandatory or optional arguments to long options are also mandatory or "
      "optional\r\n"
      "for any corresponding short options.\r\n"
      "\r\n"
      "post-doc body\r\n",
      mock_stdout);
  TEST_ASSERT_EQUAL_STRING("", mock_stderr);
}

/* `--usage` returns HELP_OPT and emits only the short usage line to
 * stdout (no options table, no pre/post doc). Long args_doc wraps onto
 * a continuation line with matching indent. */
static void test_usage_long_option(void) {
  char **argv = build_argv(2, "prog", "--usage");
  int r = tiny_argp_parse(&argp_basic, 2, argv, 0, NULL, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_HELP_OPT, r);
  TEST_ASSERT_EQUAL_STRING(
      "Usage: prog [a?] [-b VAL] [--alpha] [--beta=VAL] [--help] [--usage] \r\n"
      "            ARG1 [ARG2]\r\n",
      mock_stdout);
  TEST_ASSERT_EQUAL_STRING("", mock_stderr);
}

/* `-?` is the short-form alias for `--help`: same return code, same
 * built-in behavior. */
static void test_short_help(void) {
  char **argv = build_argv(2, "prog", "-?");
  int r = tiny_argp_parse(&argp_basic, 2, argv, 0, NULL, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_HELP_OPT, r);
  TEST_ASSERT_EQUAL_STRING(
      "Usage: prog [OPTION...] ARG1 [ARG2]\r\n"
      "pre-doc body\r\n"
      "\r\n"
      "  -a, --alpha                the alpha flag\r\n"
      "  -b, --beta=VAL             beta takes val\r\n"
      "  -?, --help                 Give this help list\r\n"
      "      --usage                Give a short usage message\r\n"
      "\r\n"
      "Mandatory or optional arguments to long options are also mandatory or "
      "optional\r\n"
      "for any corresponding short options.\r\n"
      "\r\n"
      "post-doc body\r\n",
      mock_stdout);
}

/* Under NO_EXIT, `--help` still prints the STD_HELP output but the parser
 * returns SUCCESS (not HELP_OPT) — NO_EXIT suppresses the special
 * HELP_OPT return, per header contract "continues as if nothing". */
static void test_help_under_no_exit_continues(void) {
  char **argv = build_argv(2, "prog", "--help");
  int r = tiny_argp_parse(&argp_basic, 2, argv, TINY_ARGP_NO_EXIT, NULL, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_SUCCESS, r);
  TEST_ASSERT_EQUAL_STRING(
      "Usage: prog [OPTION...] ARG1 [ARG2]\r\n"
      "pre-doc body\r\n"
      "\r\n"
      "  -a, --alpha                the alpha flag\r\n"
      "  -b, --beta=VAL             beta takes val\r\n"
      "  -?, --help                 Give this help list\r\n"
      "      --usage                Give a short usage message\r\n"
      "\r\n"
      "Mandatory or optional arguments to long options are also mandatory or "
      "optional\r\n"
      "for any corresponding short options.\r\n"
      "\r\n"
      "post-doc body\r\n",
      mock_stdout);
  TEST_ASSERT_EQUAL_STRING("", mock_stderr);
}

/* Standalone tiny_argp_help(HELP_LONG) renders the user options table
 * (no --help/--usage built-ins because this is called directly, not
 * through parse), followed by the "long options are also mandatory..."
 * footer. */
static void test_help_long_output(void) {
  tiny_argp_help(&argp_basic, mock_printer, TINY_ARGP_HELP_LONG, "prog");
  TEST_ASSERT_EQUAL_STRING("  -a, --alpha                the alpha flag\r\n"
                           "  -b, --beta=VAL             beta takes val\r\n"
                           "\r\n"
                           "Mandatory or optional arguments to long options "
                           "are also mandatory or optional\r\n"
                           "for any corresponding short options.\r\n",
                           mock_stdout);
}

/* HELP_USAGE emits the compact one-line "Usage: ..." form with every option
 * shown in bracket form and args_doc appended at the tail. */
static void test_help_usage_includes_args_doc(void) {
  tiny_argp_help(&argp_basic, mock_printer, TINY_ARGP_HELP_USAGE, "prog");
  TEST_ASSERT_EQUAL_STRING(
      "Usage: prog [a] [-b VAL] [--alpha] [--beta=VAL] ARG1 [ARG2]\r\n",
      mock_stdout);
}

/* HELP_PRE_DOC emits only the portion of `doc` that precedes the `\v`
 * split marker. */
static void test_help_pre_doc(void) {
  tiny_argp_help(&argp_basic, mock_printer, TINY_ARGP_HELP_PRE_DOC, "prog");
  TEST_ASSERT_EQUAL_STRING("pre-doc body\r\n", mock_stdout);
}

/* HELP_POST_DOC emits only the portion of `doc` that follows the `\v`
 * split marker. */
static void test_help_post_doc(void) {
  tiny_argp_help(&argp_basic, mock_printer, TINY_ARGP_HELP_POST_DOC, "prog");
  TEST_ASSERT_EQUAL_STRING("post-doc body\r\n", mock_stdout);
}

/* HELP_SEE emits the standalone "Try ... --help" hint (nothing else). */
static void test_help_see(void) {
  tiny_argp_help(&argp_basic, mock_printer, TINY_ARGP_HELP_SEE, "prog");
  TEST_ASSERT_EQUAL_STRING(
      "Try `prog --help' or `prog --usage' for more information.\r\n",
      mock_stdout);
}

/* HELP_STD_HELP concatenates every section in the canonical order:
 * short usage line, pre-doc, options table, boilerplate, post-doc. */
static void test_help_std_help_composite(void) {
  tiny_argp_help(&argp_basic, mock_printer, TINY_ARGP_HELP_STD_HELP, "prog");
  TEST_ASSERT_EQUAL_STRING("Usage: prog [OPTION...] ARG1 [ARG2]\r\n"
                           "pre-doc body\r\n"
                           "\r\n"
                           "  -a, --alpha                the alpha flag\r\n"
                           "  -b, --beta=VAL             beta takes val\r\n"
                           "\r\n"
                           "Mandatory or optional arguments to long options "
                           "are also mandatory or optional\r\n"
                           "for any corresponding short options.\r\n"
                           "\r\n"
                           "post-doc body\r\n",
                           mock_stdout);
}

/* Default mode `--help`: "Instant exit" path per the header contract. Pin
 * down what the parser actually observes before the library short-circuits.
 * Complement to test_help_long_option, which asserts the return code and
 * printed output but not the delivered key sequence. */
static void test_help_default_key_sequence(void) {
  char **argv = build_argv(2, "prog", "--help");
  int r = tiny_argp_parse(&argp_basic, 2, argv, 0, NULL, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_HELP_OPT, r);
  const int expected[] = {TINY_ARGP_KEY_INIT};
  assert_key_sequence_exact(expected, 1);
}

/* NO_EXIT `--help`: "continues as if nothing". The built-in help key is not
 * a user option, so nothing extra shows up between INIT and NO_ARGS — the
 * sequence matches an empty argv. */
static void test_help_no_exit_key_sequence(void) {
  char **argv = build_argv(2, "prog", "--help");
  int r = tiny_argp_parse(&argp_basic, 2, argv, TINY_ARGP_NO_EXIT, NULL, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_SUCCESS, r);
  const int expected[] = {TINY_ARGP_KEY_INIT, TINY_ARGP_KEY_NO_ARGS,
                          TINY_ARGP_KEY_END, TINY_ARGP_KEY_SUCCESS,
                          TINY_ARGP_KEY_FINI};
  assert_key_sequence_exact(expected, 5);
}

/* Same as test_help_default_key_sequence but for `--usage` — the other
 * built-in "instant exit" trigger. */
static void test_usage_default_key_sequence(void) {
  char **argv = build_argv(2, "prog", "--usage");
  int r = tiny_argp_parse(&argp_basic, 2, argv, 0, NULL, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_HELP_OPT, r);
  const int expected[] = {TINY_ARGP_KEY_INIT};
  assert_key_sequence_exact(expected, 1);
}

/* NO_EXIT `--usage`: mirrors the NO_EXIT --help case. */
static void test_usage_no_exit_key_sequence(void) {
  char **argv = build_argv(2, "prog", "--usage");
  int r = tiny_argp_parse(&argp_basic, 2, argv, TINY_ARGP_NO_EXIT, NULL, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_SUCCESS, r);
  const int expected[] = {TINY_ARGP_KEY_INIT, TINY_ARGP_KEY_NO_ARGS,
                          TINY_ARGP_KEY_END, TINY_ARGP_KEY_SUCCESS,
                          TINY_ARGP_KEY_FINI};
  assert_key_sequence_exact(expected, 5);
}

/* Default mode with an option preceding `--help`: options seen before the
 * built-in are delivered normally, then the instant-exit path is taken. */
static void test_help_after_option_default_key_sequence(void) {
  char **argv = build_argv(3, "prog", "-a", "--help");
  int r = tiny_argp_parse(&argp_basic, 3, argv, 0, NULL, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_HELP_OPT, r);
  const int expected[] = {TINY_ARGP_KEY_INIT, 'a'};
  assert_key_sequence_exact(expected, 2);
}

/* Default mode with `--help` before options: options placed after --help
 * are never delivered — the built-in short-circuits parsing. */
static void test_help_before_option_default_key_sequence(void) {
  char **argv = build_argv(3, "prog", "--help", "-a");
  int r = tiny_argp_parse(&argp_basic, 3, argv, 0, NULL, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_HELP_OPT, r);
  const int expected[] = {TINY_ARGP_KEY_INIT};
  assert_key_sequence_exact(expected, 1);
}

/* Default mode with positionals mixed around `--help`: options before
 * --help are delivered as normal, positionals are not — the instant-exit
 * fires before the positional-delivery phase begins. */
static void test_help_mixed_default_key_sequence(void) {
  char **argv = build_argv(5, "prog", "arg1", "-a", "--help", "arg2");
  int r = tiny_argp_parse(&argp_basic, 5, argv, 0, NULL, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_HELP_OPT, r);
  const int expected[] = {TINY_ARGP_KEY_INIT, 'a'};
  assert_key_sequence_exact(expected, 2);
}

/* NO_EXIT with positionals mixed around `--help`: --help is treated as a
 * no-op ("continues as if nothing"), so every option and positional is
 * delivered — options first, then positionals, per default reorder. */
static void test_help_mixed_no_exit_key_sequence(void) {
  char **argv = build_argv(5, "prog", "arg1", "-a", "--help", "arg2");
  int r = tiny_argp_parse(&argp_basic, 5, argv, TINY_ARGP_NO_EXIT, NULL, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_SUCCESS, r);
  const int expected[] = {TINY_ARGP_KEY_INIT, 'a',
                          TINY_ARGP_KEY_ARG,  TINY_ARGP_KEY_ARG,
                          TINY_ARGP_KEY_END,  TINY_ARGP_KEY_SUCCESS,
                          TINY_ARGP_KEY_FINI};
  assert_key_sequence_exact(expected, 7);
}

/* IN_ORDER default: keys arrive in argv order, so a leading positional is
 * delivered before 'a', and the instant exit at --help drops the trailing
 * positional. */
static void test_help_in_order_default_key_sequence(void) {
  char **argv = build_argv(5, "prog", "arg1", "-a", "--help", "arg2");
  int r = tiny_argp_parse(&argp_basic, 5, argv, TINY_ARGP_IN_ORDER, NULL, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_HELP_OPT, r);
  const int expected[] = {TINY_ARGP_KEY_INIT, TINY_ARGP_KEY_ARG, 'a'};
  assert_key_sequence_exact(expected, 3);
}

/* IN_ORDER | NO_EXIT: strict argv-order delivery with --help silent as a
 * control signal — every option and positional lands in the order it
 * appeared in argv. */
static void test_help_in_order_no_exit_key_sequence(void) {
  char **argv = build_argv(5, "prog", "arg1", "-a", "--help", "arg2");
  int r = tiny_argp_parse(&argp_basic, 5, argv,
                          TINY_ARGP_IN_ORDER | TINY_ARGP_NO_EXIT, NULL, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_SUCCESS, r);
  const int expected[] = {
      TINY_ARGP_KEY_INIT, TINY_ARGP_KEY_ARG, 'a',
      TINY_ARGP_KEY_ARG,  TINY_ARGP_KEY_END, TINY_ARGP_KEY_SUCCESS,
      TINY_ARGP_KEY_FINI};
  assert_key_sequence_exact(expected, 7);
  TEST_ASSERT_EQUAL_STRING("arg1", parser_log[1].arg);
  TEST_ASSERT_EQUAL_STRING("arg2", parser_log[3].arg);
}

/* Default mode `--usage` with mixed args: --usage triggers the same
 * instant-exit as --help, so pre-usage 'a' is delivered and the trailing
 * positional is dropped. */
static void test_usage_mixed_default_key_sequence(void) {
  char **argv = build_argv(5, "prog", "arg1", "-a", "--usage", "arg2");
  int r = tiny_argp_parse(&argp_basic, 5, argv, 0, NULL, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_HELP_OPT, r);
  const int expected[] = {TINY_ARGP_KEY_INIT, 'a'};
  assert_key_sequence_exact(expected, 2);
}

/* IN_ORDER | NO_EXIT `--usage`: same argv-order delivery guarantee as the
 * --help variant; both built-ins are treated identically under NO_EXIT. */
static void test_usage_in_order_no_exit_key_sequence(void) {
  char **argv = build_argv(5, "prog", "arg1", "-a", "--usage", "arg2");
  int r = tiny_argp_parse(&argp_basic, 5, argv,
                          TINY_ARGP_IN_ORDER | TINY_ARGP_NO_EXIT, NULL, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_SUCCESS, r);
  const int expected[] = {
      TINY_ARGP_KEY_INIT, TINY_ARGP_KEY_ARG, 'a',
      TINY_ARGP_KEY_ARG,  TINY_ARGP_KEY_END, TINY_ARGP_KEY_SUCCESS,
      TINY_ARGP_KEY_FINI};
  assert_key_sequence_exact(expected, 7);
  TEST_ASSERT_EQUAL_STRING("arg1", parser_log[1].arg);
  TEST_ASSERT_EQUAL_STRING("arg2", parser_log[3].arg);
}

/* Calling state_help() inside the parser emits help output mid-parse and
 * lets parsing continue to a successful return — it's a printing side
 * effect, not a termination. */
static void test_state_help_from_parser(void) {
  char **argv = build_argv(2, "prog", "-a");
  int r = tiny_argp_parse(&argp_state_help, 2, argv, 0, NULL, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_SUCCESS, r);
  TEST_ASSERT_EQUAL_STRING(
      "  -a, --alpha                the alpha flag\r\n"
      "  -b, --beta=VAL             beta takes val\r\n"
      "  -?, --help                 Give this help list\r\n"
      "      --usage                Give a short usage message\r\n"
      "\r\n"
      "Mandatory or optional arguments to long options are also mandatory or "
      "optional\r\n"
      "for any corresponding short options.\r\n",
      mock_stdout);
}

/* --- tiny_argp_usage() writes to err_printer, not printer. */
static void test_argp_usage_writes_to_err(void) {
  char **argv = build_argv(2, "prog", "-a");
  int r = tiny_argp_parse(&argp_usage, 2, argv, 0, NULL, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_SUCCESS, r);
  TEST_ASSERT_EQUAL_STRING(
      "Usage: prog [OPTION...] \r\n"
      "Try `prog --help' or `prog --usage' for more information.\r\n",
      mock_stderr);
  TEST_ASSERT_EQUAL_STRING("", mock_stdout);
}

/* A parser that calls tiny_argp_error() and then returns a non-UNKNOWN
 * error takes the standard error path: user key, then ERROR/FINI. The
 * emitted message is prefixed with the program name and followed by the
 * standard "Try ... --help" hint. */
static void test_argp_error_format(void) {
  char **argv = build_argv(2, "myprog", "-a");
  int r = tiny_argp_parse(&argp_error, 2, argv, TINY_ARGP_NO_EXIT, NULL, NULL);
  TEST_ASSERT_EQUAL(EINVAL, r);
  const int expected[] = {TINY_ARGP_KEY_INIT, 'a', TINY_ARGP_KEY_ERROR,
                          TINY_ARGP_KEY_FINI};
  assert_key_sequence_exact(expected, 4);
  TEST_ASSERT_EQUAL_STRING(
      "myprog: bad input\r\n"
      "Try `myprog --help' or `myprog --usage' for more information.\r\n",
      mock_stderr);
}

/* tiny_argp_exit(N) makes parse() return N and stops immediately. The exact
 * sequence being {INIT, 'a'}, no ERROR/FINI/SUCCESS follow the user key that
 * called exit. */
static void test_argp_exit_forces_return(void) {
  char **argv = build_argv(3, "prog", "-a", "arg");
  int r = tiny_argp_parse(&argp_exit, 3, argv, TINY_ARGP_NO_EXIT, NULL, NULL);
  TEST_ASSERT_EQUAL(42, r);
  const int expected[] = {TINY_ARGP_KEY_INIT, 'a'};
  assert_key_sequence_exact(expected, 2);
}

/* tiny_argp_state_help is NOT gated by NO_ERRS — it is a direct help
 * emitter, not an error message. NO_ERRS only silences error/usage output.
 * Full STD_HELP is rendered and parsing continues to completion. */
static void test_state_help_no_errs_still_emits(void) {
  char **argv = build_argv(2, "prog", "-a");
  int r = tiny_argp_parse(&argp_state_help_std, 2, argv,
                          TINY_ARGP_NO_ERRS | TINY_ARGP_NO_EXIT, NULL, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_SUCCESS, r);
  const int expected[] = {TINY_ARGP_KEY_INIT,    'a',
                          TINY_ARGP_KEY_NO_ARGS, TINY_ARGP_KEY_END,
                          TINY_ARGP_KEY_SUCCESS, TINY_ARGP_KEY_FINI};
  assert_key_sequence_exact(expected, 6);
  TEST_ASSERT_EQUAL_STRING(
      "Usage: prog [OPTION...] \r\n"
      "\r\n"
      "\r\n"
      "  -a, --alpha                the alpha flag\r\n"
      "  -b, --beta=VAL             beta takes val\r\n"
      "  -?, --help                 Give this help list\r\n"
      "      --usage                Give a short usage message\r\n"
      "\r\n"
      "Mandatory or optional arguments to long options "
      "are also mandatory or optional\r\n"
      "for any corresponding short options.\r\n",
      mock_stdout);
  TEST_ASSERT_EQUAL_STRING("", mock_stderr);
}

/* tiny_argp_usage IS gated by NO_ERRS — it is an error-path helper (writes
 * the usage hint to err_printer when the user did something wrong). Under
 * NO_ERRS it must be fully silent; parsing still proceeds to completion. */
static void test_argp_usage_no_errs_silent(void) {
  char **argv = build_argv(2, "prog", "-a");
  int r = tiny_argp_parse(&argp_usage, 2, argv,
                          TINY_ARGP_NO_ERRS | TINY_ARGP_NO_EXIT, NULL, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_SUCCESS, r);
  const int expected[] = {TINY_ARGP_KEY_INIT,    'a',
                          TINY_ARGP_KEY_NO_ARGS, TINY_ARGP_KEY_END,
                          TINY_ARGP_KEY_SUCCESS, TINY_ARGP_KEY_FINI};
  assert_key_sequence_exact(expected, 6);
  TEST_ASSERT_EQUAL_STRING("", mock_stderr);
  TEST_ASSERT_EQUAL_STRING("", mock_stdout);
}

/* --- tiny_argp_state_help with NO_HELP excludes --help/--usage from
 * output. User options 'a' and 'b' must appear; built-in help must not. */
static void test_state_help_no_help_excludes_builtins(void) {
  char **argv = build_argv(2, "prog", "-a");
  int r = tiny_argp_parse(&argp_state_help_std, 2, argv,
                          TINY_ARGP_NO_HELP | TINY_ARGP_NO_EXIT, NULL, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_SUCCESS, r);
  TEST_ASSERT_EQUAL_STRING("Usage: prog [OPTION...] \r\n"
                           "\r\n"
                           "\r\n"
                           "  -a, --alpha                the alpha flag\r\n"
                           "  -b, --beta=VAL             beta takes val\r\n"
                           "\r\n"
                           "Mandatory or optional arguments to long options "
                           "are also mandatory or optional\r\n"
                           "for any corresponding short options.\r\n",
                           mock_stdout);
}

/* --- tiny_argp_error under NO_ERRS is fully silent. */
static void test_argp_error_no_errs_fully_silent(void) {
  char **argv = build_argv(2, "prog", "-a");
  tiny_argp_parse(&argp_error, 2, argv, TINY_ARGP_NO_ERRS | TINY_ARGP_NO_EXIT,
                  NULL, NULL);
  TEST_ASSERT_EQUAL_STRING("", mock_stderr);
  TEST_ASSERT_EQUAL_STRING("", mock_stdout);
}

/* --- OPTION_HIDDEN is absent from HELP_USAGE output in both the no-arg-shorts
 * compact bracket and the explicit [-h] / [--secret] forms. */
static void test_hidden_option_absent_from_help_usage(void) {
  tiny_argp_help(&argp_hidden, mock_printer, TINY_ARGP_HELP_USAGE, "prog");
  TEST_ASSERT_EQUAL_STRING("Usage: prog [a] [--visible] \r\n", mock_stdout);
}

/* --- NULL args_doc/doc via --usage route returns HELP_OPT and prints
 * a valid usage message. */
static void test_null_doc_usage_ok(void) {
  static struct tiny_argp argp_null_doc2 = {
      opts, rec, NULL, NULL, mock_printer, mock_err_printer};
  char **argv = build_argv(2, "prog", "--usage");
  int r = tiny_argp_parse(&argp_null_doc2, 2, argv, 0, NULL, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_HELP_OPT, r);
  TEST_ASSERT_EQUAL_STRING("Usage: prog [a?] [-b VAL] [--alpha] [--beta=VAL] "
                           "[--help] [--usage] \r\n",
                           mock_stdout);
}

/* --- NULL args_doc/doc via direct tiny_argp_help() call must not crash
 * and must produce the expected HELP_STD_HELP composite output. */
static void test_null_doc_direct_help_call(void) {
  static struct tiny_argp argp_null_doc3 = {
      opts, rec, NULL, NULL, mock_printer, mock_err_printer};
  tiny_argp_help(&argp_null_doc3, mock_printer, TINY_ARGP_HELP_STD_HELP,
                 "prog");
  TEST_ASSERT_EQUAL_STRING("Usage: prog [OPTION...] \r\n"
                           "\r\n"
                           "\r\n"
                           "  -a, --alpha                the alpha flag\r\n"
                           "  -b, --beta=VAL             beta takes val\r\n"
                           "\r\n"
                           "Mandatory or optional arguments to long options "
                           "are also mandatory or optional\r\n"
                           "for any corresponding short options.\r\n",
                           mock_stdout);
}

/* --- NULL args_doc and doc must not crash on help emission; they behave
 * like empty strings. */
static void test_null_doc_ok(void) {
  static struct tiny_argp argp_null_doc = {
      opts, rec, NULL, NULL, mock_printer, mock_err_printer};
  char **argv = build_argv(2, "prog", "--help");
  int r = tiny_argp_parse(&argp_null_doc, 2, argv, 0, NULL, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_HELP_OPT, r);
  TEST_ASSERT_EQUAL_STRING(
      "Usage: prog [OPTION...] \r\n"
      "\r\n"
      "\r\n"
      "  -a, --alpha                the alpha flag\r\n"
      "  -b, --beta=VAL             beta takes val\r\n"
      "  -?, --help                 Give this help list\r\n"
      "      --usage                Give a short usage message\r\n"
      "\r\n"
      "Mandatory or optional arguments to long options are also mandatory or "
      "optional\r\n"
      "for any corresponding short options.\r\n",
      mock_stdout);
}

void run_help_tests(void) {
  RUN_TEST(test_help_long_option);
  RUN_TEST(test_usage_long_option);
  RUN_TEST(test_short_help);
  RUN_TEST(test_help_under_no_exit_continues);
  RUN_TEST(test_help_default_key_sequence);
  RUN_TEST(test_help_no_exit_key_sequence);
  RUN_TEST(test_usage_default_key_sequence);
  RUN_TEST(test_usage_no_exit_key_sequence);
  RUN_TEST(test_help_after_option_default_key_sequence);
  RUN_TEST(test_help_before_option_default_key_sequence);
  RUN_TEST(test_help_mixed_default_key_sequence);
  RUN_TEST(test_help_mixed_no_exit_key_sequence);
  RUN_TEST(test_help_in_order_default_key_sequence);
  RUN_TEST(test_help_in_order_no_exit_key_sequence);
  RUN_TEST(test_usage_mixed_default_key_sequence);
  RUN_TEST(test_usage_in_order_no_exit_key_sequence);
  RUN_TEST(test_help_long_output);
  RUN_TEST(test_help_usage_includes_args_doc);
  RUN_TEST(test_help_pre_doc);
  RUN_TEST(test_help_post_doc);
  RUN_TEST(test_help_see);
  RUN_TEST(test_help_std_help_composite);
  RUN_TEST(test_state_help_from_parser);
  RUN_TEST(test_argp_usage_writes_to_err);
  RUN_TEST(test_argp_error_format);
  RUN_TEST(test_argp_exit_forces_return);
  RUN_TEST(test_argp_error_no_errs_fully_silent);
  RUN_TEST(test_argp_usage_no_errs_silent);
  RUN_TEST(test_state_help_no_errs_still_emits);
  RUN_TEST(test_state_help_no_help_excludes_builtins);
  RUN_TEST(test_hidden_option_absent_from_help_usage);
  RUN_TEST(test_null_doc_ok);
  RUN_TEST(test_null_doc_usage_ok);
  RUN_TEST(test_null_doc_direct_help_call);
}
