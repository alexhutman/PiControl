#include "keyboard/backend/uinput.h"

#include "logging/logger.h"
#include "pitest/api.h"
#include "util.h"

// Fixtures
static pictrl_uinput_t *uinput_keyboard;

static int before_all() {
  uinput_keyboard = pictrl_uinput_backend_new();
  if (!uinput_keyboard) {
    pictrl_log_error("Could not create uinput backend!\n");
    return 1;
  }

  pictrl_log_debug("Created uinput backend\n");
  return 0;
}

static int after_all() {
  pictrl_uinput_backend_free(uinput_keyboard);

  pictrl_log_debug("Closed virtual keyboard\n");
  return 0;
}

static int test_mv_mouse() {
  PiCtrlMouseCoord coords_diff = {5, 5};

  bool ret = true;
  for (int i = 0; i < 50; i++) {
    pictrl_uinput_move_mouse_rel(uinput_keyboard, coords_diff);
  }

  return ret ? 0 : 1;
}

static int test_ctrl_g() {
  pictrl_uinput_type_keysym(uinput_keyboard, "Ctrl+G");

  return 0;
}

static int test_all_ascii_chars() {
  for (char c = 0x20; c < 0x7F; c++) {
    if (!pictrl_uinput_type_char(uinput_keyboard, c)) {
      return 1;
    }
  }
  return 0;
}

static int test_typing() {
  const char str[] = "echo Hello World!\n";
  return pictrl_uinput_print_str(uinput_keyboard, str) == (sizeof(str) - 1) ? 0 : 1;
}

int main() {
  const TestCase test_cases[] = {
      {
          .test_name = "Mouse movement",
          .test_function = &test_mv_mouse,
      },
      {
          .test_name = "All ASCII characters",
          .test_function = &test_all_ascii_chars,
      },
      {
          .test_name = "Ctrl+G",
          .test_function = &test_ctrl_g,
      },
      {
          .test_name = "Normal typing (echo command)",
          .test_function = &test_typing,
      },
  };

  const TestSuite suite = {.name = "Uinput tests (manual)",
                           .test_cases = test_cases,
                           .num_tests = PICTRL_SIZE(test_cases),
                           .before_after_all = {.setup = &before_all, .teardown = &after_all},
                           .before_after_each = {.setup = NULL, .teardown = NULL}};

  return run_test_suite(&suite);
}
