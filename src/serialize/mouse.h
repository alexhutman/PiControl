#ifndef _PICTRL_SERIALIZE_MOUSE_H
#define _PICTRL_SERIALIZE_MOUSE_H

#include <stdint.h>

#include "data_structures/ring_buffer.h"
#include "model/mouse.h"

// Assumes it's the first item in the ring buffer
//
// All bytes are unsigned
// -------------------------
// | MOUSE_STATUS (1 byte) |
// -------------------------
//
// xxxxxxAB <-- MOUSE_STATUS
// Bit A: PiCtrlMouseBtn
// Bit B: PiCtrlMouseClick
static inline PiCtrlMouseBtnStatus pictrl_rb_get_mouse_status(pictrl_rb_t *rb) {
  uint8_t byte = pictrl_rb_get(rb, 0);
  const PiCtrlMouseBtnStatus ret = {
      .btn = byte & (1 << 1),
      .click = byte & (1 << 0),
  };

  const size_t new_data_start_idx = (rb->data_start + 1) % rb->capacity;
  rb->data_start = new_data_start_idx;

  rb->num_items -= 1;
  return ret;
}

// Assumes they're the first 2 items in the ring buffer
// This should probably only be used for relative coordinates due to the
// signedness
//
// All bytes are signed
// ---------------------------
// | X (1 byte) | Y (1 byte) |
// ---------------------------
static inline PiCtrlMouseCoord pictrl_rb_get_mouse_coords(pictrl_rb_t *rb) {
  const PiCtrlMouseCoord ret = {.x = (int8_t)pictrl_rb_get(rb, 0),
                                .y = (int8_t)pictrl_rb_get(rb, 1)};

  const size_t new_data_start_idx = (rb->data_start + 2) % rb->capacity;
  rb->data_start = new_data_start_idx;

  rb->num_items -= 2;
  return ret;
}
#endif
