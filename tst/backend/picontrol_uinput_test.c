#include <inttypes.h>
#include <limits.h>
#include <linux/uinput.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "backend/picontrol_uinput.h"
#include "logging/log_utils.h"
#include "util.h"

#include "pitest/api.h"
#include "pitest/api/assertions.h"

static int test_mv_mouse();
static int test_all_ascii_chars();
static int test_ctrl_c();
static int test_typing();

// Fixtures
static int virt_keyboard_fd;

int before_all() {
    virt_keyboard_fd  = picontrol_create_virtual_keyboard();
    if (virt_keyboard_fd < 0) {
        pictrl_log_error("Could not open file descriptor for new virtual device.\n");
        return 1;
    }

    sleep(1);
    pictrl_log_debug("Created virtual keyboard\n");
    return 0;
}

int after_all() {
    if (picontrol_destroy_virtual_keyboard(virt_keyboard_fd) < 0) {
        pictrl_log_error("Couldn't close PiControl virtual keyboard.\n");
        return 1;
    }

    sleep(1);
    pictrl_log_debug("Closed virtual keyboard\n");
    return 0;
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
            .test_name = "Ctrl+C",
            .test_function = &test_ctrl_c,
        },
        {
            .test_name = "Normal typing test",
            .test_function = &test_typing,
        },
    };

    const TestSuite suite = {
        .name = "Uinput Test Suite",
        .test_cases = test_cases,
        .num_tests = PICTRL_SIZE(test_cases),
        .before_after_all = {
            .setup = &before_all,
            .teardown = &after_all
        },
        .before_after_each = {
            .setup = NULL,
            .teardown = NULL
        }
    };

    return run_test_suite(&suite);
}

static int test_mv_mouse() {
    struct timeval cur_time;
    for (int i=0; i<50; i++) {
        // Move mouse diagonally by about 7 units
        picontrol_emit(virt_keyboard_fd, EV_REL, REL_X, 5, &cur_time);
        picontrol_emit(virt_keyboard_fd, EV_REL, REL_Y, 5, &cur_time);
        picontrol_emit(virt_keyboard_fd, EV_SYN, SYN_REPORT, 0, &cur_time);
    }

    // TODO: fix these return vals when/if error handling is added
    // I also can't test this atm since I'm on WSL :(
    return 1;
}

static int test_ctrl_c() {
    struct timeval cur_time;
    picontrol_emit(virt_keyboard_fd, EV_KEY, KEY_LEFTCTRL, PICTRL_KEY_DOWN, &cur_time);
    picontrol_emit(virt_keyboard_fd, EV_KEY, KEY_C, PICTRL_KEY_DOWN, &cur_time);
    picontrol_emit(virt_keyboard_fd, EV_SYN, SYN_REPORT, 0, &cur_time);
    picontrol_emit(virt_keyboard_fd, EV_KEY, KEY_LEFTCTRL, PICTRL_KEY_UP, &cur_time);
    picontrol_emit(virt_keyboard_fd, EV_KEY, KEY_C, PICTRL_KEY_UP, &cur_time);
    picontrol_emit(virt_keyboard_fd, EV_SYN, SYN_REPORT, 0, &cur_time);

    return 1;
}

static int test_all_ascii_chars() {
    for (char c = 0x20; c < 0x7F; c++) {
        picontrol_type_char(virt_keyboard_fd, c);
    }
    return 1;
}

static int test_typing() {
    picontrol_print_str(virt_keyboard_fd, "echo Hello World!\n");
    return 1;
}
