#include "model/protocol.h"

#include "logging/log_utils.h"

RawPiCtrlMessage parse_to_pictrl_msg(void *in, size_t len) {
  RawPictrlHeader header = *(RawPictrlHeader *)in;
  const size_t expected_msg_len = (sizeof(header) + header.payload_size);
  if (len != expected_msg_len) {
    pictrl_log_error("Shit. Expected %zu bytes\n", expected_msg_len);
    // Probably have to somehow fill out the structure as we go,
    // then act once we have a full one.
  }
  RawPiCtrlMessage msg = {.header = header, .payload = in + sizeof(header)};
  return msg;
}
