#ifndef _PICTRL_CONFIG_H
#define _PICTRL_CONFIG_H

#define SERVER_PORT           14741

/* (in bytes) */
#define MAX_BUF               4096

/*
Only 1 client will be connected (i.e. sending commands) to the server at once,
but we can tell the others to get lost if there is an existing open connection.
*/
#define MAX_CONNS             1

/*
 * (CURRENTLY UNUSED) Timeout in seconds - if we haven't received a heartbeat or
 * command in this amount of time, disconnect
*/
#define TIMEOUT_SECS          5

// Maximum simultaneous keys pressed during a combo. Surely we wouldn't need more than this... right?
#define PICTRL_MAX_SIMUL_KEYS 10

#endif
