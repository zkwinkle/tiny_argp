#include "test_utils.h"
#include "unity.h"

/* Parser that accepts all KEY_ARG. */
static int accept_all(int key, char *arg, struct tiny_argp_state *state) {
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

/* Parser that returns UNKNOWN on KEY_ARG to trigger KEY_ARGS. */
static int reject_arg(int key, char *arg, struct tiny_argp_state *state) {
  (void)state;
  log_parser_call(key, arg);
  switch (key) {
  case 'a':
  case 'b':
  case TINY_ARGP_KEY_ARGS:
  case TINY_ARGP_KEY_INIT:
  case TINY_ARGP_KEY_END:
  case TINY_ARGP_KEY_NO_ARGS:
  case TINY_ARGP_KEY_SUCCESS:
  case TINY_ARGP_KEY_ERROR:
  case TINY_ARGP_KEY_FINI:
    return TINY_ARGP_SUCCESS;
  case TINY_ARGP_KEY_ARG:
    return TINY_ARGP_ERR_UNKNOWN;
  default:
    return TINY_ARGP_ERR_UNKNOWN;
  }
}

static const struct tiny_argp_option opts[] = {
    {"alpha", 'a', 0, 0, "flag"}, {"beta", 'b', 0, 0, "flag"}, {0}};

static struct tiny_argp argp_accept = {opts, accept_all,   "",
                                       "",   mock_printer, mock_err_printer};
static struct tiny_argp argp_reject = {opts, reject_arg,   "",
                                       "",   mock_printer, mock_err_printer};

/* A lone positional is delivered as a single KEY_ARG carrying that arg. */
static void test_single_positional(void) {
  char **argv = build_argv(2, "prog", "hello");
  int r = tiny_argp_parse(&argp_accept, 2, argv, 0, NULL, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_SUCCESS, r);
  const int expected[] = {
      TINY_ARGP_KEY_INIT,    TINY_ARGP_KEY_ARG,  TINY_ARGP_KEY_END,
      TINY_ARGP_KEY_SUCCESS, TINY_ARGP_KEY_FINI,
  };
  assert_key_sequence_exact(expected, 5);
  TEST_ASSERT_TRUE(parser_log[1].has_arg);
  TEST_ASSERT_EQUAL_STRING("hello", parser_log[1].arg);
}

/* Each positional is delivered as its own KEY_ARG, in the given order. */
static void test_multiple_positionals(void) {
  char **argv = build_argv(4, "prog", "one", "two", "three");
  int r = tiny_argp_parse(&argp_accept, 4, argv, 0, NULL, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_SUCCESS, r);
  const int expected[] = {
      TINY_ARGP_KEY_INIT, TINY_ARGP_KEY_ARG, TINY_ARGP_KEY_ARG,
      TINY_ARGP_KEY_ARG,  TINY_ARGP_KEY_END, TINY_ARGP_KEY_SUCCESS,
      TINY_ARGP_KEY_FINI,
  };
  assert_key_sequence_exact(expected, 7);
  TEST_ASSERT_FALSE(parser_log[0].has_arg);
  TEST_ASSERT_TRUE(parser_log[1].has_arg);
  TEST_ASSERT_EQUAL_STRING("one", parser_log[1].arg);
  TEST_ASSERT_TRUE(parser_log[2].has_arg);
  TEST_ASSERT_EQUAL_STRING("two", parser_log[2].arg);
  TEST_ASSERT_TRUE(parser_log[3].has_arg);
  TEST_ASSERT_EQUAL_STRING("three", parser_log[3].arg);
  TEST_ASSERT_FALSE(parser_log[4].has_arg);
}

/* Default mode reorders options ahead of positionals: `-a` fires before the
 * KEY_ARG even though it followed the positional on the command line. */
static void test_default_reorder(void) {
  char **argv = build_argv(3, "prog", "file", "-a");
  int r = tiny_argp_parse(&argp_accept, 3, argv, 0, NULL, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_SUCCESS, r);
  const int expected[] = {
      TINY_ARGP_KEY_INIT,    'a',
      TINY_ARGP_KEY_ARG,     TINY_ARGP_KEY_END,
      TINY_ARGP_KEY_SUCCESS, TINY_ARGP_KEY_FINI,
  };
  assert_key_sequence_exact(expected, 6);
  TEST_ASSERT_EQUAL_STRING("file", parser_log[2].arg);
}

/* IN_ORDER suppresses reordering: keys arrive in command-line order, so the
 * positional `file` (as KEY_ARG) precedes `-a`. */
static void test_in_order(void) {
  char **argv = build_argv(3, "prog", "file", "-a");
  int r =
      tiny_argp_parse(&argp_accept, 3, argv, TINY_ARGP_IN_ORDER, NULL, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_SUCCESS, r);
  const int expected[] = {
      TINY_ARGP_KEY_INIT, TINY_ARGP_KEY_ARG,     'a',
      TINY_ARGP_KEY_END,  TINY_ARGP_KEY_SUCCESS, TINY_ARGP_KEY_FINI,
  };
  assert_key_sequence_exact(expected, 6);
  TEST_ASSERT_EQUAL_STRING("file", parser_log[1].arg);
}

/* `--` terminates option parsing: subsequent tokens are positionals verbatim,
 * even ones that lexically look like long options. */
static void test_double_dash_separator(void) {
  char **argv = build_argv(4, "prog", "-a", "--", "--not-opt");
  int r = tiny_argp_parse(&argp_accept, 4, argv, 0, NULL, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_SUCCESS, r);
  const int expected[] = {
      TINY_ARGP_KEY_INIT,    'a',
      TINY_ARGP_KEY_ARG,     TINY_ARGP_KEY_END,
      TINY_ARGP_KEY_SUCCESS, TINY_ARGP_KEY_FINI,
  };
  assert_key_sequence_exact(expected, 6);
  TEST_ASSERT_EQUAL_STRING("--not-opt", parser_log[2].arg);
}

/* No positional args at all: KEY_NO_ARGS is delivered exactly once, between
 * option processing and END, so the parser can distinguish "no positionals"
 * from "positionals all rejected". */
static void test_no_args_key_when_empty(void) {
  char **argv = build_argv(1, "prog");
  int r = tiny_argp_parse(&argp_accept, 1, argv, 0, NULL, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_SUCCESS, r);
  const int expected[] = {
      TINY_ARGP_KEY_INIT,    TINY_ARGP_KEY_NO_ARGS, TINY_ARGP_KEY_END,
      TINY_ARGP_KEY_SUCCESS, TINY_ARGP_KEY_FINI,
  };
  assert_key_sequence_exact(expected, 5);
}

/* Options-only argv still counts as "no positionals": KEY_NO_ARGS fires after
 * the option keys, since NO_ARGS tracks absence of positionals, not tokens. */
static void test_no_args_key_when_only_options(void) {
  char **argv = build_argv(3, "prog", "-a", "-b");
  int r = tiny_argp_parse(&argp_accept, 3, argv, 0, NULL, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_SUCCESS, r);
  const int expected[] = {
      TINY_ARGP_KEY_INIT,
      'a',
      'b',
      TINY_ARGP_KEY_NO_ARGS,
      TINY_ARGP_KEY_END,
      TINY_ARGP_KEY_SUCCESS,
      TINY_ARGP_KEY_FINI,
  };
  assert_key_sequence_exact(expected, 7);
}

/* KEY_ARG returning UNKNOWN falls back to a single KEY_ARGS bulk-delivery
 * call. Subsequent positionals are not re-offered as KEY_ARG. */
static void test_reject_arg_triggers_args_key(void) {
  char **argv = build_argv(3, "prog", "one", "two");
  int r = tiny_argp_parse(&argp_reject, 3, argv, 0, NULL, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_SUCCESS, r);
  const int expected[] = {
      TINY_ARGP_KEY_INIT, TINY_ARGP_KEY_ARG,     TINY_ARGP_KEY_ARGS,
      TINY_ARGP_KEY_END,  TINY_ARGP_KEY_SUCCESS, TINY_ARGP_KEY_FINI,
  };
  assert_key_sequence_exact(expected, 6);
}

/* Leading `--` disables all option parsing: the rest of argv is positional
 * even when tokens (like `-a`) match declared options. */
static void test_double_dash_at_start(void) {
  char **argv = build_argv(3, "prog", "--", "-a");
  int r = tiny_argp_parse(&argp_accept, 3, argv, 0, NULL, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_SUCCESS, r);
  const int expected[] = {
      TINY_ARGP_KEY_INIT,    TINY_ARGP_KEY_ARG,  TINY_ARGP_KEY_END,
      TINY_ARGP_KEY_SUCCESS, TINY_ARGP_KEY_FINI,
  };
  assert_key_sequence_exact(expected, 5);
  TEST_ASSERT_EQUAL_STRING("-a", parser_log[1].arg);
}

/* Trailing `--` with nothing after: no positional was actually supplied, so
 * KEY_NO_ARGS still fires and no KEY_ARG is delivered for the separator. */
static void test_double_dash_at_end(void) {
  char **argv = build_argv(3, "prog", "-a", "--");
  int r = tiny_argp_parse(&argp_accept, 3, argv, 0, NULL, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_SUCCESS, r);
  const int expected[] = {
      TINY_ARGP_KEY_INIT,    'a',
      TINY_ARGP_KEY_NO_ARGS, TINY_ARGP_KEY_END,
      TINY_ARGP_KEY_SUCCESS, TINY_ARGP_KEY_FINI,
  };
  assert_key_sequence_exact(expected, 6);
}

/* Bare `-` is a positional, following the Unix stdin/stdout convention
 * (`cat -`, `sort -`, etc.). Parser sees it as KEY_ARG with arg="-".
 * KEY_NO_ARGS must be absent: bare `-` counts as a positional argument. */
static void test_bare_dash_is_positional(void) {
  char **argv = build_argv(2, "prog", "-");
  int r = tiny_argp_parse(&argp_accept, 2, argv, 0, NULL, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_SUCCESS, r);
  /* Exact key sequence: INIT, KEY_ARG, END, SUCCESS, FINI (5 keys). */
  const int expected[] = {
      TINY_ARGP_KEY_INIT,    TINY_ARGP_KEY_ARG,  TINY_ARGP_KEY_END,
      TINY_ARGP_KEY_SUCCESS, TINY_ARGP_KEY_FINI,
  };
  assert_key_sequence_exact(expected, 5);
  /* parser_log[1] is the KEY_ARG call; confirm the arg is exactly "-". */
  TEST_ASSERT_EQUAL_STRING("-", parser_log[1].arg);
  TEST_ASSERT_TRUE(parser_log[1].has_arg);
}

/* With an empty options list (just the terminator), every non-option token is
 * still delivered as its own KEY_ARG; only the option-key channel is silent. */
static void test_empty_options_list(void) {
  static const struct tiny_argp_option empty_opts[] = {{0}};
  static struct tiny_argp empty_argp = {empty_opts,   accept_all,      "", "",
                                        mock_printer, mock_err_printer};
  char **argv = build_argv(3, "prog", "one", "two");
  int r = tiny_argp_parse(&empty_argp, 3, argv, 0, NULL, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_SUCCESS, r);
  const int expected[] = {
      TINY_ARGP_KEY_INIT, TINY_ARGP_KEY_ARG,     TINY_ARGP_KEY_ARG,
      TINY_ARGP_KEY_END,  TINY_ARGP_KEY_SUCCESS, TINY_ARGP_KEY_FINI,
  };
  assert_key_sequence_exact(expected, 6);
  TEST_ASSERT_EQUAL_STRING("one", parser_log[1].arg);
  TEST_ASSERT_EQUAL_STRING("two", parser_log[2].arg);
}

/* On success with everything consumed, *arg_index is set to state->next which
 * equals argc — the caller can slice argv[arg_index..] to get the tail. */
static void test_arg_index_reported(void) {
  char **argv = build_argv(3, "prog", "-a", "positional");
  size_t idx = 42;
  int r = tiny_argp_parse(&argp_accept, 3, argv, 0, &idx, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_SUCCESS, r);
  TEST_ASSERT_EQUAL(3, idx);
  const int expected[] = {
      TINY_ARGP_KEY_INIT,    'a',
      TINY_ARGP_KEY_ARG,     TINY_ARGP_KEY_END,
      TINY_ARGP_KEY_SUCCESS, TINY_ARGP_KEY_FINI,
  };
  assert_key_sequence_exact(expected, 6);
}

/* Shared state struct for observing arg_num via state->input. */
struct args_key_state {
  int arg_num_at_success;
};

/* Parser for Branch A: returns UNKNOWN on KEY_ARG (triggers KEY_ARGS),
 * then returns SUCCESS on KEY_ARGS without touching state->next.
 * Records state->arg_num at KEY_SUCCESS via state->input. */
static int reject_arg_observe(int key, char *arg,
                              struct tiny_argp_state *state) {
  log_parser_call(key, arg);
  struct args_key_state *s = (struct args_key_state *)state->input;
  switch (key) {
  case TINY_ARGP_KEY_ARG:
    return TINY_ARGP_ERR_UNKNOWN;
  case TINY_ARGP_KEY_SUCCESS:
    if (s)
      s->arg_num_at_success = (int)state->arg_num;
    return TINY_ARGP_SUCCESS;
  case TINY_ARGP_KEY_ARGS:
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

/* Parser for Branch B: returns UNKNOWN on KEY_ARG (triggers KEY_ARGS),
 * then on KEY_ARGS does state->next += 1 (claims one arg) before returning
 * SUCCESS. Records state->arg_num at KEY_SUCCESS via state->input. */
static int reject_arg_advance(int key, char *arg,
                              struct tiny_argp_state *state) {
  log_parser_call(key, arg);
  struct args_key_state *s = (struct args_key_state *)state->input;
  switch (key) {
  case TINY_ARGP_KEY_ARG:
    return TINY_ARGP_ERR_UNKNOWN;
  case TINY_ARGP_KEY_ARGS:
    state->next += 1;
    return TINY_ARGP_SUCCESS;
  case TINY_ARGP_KEY_SUCCESS:
    if (s)
      s->arg_num_at_success = (int)state->arg_num;
    return TINY_ARGP_SUCCESS;
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

static const struct tiny_argp_option opts_no_named[] = {{0}};

static struct tiny_argp argp_observe = {
    opts_no_named, reject_arg_observe, "", "", mock_printer, mock_err_printer};
static struct tiny_argp argp_advance = {
    opts_no_named, reject_arg_advance, "", "", mock_printer, mock_err_printer};

/* Branch A: when the KEY_ARGS handler leaves state->next untouched the library
 * treats the whole tail as consumed — arg_index jumps to argc and arg_num is
 * credited for every remaining positional. */
static void test_args_key_untouched_next_auto_advances(void) {
  struct args_key_state s = {0};
  char **argv = build_argv(3, "prog", "a", "b");
  size_t arg_index = 42;
  int r = tiny_argp_parse(&argp_observe, 3, argv, TINY_ARGP_NO_EXIT, &arg_index,
                          &s);
  TEST_ASSERT_EQUAL(TINY_ARGP_SUCCESS, r);
  TEST_ASSERT_EQUAL(3, (int)arg_index);
  TEST_ASSERT_EQUAL(2, s.arg_num_at_success);
  const int expected[] = {
      TINY_ARGP_KEY_INIT, TINY_ARGP_KEY_ARG,     TINY_ARGP_KEY_ARGS,
      TINY_ARGP_KEY_END,  TINY_ARGP_KEY_SUCCESS, TINY_ARGP_KEY_FINI,
  };
  assert_key_sequence_exact(expected, 6);
}

/* Branch B: when the KEY_ARGS handler advances state->next itself the library
 * respects that value — the parser's own claim of one arg is the only thing
 * credited, and arg_index stops there. */
static void test_args_key_parser_advances_next(void) {
  struct args_key_state s = {0};
  char **argv = build_argv(3, "prog", "a", "b");
  size_t arg_index = 42;
  int r = tiny_argp_parse(&argp_advance, 3, argv, TINY_ARGP_NO_EXIT, &arg_index,
                          &s);
  TEST_ASSERT_EQUAL(TINY_ARGP_SUCCESS, r);
  TEST_ASSERT_EQUAL(2, (int)arg_index);
  TEST_ASSERT_EQUAL(0, s.arg_num_at_success);
  /* No KEY_END here: once the KEY_ARGS handler advances state->next itself
   * the library skips END and goes straight to SUCCESS. */
  const int expected[] = {
      TINY_ARGP_KEY_INIT,    TINY_ARGP_KEY_ARG,  TINY_ARGP_KEY_ARGS,
      TINY_ARGP_KEY_SUCCESS, TINY_ARGP_KEY_FINI,
  };
  assert_key_sequence_exact(expected, 5);
}

/* `--` terminates options; bare `-` following it is a positional. */
static void test_bare_dash_after_double_dash(void) {
  char **argv = build_argv(3, "prog", "--", "-");
  int r = tiny_argp_parse(&argp_accept, 3, argv, 0, NULL, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_SUCCESS, r);
  const int expected[] = {
      TINY_ARGP_KEY_INIT,    TINY_ARGP_KEY_ARG,  TINY_ARGP_KEY_END,
      TINY_ARGP_KEY_SUCCESS, TINY_ARGP_KEY_FINI,
  };
  assert_key_sequence_exact(expected, 5);
  TEST_ASSERT_EQUAL_STRING("-", parser_log[1].arg);
}

/* Default reorder mode: options -a and -b are moved to the front, bare `-`
 * becomes a positional at the end. */
static void test_bare_dash_between_options(void) {
  char **argv = build_argv(4, "prog", "-a", "-", "-b");
  int r = tiny_argp_parse(&argp_accept, 4, argv, 0, NULL, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_SUCCESS, r);
  /* Reorder: 'a' fires, 'b' fires (moved forward), then `-` as positional. */
  const int expected[] = {
      TINY_ARGP_KEY_INIT,
      'a',
      'b',
      TINY_ARGP_KEY_ARG,
      TINY_ARGP_KEY_END,
      TINY_ARGP_KEY_SUCCESS,
      TINY_ARGP_KEY_FINI,
  };
  assert_key_sequence_exact(expected, 7);
  TEST_ASSERT_EQUAL_STRING("-", parser_log[3].arg);
}

/* IN_ORDER: order is preserved — `-` as positional before `-a` as option. */
static void test_bare_dash_in_order_mode(void) {
  char **argv = build_argv(3, "prog", "-", "-a");
  int r =
      tiny_argp_parse(&argp_accept, 3, argv, TINY_ARGP_IN_ORDER, NULL, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_SUCCESS, r);
  const int expected[] = {
      TINY_ARGP_KEY_INIT, TINY_ARGP_KEY_ARG,     'a',
      TINY_ARGP_KEY_END,  TINY_ARGP_KEY_SUCCESS, TINY_ARGP_KEY_FINI,
  };
  assert_key_sequence_exact(expected, 6);
  TEST_ASSERT_EQUAL_STRING("-", parser_log[1].arg);
}

/* Default reorder mode with `-` at start: option -a is moved to front,
 * bare `-` becomes a positional after it. */
static void test_bare_dash_at_start(void) {
  char **argv = build_argv(3, "prog", "-", "-a");
  int r = tiny_argp_parse(&argp_accept, 3, argv, 0, NULL, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_SUCCESS, r);
  /* Reorder shifts -a before `-`. */
  const int expected[] = {
      TINY_ARGP_KEY_INIT,    'a',
      TINY_ARGP_KEY_ARG,     TINY_ARGP_KEY_END,
      TINY_ARGP_KEY_SUCCESS, TINY_ARGP_KEY_FINI,
  };
  assert_key_sequence_exact(expected, 6);
  TEST_ASSERT_EQUAL_STRING("-", parser_log[2].arg);
}

/* Bundled short `-a-`: after valid 'a', the '-' char in the bundle is an
 * unknown option, which produces an error. Use NO_EXIT (not NO_ERRS) so
 * stderr is captured and we can assert the exact error message. */
static void test_bundled_short_with_trailing_dash(void) {
  char **argv = build_argv(2, "prog", "-a-");
  int r = tiny_argp_parse(&argp_accept, 2, argv, TINY_ARGP_NO_EXIT, NULL, NULL);
  TEST_ASSERT_EQUAL(EINVAL, r);
  /* 'a' is delivered before the '-' char triggers the error. */
  const int expected[] = {TINY_ARGP_KEY_INIT, 'a', TINY_ARGP_KEY_ERROR,
                          TINY_ARGP_KEY_FINI};
  assert_key_sequence_exact(expected, 4);
  TEST_ASSERT_EQUAL_STRING(
      "prog: invalid option '-'\r\n"
      "Try `prog --help' or `prog --usage' for more information.\r\n",
      mock_stderr);
}

void run_non_option_tests(void) {
  RUN_TEST(test_single_positional);
  RUN_TEST(test_multiple_positionals);
  RUN_TEST(test_default_reorder);
  RUN_TEST(test_in_order);
  RUN_TEST(test_double_dash_separator);
  RUN_TEST(test_double_dash_at_start);
  RUN_TEST(test_double_dash_at_end);
  RUN_TEST(test_no_args_key_when_empty);
  RUN_TEST(test_no_args_key_when_only_options);
  RUN_TEST(test_reject_arg_triggers_args_key);
  RUN_TEST(test_args_key_untouched_next_auto_advances);
  RUN_TEST(test_args_key_parser_advances_next);
  RUN_TEST(test_arg_index_reported);
  RUN_TEST(test_empty_options_list);
  RUN_TEST(test_bare_dash_is_positional);
  RUN_TEST(test_bare_dash_after_double_dash);
  RUN_TEST(test_bare_dash_between_options);
  RUN_TEST(test_bare_dash_in_order_mode);
  RUN_TEST(test_bare_dash_at_start);
  RUN_TEST(test_bundled_short_with_trailing_dash);
}
