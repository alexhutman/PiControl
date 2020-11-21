#ifndef _PICONTROL_COMMON_H
#define _PICONTROL_COMMON_H

// Includes
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>



// Constants
#define SERVER_PORT 14741
#define MAX_BUF     4096
#define MAX_CONNS   5 
// Only 1 client will be connected (i.e. sending commands) to the server at once, but we can tell
// the others to get lost if there is an existing open connection.


// Types
typedef enum {
	PI_CTRL_ACK,        // Server: You are now connected - continue
	PI_CTRL_BUSY,       // Server: Someone is already connected - you must disconnect

	PI_CTRL_SET_NAME,   // Client: Send current name so server can say who is currently connected on a PI_CTRL_BUSY
	PI_CTRL_MOUSE_MV,   // Client: Send x,y of relative position to move mouse to
	PI_CTRL_MOUSE_DOWN, // Client: Say to press mouse down (no data required)
	PI_CTRL_MOUSE_UP,   // Client: Say to press mouse up (no data required)
	PI_CTRL_MOUSE_CLK,  // Client: Say to click (mouseup, then mousedown) mouse (no data required)
	PI_CTRL_KEY_PRESS,  // Client: Send UTF-8 value of key to be pressed (details TBD)
} PiControlCmd;



#endif
