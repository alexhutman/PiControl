#pragma once

#include <stdbool.h>
#include <stdint.h>

#define MAX_PAYLOAD_SIZE    (UINT8_MAX)
#define MAX_PICTRL_MSG_SIZE (sizeof(uint8_t) + sizeof(uint8_t) + MAX_PAYLOAD_SIZE)

typedef enum {
  PI_CTRL_HEARTBEAT,   // Client: Send heartbeat so server can disconnect if
                       //         connection is lost
  PI_CTRL_MOUSE_MV,    // Client: Send x,y of relative position to move mouse to
  PI_CTRL_MOUSE_CLICK, // Client: Say to click (mouseup or mousedown) mouse
  PI_CTRL_TEXT,        // Client: Send UTF-8 bytes to be typed
  PI_CTRL_KEYSYM,      // Client: Send keysym (combination)
} Cmd;

typedef struct {
  uint8_t cmd;
  uint8_t payload_size;
} Header;

typedef struct {
  Header header;
  uint8_t *payload;
} Message;

bool pictrl_validate_message(Message *msg);
