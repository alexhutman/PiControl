#pragma once

#include "shared/model/protocol.h"

#include <stddef.h>
#include <stdint.h>

typedef struct {
  struct {
    uint8_t rx_buffer[MAX_PICTRL_MSG_SIZE];
    size_t rx_buffered_bytes;
  } in;
  struct {
    Message msg;
  } out;
} MsgDeserializer;

int pictrl_initialize_deserializer(MsgDeserializer *des);

int pictrl_serialize_network_data(MsgDeserializer *des);
int pictrl_deserialize_network_data(MsgDeserializer *des);
