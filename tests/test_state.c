#include <string.h>

#include "test_utils.h"
#include "unity.h"

/* Captured by inspect_parser at KEY_INIT to prove state->input reached the
 * callback unchanged. */
static void *captured_input_ptr;

/* Passed as `input` to tiny_argp_parse; the parser reads state fields
 * during KEY_END and copies them here so tests can inspect after parse
 * returns. */
struct my_state {
  /* Test-side sentinel used to verify state->input reached the parser
   * unmodified. */
  int magic;
  /* state->arg_num as observed at KEY_END (count of processed positionals). */
  int arg_num_at_end;
  /* state->quoted as observed at KEY_END (index of first arg after `--`,
   * or 0 if `--` never appeared). */
  size_t quoted_at_end;
  /* state->argc as observed at KEY_END. */
  size_t argc_seen;
  /* state->flags as observed at KEY_END. */
  unsigned flags_seen;
  /* state->name as observed at KEY_END. */
  char *name_at_end;
};

static int inspect_parser(int key, char *arg, struct tiny_argp_state *state) {
  struct my_state *ms = (struct my_state *)state->input;
  log_parser_call(key, arg);
  switch (key) {
  case TINY_ARGP_KEY_INIT:
    captured_input_ptr = state->input;
    break;
  case TINY_ARGP_KEY_END:
    if (ms) {
      ms->arg_num_at_end = (int)state->arg_num;
      ms->quoted_at_end = state->quoted;
      ms->argc_seen = state->argc;
      ms->flags_seen = state->flags;
      ms->name_at_end = state->name;
    }
    break;
  case 'a':
  case TINY_ARGP_KEY_ARG:
  case TINY_ARGP_KEY_ARGS:
  case TINY_ARGP_KEY_NO_ARGS:
  case TINY_ARGP_KEY_SUCCESS:
  case TINY_ARGP_KEY_ERROR:
  case TINY_ARGP_KEY_FINI:
    break;
  default:
    return TINY_ARGP_ERR_UNKNOWN;
  }
  return TINY_ARGP_SUCCESS;
}

/* Parser that sets state->name in INIT — needed for PARSE_ARGV0. */
static int name_setting_parser(int key, char *arg,
                               struct tiny_argp_state *state) {
  log_parser_call(key, arg);
  if (key == TINY_ARGP_KEY_INIT) {
    state->name = (char *)"user-set-name";
  }
  return TINY_ARGP_SUCCESS;
}

static const struct tiny_argp_option opts[] = {{"alpha", 'a', 0, 0, "flag"},
                                               {0}};

static struct tiny_argp argp = {opts, inspect_parser, "",
                                "",   mock_printer,   mock_err_printer};

static struct tiny_argp argp_naming = {
    opts, name_setting_parser, "", "", mock_printer, mock_err_printer};

/* --- state->input is passed through unchanged. */
static void test_input_passthrough(void) {
  struct my_state ms = {.magic = 0xABCD};
  captured_input_ptr = NULL;
  char **argv = build_argv(1, "prog");
  int r = tiny_argp_parse(&argp, 1, argv, 0, NULL, &ms);
  TEST_ASSERT_EQUAL(TINY_ARGP_SUCCESS, r);
  TEST_ASSERT_EQUAL_HEX(0xABCD, ms.magic);
  TEST_ASSERT_EQUAL_PTR(&ms, captured_input_ptr);
}

/* --- state->arg_num tracks accepted KEY_ARG calls. */
static void test_arg_num_tracks(void) {
  struct my_state ms = {0};
  char **argv = build_argv(4, "prog", "one", "two", "three");
  int r = tiny_argp_parse(&argp, 4, argv, 0, NULL, &ms);
  TEST_ASSERT_EQUAL(TINY_ARGP_SUCCESS, r);
  TEST_ASSERT_EQUAL(3, ms.arg_num_at_end);
}

/* --- state->name defaults to argv[0] by the time END is reached. */
static void test_name_defaults_to_argv0(void) {
  struct my_state ms = {0};
  char **argv = build_argv(1, "the-prog-name");
  int r = tiny_argp_parse(&argp, 1, argv, 0, NULL, &ms);
  TEST_ASSERT_EQUAL(TINY_ARGP_SUCCESS, r);
  TEST_ASSERT_NOT_NULL(ms.name_at_end);
  TEST_ASSERT_EQUAL_STRING("the-prog-name", ms.name_at_end);
}

/* Capture parser used by test_name_is_null_at_init. Records
 * state->name into (char **)state->input at KEY_INIT time. */
static int capture_init_name(int key, char *arg,
                             struct tiny_argp_state *state) {
  (void)arg;
  if (key == TINY_ARGP_KEY_INIT) {
    *(char **)state->input = state->name;
  }
  return TINY_ARGP_SUCCESS;
}

/* --- state->name is NULL when the parser observes it during INIT (default
 * mode assigns it *after* INIT). */
static void test_name_is_null_at_init(void) {
  char *name_at_init =
      (char *)"not-null"; /* non-NULL so a missed write is obvious */
  struct tiny_argp capture_argp = {opts, capture_init_name, "",
                                   "",   mock_printer,      mock_err_printer};
  char **argv = build_argv(1, "prog");
  int r = tiny_argp_parse(&capture_argp, 1, argv, 0, NULL, &name_at_init);
  TEST_ASSERT_EQUAL(TINY_ARGP_SUCCESS, r);
  TEST_ASSERT_NULL(name_at_init);
}

/* --- state->quoted stays 0 when `--` never appears. */
static void test_quoted_zero_without_dashes(void) {
  struct my_state ms = {0};
  ms.quoted_at_end = SIZE_MAX;
  char **argv = build_argv(3, "prog", "-a", "arg");
  int r = tiny_argp_parse(&argp, 3, argv, 0, NULL, &ms);
  TEST_ASSERT_EQUAL(TINY_ARGP_SUCCESS, r);
  TEST_ASSERT_EQUAL(0, ms.quoted_at_end);
}

/* --- state->quoted set to index of first arg after `--`. */
static void test_quoted_set_after_dashes(void) {
  struct my_state ms = {0};
  char **argv = build_argv(5, "prog", "-a", "--", "one", "two");
  int r = tiny_argp_parse(&argp, 5, argv, 0, NULL, &ms);
  TEST_ASSERT_EQUAL(TINY_ARGP_SUCCESS, r);
  TEST_ASSERT_EQUAL(3, ms.quoted_at_end);
}

/* --- state->argc matches the argc passed in. */
static void test_argc_matches(void) {
  struct my_state ms = {0};
  char **argv = build_argv(3, "prog", "-a", "arg");
  int r = tiny_argp_parse(&argp, 3, argv, 0, NULL, &ms);
  TEST_ASSERT_EQUAL(TINY_ARGP_SUCCESS, r);
  TEST_ASSERT_EQUAL(3, ms.argc_seen);
}

/* --- state->flags matches the flags passed in. */
static void test_flags_match(void) {
  struct my_state ms = {0};
  char **argv = build_argv(1, "prog");
  int r = tiny_argp_parse(&argp, 1, argv, TINY_ARGP_NO_EXIT, NULL, &ms);
  TEST_ASSERT_EQUAL(TINY_ARGP_SUCCESS, r);
  TEST_ASSERT_EQUAL(TINY_ARGP_NO_EXIT, ms.flags_seen);
}

/* --- With PARSE_ARGV0 the parser can set state->name in INIT and it is
 * used in error output. Verifies state->name (set at INIT) drives the
 * program-name prefix in the diagnostic instead of argv[0]. */
static void test_parse_argv0_user_name(void) {
  char **argv = build_argv(2, "argv0-is-arg", "--bogus");
  int r =
      tiny_argp_parse(&argp_naming, 2, argv,
                      TINY_ARGP_PARSE_ARGV0 | TINY_ARGP_NO_EXIT, NULL, NULL);
  TEST_ASSERT_EQUAL(EINVAL, r);
  const int expected[] = {TINY_ARGP_KEY_INIT, TINY_ARGP_KEY_ERROR,
                          TINY_ARGP_KEY_FINI};
  assert_key_sequence_exact(expected, 3);
  TEST_ASSERT_EQUAL_STRING("user-set-name: unrecognized option '--bogus'\r\n"
                           "Try `user-set-name --help' or `user-set-name "
                           "--usage' for more information.\r\n",
                           mock_stderr);
}

/* --- arg_index NULL: no crash. */
static void test_arg_index_null_ok(void) {
  char **argv = build_argv(2, "prog", "-a");
  int r = tiny_argp_parse(&argp, 2, argv, 0, NULL, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_SUCCESS, r);
}

/* --- argc == 0 with TINY_ARGP_PARSE_ARGV0 should not crash. */
static void test_argc_zero_with_parse_argv0(void) {
  /* One-element array of NULL. */

  /* TODO: We use argv = {NULL}, a 1-element array containing a
   * NULL. A truly zero-length argv (a pointer to nothing) would UB in
   * default mode via the `state->name = *argv` deref at tiny_argp.c:979.
   * Add a test that passes such an argv and confirms safe handling — or
   * add a guard to the impl. */
  char *safe_argv[] = {NULL};
  int r =
      tiny_argp_parse(&argp, 0, safe_argv, TINY_ARGP_PARSE_ARGV0, NULL, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_SUCCESS, r);
  const int expected[] = {TINY_ARGP_KEY_INIT, TINY_ARGP_KEY_NO_ARGS,
                          TINY_ARGP_KEY_END, TINY_ARGP_KEY_SUCCESS,
                          TINY_ARGP_KEY_FINI};
  assert_key_sequence_exact(expected, 5);
}

/* --- argc == 0 with default flags (no PARSE_ARGV0). */
static void test_argc_zero_default_flags(void) {
  char *safe_argv[] = {NULL};
  int r = tiny_argp_parse(&argp, 0, safe_argv, 0, NULL, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_SUCCESS, r);
  const int expected[] = {TINY_ARGP_KEY_INIT, TINY_ARGP_KEY_NO_ARGS,
                          TINY_ARGP_KEY_END, TINY_ARGP_KEY_SUCCESS,
                          TINY_ARGP_KEY_FINI};
  assert_key_sequence_exact(expected, 5);
}

/* --- state->arg_num has correct incremental value inside each KEY_ARG.
 *
 * call_parser syncs state->arg_num (public) from PSTATE->arg_num at the END
 * of each call, AFTER the callback returns.  So during the N-th KEY_ARG call
 * state->arg_num still holds the value set after the (N-1)-th call:
 *   1st KEY_ARG → state->arg_num == 0
 *   2nd KEY_ARG → state->arg_num == 1
 *   3rd KEY_ARG → state->arg_num == 2
 */
#define ARG_NUM_LOG_MAX 8
static int arg_num_log[ARG_NUM_LOG_MAX];
static size_t arg_num_log_count;

static int incremental_arg_num_parser(int key, char *arg,
                                      struct tiny_argp_state *state) {
  log_parser_call(key, arg);
  if (key == TINY_ARGP_KEY_ARG) {
    if (arg_num_log_count < ARG_NUM_LOG_MAX) {
      arg_num_log[arg_num_log_count++] = (int)state->arg_num;
    }
  }
  switch (key) {
  case TINY_ARGP_KEY_INIT:
  case TINY_ARGP_KEY_ARG:
  case TINY_ARGP_KEY_NO_ARGS:
  case TINY_ARGP_KEY_END:
  case TINY_ARGP_KEY_SUCCESS:
  case TINY_ARGP_KEY_ERROR:
  case TINY_ARGP_KEY_FINI:
    return TINY_ARGP_SUCCESS;
  default:
    return TINY_ARGP_ERR_UNKNOWN;
  }
}

/* --- state->arg_num observed during KEY_ARG reflects the count of prior
 * KEY_ARG calls (0, 1, 2, ... for successive positionals). */
static void test_arg_num_incremental(void) {
  arg_num_log_count = 0;
  static const struct tiny_argp_option no_opts[] = {{0}};
  static struct tiny_argp inc_argp = {no_opts,      incremental_arg_num_parser,
                                      "",           "",
                                      mock_printer, mock_err_printer};
  char **argv = build_argv(4, "prog", "a", "b", "c");
  int r = tiny_argp_parse(&inc_argp, 4, argv, 0, NULL, NULL);
  TEST_ASSERT_EQUAL(TINY_ARGP_SUCCESS, r);
  TEST_ASSERT_EQUAL(3, (int)arg_num_log_count);
  TEST_ASSERT_EQUAL(0, arg_num_log[0]);
  TEST_ASSERT_EQUAL(1, arg_num_log[1]);
  TEST_ASSERT_EQUAL(2, arg_num_log[2]);
}

/* --- state->quoted exact value under TINY_ARGP_IN_ORDER.
 *
 * argv = ["prog", "-a", "--", "one", "two"], argc = 5.
 * Under IN_ORDER, args iterate directly:
 *   next=1: "-a" → option consumed.
 *   next=2: "--" → quote_index=2, quoted=3, next=3.
 *   next=3,4: positional args.
 * So state->quoted == 3 at KEY_END — identical to the default (reorder)
 * mode tested by test_quoted_set_after_dashes. */
static void test_quoted_exact_after_dashes_in_order(void) {
  struct my_state ms = {0};
  ms.quoted_at_end = SIZE_MAX;
  char **argv = build_argv(5, "prog", "-a", "--", "one", "two");
  int r = tiny_argp_parse(&argp, 5, argv, TINY_ARGP_IN_ORDER, NULL, &ms);
  TEST_ASSERT_EQUAL(TINY_ARGP_SUCCESS, r);
  TEST_ASSERT_EQUAL(3, ms.quoted_at_end);
}

void run_state_tests(void) {
  RUN_TEST(test_input_passthrough);
  RUN_TEST(test_arg_num_tracks);
  RUN_TEST(test_name_defaults_to_argv0);
  RUN_TEST(test_name_is_null_at_init);
  RUN_TEST(test_quoted_zero_without_dashes);
  RUN_TEST(test_quoted_set_after_dashes);
  RUN_TEST(test_argc_matches);
  RUN_TEST(test_flags_match);
  RUN_TEST(test_arg_index_null_ok);
  RUN_TEST(test_parse_argv0_user_name);
  RUN_TEST(test_argc_zero_with_parse_argv0);
  RUN_TEST(test_argc_zero_default_flags);
  RUN_TEST(test_arg_num_incremental);
  RUN_TEST(test_quoted_exact_after_dashes_in_order);
}
