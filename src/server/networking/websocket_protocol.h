#pragma once

#include "ipc/daemon.h"
#include "shared/data_structures/pool.h"
#include "shared/data_structures/queue.h"

#include <libwebsockets.h>
#include <uv.h>

#define MAX_CLIENT_IP_SIZE (46)

typedef struct {
  Pool deserializer_pool;
  Queue deserializer_queue;
  Daemon daemon;
  uv_thread_t writer_thread;
} Runtime;

extern const struct lws_protocols protocols[];
void keyboard_writer_thread(void *arg);
