#include <fcntl.h>

#include "picontrol_uinput.h"
#include "util.h"


static const pictrl_key_range valid_key_ranges[] = {
    // https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/include/uapi/linux/input-event-codes.h
    {.lower_bound = KEY_ESC, .upper_bound = KEY_KPDOT},
    {.lower_bound = KEY_F11, .upper_bound = KEY_F12}
};

/*
Index ("key") = ascii char
Entry ("value") = keyscan combination to produce the ascii

Ex. pictrl_ascii_to_event_codes[(unsigned int)"H" = 0x48] = [KEY_LEFTSHIFT, KEY_H]
*/
static const pictrl_key_combo pictrl_ascii_to_event_codes[] = {
    // TODO: Make these repetitive ones a macro or something?
    PICTRL_EMPTY_KEY_COMB(),
    PICTRL_EMPTY_KEY_COMB(),
    PICTRL_EMPTY_KEY_COMB(),
    PICTRL_EMPTY_KEY_COMB(),
    PICTRL_EMPTY_KEY_COMB(),
    PICTRL_EMPTY_KEY_COMB(),
    PICTRL_EMPTY_KEY_COMB(),
    PICTRL_EMPTY_KEY_COMB(),

    PICTRL_KEY_COMB(KEY_BACKSPACE),
    PICTRL_KEY_COMB(KEY_TAB),
    PICTRL_KEY_COMB(KEY_ENTER),

    PICTRL_EMPTY_KEY_COMB(),
    PICTRL_EMPTY_KEY_COMB(),
    PICTRL_EMPTY_KEY_COMB(),
    PICTRL_EMPTY_KEY_COMB(),
    PICTRL_EMPTY_KEY_COMB(),
    PICTRL_EMPTY_KEY_COMB(),
    PICTRL_EMPTY_KEY_COMB(),
    PICTRL_EMPTY_KEY_COMB(),
    PICTRL_EMPTY_KEY_COMB(),
    PICTRL_EMPTY_KEY_COMB(),
    PICTRL_EMPTY_KEY_COMB(),
    PICTRL_EMPTY_KEY_COMB(),
    PICTRL_EMPTY_KEY_COMB(),
    PICTRL_EMPTY_KEY_COMB(),
    PICTRL_EMPTY_KEY_COMB(),
    PICTRL_EMPTY_KEY_COMB(),

    PICTRL_KEY_COMB(KEY_ESC),

    PICTRL_EMPTY_KEY_COMB(),
    PICTRL_EMPTY_KEY_COMB(),
    PICTRL_EMPTY_KEY_COMB(),
    PICTRL_EMPTY_KEY_COMB(),

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

    PICTRL_EMPTY_KEY_COMB()
};

void picontrol_type_char(int fd, char c) {
    // TODO: Error handling on `picontrol_emit` calls
    struct timeval cur_time;

    // Key down
    for (size_t i=0; i < pictrl_ascii_to_event_codes[(size_t)c].num_keys; i++) {
        picontrol_emit(fd, EV_KEY, pictrl_ascii_to_event_codes[(size_t)c].keys[i], PICTRL_KEY_DOWN, &cur_time);
    }
    picontrol_emit(fd, EV_SYN, SYN_REPORT, 0, &cur_time);

    // Key up
    for (size_t i=0; i < pictrl_ascii_to_event_codes[(size_t)c].num_keys; i++) {
        picontrol_emit(fd, EV_KEY, pictrl_ascii_to_event_codes[(size_t)c].keys[i], PICTRL_KEY_UP, &cur_time);
    }
    picontrol_emit(fd, EV_SYN, SYN_REPORT, 0, &cur_time);
}

int picontrol_create_virtual_keyboard() {
    static const struct uinput_setup usetup = {
        .id.bustype = BUS_USB,
        .id.vendor = 0x1337,
        .id.product = 0x0420,
        .name = "PiControl Virtual Keyboard"
    };

    int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (fd < 0) {
        return fd;
    }

    // Enable device to pass key events
    ioctl(fd, UI_SET_EVBIT, EV_KEY);
    for (size_t i=0; i < PICTRL_SIZE(valid_key_ranges); i++) {
        for (int key = valid_key_ranges[i].lower_bound; key <= valid_key_ranges[i].upper_bound; key++) {
            ioctl(fd, UI_SET_KEYBIT, key);
        }
    }

    // Enable left, right mouse button clicks, touchpad taps
    const int buttons[] = {
        BTN_LEFT, BTN_RIGHT, BTN_TOUCH, BTN_TOOL_DOUBLETAP, BTN_TOOL_TRIPLETAP
    };
    for (size_t i=0; i < PICTRL_SIZE(buttons); i++) {
        ioctl(fd, UI_SET_KEYBIT, buttons[i]);
    }

    // Enable mousewheel
    ioctl(fd, UI_SET_RELBIT, REL_WHEEL);

    // Enable mouse movement
    ioctl(fd, UI_SET_EVBIT, EV_REL);
    ioctl(fd, UI_SET_RELBIT, REL_X);
    ioctl(fd, UI_SET_RELBIT, REL_Y);

    // Set up and create device
    ioctl(fd, UI_DEV_SETUP, &usetup);
    ioctl(fd, UI_DEV_CREATE);

    return fd;
}

int picontrol_destroy_virtual_keyboard(int fd) {
    // TODO: Better error handling
    return (ioctl(fd, UI_DEV_DESTROY) >= 0 && close(fd) >= 0) ? 0 : -1;
}

void picontrol_print_str(int fd, char *str) {
    char *p = str;
    while (*p) {
        picontrol_type_char(fd, *p++);
    }
}
