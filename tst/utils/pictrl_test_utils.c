#include "logging/log_utils.h"
#include "utils/pictrl_test_utils.h"

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
