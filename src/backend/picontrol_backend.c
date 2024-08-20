#include "backend/picontrol_backend.h"

#include <stdlib.h>
#include <string.h>

#include "backend/picontrol_uinput.h"
#include "logging/log_utils.h"
#include "picontrol_config.h"
#include "serialize/mouse.h"

#ifdef PICTRL_XDO  // TODO: Use an xdo definition directly?
#include <xdo.h>

#include "backend/picontrol_xdo.h"
#endif

static const char *PICTRL_BACKEND_NAMES[] = {"uinput", "xdo"};

const char *pictrl_backend_name(pictrl_backend_type type) {
  // TODO: more robust way?
  return PICTRL_BACKEND_NAMES[type];
}

pictrl_backend *pictrl_backend_new() {
  pictrl_backend *new_backend = malloc(sizeof(*new_backend));
  int init_ret = 0;
#ifdef PICTRL_XDO
  new_backend->backend = (pictrl_backend_t *)pictrl_xdo_backend_new();
  new_backend->type = PICTRL_BACKEND_XDO;
#else  // default
  new_backend->backend = (pictrl_backend_t *)pictrl_uinput_backend_new();
  new_backend->type = PICTRL_BACKEND_UINPUT;
  init_ret = pictrl_uinput_backend_init(&new_backend->backend->uinput);
#endif
  if (new_backend->backend == NULL || init_ret < 0) {
    free(new_backend);
    return NULL;
  }

  return new_backend;
}

void pictrl_backend_free(pictrl_backend *backend) {
#ifdef PICTRL_XDO
  pictrl_xdo_backend_free(&backend->backend->xdo);
#else  // default
  pictrl_uinput_backend_destroy(&backend->backend->uinput);
  pictrl_uinput_backend_free(&backend->backend->uinput);
#endif
  free(backend);
}

void handle_mouse_click(pictrl_backend *backend, RawPiCtrlMessage *msg) {
#ifdef PICTRL_XDO
  (void)msg;
  (void)backend;
  pictrl_log_stub("Not implemented\n");
#else
  const PiCtrlMouseBtnStatus btn = pictrl_get_mouse_status(msg);
  picontrol_uinput_click_mouse(&backend->backend->uinput, btn);
#endif
}

void handle_mouse_move(pictrl_backend *backend, RawPiCtrlMessage *msg) {
  // extract the relative X and Y mouse locations to move by
  const PiCtrlMouseCoord coords = pictrl_get_mouse_coords(msg);

#ifdef PICTRL_XDO
  pictrl_log_debug("Moving mouse (%d, %d) relative units using xdo.\n\n",
                   coords.x, coords.y);
  if (xdo_move_mouse_relative(&backend->backend->xdo, coords.x, coords.y) !=
      0) {
    pictrl_log_warn("Mouse was unable to be moved (%d, %d) relative units.\n",
                    coords.x, coords.y);
  }
#else
  picontrol_uinput_move_mouse_rel(&backend->backend->uinput, coords);
#endif
}

void handle_text(pictrl_backend *backend, RawPiCtrlMessage *msg) {
#ifdef PICTRL_XDO
  // `xdo_enter_text_window` expects a null-terminated string, there are more
  // efficient approaches but this works
  static char text[MAX_BUF];
  memcpy(text, msg->payload, msg->header.payload_size);
  text[msg->header.payload_size] = 0;

  xdo_enter_text_window(
      &backend->backend->xdo, CURRENTWINDOW, text,
      XDO_KEYSTROKE_DELAY);  // TODO: what if sizeof(char) != sizeof(uint8_t)?
#else
  picontrol_uinput_type_char(&backend->backend->uinput, *msg->payload);
#endif
}

void handle_keysym(pictrl_backend *backend, RawPiCtrlMessage *msg) {
#ifdef PICTRL_XDO
  // `xdo_send_keysequence_window` expects a null-terminated string, there are
  // more efficient approaches but this works
  static char keysym[MAX_BUF];
  memcpy(keysym, msg->payload, msg->header.payload_size);
  keysym[msg->header.payload_size] = 0;

  xdo_send_keysequence_window(&backend->backend->xdo, CURRENTWINDOW, keysym,
                              XDO_KEYSTROKE_DELAY);
#else
  picontrol_uinput_type_keysym(&backend->backend->uinput, (char *)msg->payload);
#endif
}
