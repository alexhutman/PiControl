#include "logging/log_utils.h"
#include "utils/pictrl_test_utils.h"

int run_test(const char *test_name, int (*test_func)()) {
	pictrl_log_test_case("%s\n", test_name);
	int ret = test_func();
	if (ret != 0) {
		pictrl_log_error("%s test failed.\n", test_name);
		return ret;
	}

	pictrl_log_info("%s test passed!\n", test_name);
	return 0;
}
