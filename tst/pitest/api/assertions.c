#include "pitest/api/assertions.h"

#include "logging/log_utils.h"

bool array_equals(uint8_t arr1[], size_t count1, uint8_t arr2[],
                  size_t count2) {
  if (count1 != count2) {
    pictrl_log_error("(count1, count2) = (%zu, %zu)\n", count1, count2);
    return false;
  }

  for (size_t cur = 0; cur < count1; cur++) {
    if (arr1[cur] != arr2[cur]) {
      pictrl_log_error("(arr1[%zu], arr2[%zu]) = (%u, %u)\n", cur, cur,
                       arr1[cur], arr2[cur]);
      return false;
    }
  }

  return true;
}
