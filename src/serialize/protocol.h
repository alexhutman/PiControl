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
    Message msg;
    uint8_t payload_buf_size;
  } out;
} MsgDeserializer;

int pictrl_initialize_deserializer(MsgDeserializer *des);
int pictrl_destroy_deserializer(MsgDeserializer *des);

int pictrl_deserialize_network_data(MsgDeserializer *des);
