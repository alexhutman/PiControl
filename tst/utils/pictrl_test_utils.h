#ifndef _PICTRL_TEST_H
#define _PICTRL_TEST_H

#define pictrl_size(arr) sizeof(arr)/sizeof(arr[0])

int run_test(const char *test_name, int (*test_func)());
#endif
