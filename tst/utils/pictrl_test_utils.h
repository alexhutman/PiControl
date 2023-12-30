#ifndef _PICTRL_TEST_H
#define _PICTRL_TEST_H

#define pictrl_size(arr) sizeof(arr)/sizeof(arr[0])

// Function pointer: () -> int
typedef int (*TestCase)();

int run_test(const char *test_name, TestCase);
#endif
