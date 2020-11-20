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


#endif
