#pragma once

#include "shared/serde/protocol.h"

#include <stdbool.h>

typedef struct {
    int fd;
} Daemon;

bool open_daemon_socket(Daemon *daemon);
void close_daemon_socket(Daemon *daemon);

bool send_msg_to_daemon(Daemon *daemon, MsgDeserializer *ser);
