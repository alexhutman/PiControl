#ifndef _PICTRL_COMMON_H
#define _PICTRL_COMMON_H

#include <unistd.h>

// Constants
#define SERVER_PORT      14741
#define MAX_BUF          4096

// Only 1 client will be connected (i.e. sending commands) to the server at once, but we can tell
// the others to get lost if there is an existing open connection.
#define MAX_CONNS        1 

// Timeout in seconds - if we haven't received a heartbeat or command in this amount of time, disconnect
#define TIMEOUT_SECS     5


// Types
typedef enum {
    PI_CTRL_HEARTBEAT,  // Client: Send heartbeat so server can disconnect if connection is lost

    PI_CTRL_MOUSE_MV,   // Client: Send x,y of relative position to move mouse to
    PI_CTRL_MOUSE_DOWN, // Client: Say to press mouse down (no data required)
    PI_CTRL_MOUSE_UP,   // Client: Say to press mouse up (no data required)
    PI_CTRL_MOUSE_CLK,  // Client: Say to click (mouseup, then mousedown) mouse (no data required)
    PI_CTRL_TEXT,       // Client: Send UTF-8 bytes to be typed
    PI_CTRL_KEYSYM,     // Client: Send keysym (combination)
} PiCtrlCmd;

typedef struct {
    PiCtrlCmd command;
    size_t payload_size;
} PiCtrlHeader;

typedef struct {
    int x,y;
} PiCtrlMouseCoord;

#endif
