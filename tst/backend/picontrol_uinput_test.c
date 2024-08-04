#include <inttypes.h>
#include <limits.h>
#include <linux/uinput.h>
#include <stddef.h>
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
static int test_ctrl_g();
static int test_typing();

// Fixtures
static pictrl_uinput_t virt_keyboard;

int before_all() {
    virt_keyboard.fd = picontrol_create_virtual_keyboard();
    if (virt_keyboard.fd < 0) {
        pictrl_log_error("Could not open file descriptor for new virtual device.\n");
        return 1;
    }

    sleep(1);
    pictrl_log_debug("Created virtual keyboard\n");
    return 0;
}

int after_all() {
    if (picontrol_destroy_virtual_keyboard(virt_keyboard.fd) < 0) {
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
            .test_name = "Ctrl+G",
            .test_function = &test_ctrl_g,
        },
        {
            .test_name = "Normal typing (echo command)",
            .test_function = &test_typing,
        },
    };

    const TestSuite suite = {
        .name = "Uinput tests (manual)",
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
    struct input_event ie;
    struct timeval cur_time;
    gettimeofday(&cur_time, NULL);

    const size_t ie_sz = sizeof(ie);
    const size_t delay_us = 1000;

    bool ret = true;
    for (int i=0; i<50; i++) {
        // Move mouse diagonally by about 7 units
        ret &= picontrol_emit(&ie, virt_keyboard.fd, EV_REL, REL_X, 5, &cur_time) == ie_sz;
        ret &= picontrol_emit(&ie, virt_keyboard.fd, EV_REL, REL_Y, 5, &cur_time) == ie_sz;
        ret &= picontrol_emit(&ie, virt_keyboard.fd, EV_SYN, SYN_REPORT, 0, &cur_time) == ie_sz;
        cur_time.tv_usec += delay_us;
    }

    return ret ? 0 : 1;
}

static int test_ctrl_g() {
    struct input_event ie;
    struct timeval cur_time;
    gettimeofday(&cur_time, NULL);

    const size_t ie_sz = sizeof(ie);
    const size_t delay_us = 1000;

    bool ret = true;
    ret &= picontrol_emit(&ie, virt_keyboard.fd, EV_KEY, KEY_LEFTCTRL, PICTRL_KEY_DOWN, &cur_time) == ie_sz;
    cur_time.tv_usec += delay_us;
    ret &= picontrol_emit(&ie, virt_keyboard.fd, EV_KEY, KEY_G, PICTRL_KEY_DOWN, &cur_time) == ie_sz;
    cur_time.tv_usec += delay_us;
    ret &= picontrol_emit(&ie, virt_keyboard.fd, EV_SYN, SYN_REPORT, 0, &cur_time) == ie_sz;
    cur_time.tv_usec += delay_us;
    ret &= picontrol_emit(&ie, virt_keyboard.fd, EV_KEY, KEY_LEFTCTRL, PICTRL_KEY_UP, &cur_time) == ie_sz;
    cur_time.tv_usec += delay_us;
    ret &= picontrol_emit(&ie, virt_keyboard.fd, EV_KEY, KEY_G, PICTRL_KEY_UP, &cur_time) == ie_sz;
    cur_time.tv_usec += delay_us;
    ret &= picontrol_emit(&ie, virt_keyboard.fd, EV_SYN, SYN_REPORT, 0, &cur_time) == ie_sz;

    return ret ? 0 : 1;
}

static int test_all_ascii_chars() {
    for (char c = 0x20; c < 0x7F; c++) {
        if (!picontrol_uinput_type_char(&virt_keyboard, c)) {
            return 1;
        }
    }
    return 0;
}

static int test_typing() {
    const char str[] = "echo Hello World!\n";
    return picontrol_uinput_print_str(&virt_keyboard, str) == (sizeof(str)-1) ? 0 : 1;
}
