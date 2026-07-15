#include "test_utils.h"
#include "unity.h"

static int rec(int key, char *arg, struct tiny_argp_state *state) {
  (void)state;
  log_parser_call(key, arg);
  switch (key) {
  case 'a':
  case 'f':
  case 'h':
  case 1: /* SOH — long-only option key */
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

static const struct tiny_argp_option opts_optional[] = {
    {"foo", 'f', "V", OPTION_ARG_OPTIONAL, "optional-arg opt"}, {0}};

static const struct tiny_argp_option opts_hidden[] = {
    {"visible", 'a', 0, 0, "shown flag"},
    {"secret", 'h', 0, OPTION_HIDDEN, "hidden flag"},
    {0}};

static const struct tiny_argp_option opts_longonly[] = {
    {"longonly", 1, 0, 0, "long-only doc"}, {0}};

static struct tiny_argp argp_optional = {
    opts_optional, rec, "", "", mock_printer, mock_err_printer};
static struct tiny_argp argp_hidden = {
    opts_hidden, rec, "", "", mock_printer, mock_err_printer};
static struct tiny_argp argp_longonly = {
    opts_longonly, rec, "", "", mock_printer, mock_err_printer};

/* OPTION_ARG_OPTIONAL: `--foo` alone delivers key 'f' with no arg. The
 * optional flag lets the option stand without a value. */
static void test_optional_arg_long_no_value(void) {
  char **argv = build_argv(2, "prog", "--foo");
  int r = tiny_argp_parse(&argp_optional, 2, argv, 0, NULL, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_SUCCESS, r);
  TEST_ASSERT_EQUAL('f', parser_log[1].key);
  TEST_ASSERT_FALSE(parser_log[1].has_arg);
}

/* OPTION_ARG_OPTIONAL: `--foo=X` attaches X as the option value. */
static void test_optional_arg_long_with_equals(void) {
  char **argv = build_argv(2, "prog", "--foo=hi");
  int r = tiny_argp_parse(&argp_optional, 2, argv, 0, NULL, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_SUCCESS, r);
  TEST_ASSERT_EQUAL('f', parser_log[1].key);
  TEST_ASSERT_TRUE(parser_log[1].has_arg);
  TEST_ASSERT_EQUAL_STRING("hi", parser_log[1].arg);
}

/* OPTION_ARG_OPTIONAL: `--foo hi` — a space-separated token is NOT taken as
 * the option value. `hi` becomes a positional instead. Contrasts with the
 * required-arg case where the next argv would be consumed. */
static void test_optional_arg_long_space_not_attached(void) {
  char **argv = build_argv(3, "prog", "--foo", "hi");
  int r = tiny_argp_parse(&argp_optional, 3, argv, 0, NULL, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_SUCCESS, r);
  TEST_ASSERT_EQUAL('f', parser_log[1].key);
  TEST_ASSERT_FALSE_MESSAGE(parser_log[1].has_arg,
                            "space-separated value should NOT attach to "
                            "optional-arg option");
  TEST_ASSERT_EQUAL(TINY_ARGP_KEY_ARG, parser_log[2].key);
  TEST_ASSERT_EQUAL_STRING("hi", parser_log[2].arg);
}

/* OPTION_ARG_OPTIONAL: `-f` alone delivers key 'f' with no arg. */
static void test_optional_arg_short_no_value(void) {
  char **argv = build_argv(2, "prog", "-f");
  int r = tiny_argp_parse(&argp_optional, 2, argv, 0, NULL, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_SUCCESS, r);
  TEST_ASSERT_EQUAL('f', parser_log[1].key);
  TEST_ASSERT_FALSE(parser_log[1].has_arg);
}

/* OPTION_ARG_OPTIONAL: `-fX` (attached, no space) attaches X. */
static void test_optional_arg_short_attached(void) {
  char **argv = build_argv(2, "prog", "-fhi");
  int r = tiny_argp_parse(&argp_optional, 2, argv, 0, NULL, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_SUCCESS, r);
  TEST_ASSERT_EQUAL('f', parser_log[1].key);
  TEST_ASSERT_TRUE(parser_log[1].has_arg);
  TEST_ASSERT_EQUAL_STRING("hi", parser_log[1].arg);
}

/* OPTION_ARG_OPTIONAL: `-f hi` — a space-separated token is NOT taken as the
 * option value. `hi` becomes a positional instead. Short-opt counterpart to
 * test_optional_arg_long_space_not_attached. */
static void test_optional_arg_short_space_not_attached(void) {
  char **argv = build_argv(3, "prog", "-f", "hi");
  int r = tiny_argp_parse(&argp_optional, 3, argv, 0, NULL, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_SUCCESS, r);
  TEST_ASSERT_EQUAL('f', parser_log[1].key);
  TEST_ASSERT_FALSE_MESSAGE(parser_log[1].has_arg,
                            "space-separated value should NOT attach to "
                            "optional-arg short option");
  TEST_ASSERT_EQUAL(TINY_ARGP_KEY_ARG, parser_log[2].key);
  TEST_ASSERT_EQUAL_STRING("hi", parser_log[2].arg);
}

/* OPTION_HIDDEN: option is suppressed from long help output while its
 * sibling with the default flags renders normally. */
static void test_hidden_option_absent_from_help(void) {
  tiny_argp_help(&argp_hidden, mock_printer, TINY_ARGP_HELP_LONG, "prog");
  TEST_ASSERT_EQUAL_STRING("  -a, --visible              shown flag\r\n",
                           mock_stdout);
}

/* OPTION_HIDDEN: hidden only affects help rendering — the option is still
 * accepted on the command line and delivered to the parser. */
static void test_hidden_option_still_callable(void) {
  char **argv = build_argv(2, "prog", "--secret");
  int r = tiny_argp_parse(&argp_hidden, 2, argv, 0, NULL, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_SUCCESS, r);
  TEST_ASSERT_EQUAL('h', parser_log[1].key);
}

/* Long-only option (non-printable key byte): accepted as `--longonly` on the
 * CLI and delivered under its user-chosen key. */
static void test_long_only_option_callable_from_cli(void) {
  char **argv = build_argv(2, "prog", "--longonly");
  int r = tiny_argp_parse(&argp_longonly, 2, argv, 0, NULL, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_SUCCESS, r);
  TEST_ASSERT_EQUAL(1, parser_log[1].key);
}

/* Long-only option: help output uses the 6-space (no `-x,`) indent since
 * there's no printable short form to display. */
static void test_long_only_option_in_help_output(void) {
  tiny_argp_help(&argp_longonly, mock_printer, TINY_ARGP_HELP_LONG, "prog");
  TEST_ASSERT_EQUAL_STRING("      --longonly             long-only doc\r\n",
                           mock_stdout);
}

/* Long-only option: passing the raw SOH byte as `-<SOH>` is NOT a way to
 * invoke it. check_option's SHORT path guards with isprint(option.key) and
 * breaks early for non-printable keys, so the byte is treated as an unknown
 * short option and the parser sees INIT -> ERROR -> FINI. */
static void test_long_only_option_not_short_settable(void) {
  /* Build argv with a literal SOH byte as the short-option character. */
  char soh_arg[3] = {'-', '\x01', '\0'};
  char **argv = build_argv(2, "prog", soh_arg);
  int r =
      tiny_argp_parse(&argp_longonly, 2, argv, TINY_ARGP_NO_EXIT, NULL, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_ERR_PARSE, r);
  const int expected[] = {TINY_ARGP_KEY_INIT, TINY_ARGP_KEY_ERROR,
                          TINY_ARGP_KEY_FINI};
  assert_key_sequence_exact(expected, 3);
  TEST_ASSERT_EQUAL_STRING(
      "prog: invalid option '\x01'\r\n"
      "Try `prog --help' or `prog --usage' for more information.\r\n",
      mock_stderr);
}

void run_option_flags_tests(void) {
  RUN_TEST(test_optional_arg_long_no_value);
  RUN_TEST(test_optional_arg_long_with_equals);
  RUN_TEST(test_optional_arg_long_space_not_attached);
  RUN_TEST(test_optional_arg_short_no_value);
  RUN_TEST(test_optional_arg_short_attached);
  RUN_TEST(test_optional_arg_short_space_not_attached);
  RUN_TEST(test_hidden_option_absent_from_help);
  RUN_TEST(test_hidden_option_still_callable);
  RUN_TEST(test_long_only_option_callable_from_cli);
  RUN_TEST(test_long_only_option_in_help_output);
  RUN_TEST(test_long_only_option_not_short_settable);
}
