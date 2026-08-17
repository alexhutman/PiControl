#pragma once

#include "backend/picontrol_backend.h"
#include "serialize/protocol.h"

#include <libwebsockets.h>

#include <stdbool.h>

typedef struct {
    bool init_failed;
} pictrl_app_runtime_t;

typedef struct {
  pictrl_backend *backend;
} PerVHostData;

typedef struct {
  PiCtrlMsgDeserializer des;
  char client_ip[46];
} SessionData;

extern const struct lws_protocols protocols[];
lws_callback_function callback_picontrol;
