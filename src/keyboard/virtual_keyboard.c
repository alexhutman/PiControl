#include "keyboard/virtual_keyboard.h"

#include "serialize/mouse.h"

#ifdef PICTRL_XDO
  #include "keyboard/backend/xdo.h"
#else
  #include "keyboard/backend/uinput.h"
#endif

#include <stdlib.h>

static const char *PICTRL_BACKEND_NAMES[] = {"uinput",
#ifdef PICTRL_XDO
                                             "xdo"
#endif
};

const char *pictrl_backend_name(pictrl_keyboard *keyboard) {
  return PICTRL_BACKEND_NAMES[keyboard->backend_type];
}

pictrl_keyboard *pictrl_keyboard_new() {
  pictrl_keyboard *new_keyboard = malloc(sizeof(*new_keyboard));

#ifdef PICTRL_XDO
  new_keyboard->backend.xdo = pictrl_xdo_backend_new();
  if (!new_keyboard->backend.xdo) {
    free(new_keyboard);
    return NULL;
  }
  new_keyboard->backend_type = PICTRL_BACKEND_XDO;
#else // default
  new_keyboard->backend.uinput = pictrl_uinput_backend_new();
  if (!new_keyboard->backend.uinput) {
    free(new_keyboard);
    return NULL;
  }
  new_keyboard->backend_type = PICTRL_BACKEND_UINPUT;
#endif

  return new_keyboard;
}

void pictrl_keyboard_free(pictrl_keyboard *keyboard) {
#ifdef PICTRL_XDO
  pictrl_xdo_backend_free(keyboard->backend.xdo);
#else // default
  pictrl_uinput_backend_free(keyboard->backend.uinput);
#endif
  free(keyboard);
}

void handle_mouse_click(pictrl_keyboard *keyboard, RawPiCtrlMessage *msg) {
  const PiCtrlMouseBtnStatus mouse_buttons = pictrl_get_mouse_status(msg);
#ifdef PICTRL_XDO
  (void)msg;
  (void)keyboard;
  picontrol_xdo_click_mouse(keyboard->backend.xdo, mouse_buttons);
#else
  picontrol_uinput_click_mouse(keyboard->backend.uinput, mouse_buttons);
#endif
}

void handle_mouse_move(pictrl_keyboard *keyboard, RawPiCtrlMessage *msg) {
  const PiCtrlMouseCoord coords = pictrl_get_mouse_coords(msg);

#ifdef PICTRL_XDO
  picontrol_xdo_move_mouse_rel(keyboard->backend.xdo, coords);
#else
  picontrol_uinput_move_mouse_rel(keyboard->backend.uinput, coords);
#endif
}

void handle_text(pictrl_keyboard *keyboard, RawPiCtrlMessage *msg) {
#ifdef PICTRL_XDO
  picontrol_xdo_print_str(keyboard->backend.xdo, msg);
#else
  picontrol_uinput_type_char(keyboard->backend.uinput, *msg->payload);
#endif
}

void handle_keysym(pictrl_keyboard *keyboard, RawPiCtrlMessage *msg) {
#ifdef PICTRL_XDO
  picontrol_xdo_type_keysym(keyboard->backend.xdo, msg);
#else
  picontrol_uinput_type_keysym(keyboard->backend.uinput, (char *)msg->payload);
#endif
}
