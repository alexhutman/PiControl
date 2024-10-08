#ifndef _PICTRL_UINPUT_H
#define _PICTRL_UINPUT_H

#include <linux/uinput.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>

#include "model/mouse.h"
#include "model/protocol.h"
#include "picontrol_config.h"

#define PICTRL_NOOP_KEY_COMB() \
  {                            \
    .num_keys = 0, .keys = {}  \
  }

// https://stackoverflow.com/a/2124433
#define PICTRL_KEY_COMB(...)                                            \
  {                                                                     \
    .num_keys = (sizeof((int[]){__VA_ARGS__}) / sizeof(int)), .keys = { \
      __VA_ARGS__                                                       \
    }                                                                   \
  }

#define PICTRL_KEY_DELAY_USEC 200000  // 200ms

typedef struct {
  // INCLUSIVE ranges (both ends)
  int lower_bound;
  int upper_bound;
} pictrl_key_range;

typedef struct {
  size_t num_keys;
  int keys[PICTRL_MAX_SIMUL_KEYS];
} pictrl_key_combo;

typedef enum { PICTRL_KEY_UP = 0, PICTRL_KEY_DOWN = 1 } pictrl_key_status;

// TODO: Clean up this abomination?
static inline ssize_t picontrol_emit(struct input_event *ie, int fd, int type,
                                     int code, pictrl_key_status val,
                                     struct timeval *cur_time) {
  ie->type = type;
  ie->code = code;
  ie->value = val;
  ie->time = *cur_time;

  return write(fd, ie, sizeof(*ie));
}

typedef struct {
  int fd;
} pictrl_uinput_t;

pictrl_uinput_t *pictrl_uinput_backend_new();
int picontrol_create_virtual_keyboard();
int picontrol_destroy_virtual_keyboard(int fd);
bool picontrol_uinput_type_char(pictrl_uinput_t *uinput, char c);
size_t picontrol_uinput_print_str(pictrl_uinput_t *uinput, const char *str);
void picontrol_uinput_click_mouse(pictrl_uinput_t *uinput,
                                  PiCtrlMouseBtnStatus status);
void picontrol_uinput_move_mouse_rel(pictrl_uinput_t *uinput,
                                     PiCtrlMouseCoord coords);
void picontrol_uinput_type_keysym(pictrl_uinput_t *uinput, char *keysym);
int pictrl_uinput_backend_init(pictrl_uinput_t *uinput);
int pictrl_uinput_backend_destroy(pictrl_uinput_t *uinput);
void pictrl_uinput_backend_free(pictrl_uinput_t *uinput);
#endif
