#ifndef _PICTRL_TEST_H
#define _PICTRL_TEST_H

#include <stdbool.h>
#include <stddef.h>

#define pictrl_size(arr) sizeof(arr)/sizeof(arr[0])

// Function pointer: () -> int
typedef int (*TestFunction)();

typedef struct TestCase {
    const char *test_name;
    const TestFunction test_function;
} TestCase;

int run_test(const TestCase*);
size_t run_test_suite(const TestCase test_cases[], size_t num_tests);
#endif
