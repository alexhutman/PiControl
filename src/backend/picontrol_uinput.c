#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <sys/time.h>

#include "logging/log_utils.h"
#include "picontrol_uinput.h"
#include "util.h"

// `errmsg` currently MUST take exactly 1 param: the string of the error
#define IOCTL_AND_LOG_ERR(errmsg, fd, ...) {\
    if (ioctl(fd, __VA_ARGS__) < 0) {\
        pictrl_log_error(errmsg, strerror(errno));\
    }\
}

static const pictrl_key_range valid_key_ranges[] = {
    // https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/include/uapi/linux/input-event-codes.h
    {.lower_bound = KEY_ESC, .upper_bound = KEY_KPDOT},
    {.lower_bound = KEY_F11, .upper_bound = KEY_F12}
};

/*
Index ("key") = ascii char
Entry ("value") = keyscan combination to produce the ascii

Ex. pictrl_ascii_to_event_codes[(size_t)"H" = 0x48] = [KEY_LEFTSHIFT, KEY_H]
*/
static const pictrl_key_combo pictrl_ascii_to_event_codes[] = {
    // TODO: Make these repetitive ones a macro or something?
    PICTRL_NOOP_KEY_COMB(),
    PICTRL_NOOP_KEY_COMB(),
    PICTRL_NOOP_KEY_COMB(),
    PICTRL_NOOP_KEY_COMB(),
    PICTRL_NOOP_KEY_COMB(),
    PICTRL_NOOP_KEY_COMB(),
    PICTRL_NOOP_KEY_COMB(),
    PICTRL_NOOP_KEY_COMB(),

    PICTRL_KEY_COMB(KEY_BACKSPACE),
    PICTRL_KEY_COMB(KEY_TAB),
    PICTRL_KEY_COMB(KEY_ENTER),

    PICTRL_NOOP_KEY_COMB(),
    PICTRL_NOOP_KEY_COMB(),
    PICTRL_NOOP_KEY_COMB(),
    PICTRL_NOOP_KEY_COMB(),
    PICTRL_NOOP_KEY_COMB(),
    PICTRL_NOOP_KEY_COMB(),
    PICTRL_NOOP_KEY_COMB(),
    PICTRL_NOOP_KEY_COMB(),
    PICTRL_NOOP_KEY_COMB(),
    PICTRL_NOOP_KEY_COMB(),
    PICTRL_NOOP_KEY_COMB(),
    PICTRL_NOOP_KEY_COMB(),
    PICTRL_NOOP_KEY_COMB(),
    PICTRL_NOOP_KEY_COMB(),
    PICTRL_NOOP_KEY_COMB(),
    PICTRL_NOOP_KEY_COMB(),

    PICTRL_KEY_COMB(KEY_ESC),

    PICTRL_NOOP_KEY_COMB(),
    PICTRL_NOOP_KEY_COMB(),
    PICTRL_NOOP_KEY_COMB(),
    PICTRL_NOOP_KEY_COMB(),

    PICTRL_KEY_COMB(KEY_SPACE),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_1),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_APOSTROPHE),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_3),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_4),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_5),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_7),
    PICTRL_KEY_COMB(KEY_APOSTROPHE),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_9),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_0),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_8),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_EQUAL),
    PICTRL_KEY_COMB(KEY_COMMA),
    PICTRL_KEY_COMB(KEY_MINUS),
    PICTRL_KEY_COMB(KEY_DOT),
    PICTRL_KEY_COMB(KEY_SLASH),
    PICTRL_KEY_COMB(KEY_0),
    PICTRL_KEY_COMB(KEY_1),
    PICTRL_KEY_COMB(KEY_2),
    PICTRL_KEY_COMB(KEY_3),
    PICTRL_KEY_COMB(KEY_4),
    PICTRL_KEY_COMB(KEY_5),
    PICTRL_KEY_COMB(KEY_6),
    PICTRL_KEY_COMB(KEY_7),
    PICTRL_KEY_COMB(KEY_8),
    PICTRL_KEY_COMB(KEY_9),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_SEMICOLON),
    PICTRL_KEY_COMB(KEY_SEMICOLON),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_COMMA),
    PICTRL_KEY_COMB(KEY_EQUAL),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_DOT),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_SLASH),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_2),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_A),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_B),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_C),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_D),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_E),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_F),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_G),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_H),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_I),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_J),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_K),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_L),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_M),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_N),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_O),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_P),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_Q),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_R),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_S),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_T),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_U),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_V),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_W),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_X),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_Y),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_Z),
    PICTRL_KEY_COMB(KEY_LEFTBRACE),
    PICTRL_KEY_COMB(KEY_BACKSLASH),
    PICTRL_KEY_COMB(KEY_RIGHTBRACE),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_6),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_MINUS),
    PICTRL_KEY_COMB(KEY_GRAVE),
    PICTRL_KEY_COMB(KEY_A),
    PICTRL_KEY_COMB(KEY_B),
    PICTRL_KEY_COMB(KEY_C),
    PICTRL_KEY_COMB(KEY_D),
    PICTRL_KEY_COMB(KEY_E),
    PICTRL_KEY_COMB(KEY_F),
    PICTRL_KEY_COMB(KEY_G),
    PICTRL_KEY_COMB(KEY_H),
    PICTRL_KEY_COMB(KEY_I),
    PICTRL_KEY_COMB(KEY_J),
    PICTRL_KEY_COMB(KEY_K),
    PICTRL_KEY_COMB(KEY_L),
    PICTRL_KEY_COMB(KEY_M),
    PICTRL_KEY_COMB(KEY_N),
    PICTRL_KEY_COMB(KEY_O),
    PICTRL_KEY_COMB(KEY_P),
    PICTRL_KEY_COMB(KEY_Q),
    PICTRL_KEY_COMB(KEY_R),
    PICTRL_KEY_COMB(KEY_S),
    PICTRL_KEY_COMB(KEY_T),
    PICTRL_KEY_COMB(KEY_U),
    PICTRL_KEY_COMB(KEY_V),
    PICTRL_KEY_COMB(KEY_W),
    PICTRL_KEY_COMB(KEY_X),
    PICTRL_KEY_COMB(KEY_Y),
    PICTRL_KEY_COMB(KEY_Z),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_LEFTBRACE),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_BACKSLASH),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_RIGHTBRACE),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_GRAVE),

    PICTRL_NOOP_KEY_COMB()
};

pictrl_uinput_t *pictrl_uinput_backend_new() {
    return malloc(sizeof(pictrl_uinput_t));
}

void pictrl_uinput_backend_free(pictrl_uinput_t *uinput) {
    free(uinput);
}

bool picontrol_type_char(int fd, char c) {
    // TODO: Error handling on `picontrol_emit` calls
    struct input_event ie;
    struct timeval cur_time;
    const size_t ie_sz = sizeof(ie);
    gettimeofday(&cur_time, NULL);
    bool ret = true;

    // Key down
    for (size_t i=0; i < pictrl_ascii_to_event_codes[(size_t)c].num_keys; i++) {
        ret &= picontrol_emit(&ie, fd, EV_KEY, pictrl_ascii_to_event_codes[(size_t)c].keys[i], PICTRL_KEY_DOWN, &cur_time) == ie_sz;
        cur_time.tv_usec += PICTRL_KEY_DELAY_USEC;
    }
    ret &= picontrol_emit(&ie, fd, EV_SYN, SYN_REPORT, 0, &cur_time) == ie_sz;
    if (!ret) {
        return false;
    }

    // Key up
    for (size_t i=0; i < pictrl_ascii_to_event_codes[(size_t)c].num_keys; i++) {
        ret &= picontrol_emit(&ie, fd, EV_KEY, pictrl_ascii_to_event_codes[(size_t)c].keys[i], PICTRL_KEY_UP, &cur_time) == ie_sz;
        cur_time.tv_usec += PICTRL_KEY_DELAY_USEC;
    }
    ret &= picontrol_emit(&ie, fd, EV_SYN, SYN_REPORT, 0, &cur_time) == ie_sz;

    return ret;
}

int picontrol_create_virtual_keyboard() {
    int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (fd < 0) {
        pictrl_log_error("Could not open /dev/uinput: %s\n", strerror(errno));
        return -1;
    }

    // Enable device to pass key events
    IOCTL_AND_LOG_ERR("Could not enable key events: %s\n", fd, UI_SET_EVBIT, EV_KEY);
    for (size_t i=0; i < PICTRL_SIZE(valid_key_ranges); i++) {
        for (int key = valid_key_ranges[i].lower_bound; key <= valid_key_ranges[i].upper_bound; key++) {
            IOCTL_AND_LOG_ERR("Could not enable key: %s\n", fd, UI_SET_KEYBIT, key);
        }
    }

    // Enable left, right mouse button clicks, touchpad taps
    const int buttons[] = {
        BTN_LEFT, BTN_RIGHT, BTN_TOUCH, BTN_TOOL_DOUBLETAP, BTN_TOOL_TRIPLETAP
    };
    for (size_t i=0; i < PICTRL_SIZE(buttons); i++) {
        IOCTL_AND_LOG_ERR("Could not enable clicks/taps: %s\n", fd, UI_SET_KEYBIT, buttons[i]);
    }

    // Enable mousewheel
    IOCTL_AND_LOG_ERR("Could not enable mousewheel: %s\n", fd, UI_SET_RELBIT, REL_WHEEL);

    // Enable mouse movement
    IOCTL_AND_LOG_ERR("Could not enable mouse: %s\n", fd, UI_SET_EVBIT, EV_REL);
    IOCTL_AND_LOG_ERR("Could not enable mouse's X movement: %s\n", fd, UI_SET_RELBIT, REL_X);
    IOCTL_AND_LOG_ERR("Could not enable mouse's Y movement: %s\n", fd, UI_SET_RELBIT, REL_Y);

    static const struct uinput_setup usetup = {
        .id = {
            .bustype = BUS_USB,
            .vendor = 0x1337,
            .product = 0x0420,
        },
        .name = "PiControl Virtual Keyboard"
    };
    // Set up and create device
    IOCTL_AND_LOG_ERR("Could not set up virtual keyboard: %s\n", fd, UI_DEV_SETUP, &usetup);
    IOCTL_AND_LOG_ERR("Could not create virtual keyboard: %s\n", fd, UI_DEV_CREATE);
    return fd;
}

int picontrol_destroy_virtual_keyboard(int fd) {
    int destroy_ret = ioctl(fd, UI_DEV_DESTROY);
    if (destroy_ret < 0) {
        pictrl_log_error("Could not destroy virtual keyboard: %s\n", strerror(errno));
    }

    int close_ret = close(fd);
    if (close_ret == -1) {
        pictrl_log_error("Could not close file descriptor %d: %s\n", fd, strerror(errno));
    }
    return (destroy_ret >= 0 && close_ret == 0) ? 0 : -1;
}

size_t picontrol_print_str(int fd, const char *str) {
    char *p = (char *)str;
    size_t chars_written = 0;
    while (*p) {
        size_t written = picontrol_type_char(fd, *p++) ? 1 : 0;
        chars_written += written;
        if (written == 0) {
            break;
        }
    }
    return chars_written;
}
