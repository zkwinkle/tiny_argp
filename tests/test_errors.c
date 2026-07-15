#include <errno.h>

#include "test_utils.h"
#include "unity.h"

static int rec(int key, char *arg, struct tiny_argp_state *state) {
  (void)state;
  log_parser_call(key, arg);
  switch (key) {
  case 'a':
  case 'f':
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

/* Returns a custom code on 'a'. */
static int rec_returns_erange(int key, char *arg,
                              struct tiny_argp_state *state) {
  (void)state;
  log_parser_call(key, arg);
  if (key == 'a')
    return ERANGE;
  switch (key) {
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

/* Returns ERR_UNKNOWN on 'a' (a user-defined option key) — this is the
 * PROGRAM ERROR path. */
static int rec_returns_unknown_on_a(int key, char *arg,
                                    struct tiny_argp_state *state) {
  (void)state;
  log_parser_call(key, arg);
  if (key == 'a')
    return TINY_ARGP_ERR_UNKNOWN;
  switch (key) {
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

/* Returns EINVAL on KEY_ARG. */
static int rec_einval_on_key_arg(int key, char *arg,
                                 struct tiny_argp_state *state) {
  (void)state;
  log_parser_call(key, arg);
  if (key == TINY_ARGP_KEY_ARG)
    return EINVAL;
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

/* Returns EINVAL on KEY_END. */
static int rec_einval_on_key_end(int key, char *arg,
                                 struct tiny_argp_state *state) {
  (void)state;
  log_parser_call(key, arg);
  if (key == TINY_ARGP_KEY_END)
    return EINVAL;
  switch (key) {
  case 'a':
  case TINY_ARGP_KEY_ARG:
  case TINY_ARGP_KEY_INIT:
  case TINY_ARGP_KEY_NO_ARGS:
  case TINY_ARGP_KEY_SUCCESS:
  case TINY_ARGP_KEY_ERROR:
  case TINY_ARGP_KEY_FINI:
    return TINY_ARGP_SUCCESS;
  default:
    return TINY_ARGP_ERR_UNKNOWN;
  }
}

/* Returns EINVAL on KEY_NO_ARGS. */
static int rec_einval_on_key_no_args(int key, char *arg,
                                     struct tiny_argp_state *state) {
  (void)state;
  log_parser_call(key, arg);
  if (key == TINY_ARGP_KEY_NO_ARGS)
    return EINVAL;
  switch (key) {
  case 'a':
  case TINY_ARGP_KEY_ARG:
  case TINY_ARGP_KEY_INIT:
  case TINY_ARGP_KEY_END:
  case TINY_ARGP_KEY_SUCCESS:
  case TINY_ARGP_KEY_ERROR:
  case TINY_ARGP_KEY_FINI:
    return TINY_ARGP_SUCCESS;
  default:
    return TINY_ARGP_ERR_UNKNOWN;
  }
}

/* Returns UNKNOWN on KEY_ARG (triggers KEY_ARGS), then EINVAL on KEY_ARGS. */
static int rec_einval_on_key_args(int key, char *arg,
                                  struct tiny_argp_state *state) {
  (void)state;
  log_parser_call(key, arg);
  if (key == TINY_ARGP_KEY_ARG)
    return TINY_ARGP_ERR_UNKNOWN;
  if (key == TINY_ARGP_KEY_ARGS)
    return EINVAL;
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

/* Returns EINVAL on KEY_FINI. */
static int rec_einval_on_key_fini(int key, char *arg,
                                  struct tiny_argp_state *state) {
  (void)state;
  log_parser_call(key, arg);
  if (key == TINY_ARGP_KEY_FINI)
    return EINVAL;
  switch (key) {
  case 'a':
  case TINY_ARGP_KEY_ARG:
  case TINY_ARGP_KEY_INIT:
  case TINY_ARGP_KEY_END:
  case TINY_ARGP_KEY_NO_ARGS:
  case TINY_ARGP_KEY_SUCCESS:
  case TINY_ARGP_KEY_ERROR:
    return TINY_ARGP_SUCCESS;
  default:
    return TINY_ARGP_ERR_UNKNOWN;
  }
}

/* Returns EINVAL on KEY_SUCCESS. */
static int rec_einval_on_key_success(int key, char *arg,
                                     struct tiny_argp_state *state) {
  (void)state;
  log_parser_call(key, arg);
  if (key == TINY_ARGP_KEY_SUCCESS)
    return EINVAL;
  switch (key) {
  case 'a':
  case TINY_ARGP_KEY_ARG:
  case TINY_ARGP_KEY_INIT:
  case TINY_ARGP_KEY_END:
  case TINY_ARGP_KEY_NO_ARGS:
  case TINY_ARGP_KEY_ERROR:
  case TINY_ARGP_KEY_FINI:
    return TINY_ARGP_SUCCESS;
  default:
    return TINY_ARGP_ERR_UNKNOWN;
  }
}

static const struct tiny_argp_option opts[] = {
    {"alpha", 'a', 0, 0, "flag"}, {"file", 'f', "PATH", 0, "path"}, {0}};

/* Options with only 'a' defined — used by the bundled-unknown tests. */
static const struct tiny_argp_option opts_a_only[] = {
    {"alpha", 'a', 0, 0, "flag"}, {0}};

static int rec_a_only(int key, char *arg, struct tiny_argp_state *state) {
  (void)state;
  log_parser_call(key, arg);
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

static struct tiny_argp argp = {opts, rec,          "",
                                "",   mock_printer, mock_err_printer};
static struct tiny_argp argp_a_only = {opts_a_only,  rec_a_only,      "", "",
                                       mock_printer, mock_err_printer};
static struct tiny_argp argp_erange = {opts,         rec_returns_erange, "", "",
                                       mock_printer, mock_err_printer};
static struct tiny_argp argp_unknown_on_a = {
    opts, rec_returns_unknown_on_a, "", "", mock_printer, mock_err_printer};
static struct tiny_argp argp_einval_key_arg = {
    opts, rec_einval_on_key_arg, "", "", mock_printer, mock_err_printer};
static struct tiny_argp argp_einval_key_end = {
    opts, rec_einval_on_key_end, "", "", mock_printer, mock_err_printer};
static struct tiny_argp argp_einval_key_no_args = {
    opts, rec_einval_on_key_no_args, "", "", mock_printer, mock_err_printer};
static struct tiny_argp argp_einval_key_success = {
    opts, rec_einval_on_key_success, "", "", mock_printer, mock_err_printer};
static struct tiny_argp argp_einval_key_args = {
    opts, rec_einval_on_key_args, "", "", mock_printer, mock_err_printer};
static struct tiny_argp argp_einval_key_fini = {
    opts, rec_einval_on_key_fini, "", "", mock_printer, mock_err_printer};

/* Unknown short opt `-Z` is rejected before any user key is delivered:
 * INIT -> ERROR -> FINI, EINVAL, "invalid option 'Z'" on stderr. */
static void test_unknown_short(void) {
  char **argv = build_argv(2, "prog", "-Z");
  int r = tiny_argp_parse(&argp, 2, argv, TINY_ARGP_NO_EXIT, NULL, NULL);
  TEST_ASSERT_EQUAL(EINVAL, r);
  const int expected[] = {TINY_ARGP_KEY_INIT, TINY_ARGP_KEY_ERROR,
                          TINY_ARGP_KEY_FINI};
  assert_key_sequence_exact(expected, 3);
  TEST_ASSERT_EQUAL_STRING(
      "prog: invalid option 'Z'\r\n"
      "Try `prog --help' or `prog --usage' for more information.\r\n",
      mock_stderr);
}

/* Same unknown-short case under default flags: dispatch stops after INIT
 * with no ERROR / FINI delivered. Return code and stderr message match
 * the NO_EXIT variant. */
static void test_unknown_short_default(void) {
  char **argv = build_argv(2, "prog", "-Z");
  int r = tiny_argp_parse(&argp, 2, argv, 0, NULL, NULL);
  TEST_ASSERT_EQUAL(EINVAL, r);
  const int expected[] = {TINY_ARGP_KEY_INIT};
  assert_key_sequence_exact(expected, 1);
  TEST_ASSERT_EQUAL_STRING(
      "prog: invalid option 'Z'\r\n"
      "Try `prog --help' or `prog --usage' for more information.\r\n",
      mock_stderr);
}

/* Unknown long opt `--bogus` is rejected before any user key is delivered.
 * Note "unrecognized option" wording (distinct from short-opt "invalid
 * option"). */
static void test_unknown_long(void) {
  char **argv = build_argv(2, "prog", "--bogus");
  int r = tiny_argp_parse(&argp, 2, argv, TINY_ARGP_NO_EXIT, NULL, NULL);
  TEST_ASSERT_EQUAL(EINVAL, r);
  const int expected[] = {TINY_ARGP_KEY_INIT, TINY_ARGP_KEY_ERROR,
                          TINY_ARGP_KEY_FINI};
  assert_key_sequence_exact(expected, 3);
  TEST_ASSERT_EQUAL_STRING(
      "prog: unrecognized option '--bogus'\r\n"
      "Try `prog --help' or `prog --usage' for more information.\r\n",
      mock_stderr);
}

/* Same unknown-long case under default flags: dispatch stops after INIT
 * with no ERROR / FINI delivered. */
static void test_unknown_long_default(void) {
  char **argv = build_argv(2, "prog", "--bogus");
  int r = tiny_argp_parse(&argp, 2, argv, 0, NULL, NULL);
  TEST_ASSERT_EQUAL(EINVAL, r);
  const int expected[] = {TINY_ARGP_KEY_INIT};
  assert_key_sequence_exact(expected, 1);
  TEST_ASSERT_EQUAL_STRING(
      "prog: unrecognized option '--bogus'\r\n"
      "Try `prog --help' or `prog --usage' for more information.\r\n",
      mock_stderr);
}

/* Short opt at end of argv with no attached/next-slot value: EINVAL, key 'f'
 * is NOT delivered. */
static void test_short_missing_arg(void) {
  char **argv = build_argv(2, "prog", "-f");
  int r = tiny_argp_parse(&argp, 2, argv, TINY_ARGP_NO_EXIT, NULL, NULL);
  TEST_ASSERT_EQUAL(EINVAL, r);
  const int expected[] = {TINY_ARGP_KEY_INIT, TINY_ARGP_KEY_ERROR,
                          TINY_ARGP_KEY_FINI};
  assert_key_sequence_exact(expected, 3);
  TEST_ASSERT_EQUAL_STRING(
      "prog: option requires an argument -- 'f'\r\n"
      "Try `prog --help' or `prog --usage' for more information.\r\n",
      mock_stderr);
}

/* Long opt at end of argv with no value: EINVAL, key 'f' is NOT delivered.
 * Long-form message reads "option '--file' requires an argument" — different
 * wording from the short-form counterpart. */
static void test_long_missing_arg(void) {
  char **argv = build_argv(2, "prog", "--file");
  int r = tiny_argp_parse(&argp, 2, argv, TINY_ARGP_NO_EXIT, NULL, NULL);
  TEST_ASSERT_EQUAL(EINVAL, r);
  const int expected[] = {TINY_ARGP_KEY_INIT, TINY_ARGP_KEY_ERROR,
                          TINY_ARGP_KEY_FINI};
  assert_key_sequence_exact(expected, 3);
  TEST_ASSERT_EQUAL_STRING(
      "prog: option '--file' requires an argument\r\n"
      "Try `prog --help' or `prog --usage' for more information.\r\n",
      mock_stderr);
}

/* Long boolean opt given a value (`--alpha=nope`) is rejected before 'a' is
 * delivered; error message quotes the full token including `=nope`. */
static void test_long_unwanted_arg(void) {
  char **argv = build_argv(2, "prog", "--alpha=nope");
  int r = tiny_argp_parse(&argp, 2, argv, TINY_ARGP_NO_EXIT, NULL, NULL);
  TEST_ASSERT_EQUAL(EINVAL, r);
  const int expected[] = {TINY_ARGP_KEY_INIT, TINY_ARGP_KEY_ERROR,
                          TINY_ARGP_KEY_FINI};
  assert_key_sequence_exact(expected, 3);
  TEST_ASSERT_EQUAL_STRING(
      "prog: option '--alpha=nope' doesn't allow an argument\r\n"
      "Try `prog --help' or `prog --usage' for more information.\r\n",
      mock_stderr);
}

/* Custom (non-UNKNOWN) return code from user parser propagates as-is.
 * Under default mode dispatch stops after the offending user key —
 * KEY_ERROR / KEY_SUCCESS / KEY_FINI are NOT fired. tiny_argp itself
 * prints nothing for user-returned codes. */
static void test_parser_erange_propagates_default(void) {
  char **argv = build_argv(2, "prog", "-a");
  int r = tiny_argp_parse(&argp_erange, 2, argv, 0, NULL, NULL);
  TEST_ASSERT_EQUAL(ERANGE, r);
  const int expected[] = {TINY_ARGP_KEY_INIT, 'a'};
  assert_key_sequence_exact(expected, 2);
  TEST_ASSERT_EQUAL_STRING("", mock_stdout);
  TEST_ASSERT_EQUAL_STRING("", mock_stderr);
}

/* Same custom code under NO_EXIT: ERROR + FINI *are* delivered after the
 * user key that failed. tiny_argp still prints nothing (user-returned code,
 * not one of the parser-detected errors). */
static void test_parser_erange_no_exit(void) {
  char **argv = build_argv(2, "prog", "-a");
  int r = tiny_argp_parse(&argp_erange, 2, argv, TINY_ARGP_NO_EXIT, NULL, NULL);
  TEST_ASSERT_EQUAL(ERANGE, r);
  const int expected[] = {TINY_ARGP_KEY_INIT, 'a', TINY_ARGP_KEY_ERROR,
                          TINY_ARGP_KEY_FINI};
  assert_key_sequence_exact(expected, 4);
  TEST_ASSERT_EQUAL_STRING("", mock_stderr);
}

/* Returning TINY_ARGP_ERR_UNKNOWN from a user-DEFINED key ('a') is the
 * PROGRAM ERROR path: library treats it as a user bug. Under NO_EXIT the
 * user key IS delivered before dispatch flips to ERROR/FINI. stderr carries
 * the distinctive "(PROGRAM ERROR)" message. */
static void test_parser_unknown_on_user_key_is_program_error(void) {
  char **argv = build_argv(2, "prog", "-a");
  int r = tiny_argp_parse(&argp_unknown_on_a, 2, argv, TINY_ARGP_NO_EXIT, NULL,
                          NULL);
  TEST_ASSERT_EQUAL(EINVAL, r);
  const int expected[] = {TINY_ARGP_KEY_INIT, 'a', TINY_ARGP_KEY_ERROR,
                          TINY_ARGP_KEY_FINI};
  assert_key_sequence_exact(expected, 4);
  TEST_ASSERT_EQUAL_STRING(
      "prog: -a: (PROGRAM ERROR) Option should have been recognized!?\r\n"
      "Try `prog --help' or `prog --usage' for more information.\r\n",
      mock_stderr);
}

/* The program name string in error output comes verbatim from argv[0] —
 * no basename stripping, no default. */
static void test_program_name_in_err_output(void) {
  char **argv = build_argv(2, "my-prog-name", "--bogus");
  int r = tiny_argp_parse(&argp, 2, argv, TINY_ARGP_NO_EXIT, NULL, NULL);
  TEST_ASSERT_EQUAL(EINVAL, r);
  const int expected[] = {TINY_ARGP_KEY_INIT, TINY_ARGP_KEY_ERROR,
                          TINY_ARGP_KEY_FINI};
  assert_key_sequence_exact(expected, 3);
  TEST_ASSERT_EQUAL_STRING("my-prog-name: unrecognized option '--bogus'\r\n"
                           "Try `my-prog-name --help' or `my-prog-name "
                           "--usage' for more information.\r\n",
                           mock_stderr);
}

/* Error output goes exclusively through err_printer — the regular printer
 * (mock_stdout) must remain untouched on an unknown option. */
static void test_err_output_uses_err_printer(void) {
  char **argv = build_argv(2, "prog", "--bogus");
  int r = tiny_argp_parse(&argp, 2, argv, TINY_ARGP_NO_EXIT, NULL, NULL);
  TEST_ASSERT_EQUAL(EINVAL, r);
  const int expected[] = {TINY_ARGP_KEY_INIT, TINY_ARGP_KEY_ERROR,
                          TINY_ARGP_KEY_FINI};
  assert_key_sequence_exact(expected, 3);
  TEST_ASSERT_EQUAL_STRING(
      "prog: unrecognized option '--bogus'\r\n"
      "Try `prog --help' or `prog --usage' for more information.\r\n",
      mock_stderr);
  TEST_ASSERT_EQUAL_STRING("", mock_stdout);
}

/* User error returned from KEY_ARG under NO_EXIT: dispatch pivots to
 * ERROR + FINI (not SUCCESS). Library emits nothing on stderr — this is a
 * user-parser return, not a library-detected error. */
static void test_parser_error_from_key_arg(void) {
  char **argv = build_argv(2, "prog", "positional");
  int r = tiny_argp_parse(&argp_einval_key_arg, 2, argv, TINY_ARGP_NO_EXIT,
                          NULL, NULL);
  TEST_ASSERT_EQUAL(EINVAL, r);
  const int expected[] = {TINY_ARGP_KEY_INIT, TINY_ARGP_KEY_ARG,
                          TINY_ARGP_KEY_ERROR, TINY_ARGP_KEY_FINI};
  assert_key_sequence_exact(expected, 4);
  TEST_ASSERT_EQUAL_STRING("", mock_stderr);
}

/* User error returned from KEY_END: dispatch still pivots to ERROR + FINI
 * (not SUCCESS), even though the error came from a late-lifecycle key. */
static void test_parser_error_from_key_end(void) {
  char **argv = build_argv(2, "prog", "-a");
  int r = tiny_argp_parse(&argp_einval_key_end, 2, argv, TINY_ARGP_NO_EXIT,
                          NULL, NULL);
  TEST_ASSERT_EQUAL(EINVAL, r);
  const int expected[] = {TINY_ARGP_KEY_INIT,    'a',
                          TINY_ARGP_KEY_NO_ARGS, TINY_ARGP_KEY_END,
                          TINY_ARGP_KEY_ERROR,   TINY_ARGP_KEY_FINI};
  assert_key_sequence_exact(expected, 6);
  TEST_ASSERT_EQUAL_STRING("", mock_stderr);
}

/* TODO: this test locks in undesired behaviour. When the parser errors on
 * KEY_NO_ARGS, KEY_END is still delivered afterwards. Ideally the parser's
 * error would short-circuit remaining dispatch. Update this test's
 * expected sequence once the behaviour is fixed.
 *
 * Pins down the current behaviour: KEY_END fires even after KEY_NO_ARGS
 * returned an error, then ERROR + FINI close things out. */
static void test_parser_error_from_key_no_args(void) {
  char **argv = build_argv(1, "prog");
  int r = tiny_argp_parse(&argp_einval_key_no_args, 1, argv, TINY_ARGP_NO_EXIT,
                          NULL, NULL);
  TEST_ASSERT_EQUAL(EINVAL, r);
  const int expected[] = {TINY_ARGP_KEY_INIT, TINY_ARGP_KEY_NO_ARGS,
                          TINY_ARGP_KEY_END, TINY_ARGP_KEY_ERROR,
                          TINY_ARGP_KEY_FINI};
  assert_key_sequence_exact(expected, 5);
  TEST_ASSERT_EQUAL_STRING("", mock_stderr);
}

/* An error returned from KEY_SUCCESS does NOT rewind to
 * KEY_ERROR. By the time KEY_SUCCESS fires the success-vs-error branch has
 * already been chosen; dispatch continues straight to FINI, but the
 * returned code is still reported as the parse return value. */
static void test_parser_error_from_key_success(void) {
  char **argv = build_argv(2, "prog", "-a");
  int r = tiny_argp_parse(&argp_einval_key_success, 2, argv, TINY_ARGP_NO_EXIT,
                          NULL, NULL);
  TEST_ASSERT_EQUAL(EINVAL, r);
  const int expected[] = {TINY_ARGP_KEY_INIT,    'a',
                          TINY_ARGP_KEY_NO_ARGS, TINY_ARGP_KEY_END,
                          TINY_ARGP_KEY_SUCCESS, TINY_ARGP_KEY_FINI};
  assert_key_sequence_exact(expected, 6);
  TEST_ASSERT_EQUAL_STRING("", mock_stderr);
}

/* Same PROGRAM ERROR path but under default flags (no NO_EXIT): the library
 * still detects the "UNKNOWN on known key" condition and prints its
 * diagnostic, then dispatch stops after the offending user key — no
 * ERROR / FINI delivered. */
static void test_parser_unknown_on_user_key_is_program_error_default(void) {
  char **argv = build_argv(2, "prog", "-a");
  int r = tiny_argp_parse(&argp_unknown_on_a, 2, argv, 0, NULL, NULL);
  TEST_ASSERT_EQUAL(EINVAL, r);
  const int expected[] = {TINY_ARGP_KEY_INIT, 'a'};
  assert_key_sequence_exact(expected, 2);
  TEST_ASSERT_EQUAL_STRING(
      "prog: -a: (PROGRAM ERROR) Option should have been recognized!?\r\n"
      "Try `prog --help' or `prog --usage' for more information.\r\n",
      mock_stderr);
}

/* User error returned from KEY_ARGS under NO_EXIT: KEY_ARG was rejected with
 * UNKNOWN to trigger the bulk KEY_ARGS call, which then fails. Dispatch
 * pivots to ERROR + FINI. */
static void test_parser_error_from_key_args(void) {
  char **argv = build_argv(2, "prog", "positional");
  int r = tiny_argp_parse(&argp_einval_key_args, 2, argv, TINY_ARGP_NO_EXIT,
                          NULL, NULL);
  TEST_ASSERT_EQUAL(EINVAL, r);
  const int expected[] = {TINY_ARGP_KEY_INIT, TINY_ARGP_KEY_ARG,
                          TINY_ARGP_KEY_ARGS, TINY_ARGP_KEY_ERROR,
                          TINY_ARGP_KEY_FINI};
  assert_key_sequence_exact(expected, 5);
  TEST_ASSERT_EQUAL_STRING("", mock_stderr);
}

/* User error returned from KEY_FINI is **silently discarded**: the parse
 * return code stays SUCCESS. FINI is the final key by contract, so its
 * return value has no dispatch left to influence. This test locks in that
 * observed behavior from argp. */
static void test_parser_error_from_key_fini(void) {
  char **argv = build_argv(1, "prog");
  int r = tiny_argp_parse(&argp_einval_key_fini, 1, argv, TINY_ARGP_NO_EXIT,
                          NULL, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_SUCCESS, r);
  const int expected[] = {TINY_ARGP_KEY_INIT, TINY_ARGP_KEY_NO_ARGS,
                          TINY_ARGP_KEY_END, TINY_ARGP_KEY_SUCCESS,
                          TINY_ARGP_KEY_FINI};
  assert_key_sequence_exact(expected, 5);
  TEST_ASSERT_EQUAL_STRING("", mock_stderr);
}

/* Bundled short opts — error names 'Z', not the full bundle "-aZb".
 * 'a' is processed successfully first; then 'Z' triggers the error.
 *
 * The error message names only the offending char ('Z'), not the whole
 * bundle ('-aZb'). This matches a "bundle == `-a -Z -b`" mental model and
 * is intentional. TODO: consider whether reporting the full bundle would
 * be clearer in some cases.
 */
static void test_bundled_short_unknown_char_error_message(void) {
  char **argv = build_argv(2, "prog", "-aZb");
  int r = tiny_argp_parse(&argp_a_only, 2, argv, TINY_ARGP_NO_EXIT, NULL, NULL);
  TEST_ASSERT_EQUAL(EINVAL, r);
  /* 'a' is delivered before 'Z' triggers the error. */
  const int expected[] = {TINY_ARGP_KEY_INIT, 'a', TINY_ARGP_KEY_ERROR,
                          TINY_ARGP_KEY_FINI};
  assert_key_sequence_exact(expected, 4);
  /* Error message names just 'Z', not "-aZb". */
  TEST_ASSERT_EQUAL_STRING(
      "prog: invalid option 'Z'\r\n"
      "Try `prog --help' or `prog --usage' for more information.\r\n",
      mock_stderr);
}

/* '-Za' — 'Z' is first, error fires before 'a' is ever visited. */
static void test_bundled_short_unknown_first_char_error(void) {
  char **argv = build_argv(2, "prog", "-Za");
  int r = tiny_argp_parse(&argp_a_only, 2, argv, TINY_ARGP_NO_EXIT, NULL, NULL);
  TEST_ASSERT_EQUAL(EINVAL, r);
  /* 'a' must not have been delivered — error fires before we get to it. */
  const int expected[] = {TINY_ARGP_KEY_INIT, TINY_ARGP_KEY_ERROR,
                          TINY_ARGP_KEY_FINI};
  assert_key_sequence_exact(expected, 3);
  TEST_ASSERT_EQUAL_STRING(
      "prog: invalid option 'Z'\r\n"
      "Try `prog --help' or `prog --usage' for more information.\r\n",
      mock_stderr);
}

void run_errors_tests(void) {
  RUN_TEST(test_unknown_short);
  RUN_TEST(test_unknown_short_default);
  RUN_TEST(test_unknown_long);
  RUN_TEST(test_unknown_long_default);
  RUN_TEST(test_short_missing_arg);
  RUN_TEST(test_long_missing_arg);
  RUN_TEST(test_long_unwanted_arg);
  RUN_TEST(test_parser_erange_propagates_default);
  RUN_TEST(test_parser_erange_no_exit);
  RUN_TEST(test_parser_unknown_on_user_key_is_program_error);
  RUN_TEST(test_parser_unknown_on_user_key_is_program_error_default);
  RUN_TEST(test_program_name_in_err_output);
  RUN_TEST(test_err_output_uses_err_printer);
  RUN_TEST(test_parser_error_from_key_arg);
  RUN_TEST(test_parser_error_from_key_end);
  RUN_TEST(test_parser_error_from_key_no_args);
  RUN_TEST(test_parser_error_from_key_success);
  RUN_TEST(test_parser_error_from_key_args);
  RUN_TEST(test_parser_error_from_key_fini);
  RUN_TEST(test_bundled_short_unknown_char_error_message);
  RUN_TEST(test_bundled_short_unknown_first_char_error);
}
