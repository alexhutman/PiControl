#pragma once

#include <stdbool.h>
#include <stddef.h>

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

size_t run_test_suite(const TestSuite *);
