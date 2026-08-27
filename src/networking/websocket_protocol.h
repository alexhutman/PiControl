#pragma once

#include "data_structures/multithread_pool.h"
#include "data_structures/multithread_queue.h"
#include "keyboard/virtual_keyboard.h"
#include "serialize/protocol.h"

#include <libwebsockets.h>
#include <uv.h>

#define MAX_CLIENT_IP_SIZE (46)

typedef struct {
  pictrl_pool_t deserializer_pool;
  pictrl_queue_t queue;
  uv_thread_t writer_thread;
  pictrl_keyboard *keyboard;
} pictrl_app_runtime_t;

typedef struct {
  char client_ip[MAX_CLIENT_IP_SIZE];
  PiCtrlMsgDeserializer *cur_deserializer;
} SessionData;

extern const struct lws_protocols protocols[];
void keyboard_writer_thread(void *arg);
