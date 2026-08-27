#include "backend/picontrol_backend.h"

#include "serialize/mouse.h"

#ifdef PICTRL_XDO
  #include "backend/picontrol_xdo.h"
#else
  #include "backend/picontrol_uinput.h"
#endif

#include <stdlib.h>

static const char *PICTRL_BACKEND_NAMES[] = {"uinput",
#ifdef PICTRL_XDO
                                             "xdo"
#endif
};

const char *pictrl_backend_name(pictrl_backend_type type) {
  return PICTRL_BACKEND_NAMES[type];
}

pictrl_backend *pictrl_backend_new() {
  pictrl_backend *new_backend = malloc(sizeof(*new_backend));
#ifdef PICTRL_XDO
  new_backend->backend = (pictrl_backend_t *)pictrl_xdo_backend_new();
  new_backend->type = PICTRL_BACKEND_XDO;
#else // default
  new_backend->backend = (pictrl_backend_t *)pictrl_uinput_backend_new();
  new_backend->type = PICTRL_BACKEND_UINPUT;
#endif
  if (!new_backend->backend) {
    free(new_backend);
    return NULL;
  }

  return new_backend;
}

void pictrl_backend_free(pictrl_backend *backend) {
#ifdef PICTRL_XDO
  pictrl_xdo_backend_free(&backend->backend->xdo);
#else // default
  pictrl_uinput_backend_free(&backend->backend->uinput);
#endif
  free(backend);
}

void handle_mouse_click(pictrl_backend *backend, RawPiCtrlMessage *msg) {
  const PiCtrlMouseBtnStatus mouse_buttons = pictrl_get_mouse_status(msg);
#ifdef PICTRL_XDO
  (void)msg;
  (void)backend;
  picontrol_xdo_click_mouse(&backend->backend->xdo, mouse_buttons);
#else
  picontrol_uinput_click_mouse(&backend->backend->uinput, mouse_buttons);
#endif
}

void handle_mouse_move(pictrl_backend *backend, RawPiCtrlMessage *msg) {
  const PiCtrlMouseCoord coords = pictrl_get_mouse_coords(msg);

#ifdef PICTRL_XDO
  picontrol_xdo_move_mouse_rel(&backend->backend->xdo, coords);
#else
  picontrol_uinput_move_mouse_rel(&backend->backend->uinput, coords);
#endif
}

void handle_text(pictrl_backend *backend, RawPiCtrlMessage *msg) {
#ifdef PICTRL_XDO
  picontrol_xdo_print_str(&backend->backend->xdo, msg);
#else
  picontrol_uinput_type_char(&backend->backend->uinput, *msg->payload);
#endif
}

void handle_keysym(pictrl_backend *backend, RawPiCtrlMessage *msg) {
#ifdef PICTRL_XDO
  picontrol_xdo_type_keysym(&backend->backend->xdo, msg);
#else
  picontrol_uinput_type_keysym(&backend->backend->uinput, (char *)msg->payload);
#endif
}
