#pragma once

#include "data_structures/pool.h"
#include "data_structures/queue.h"
#include "keyboard/virtual_keyboard.h"
#include "serialize/protocol.h"

#include <libwebsockets.h>
#include <uv.h>

#define MAX_CLIENT_IP_SIZE (46)

typedef struct {
  Pool deserializer_pool;
  Queue deserializer_queue;
  uv_thread_t writer_thread;
  Keyboard *keyboard;
} Runtime;

typedef struct {
  char client_ip[MAX_CLIENT_IP_SIZE];
  MsgDeserializer *cur_deserializer;
} SessionData;

extern const struct lws_protocols protocols[];
void keyboard_writer_thread(void *arg);
