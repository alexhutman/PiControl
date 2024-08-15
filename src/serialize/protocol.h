#ifndef _PICTRL_SERIALIZE_PROTOCOL_H
#define _PICTRL_SERIALIZE_PROTOCOL_H

#include <stdint.h>

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
RawPiCtrlMessage parse_to_pictrl_msg(void *in, size_t len);
#endif
