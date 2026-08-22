#pragma once

#include "model/protocol.h"

#include <stddef.h>
#include <stdint.h>

typedef struct {
  struct {
      uint8_t *rx_buffer;
      size_t rx_buffered_bytes;
  } in;
  struct {
      RawPiCtrlMessage msg;
      uint8_t payload_buf_size;
  } out;
} PiCtrlMsgDeserializer;

int pictrl_initialize_deserializer(PiCtrlMsgDeserializer *des);
int pictrl_destroy_deserializer(PiCtrlMsgDeserializer *des);

// All bytes are unsigned
//
// |---------------- HEADER --------------|
// -------------------------------------------------------------------------
// | CMD (1 byte) | PAYLOAD_SIZE (1 byte) | PAYLOAD (MAX: UINT8_MAX bytes) |
// -------------------------------------------------------------------------
int pictrl_deserialize_network_data(PiCtrlMsgDeserializer *des);
