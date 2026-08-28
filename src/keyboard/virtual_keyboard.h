#pragma once

#include "keyboard/backend/uinput.h"
#include "model/protocol.h"

#ifdef PICTRL_XDO
  #include <xdo.h>
#endif

typedef enum {
  PICTRL_BACKEND_UINPUT,
#ifdef PICTRL_XDO
  PICTRL_BACKEND_XDO
#endif
} BackendType;

typedef union {
  Uinput *uinput;
#ifdef PICTRL_XDO
  xdo_t *xdo;
#endif
} Backend;

typedef struct {
  BackendType backend_type;
  Backend backend;
} Keyboard;

Keyboard *pictrl_keyboard_new(BackendType backend_type);
void pictrl_keyboard_free(Keyboard *keyboard);
const char *pictrl_backend_name(BackendType backend_type);

void pictrl_handle_mouse_click(Keyboard *keyboard, Message *msg);
void pictrl_handle_mouse_move(Keyboard *keyboard, Message *msg);
void pictrl_handle_text(Keyboard *keyboard, Message *msg);
void pictrl_handle_keysym(Keyboard *keyboard, Message *msg);
