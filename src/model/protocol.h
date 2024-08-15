#ifndef _PICTRL_MODEL_PROTOCOL_H
#define _PICTRL_MODEL_PROTOCOL_H

#include <stdint.h>
#include <unistd.h>

typedef enum {
  PI_CTRL_HEARTBEAT,  // Client: Send heartbeat so server can disconnect if
                      //         connection is lost
  PI_CTRL_MOUSE_MV,   // Client: Send x,y of relative position to move mouse to
  PI_CTRL_MOUSE_CLICK,  // Client: Say to click (mouseup or mousedown) mouse
  PI_CTRL_TEXT,         // Client: Send UTF-8 bytes to be typed
  PI_CTRL_KEYSYM,       // Client: Send keysym (combination)
} PiCtrlCmd;

typedef struct {
	uint8_t cmd;
	uint8_t payload_size;
} RawPictrlHeader;

typedef struct {
    RawPictrlHeader header;
    uint8_t *payload;
} RawPiCtrlMessage;


#endif
