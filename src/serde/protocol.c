#include "serde/protocol.h"

#include "logging/logger.h"
#include "model/protocol.h"

#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

int pictrl_initialize_deserializer(MsgDeserializer *des) {
  if (!des)
    return -1;

  des->in.rx_buffered_bytes = 0;
  return 0;
}

int pictrl_deserialize_network_data(MsgDeserializer *des) {
  const uint8_t payload_size = des->in.rx_buffer[1];
  const size_t expected_wire_size =
      sizeof(des->out.msg.header.cmd) + sizeof(des->out.msg.header.payload_size) + payload_size;
  if (des->in.rx_buffered_bytes != expected_wire_size) {
    pictrl_log_warn("Transmission struct size mismatch. Got %zu bytes, expected %zu\n",
                    des->in.rx_buffered_bytes, expected_wire_size);
    return -2;
  }

  size_t offset = 0;

  uint8_t cmd;
  memcpy(&cmd, &des->in.rx_buffer[offset], sizeof(cmd));
  // Handle endianness here if struct winds up containing multi-byte members
  // (e.g. out->some_uint32_t_member = ntohl(some_uint32_t_member);
  des->out.msg.header.cmd = cmd;
  offset += sizeof(des->out.msg.header.cmd);

  des->out.msg.header.payload_size = payload_size;
  offset += sizeof(des->out.msg.header.payload_size);

  memcpy(des->out.msg.payload, &des->in.rx_buffer[offset], payload_size);

  return 0;
}
