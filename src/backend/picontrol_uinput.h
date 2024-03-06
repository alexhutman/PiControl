#ifndef _PICTRL_UINPUT_H
#define _PICTRL_UINPUT_H

#include <linux/uinput.h>
#include <stddef.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

#include "picontrol_config.h"


#define PICTRL_NOOP_KEY_COMB() {\
    .num_keys = 0, \
    .keys = {} \
}

// https://stackoverflow.com/a/2124433
#define PICTRL_KEY_COMB(...)  {\
    .num_keys = (sizeof((int[]){__VA_ARGS__})/sizeof(int)), \
    .keys = {__VA_ARGS__} \
}

int picontrol_create_virtual_keyboard();
int picontrol_destroy_virtual_keyboard(int fd);
void picontrol_type_char(int fd, char c);
void picontrol_print_str(int fd, char *str);

typedef struct {
    // INCLUSIVE ranges (both ends)
    int lower_bound;
    int upper_bound;
} pictrl_key_range;

typedef struct {
    size_t num_keys;
    int keys[PICTRL_MAX_SIMUL_KEYS];
} pictrl_key_combo;

typedef enum {
    PICTRL_KEY_UP = 0,
    PICTRL_KEY_DOWN = 1
} pictrl_key_status;

// TODO: Clean up this abomination?
static inline ssize_t picontrol_emit(int fd, int type, int code,
        pictrl_key_status val, struct timeval *cur_time) {
    gettimeofday(cur_time, NULL); // Does the time actually matter?

    struct input_event ie = {
        .type = type,
        .code = code,
        .value = val,
        .time = *cur_time
    };

    return write(fd, &ie, sizeof(ie));
}

/*** TODO: REVISIT! ***/
typedef int pictrl_uinput_t;
/**********************/


pictrl_uinput_t *pictrl_uinput_backend_new();
void pictrl_uinput_backend_free(pictrl_uinput_t *uinput);
#endif
