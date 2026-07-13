#include "test_utils.h"
#include "unity.h"

static int rec_parser(int key, char *arg, struct tiny_argp_state *state) {
  (void)state;
  log_parser_call(key, arg);
  switch (key) {
  case 'a':
  case 'f':
  case 'v':
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

static const struct tiny_argp_option opts_bool[] = {
    {"alpha", 'a', 0, 0, "flag"}, {"verbose", 'v', 0, 0, "verbose"}, {0}};

static const struct tiny_argp_option opts_takes_arg[] = {
    {"alpha", 'a', 0, 0, "flag"}, {"file", 'f', "PATH", 0, "path"}, {0}};

static struct tiny_argp argp_bool = {opts_bool, rec_parser,   "",
                                     "",        mock_printer, mock_err_printer};
static struct tiny_argp argp_takes_arg = {
    opts_takes_arg, rec_parser, "", "", mock_printer, mock_err_printer};

/* Long boolean flag `--alpha` delivers key 'a' with no arg. */
static void test_long_flag(void) {
  char **argv = build_argv(2, "prog", "--alpha");
  int r = tiny_argp_parse(&argp_bool, 2, argv, 0, NULL, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_SUCCESS, r);
  TEST_ASSERT_EQUAL('a', parser_log[1].key);
  TEST_ASSERT_FALSE(parser_log[1].has_arg);
}

/* `--name=value` form: value attached to the option via `=`. */
static void test_long_with_equals(void) {
  char **argv = build_argv(2, "prog", "--file=my/path");
  int r = tiny_argp_parse(&argp_takes_arg, 2, argv, 0, NULL, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_SUCCESS, r);
  TEST_ASSERT_EQUAL('f', parser_log[1].key);
  TEST_ASSERT_EQUAL_STRING("my/path", parser_log[1].arg);
}

/* `--name value` form: value lives in the next argv slot. */
static void test_long_with_separate_arg(void) {
  char **argv = build_argv(3, "prog", "--file", "my/path");
  int r = tiny_argp_parse(&argp_takes_arg, 3, argv, 0, NULL, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_SUCCESS, r);
  TEST_ASSERT_EQUAL('f', parser_log[1].key);
  TEST_ASSERT_EQUAL_STRING("my/path", parser_log[1].arg);
}

/* Long option that requires an arg but has none at end of argv: EINVAL.
 * Parser is called with INIT, then jumps straight to ERROR/FINI — key 'f'
 * must not be delivered. stderr carries the "requires an argument" message
 * followed by the standard "Try ... --help" hint. */
static void test_long_missing_required_arg(void) {
  char **argv = build_argv(2, "prog", "--file");
  int r =
      tiny_argp_parse(&argp_takes_arg, 2, argv, TINY_ARGP_NO_EXIT, NULL, NULL);
  TEST_ASSERT_EQUAL(EINVAL, r);
  const int expected[] = {TINY_ARGP_KEY_INIT, TINY_ARGP_KEY_ERROR,
                          TINY_ARGP_KEY_FINI};
  assert_key_sequence_exact(expected, 3);
  TEST_ASSERT_EQUAL_STRING(
      "prog: option '--file' requires an argument\r\n"
      "Try `prog --help' or `prog --usage' for more information.\r\n",
      mock_stderr);
}

/* Long boolean flag with `--flag=value` should error (flag takes no arg).
 * Parser must see INIT -> ERROR -> FINI; key 'a' must not be delivered.
 * The rejection message quotes the full argv token including `=nope`. */
static void test_long_bool_gets_unwanted_arg(void) {
  char **argv = build_argv(2, "prog", "--alpha=nope");
  int r = tiny_argp_parse(&argp_bool, 2, argv, TINY_ARGP_NO_EXIT, NULL, NULL);
  TEST_ASSERT_EQUAL(EINVAL, r);
  const int expected[] = {TINY_ARGP_KEY_INIT, TINY_ARGP_KEY_ERROR,
                          TINY_ARGP_KEY_FINI};
  assert_key_sequence_exact(expected, 3);
  TEST_ASSERT_EQUAL_STRING(
      "prog: option '--alpha=nope' doesn't allow an argument\r\n"
      "Try `prog --help' or `prog --usage' for more information.\r\n",
      mock_stderr);
}

/* Unknown long option `--bogus`: EINVAL, INIT -> ERROR -> FINI. stderr
 * reports "unrecognized option". */
static void test_long_unknown(void) {
  char **argv = build_argv(2, "prog", "--bogus");
  int r = tiny_argp_parse(&argp_bool, 2, argv, TINY_ARGP_NO_EXIT, NULL, NULL);
  TEST_ASSERT_EQUAL(EINVAL, r);
  const int expected[] = {TINY_ARGP_KEY_INIT, TINY_ARGP_KEY_ERROR,
                          TINY_ARGP_KEY_FINI};
  assert_key_sequence_exact(expected, 3);
  TEST_ASSERT_EQUAL_STRING(
      "prog: unrecognized option '--bogus'\r\n"
      "Try `prog --help' or `prog --usage' for more information.\r\n",
      mock_stderr);
}

/* Empty value: `--file=` — parser sees an empty string. */
static void test_long_empty_value(void) {
  char **argv = build_argv(2, "prog", "--file=");
  int r = tiny_argp_parse(&argp_takes_arg, 2, argv, 0, NULL, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_SUCCESS, r);
  TEST_ASSERT_EQUAL('f', parser_log[1].key);
  TEST_ASSERT_TRUE(parser_log[1].has_arg);
  TEST_ASSERT_EQUAL_STRING("", parser_log[1].arg);
}

/* Repeated long option: parser called twice. */
static void test_long_repeated(void) {
  char **argv = build_argv(3, "prog", "--file=one", "--file=two");
  int r = tiny_argp_parse(&argp_takes_arg, 3, argv, 0, NULL, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_SUCCESS, r);
  const int expected[] = {TINY_ARGP_KEY_INIT,
                          'f',
                          'f',
                          TINY_ARGP_KEY_NO_ARGS,
                          TINY_ARGP_KEY_END,
                          TINY_ARGP_KEY_SUCCESS,
                          TINY_ARGP_KEY_FINI};
  assert_key_sequence_exact(expected, 7);
  TEST_ASSERT_EQUAL_STRING("one", parser_log[1].arg);
  TEST_ASSERT_EQUAL_STRING("two", parser_log[2].arg);
}

/* Long-option prefix abbreviation is NOT supported: `--fi` does not match
 * `--file`. GNU getopt supports unambiguous prefixes; this parser doesn't.
 * Rejected as an unknown option: INIT -> ERROR -> FINI, and stderr reports
 * "unrecognized option" quoting the exact `--fi` token. */
static void test_long_prefix_abbrev_not_supported(void) {
  char **argv = build_argv(2, "prog", "--fi");
  int r =
      tiny_argp_parse(&argp_takes_arg, 2, argv, TINY_ARGP_NO_EXIT, NULL, NULL);
  TEST_ASSERT_EQUAL(EINVAL, r);
  const int expected[] = {TINY_ARGP_KEY_INIT, TINY_ARGP_KEY_ERROR,
                          TINY_ARGP_KEY_FINI};
  assert_key_sequence_exact(expected, 3);
  TEST_ASSERT_EQUAL_STRING(
      "prog: unrecognized option '--fi'\r\n"
      "Try `prog --help' or `prog --usage' for more information.\r\n",
      mock_stderr);
}

/* `--foo=` (empty value) on a boolean option is rejected the same way as
 * `--foo=nope`. The `=` alone is enough to make it "an arg was supplied".
 * Key 'a' must not be delivered on rejection. */
static void test_long_bool_empty_equals_rejected(void) {
  char **argv = build_argv(2, "prog", "--alpha=");
  int r = tiny_argp_parse(&argp_bool, 2, argv, TINY_ARGP_NO_EXIT, NULL, NULL);
  TEST_ASSERT_EQUAL(EINVAL, r);
  const int expected[] = {TINY_ARGP_KEY_INIT, TINY_ARGP_KEY_ERROR,
                          TINY_ARGP_KEY_FINI};
  assert_key_sequence_exact(expected, 3);
  TEST_ASSERT_EQUAL_STRING(
      "prog: option '--alpha=' doesn't allow an argument\r\n"
      "Try `prog --help' or `prog --usage' for more information.\r\n",
      mock_stderr);
}

/* Multiple long options in one argv, delivered in order. */
static void test_long_multiple(void) {
  char **argv = build_argv(4, "prog", "--alpha", "--file", "p");
  int r = tiny_argp_parse(&argp_takes_arg, 4, argv, 0, NULL, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_SUCCESS, r);
  const int expected[] = {TINY_ARGP_KEY_INIT,
                          'a',
                          'f',
                          TINY_ARGP_KEY_NO_ARGS,
                          TINY_ARGP_KEY_END,
                          TINY_ARGP_KEY_SUCCESS,
                          TINY_ARGP_KEY_FINI};
  assert_key_sequence_exact(expected, 7);
  TEST_ASSERT_EQUAL_STRING("p", parser_log[2].arg);
}

void run_long_opts_tests(void) {
  RUN_TEST(test_long_flag);
  RUN_TEST(test_long_with_equals);
  RUN_TEST(test_long_with_separate_arg);
  RUN_TEST(test_long_missing_required_arg);
  RUN_TEST(test_long_empty_value);
  RUN_TEST(test_long_bool_gets_unwanted_arg);
  RUN_TEST(test_long_bool_empty_equals_rejected);
  RUN_TEST(test_long_multiple);
  RUN_TEST(test_long_unknown);
  RUN_TEST(test_long_repeated);
  RUN_TEST(test_long_prefix_abbrev_not_supported);
}
