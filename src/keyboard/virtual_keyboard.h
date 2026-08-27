#pragma once

#include "keyboard/backend/uinput.h"
#include "model/protocol.h"

#ifdef PICTRL_XDO // TODO: Use an xdo definition directly?
  #include <xdo.h>
#endif

typedef enum {
  PICTRL_BACKEND_UINPUT,
#ifdef PICTRL_XDO
  PICTRL_BACKEND_XDO
#endif
} pictrl_backend_type;

typedef union {
  pictrl_uinput_t uinput;
#ifdef PICTRL_XDO
  xdo_t xdo;
#endif
} pictrl_backend_t;

typedef struct {
  pictrl_backend_type type;
  pictrl_backend_t *backend;
} pictrl_keyboard;

pictrl_keyboard *pictrl_keyboard_new();
void pictrl_keyboard_free(pictrl_keyboard *keyboard);
const char *pictrl_backend_name(pictrl_keyboard *keyboard);

void handle_mouse_click(pictrl_keyboard *keyboard, RawPiCtrlMessage *msg);
void handle_mouse_move(pictrl_keyboard *keyboard, RawPiCtrlMessage *msg);
void handle_text(pictrl_keyboard *keyboard, RawPiCtrlMessage *msg);
void handle_keysym(pictrl_keyboard *keyboard, RawPiCtrlMessage *msg);
