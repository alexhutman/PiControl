#pragma once

#include "model/mouse.h"

#include <stdbool.h>
#include <stddef.h>

typedef struct {
  int fd;
} Uinput;

Uinput *pictrl_uinput_backend_new();
void pictrl_uinput_backend_free(Uinput *uinput);

bool pictrl_uinput_type_char(Uinput *uinput, char c);
size_t pictrl_uinput_print_str(Uinput *uinput, const char *str);
void pictrl_uinput_click_mouse(Uinput *uinput, MouseBtnStatus status);
void pictrl_uinput_move_mouse_rel(Uinput *uinput, MouseCoord coords);
void pictrl_uinput_type_keysym(Uinput *uinput, char *keysym);
