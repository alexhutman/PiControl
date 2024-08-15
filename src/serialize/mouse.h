#ifndef _PICTRL_SERIALIZE_MOUSE_H
#define _PICTRL_SERIALIZE_MOUSE_H

#include <stdint.h>

#include "model/mouse.h"
#include "model/protocol.h"

// All bytes are unsigned
// -------------------------
// | MOUSE_STATUS (1 byte) |
// -------------------------
//
// xxxxxxAB <-- MOUSE_STATUS
// Bit A: PiCtrlMouseBtn
// Bit B: PiCtrlMouseClick
static inline PiCtrlMouseBtnStatus pictrl_get_mouse_status(const RawPiCtrlMessage *msg) {
  uint8_t byte = *msg->payload;
  const PiCtrlMouseBtnStatus ret = {
      .btn = byte & (1 << 1),
      .click = byte & (1 << 0),
  };
  return ret;
}

// This should probably only be used for relative coordinates due to the
// signedness
//
// All bytes are signed
// ---------------------------
// | X (1 byte) | Y (1 byte) |
// ---------------------------
static inline PiCtrlMouseCoord pictrl_get_mouse_coords(const RawPiCtrlMessage *msg) {
  const PiCtrlMouseCoord ret = {.x = *(int8_t *)msg->payload,
                                .y = *(int8_t *)(msg->payload+1)};
  return ret;
}
#endif
