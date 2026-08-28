#include "model/protocol.h"

#include "logging/logger.h"

#include <stdbool.h>

bool pictrl_validate_message(Message *msg) {
  if (!msg)
    return false;
  switch (msg->header.cmd) {
  case PI_CTRL_HEARTBEAT:
    return msg->header.payload_size == 0;
  case PI_CTRL_MOUSE_MV:
    return msg->header.payload_size == 2;
  case PI_CTRL_MOUSE_CLICK:
    return msg->header.payload_size == 1;
  case PI_CTRL_TEXT:
    return msg->header.payload_size > 0;
  case PI_CTRL_KEYSYM:
    return msg->header.payload_size > 0;
  default:
    pictrl_log_debug("Invalid command: %d.\n", msg->header.cmd);
    return false;
  }
}
