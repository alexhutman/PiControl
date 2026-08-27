#include "keyboard/virtual_keyboard.h"

#include "logging/logger.h"
#include "serialize/mouse.h"

#include "keyboard/backend/uinput.h"
#ifdef PICTRL_XDO
  #include "keyboard/backend/xdo.h"
#endif // PICTRL_XDO

#include <stdlib.h>

static const char *BACKEND_NAMES[] = {"uinput",
#ifdef PICTRL_XDO
                                      "xdo"
#endif
};

const char *pictrl_backend_name(BackendType backend_type) {
  return BACKEND_NAMES[backend_type];
}

Keyboard *pictrl_keyboard_new(BackendType backend_type) {
  pictrl_log_debug("Attempting to create keyboard using %s backend\n",
                   pictrl_backend_name(backend_type));
  Keyboard *new_keyboard = malloc(sizeof(*new_keyboard));

  switch (backend_type) {
  case PICTRL_BACKEND_UINPUT: {
    new_keyboard->backend.uinput = pictrl_uinput_backend_new();
    if (!new_keyboard->backend.uinput) {
      free(new_keyboard);
      return NULL;
    }
    break;
  }
#ifdef PICTRL_XDO
  case PICTRL_BACKEND_XDO: {
    new_keyboard->backend.xdo = pictrl_xdo_backend_new();
    if (!new_keyboard->backend.xdo) {
      free(new_keyboard);
      return NULL;
    }
    break;
  }
#endif
  default: {
    pictrl_log_error("Unknown backend type: %d\n", backend_type);
    free(new_keyboard);
    return NULL;
  }
  };

  new_keyboard->backend_type = backend_type;
  pictrl_log_info("Keyboard created using %s backend\n",
                  pictrl_backend_name(new_keyboard->backend_type));
  return new_keyboard;
}

void pictrl_keyboard_free(Keyboard *keyboard) {
  switch (keyboard->backend_type) {
  case PICTRL_BACKEND_UINPUT:
    pictrl_uinput_backend_free(keyboard->backend.uinput);
    break;
#ifdef PICTRL_XDO
  case PICTRL_BACKEND_XDO:
    pictrl_xdo_backend_free(keyboard->backend.xdo);
    break;
#endif
  }
  free(keyboard);
  pictrl_log_debug("Destroyed keyboard\n");
}

void pictrl_handle_mouse_click(Keyboard *keyboard, Message *msg) {
  const MouseBtnStatus mouse_buttons = pictrl_get_mouse_status(msg);

  switch (keyboard->backend_type) {
  case PICTRL_BACKEND_UINPUT:
    pictrl_uinput_click_mouse(keyboard->backend.uinput, mouse_buttons);
    break;
#ifdef PICTRL_XDO
  case PICTRL_BACKEND_XDO:
    (void)msg;
    (void)keyboard;
    pictrl_xdo_click_mouse(keyboard->backend.xdo, mouse_buttons);
    break;
#endif
  }
}

void pictrl_handle_mouse_move(Keyboard *keyboard, Message *msg) {
  const MouseCoord coords = pictrl_get_mouse_coords(msg);

  switch (keyboard->backend_type) {
  case PICTRL_BACKEND_UINPUT:
    pictrl_uinput_move_mouse_rel(keyboard->backend.uinput, coords);
    break;
#ifdef PICTRL_XDO
  case PICTRL_BACKEND_XDO:
    pictrl_xdo_move_mouse_rel(keyboard->backend.xdo, coords);
    break;
#endif
  }
}

void pictrl_handle_text(Keyboard *keyboard, Message *msg) {
  switch (keyboard->backend_type) {
  case PICTRL_BACKEND_UINPUT:
    pictrl_uinput_type_char(keyboard->backend.uinput, *msg->payload);
    break;
#ifdef PICTRL_XDO
  case PICTRL_BACKEND_XDO:
    pictrl_xdo_print_str(keyboard->backend.xdo, msg);
    break;
#endif
  }
}

void pictrl_handle_keysym(Keyboard *keyboard, Message *msg) {
  switch (keyboard->backend_type) {
  case PICTRL_BACKEND_UINPUT:
    pictrl_uinput_type_keysym(keyboard->backend.uinput, (char *)msg->payload);
    break;
#ifdef PICTRL_XDO
  case PICTRL_BACKEND_XDO:
    pictrl_xdo_type_keysym(keyboard->backend.xdo, msg);
    break;
#endif
  }
}
