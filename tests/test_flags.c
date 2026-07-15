#include "test_utils.h"
#include "unity.h"

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

/* Parser used with PARSE_ARGV0: sets state->name from INIT and treats
 * argv[0] as a normal positional. */
static int rec_and_name(int key, char *arg, struct tiny_argp_state *state) {
  log_parser_call(key, arg);
  if (key == TINY_ARGP_KEY_INIT) {
    state->name = (char *)"prog-set-in-init";
  }
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

static const struct tiny_argp_option opts[] = {{"alpha", 'a', 0, 0, "flag"},
                                               {0}};

static const struct tiny_argp_option opts_ab[] = {
    {"alpha", 'a', 0, 0, "flag a"}, {"beta", 'b', 0, 0, "flag b"}, {0}};

static int rec_ab(int key, char *arg, struct tiny_argp_state *state) {
  (void)state;
  log_parser_call(key, arg);
  switch (key) {
  case 'a':
  case 'b':
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

static struct tiny_argp argp = {opts, rec,          "",
                                "",   mock_printer, mock_err_printer};
static struct tiny_argp argp_named = {opts, rec_and_name, "",
                                      "",   mock_printer, mock_err_printer};
static struct tiny_argp argp_ab = {opts_ab, rec_ab,       "",
                                   "",      mock_printer, mock_err_printer};

/* PARSE_ARGV0 makes argv[0] a positional instead of the program name, so the
 * parser sees KEY_ARG with that string in the log. Without IN_ORDER, argv is
 * permuted so option 'a' is delivered before the positional. */
static void test_parse_argv0_treats_argv0_as_arg(void) {
  char **argv = build_argv(2, "actually_an_arg", "-a");
  int r =
      tiny_argp_parse(&argp_named, 2, argv, TINY_ARGP_PARSE_ARGV0, NULL, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_SUCCESS, r);
  const int expected[] = {TINY_ARGP_KEY_INIT,    'a',
                          TINY_ARGP_KEY_ARG,     TINY_ARGP_KEY_END,
                          TINY_ARGP_KEY_SUCCESS, TINY_ARGP_KEY_FINI};
  assert_key_sequence_exact(expected, 6);
  TEST_ASSERT_EQUAL_STRING("actually_an_arg", parser_log[2].arg);
}

/* When NO_ERRS is present, an unknown option must still return
 * TINY_ARGP_ERR_PARSE but the error printer must stay silent. */
static void test_no_errs_suppresses_err_printer(void) {
  char **argv = build_argv(2, "prog", "--nope");
  int r = tiny_argp_parse(&argp, 2, argv, TINY_ARGP_NO_EXIT | TINY_ARGP_NO_ERRS,
                          NULL, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_ERR_PARSE, r);
  const int expected[] = {TINY_ARGP_KEY_INIT, TINY_ARGP_KEY_ERROR,
                          TINY_ARGP_KEY_FINI};
  assert_key_sequence_exact(expected, 3);
  TEST_ASSERT_EQUAL_STRING("", mock_stderr);
}

/* Without NO_ERRS the standard "unrecognized option" message plus the
 * "Try ... --help" hint reaches the error printer verbatim.
 * We test:
 *
 * - with NO_EXIT flag: Error is printed and ERROR + FINI keys are parsed.
 * - without NO_EXIT flag: Error is printed but parser aborts immediately.
 * */
static void test_prints_err_on_unknown(void) {
  bool with_no_exit = true;

  while (true) {
    test_utils_reset();
    char **argv = build_argv(2, "prog", "--nope");

    unsigned flags;
    if (with_no_exit) {
      flags = TINY_ARGP_NO_EXIT;
    } else {
      flags = 0;
    }

    int r = tiny_argp_parse(&argp, 2, argv, flags, NULL, NULL);
    TEST_ASSERT_EQUAL(TINY_ARGP_ERR_PARSE, r);
    if (with_no_exit) {
      const int expected[] = {TINY_ARGP_KEY_INIT, TINY_ARGP_KEY_ERROR,
                              TINY_ARGP_KEY_FINI};
      assert_key_sequence_exact(expected, 3);
    } else {
      const int expected[] = {TINY_ARGP_KEY_INIT};
      assert_key_sequence_exact(expected, 1);
    }
    TEST_ASSERT_EQUAL_STRING(
        "prog: unrecognized option '--nope'\r\n"
        "Try `prog --help' or `prog --usage' for more information.\r\n",
        mock_stderr);

    if (!with_no_exit)
      break;
    with_no_exit = false;
  }
}

/* NO_ARGS: positional argv entries are silently swallowed by the library, so
 * the parser never sees KEY_ARG and the parse still succeeds. */
static void test_no_args_flag_skips_positional(void) {
  char **argv = build_argv(2, "prog", "positional");
  int r = tiny_argp_parse(&argp, 2, argv, TINY_ARGP_NO_ARGS, NULL, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_SUCCESS, r);

  const int expected[] = {TINY_ARGP_KEY_INIT, TINY_ARGP_KEY_NO_ARGS,
                          TINY_ARGP_KEY_END, TINY_ARGP_KEY_SUCCESS,
                          TINY_ARGP_KEY_FINI};
  assert_key_sequence_exact(expected, 5);
}

/* When NO_HELP is present, --help must be treated as unrecognized instead of
 * triggering the built-in help path. We test:
 *
 * - with NO_ERRS: the error printer stays silent.
 * - without NO_ERRS: the standard "unrecognized option" message plus the
 *   "Try ... --help" hint reaches the error printer verbatim.
 * */
static void test_no_help_disables_help_option(void) {
  bool with_no_errs = true;

  while (true) {
    test_utils_reset();
    char **argv = build_argv(2, "prog", "--help");

    unsigned flags = TINY_ARGP_NO_HELP;
    if (with_no_errs) {
      flags |= TINY_ARGP_NO_ERRS;
    }

    int r = tiny_argp_parse(&argp, 2, argv, flags, NULL, NULL);
    TEST_ASSERT_EQUAL(TINY_ARGP_ERR_PARSE, r);
    const int expected[] = {TINY_ARGP_KEY_INIT};
    assert_key_sequence_exact(expected, 1);
    if (with_no_errs) {
      TEST_ASSERT_EQUAL_STRING("", mock_stderr);
    } else {
      // TODO: Consider changing error message since --help and --usage are
      // disabled.
      TEST_ASSERT_EQUAL_STRING(
          "prog: unrecognized option '--help'\r\n"
          "Try `prog --help' or `prog --usage' for more information.\r\n",
          mock_stderr);
    }

    if (!with_no_errs)
      break;
    with_no_errs = false;
  }
}

/* When SILENT is present (the union of NO_EXIT | NO_ERRS | NO_HELP) it must
 * return the error code with both stdout and stderr untouched and finish
 * executing with KEY_ERROR and KEY_FINI. */
static void test_silent_combo(void) {
  char **argv = build_argv(2, "prog", "--nope");
  int r = tiny_argp_parse(&argp, 2, argv, TINY_ARGP_SILENT, NULL, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_ERR_PARSE, r);
  const int expected[] = {TINY_ARGP_KEY_INIT, TINY_ARGP_KEY_ERROR,
                          TINY_ARGP_KEY_FINI};
  assert_key_sequence_exact(expected, 3);
  TEST_ASSERT_EQUAL_STRING("", mock_stdout);
  TEST_ASSERT_EQUAL_STRING("", mock_stderr);
}

/* IN_ORDER | NO_ARGS: options are delivered in argv order and the interleaved
 * positional is dropped by NO_ARGS, so KEY_ARG never appears. */
static void test_in_order_no_args_with_options(void) {
  char **argv = build_argv(4, "prog", "-a", "positional", "-b");
  int r = tiny_argp_parse(&argp_ab, 4, argv,
                          TINY_ARGP_IN_ORDER | TINY_ARGP_NO_ARGS, NULL, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_SUCCESS, r);
  const int expected[] = {TINY_ARGP_KEY_INIT, 'a', 'b',
                          /* KEY_ARG doesn't appear. */
                          TINY_ARGP_KEY_NO_ARGS, TINY_ARGP_KEY_END,
                          TINY_ARGP_KEY_SUCCESS, TINY_ARGP_KEY_FINI};
  assert_key_sequence_exact(expected, 7);
}

/* IN_ORDER | PARSE_ARGV0: argv[0] becomes the first positional and the
 * option/positional interleaving is preserved as-is. */
static void test_in_order_parse_argv0_combo(void) {
  char **argv = build_argv(3, "real-argv0", "-a", "positional");
  int r =
      tiny_argp_parse(&argp_named, 3, argv,
                      TINY_ARGP_IN_ORDER | TINY_ARGP_PARSE_ARGV0, NULL, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_SUCCESS, r);
  const int expected[] = {
      TINY_ARGP_KEY_INIT, TINY_ARGP_KEY_ARG, 'a',
      TINY_ARGP_KEY_ARG,  TINY_ARGP_KEY_END, TINY_ARGP_KEY_SUCCESS,
      TINY_ARGP_KEY_FINI};
  assert_key_sequence_exact(expected, 7);
  TEST_ASSERT_EQUAL_STRING("real-argv0", parser_log[1].arg);
  TEST_ASSERT_EQUAL_STRING("positional", parser_log[3].arg);
}

/* PARSE_ARGV0 | NO_ARGS: NO_ARGS wins — argv[0] does not become a positional,
 * so KEY_NO_ARGS fires because nothing else was delivered. */
static void test_parse_argv0_no_args_combo(void) {
  char **argv = build_argv(1, "argv0-arg positional");
  int r = tiny_argp_parse(&argp, 1, argv,
                          TINY_ARGP_PARSE_ARGV0 | TINY_ARGP_NO_ARGS |
                              TINY_ARGP_NO_EXIT,
                          NULL, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_SUCCESS, r);
  const int expected[] = {TINY_ARGP_KEY_INIT, TINY_ARGP_KEY_NO_ARGS,
                          TINY_ARGP_KEY_END, TINY_ARGP_KEY_SUCCESS,
                          TINY_ARGP_KEY_FINI};
  assert_key_sequence_exact(expected, 5);
}

/* IN_ORDER with "--": the sentinel forces the trailing "-a" to be delivered
 * as a literal positional, never as option 'a'. */
static void test_in_order_double_dash_combo(void) {
  char **argv = build_argv(7, "prog", "arg1", "-a", "--", "-a", "-b", "other");
  int r = tiny_argp_parse(&argp_ab, 7, argv, TINY_ARGP_IN_ORDER, NULL, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_SUCCESS, r);
  const int expected[] = {
      TINY_ARGP_KEY_INIT, TINY_ARGP_KEY_ARG,     'a',
      TINY_ARGP_KEY_ARG,  TINY_ARGP_KEY_ARG,     TINY_ARGP_KEY_ARG,
      TINY_ARGP_KEY_END,  TINY_ARGP_KEY_SUCCESS, TINY_ARGP_KEY_FINI};
  assert_key_sequence_exact(expected, 9);
  TEST_ASSERT_EQUAL_STRING("arg1", parser_log[1].arg);
  TEST_ASSERT_FALSE(parser_log[2].has_arg);
  TEST_ASSERT_EQUAL_STRING("-a", parser_log[3].arg);
  TEST_ASSERT_EQUAL_STRING("-b", parser_log[4].arg);
  TEST_ASSERT_EQUAL_STRING("other", parser_log[5].arg);
}

void run_flags_tests(void) {
  RUN_TEST(test_parse_argv0_treats_argv0_as_arg);
  RUN_TEST(test_no_errs_suppresses_err_printer);
  RUN_TEST(test_prints_err_on_unknown);
  RUN_TEST(test_no_args_flag_skips_positional);
  RUN_TEST(test_no_help_disables_help_option);
  RUN_TEST(test_silent_combo);
  RUN_TEST(test_in_order_no_args_with_options);
  RUN_TEST(test_in_order_parse_argv0_combo);
  RUN_TEST(test_parse_argv0_no_args_combo);
  RUN_TEST(test_in_order_double_dash_combo);
}
