#include "keyboard/backend/xdo.h"

#include "logging/logger.h"
#include "model/mouse.h"
#include "model/protocol.h"

#include <xdo.h>

#include <stdlib.h>
#include <string.h>

// Delay between xdo keystrokes in microseconds
#define XDO_KEYSTROKE_DELAY (useconds_t)10000

xdo_t *pictrl_xdo_backend_new() {
  const char *display = getenv("DISPLAY");
  return xdo_new(display);
}

void pictrl_xdo_backend_free(xdo_t *xdo) {
  xdo_free(xdo);
}

void picontrol_xdo_click_mouse(xdo_t *xdo, PiCtrlMouseBtnStatus status) {
  (void)xdo;
  (void)status;
  pictrl_log_warn("[STUBBED] %s is not implemented\n", __func__);
}

void picontrol_xdo_move_mouse_rel(xdo_t *xdo, PiCtrlMouseCoord coords) {
  pictrl_log_debug("Moving mouse (%d, %d) relative units using xdo.\n\n", coords.x, coords.y);
  if (xdo_move_mouse_relative(xdo, coords.x, coords.y) != 0) {
    pictrl_log_warn("Mouse was unable to be moved (%d, %d) relative units.\n", coords.x, coords.y);
  }
}

void picontrol_xdo_print_str(xdo_t *xdo, const RawPiCtrlMessage *msg) {
  // `xdo_enter_text_window` expects a null-terminated string, there are more
  // efficient approaches but this works
  static char text[MAX_PAYLOAD_SIZE + 1];
  memcpy(text, msg->payload, msg->header.payload_size);
  text[msg->header.payload_size] = 0;

  xdo_enter_text_window(xdo, CURRENTWINDOW, text, XDO_KEYSTROKE_DELAY);
}

void picontrol_xdo_type_keysym(xdo_t *xdo, const RawPiCtrlMessage *msg) {
  // `xdo_send_keysequence_window` expects a null-terminated string, there are
  // more efficient approaches but this works
  static char keysym[MAX_PAYLOAD_SIZE + 1];
  memcpy(keysym, msg->payload, msg->header.payload_size);
  keysym[msg->header.payload_size] = 0;

  xdo_send_keysequence_window(xdo, CURRENTWINDOW, keysym, XDO_KEYSTROKE_DELAY);
}
