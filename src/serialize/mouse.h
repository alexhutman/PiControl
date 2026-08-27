#pragma once

#include "model/mouse.h"
#include "model/protocol.h"

#include <stdint.h>

// All bytes are unsigned
// -------------------------
// | MOUSE_STATUS (1 byte) |
// -------------------------
//
// xxxxxxAB <-- MOUSE_STATUS
// Bit A: MouseButton
// Bit B: MouseClick
static inline MouseBtnStatus pictrl_get_mouse_status(const Message *msg) {
  uint8_t byte = *msg->payload;
  const MouseBtnStatus ret = {
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
static inline MouseCoord pictrl_get_mouse_coords(const Message *msg) {
  const MouseCoord ret = {.x = *(int8_t *)msg->payload, .y = *(int8_t *)(msg->payload + 1)};
  return ret;
}
