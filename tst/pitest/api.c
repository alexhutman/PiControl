#include "logging/log_utils.h"
#include "pitest/api.h"

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

size_t run_test_suite(const TestCase test_cases[], size_t num_tests) {
    size_t failed_test_count = 0;
    for (size_t test_id = 0; test_id < num_tests; test_id++) {
        int res = run_test(&test_cases[test_id]);
        if (res != 0) {
            failed_test_count++;
        }
    }

    return failed_test_count;
}