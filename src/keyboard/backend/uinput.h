#pragma once

#include "model/mouse.h"

#include <stdbool.h>
#include <stddef.h>

typedef struct {
  int fd;
} pictrl_uinput_t;

pictrl_uinput_t *pictrl_uinput_backend_new();
void pictrl_uinput_backend_free(pictrl_uinput_t *uinput);

bool picontrol_uinput_type_char(pictrl_uinput_t *uinput, char c);
size_t picontrol_uinput_print_str(pictrl_uinput_t *uinput, const char *str);
void picontrol_uinput_click_mouse(pictrl_uinput_t *uinput, PiCtrlMouseBtnStatus status);
void picontrol_uinput_move_mouse_rel(pictrl_uinput_t *uinput, PiCtrlMouseCoord coords);
void picontrol_uinput_type_keysym(pictrl_uinput_t *uinput, char *keysym);
