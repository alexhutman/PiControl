#ifndef _PICTRL_NETWORK_WS_H
#define _PICTRL_NETWORK_WS_H

#include "backend/picontrol_backend.h"
#include "model/protocol.h"

#include <libwebsockets.h>

typedef struct {
  pictrl_backend *backend;
} PerVHostData;

typedef struct {
  RawPiCtrlMessage msg;
} SessionData;

lws_callback_function callback_picontrol;

#endif
