#ifndef _PICTRL_SERIALIZE_PROTOCOL_H
#define _PICTRL_SERIALIZE_PROTOCOL_H

#include <stdint.h>

#include "data_structures/ring_buffer.h"
#include "model/protocol.h"

// Assumes that rb->data_start is pointing at the beginning of the header in the
// ring buffer already
//
// All bytes are unsigned
//
// |---------------- HEADER --------------|
// --------------------------------------------------
// | CMD (1 byte) | PAYLOAD_SIZE (1 byte) | PAYLOAD |
// --------------------------------------------------
static inline PiCtrlHeader pictrl_rb_get_header(pictrl_rb_t *rb) {
  const PiCtrlHeader ret = {.command = (PiCtrlCmd)pictrl_rb_get(rb, 0),
                            .payload_size = (size_t)pictrl_rb_get(rb, 1)};

  const size_t new_data_start_idx = (rb->data_start + 2) % rb->capacity;
  rb->data_start = new_data_start_idx;

  rb->num_items -= 2;
  return ret;
}
#endif
