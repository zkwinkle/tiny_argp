#include "test_utils.h"
#include "unity.h"

/* Standard recording parser. */
static int rec(int key, char *arg, struct tiny_argp_state *state) {
  (void)state;
  log_parser_call(key, arg);
  switch (key) {
    case 'a':
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

/* Fails on user key 'a' to trigger the error path. */
static int fail_on_a(int key, char *arg, struct tiny_argp_state *state) {
  (void)state;
  log_parser_call(key, arg);
  if (key == 'a') return EINVAL;
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

/* Fails on INIT. */
static int fail_on_init(int key, char *arg, struct tiny_argp_state *state) {
  (void)state;
  log_parser_call(key, arg);
  if (key == TINY_ARGP_KEY_INIT) return EINVAL;
  return TINY_ARGP_SUCCESS;
}

static const struct tiny_argp_option opts[] = {{"alpha", 'a', 0, 0, "flag"},
                                                {0}};

static struct tiny_argp argp = {
    opts, rec, "", "", mock_printer, mock_err_printer};
static struct tiny_argp argp_fail_a = {
    opts, fail_on_a, "", "", mock_printer, mock_err_printer};
static struct tiny_argp argp_fail_init = {
    opts, fail_on_init, "", "", mock_printer, mock_err_printer};

/* ----------------------------------------------------------------------- */

/* Empty argv still fires the full special-key lifecycle: INIT, NO_ARGS
 * (since no positional was consumed), END, SUCCESS, FINI. */
static void test_seq_empty(void) {
  char **argv = build_argv(1, "prog");
  int r = tiny_argp_parse(&argp, 1, argv, 0, NULL, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_SUCCESS, r);
  const int expected[] = {TINY_ARGP_KEY_INIT, TINY_ARGP_KEY_NO_ARGS,
                          TINY_ARGP_KEY_END, TINY_ARGP_KEY_SUCCESS,
                          TINY_ARGP_KEY_FINI};
  assert_key_sequence_exact(expected, 5);
}

/* Options-only argv: user key sits between INIT and NO_ARGS since no
 * positional arg was ever consumed. */
static void test_seq_options_only(void) {
  char **argv = build_argv(2, "prog", "-a");
  int r = tiny_argp_parse(&argp, 2, argv, 0, NULL, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_SUCCESS, r);
  const int expected[] = {TINY_ARGP_KEY_INIT,    'a',
                          TINY_ARGP_KEY_NO_ARGS, TINY_ARGP_KEY_END,
                          TINY_ARGP_KEY_SUCCESS, TINY_ARGP_KEY_FINI};
  assert_key_sequence_exact(expected, 6);
}

/* A consumed positional clears the no_args flag, so NO_ARGS must NOT
 * appear in the sequence. Exact sequence: INIT, ARG, END, SUCCESS, FINI. */
static void test_seq_positional_no_no_args(void) {
  char **argv = build_argv(2, "prog", "arg");
  int r = tiny_argp_parse(&argp, 2, argv, 0, NULL, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_SUCCESS, r);
  const int expected[] = {TINY_ARGP_KEY_INIT, TINY_ARGP_KEY_ARG,
                          TINY_ARGP_KEY_END, TINY_ARGP_KEY_SUCCESS,
                          TINY_ARGP_KEY_FINI};
  assert_key_sequence_exact(expected, 5);
}

/* NO_EXIT + user parser returning EINVAL on a known key: parser sees the
 * key, then ERROR + FINI are delivered (SUCCESS must not appear). The
 * library does NOT print anything for a user-returned error code, so
 * mock_stderr stays empty. */
static void test_seq_error_no_exit(void) {
  char **argv = build_argv(2, "prog", "-a");
  int r = tiny_argp_parse(&argp_fail_a, 2, argv, TINY_ARGP_NO_EXIT, NULL, NULL);
  TEST_ASSERT_EQUAL(EINVAL, r);
  const int expected[] = {TINY_ARGP_KEY_INIT, 'a', TINY_ARGP_KEY_ERROR,
                          TINY_ARGP_KEY_FINI};
  assert_key_sequence_exact(expected, 4);
  TEST_ASSERT_EQUAL_STRING("", mock_stderr);
}

/* INIT error is a hard stop even with NO_EXIT.
 * No ERROR/FINI is delivered. Only INIT is recorded. */
static void test_seq_init_error_stops(void) {
  char **argv = build_argv(2, "prog", "-a");
  int r =
      tiny_argp_parse(&argp_fail_init, 2, argv, TINY_ARGP_NO_EXIT, NULL, NULL);
  TEST_ASSERT_EQUAL(EINVAL, r);
  const int expected[] = {TINY_ARGP_KEY_INIT};
  assert_key_sequence_exact(expected, 1);
  TEST_ASSERT_EQUAL_STRING("", mock_stderr);
}

/* Default mode (no NO_EXIT) on user parser error: parse returns the code
 * but ERROR/FINI are NOT delivered (immediate exit path). Exact sequence
 * is just INIT then 'a'. stderr is empty because a user-returned error
 * doesn't trigger the library's opt_error printer. */
static void test_seq_error_immediate_exit(void) {
  char **argv = build_argv(2, "prog", "-a");
  int r = tiny_argp_parse(&argp_fail_a, 2, argv, 0, NULL, NULL);
  TEST_ASSERT_EQUAL(EINVAL, r);
  const int expected[] = {TINY_ARGP_KEY_INIT, 'a'};
  assert_key_sequence_exact(expected, 2);
  TEST_ASSERT_EQUAL_STRING("", mock_stderr);
}

/* Options mixed with positional under default reordering: options are
 * delivered first (`a`), then ARG for the positional, then the tail. */
static void test_seq_options_and_positional(void) {
  char **argv = build_argv(3, "prog", "arg", "-a");
  int r = tiny_argp_parse(&argp, 3, argv, 0, NULL, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_SUCCESS, r);
  const int expected[] = {TINY_ARGP_KEY_INIT,    'a',
                          TINY_ARGP_KEY_ARG,     TINY_ARGP_KEY_END,
                          TINY_ARGP_KEY_SUCCESS, TINY_ARGP_KEY_FINI};
  assert_key_sequence_exact(expected, 6);
}

/* Parser returns UNKNOWN on KEY_ARG (triggers KEY_ARGS), then UNKNOWN on
 * KEY_ARGS itself. */
static int rec_unknown_on_args(int key, char *arg,
                               struct tiny_argp_state *state) {
  (void)state;
  log_parser_call(key, arg);
  if (key == TINY_ARGP_KEY_ARG || key == TINY_ARGP_KEY_ARGS)
    return TINY_ARGP_ERR_UNKNOWN;
  return TINY_ARGP_SUCCESS;
}

/* When the parser rejects both KEY_ARG and KEY_ARGS by returning UNKNOWN,
 * the `no_args` flag is never cleared, so KEY_NO_ARGS fires per the
 * header contract. Full sequence: INIT, ARG, ARGS, NO_ARGS, SUCCESS, FINI
 * (no END because remaining args weren't considered consumed). */
static void test_seq_args_returns_unknown(void) {
  static struct tiny_argp argp_unknown_args = {
      opts, rec_unknown_on_args, "", "", mock_printer, mock_err_printer};
  char **argv = build_argv(2, "prog", "a");
  int r = tiny_argp_parse(&argp_unknown_args, 2, argv, 0, NULL, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_SUCCESS, r);
  const int expected[] = {TINY_ARGP_KEY_INIT,    TINY_ARGP_KEY_ARG,
                          TINY_ARGP_KEY_ARGS,    TINY_ARGP_KEY_NO_ARGS,
                          TINY_ARGP_KEY_SUCCESS, TINY_ARGP_KEY_FINI};
  assert_key_sequence_exact(expected, 6);
}

void run_key_sequence_tests(void) {
  RUN_TEST(test_seq_empty);
  RUN_TEST(test_seq_options_only);
  RUN_TEST(test_seq_positional_no_no_args);
  RUN_TEST(test_seq_options_and_positional);
  RUN_TEST(test_seq_error_no_exit);
  RUN_TEST(test_seq_error_immediate_exit);
  RUN_TEST(test_seq_init_error_stops);
  RUN_TEST(test_seq_args_returns_unknown);
}
