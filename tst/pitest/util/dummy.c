#include "logging/log_utils.h"
#include "pitest/api.h"
#include "pitest/util/dummy.h"


int dummy_test_case_pass() {
    pictrl_log_debug("In pass dummy test case\n");
    return 0;
}

int dummy_test_case_fail() {
    pictrl_log_debug("In fail dummy test case\n");
    return 1;
}
