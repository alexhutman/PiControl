#pragma once

#include "data_structures/pool.h"
#include "data_structures/queue.h"
#include "keyboard/virtual_keyboard.h"
#include "serde/protocol.h"

#include <libwebsockets.h>
#include <uv.h>

#define MAX_CLIENT_IP_SIZE (46)

typedef struct {
  Pool deserializer_pool;
  Queue deserializer_queue;
  uv_thread_t writer_thread;
  Keyboard *keyboard;
} Runtime;

extern const struct lws_protocols protocols[];
void keyboard_writer_thread(void *arg);
