#ifndef _PICTRL_UINPUT_H
#define _PICTRL_UINPUT_H

#include <limits.h>
#include <linux/uinput.h>
#include <unistd.h>

#ifdef PI_CTRL_DEBUG
#include <stdio.h>
#endif


extern const int valid_keyboard_keys[];
extern const size_t len_valid_keyboard_keys;

extern const int ascii_to_scancodes[][3];
extern const size_t len_ascii_to_scancodes;

int picontrol_create_uinput_fd();
int picontrol_destroy_uinput_fd(int fd);
void picontrol_print_str(int fd, char *str);

static inline ssize_t picontrol_emit(int fd, int type, int code, int val) {
    struct input_event ie = {
        .type = type,
        .code = code,
        .value = val,
        .time.tv_sec = 0,
        .time.tv_usec = 0
    };

    return write(fd, &ie, sizeof(ie));
}

static inline void picontrol_type_char(int fd, char c) {
    #ifdef PI_CTRL_DEBUG
    printf("Trying to type %c  (0x%x)...\n", c, c);
    #endif

    // Key down
    int i = 0;
    while (ascii_to_scancodes[c][i] != INT_MIN) {
        picontrol_emit(fd, EV_KEY, ascii_to_scancodes[c][i++], 1);
    }
    picontrol_emit(fd, EV_SYN, SYN_REPORT, 0);

    // Key up
    i = 0;
    while (ascii_to_scancodes[c][i] != INT_MIN) {
        picontrol_emit(fd, EV_KEY, ascii_to_scancodes[c][i++], 0);
    }
    picontrol_emit(fd, EV_SYN, SYN_REPORT, 0);
}

#endif
