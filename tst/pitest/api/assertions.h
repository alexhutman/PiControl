#ifndef _PITEST_API_ASSERTIONS_H
#define _PITEST_API_ASSERTIONS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

bool array_equals(uint8_t arr1[], size_t count1,
                  uint8_t arr2[], size_t count2);
#endif
