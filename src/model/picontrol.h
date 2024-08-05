#ifndef _PICTRL_MODEL_H
#define _PICTRL_MODEL_H

#include <unistd.h>


// Types
typedef enum {
    PI_CTRL_HEARTBEAT,   // Client: Send heartbeat so server can disconnect if connection is lost

    PI_CTRL_MOUSE_MV,    // Client: Send x,y of relative position to move mouse to
    PI_CTRL_MOUSE_CLICK, // Client: Say to click (mouseup or mousedown) mouse (left or right button)
                             //   This is 1 byte, where 00000021 the 2 == PiCtrlMouseBtn and the 1 == PiCtrlMouseClick
    PI_CTRL_TEXT,        // Client: Send UTF-8 bytes to be typed
    PI_CTRL_KEYSYM,      // Client: Send keysym (combination)
} PiCtrlCmd;

typedef struct {
    PiCtrlCmd command;
    size_t payload_size;
} PiCtrlHeader;

typedef enum {
    PI_CTRL_MOUSE_LEFT = 0,
    PI_CTRL_MOUSE_RIGHT = 1
} PiCtrlMouseBtn;

typedef enum {
    PI_CTRL_MOUSE_UP = 0,
    PI_CTRL_MOUSE_DOWN = 1,
} PiCtrlMouseClick;

typedef struct {
    PiCtrlMouseBtn btn;
    PiCtrlMouseClick click;
} PiCtrlMouseBtnStatus;

typedef struct {
    int x,y;
} PiCtrlMouseCoord;

#endif
