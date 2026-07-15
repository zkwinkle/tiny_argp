#include "test_utils.h"
#include "unity.h"

/* Recording parser: logs every call and returns SUCCESS for known user keys
 * and all special keys, TINY_ARGP_ERR_UNKNOWN otherwise. */
static int rec_parser(int key, char *arg, struct tiny_argp_state *state) {
  (void)state;
  log_parser_call(key, arg);
  switch (key) {
  case 'a':
  case 'b':
  case 'c':
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
    {"aaa", 'a', 0, 0, "flag a"},
    {"bbb", 'b', 0, 0, "flag b"},
    {"ccc", 'c', 0, 0, "flag c"},
    {0}};

static const struct tiny_argp_option opts_takes_arg[] = {
    {"aaa", 'a', 0, 0, "flag a"}, {"file", 'f', "PATH", 0, "file path"}, {0}};

static struct tiny_argp argp_bool = {opts_bool, rec_parser,   "",
                                     "",        mock_printer, mock_err_printer};

static struct tiny_argp argp_takes_arg = {
    opts_takes_arg, rec_parser, "", "", mock_printer, mock_err_printer};

/* --- single flag -------------------------------------------------------- */
static void test_short_single_flag(void) {
  char **argv = build_argv(2, "prog", "-a");
  int r = tiny_argp_parse(&argp_bool, 2, argv, 0, NULL, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_SUCCESS, r);
  const int expected[] = {TINY_ARGP_KEY_INIT,    'a',
                          TINY_ARGP_KEY_NO_ARGS, TINY_ARGP_KEY_END,
                          TINY_ARGP_KEY_SUCCESS, TINY_ARGP_KEY_FINI};
  assert_key_sequence_exact(expected, 6);
  TEST_ASSERT_FALSE(parser_log[1].has_arg);
}

/* --- bundled short flags ----------------------------------------------- */
static void test_short_bundled_flags(void) {
  char **argv = build_argv(2, "prog", "-abc");
  int r = tiny_argp_parse(&argp_bool, 2, argv, 0, NULL, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_SUCCESS, r);
  const int expected[] = {TINY_ARGP_KEY_INIT,
                          'a',
                          'b',
                          'c',
                          TINY_ARGP_KEY_NO_ARGS,
                          TINY_ARGP_KEY_END,
                          TINY_ARGP_KEY_SUCCESS,
                          TINY_ARGP_KEY_FINI};
  assert_key_sequence_exact(expected, 8);
}

/* --- short opt with attached arg --------------------------------------- */
static void test_short_attached_arg(void) {
  char **argv = build_argv(2, "prog", "-fpath/to/file");
  int r = tiny_argp_parse(&argp_takes_arg, 2, argv, 0, NULL, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_SUCCESS, r);
  const int expected[] = {TINY_ARGP_KEY_INIT,    'f',
                          TINY_ARGP_KEY_NO_ARGS, TINY_ARGP_KEY_END,
                          TINY_ARGP_KEY_SUCCESS, TINY_ARGP_KEY_FINI};
  assert_key_sequence_exact(expected, 6);
  TEST_ASSERT_TRUE(parser_log[1].has_arg);
  TEST_ASSERT_EQUAL_STRING("path/to/file", parser_log[1].arg);
}

/* --- short opt with separate arg --------------------------------------- */
static void test_short_separate_arg(void) {
  char **argv = build_argv(3, "prog", "-f", "some/path");
  int r = tiny_argp_parse(&argp_takes_arg, 3, argv, 0, NULL, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_SUCCESS, r);
  const int expected[] = {TINY_ARGP_KEY_INIT,    'f',
                          TINY_ARGP_KEY_NO_ARGS, TINY_ARGP_KEY_END,
                          TINY_ARGP_KEY_SUCCESS, TINY_ARGP_KEY_FINI};
  assert_key_sequence_exact(expected, 6);
  TEST_ASSERT_TRUE(parser_log[1].has_arg);
  TEST_ASSERT_EQUAL_STRING("some/path", parser_log[1].arg);
}

/* --- bundled where last takes arg -------------------------------------- */
static void test_short_bundled_last_takes_arg(void) {
  char **argv = build_argv(2, "prog", "-afPATH");
  int r = tiny_argp_parse(&argp_takes_arg, 2, argv, 0, NULL, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_SUCCESS, r);
  const int expected[] = {TINY_ARGP_KEY_INIT,
                          'a',
                          'f',
                          TINY_ARGP_KEY_NO_ARGS,
                          TINY_ARGP_KEY_END,
                          TINY_ARGP_KEY_SUCCESS,
                          TINY_ARGP_KEY_FINI};
  assert_key_sequence_exact(expected, 7);
  TEST_ASSERT_EQUAL_STRING("PATH", parser_log[2].arg);
}

/* --- repeated flag: parser called twice ------------------------------- */
static void test_short_repeated_flag(void) {
  char **argv = build_argv(3, "prog", "-a", "-a");
  int r = tiny_argp_parse(&argp_bool, 3, argv, 0, NULL, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_SUCCESS, r);
  const int expected[] = {TINY_ARGP_KEY_INIT,
                          'a',
                          'a',
                          TINY_ARGP_KEY_NO_ARGS,
                          TINY_ARGP_KEY_END,
                          TINY_ARGP_KEY_SUCCESS,
                          TINY_ARGP_KEY_FINI};
  assert_key_sequence_exact(expected, 7);
}

/* --- unknown short option: `-Z` ---------------------------------------- */
static void test_short_unknown(void) {
  char **argv = build_argv(2, "prog", "-Z");
  int r = tiny_argp_parse(&argp_bool, 2, argv, TINY_ARGP_NO_EXIT, NULL, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_ERR_PARSE, r);
  const int expected[] = {TINY_ARGP_KEY_INIT, TINY_ARGP_KEY_ERROR,
                          TINY_ARGP_KEY_FINI};
  assert_key_sequence_exact(expected, 3);
  TEST_ASSERT_EQUAL_STRING(
      "prog: invalid option 'Z'\r\n"
      "Try `prog --help' or `prog --usage' for more information.\r\n",
      mock_stderr);
}

/* --- short opt at end missing arg -------------------------------------- */
static void test_short_missing_required_arg(void) {
  char **argv = build_argv(2, "prog", "-f");
  int r =
      tiny_argp_parse(&argp_takes_arg, 2, argv, TINY_ARGP_NO_EXIT, NULL, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_ERR_PARSE, r);
  const int expected[] = {TINY_ARGP_KEY_INIT, TINY_ARGP_KEY_ERROR,
                          TINY_ARGP_KEY_FINI};
  assert_key_sequence_exact(expected, 3);
  TEST_ASSERT_EQUAL_STRING(
      "prog: option requires an argument -- 'f'\r\n"
      "Try `prog --help' or `prog --usage' for more information.\r\n",
      mock_stderr);
}

void run_short_opts_tests(void) {
  RUN_TEST(test_short_single_flag);
  RUN_TEST(test_short_bundled_flags);
  RUN_TEST(test_short_attached_arg);
  RUN_TEST(test_short_separate_arg);
  RUN_TEST(test_short_bundled_last_takes_arg);
  RUN_TEST(test_short_repeated_flag);
  RUN_TEST(test_short_unknown);
  RUN_TEST(test_short_missing_required_arg);
}
