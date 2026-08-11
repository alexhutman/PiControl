#ifndef _PICTRL_SERIALIZE_PROTOCOL_H
#define _PICTRL_SERIALIZE_PROTOCOL_H

#include "model/protocol.h"

#include <stddef.h>
#include <stdint.h>

#define MAX_PAYLOAD_SIZE (UINT8_MAX)
#define MAX_PICTRL_MSG_SIZE (sizeof(uint8_t) \
                           + sizeof(uint8_t) \
                           + MAX_PAYLOAD_SIZE)

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

int initialize_deserializer(PiCtrlMsgDeserializer *des);
int destroy_deserializer(PiCtrlMsgDeserializer *des);

// All bytes are unsigned
//
// |---------------- HEADER --------------|
// -------------------------------------------------------------------------
// | CMD (1 byte) | PAYLOAD_SIZE (1 byte) | PAYLOAD (MAX: UINT8_MAX bytes) |
// -------------------------------------------------------------------------
int deserialize_network_data(PiCtrlMsgDeserializer *des);
#endif
