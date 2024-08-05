#include "pitest/api.h"

#include "logging/log_utils.h"

int run_test(const TestCase *test_case) {
  pictrl_log_test_case("%s\n", test_case->test_name);
  int ret = test_case->test_function();
  if (ret != 0) {
    pictrl_log_error("%s test failed.\n", test_case->test_name);
    return ret;
  }

  pictrl_log_info("%s test passed!\n", test_case->test_name);
  return 0;
}

int run_setup_if_exists(SetupFunction setup) {
  if (setup == NULL) {
    return 0;
  }
  return setup();
}

int run_teardown_if_exists(TeardownFunction teardown) {
  if (teardown == NULL) {
    return 0;
  }
  return teardown();
}

size_t run_test_suite(const TestSuite *suite) {
  if (suite->num_tests == 0) {
    return 0;
  }
  pictrl_log_info("Running test suite '%s'\n", suite->name);

  const SetupFunction suite_setup = suite->before_after_all.setup;
  int suite_setup_ret = run_setup_if_exists(suite_setup);
  if (suite_setup_ret != 0) {
    pictrl_log_error("Setup for suite '%s' failed with code %d\n", suite->name,
                     suite_setup_ret);
    return 1;
  }

  size_t failed_test_count = 0;
  for (size_t test_id = 0; test_id < suite->num_tests; test_id++) {
    const SetupFunction case_setup = suite->before_after_each.setup;
    int case_setup_ret = run_setup_if_exists(case_setup);
    if (case_setup_ret != 0) {
      pictrl_log_error(
          "Setup for test case '%s' failed with code %d\n. Skipping...",
          suite->test_cases[test_id].test_name, case_setup_ret);
      failed_test_count++;
      continue;
    }

    bool test_failed = run_test(&suite->test_cases[test_id]) != 0;
    if (test_failed) {
      failed_test_count++;
    }

    const TeardownFunction case_teardown = suite->before_after_each.teardown;
    int case_teardown_ret = run_teardown_if_exists(case_teardown);
    if (case_teardown_ret != 0) {
      pictrl_log_error("Teardown for test case '%s' failed with code %d\n",
                       suite->test_cases[test_id].test_name, case_teardown_ret);
      if (!test_failed) {
        failed_test_count++;
      }
    }
  }

  const TestFunction suite_teardown = suite->before_after_all.teardown;
  int suite_teardown_ret = run_teardown_if_exists(suite_teardown);
  if (suite_teardown_ret != 0) {
    pictrl_log_error("Teardown for suite '%s' failed with code %d\n",
                     suite->name, suite_teardown_ret);
    if (failed_test_count == 0) {
      failed_test_count++;
    }
  }

  if (failed_test_count > 0) {
    pictrl_log_error("Test suite '%s' failed!\n", suite->name);
    return failed_test_count;
  }

  pictrl_log_info("Test suite '%s' passed!\n", suite->name);
  return 0;
}
