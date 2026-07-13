#include "test_utils.h"
#include "unity.h"

/* Each test file exposes a runner that calls RUN_TEST on its cases. */
void run_short_opts_tests(void);
void run_long_opts_tests(void);
void run_non_option_tests(void);
void run_key_sequence_tests(void);
void run_flags_tests(void);
void run_option_flags_tests(void);
void run_help_tests(void);
void run_errors_tests(void);
void run_state_tests(void);

void setUp(void) { test_utils_reset(); }
void tearDown(void) {}

int main(void) {
  UNITY_BEGIN();
  run_short_opts_tests();
  run_long_opts_tests();
  run_non_option_tests();
  run_key_sequence_tests();
  run_flags_tests();
  run_option_flags_tests();
  run_help_tests();
  run_errors_tests();
  run_state_tests();
  return UNITY_END();
}
