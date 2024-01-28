#ifndef _PITEST_API_H
#define _PITEST_API_H

#include <stdbool.h>
#include <stddef.h>

#define pictrl_size(arr) sizeof(arr)/sizeof(arr[0])

// Function pointer: () -> int (they're all currently the same but don't have to be)
typedef int (*TestFunction)();
typedef int (*SetupFunction)();
typedef int (*TeardownFunction)();

typedef struct {
    const SetupFunction setup;
    const TeardownFunction teardown;
} SetupTeardown;

typedef struct {
    const char *test_name;
    const TestFunction test_function;
} TestCase;

typedef struct {
    const char *name;
    const TestCase *test_cases;
    const size_t num_tests;
    const SetupTeardown before_after_all;
    const SetupTeardown before_after_each;
} TestSuite;

int run_test(const TestCase*);
size_t run_test_suite(const TestSuite*);
#endif
