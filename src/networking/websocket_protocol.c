#include "networking/websocket_protocol.h"

#include <libwebsockets.h>
#include <stddef.h>

#include "backend/picontrol_backend.h"
#include "model/protocol.h"
#include "networking/iputils.h"
#include "picontrol_config.h"
#include "serialize/protocol.h"

typedef struct {
  pictrl_backend *backend;
  RawPiCtrlMessage msg;
} PiContext;

static int handle_message(PiContext *pictx) {
  // Handle command
  switch (pictx->msg.header.cmd) {
    case PI_CTRL_MOUSE_MV:
      handle_mouse_move(pictx->backend, &pictx->msg);
      break;
    case PI_CTRL_MOUSE_CLICK:
      handle_mouse_click(pictx->backend, &pictx->msg);
      break;
    case PI_CTRL_TEXT:
      handle_text(pictx->backend, &pictx->msg);
      break;
    case PI_CTRL_KEYSYM:
      handle_keysym(pictx->backend, &pictx->msg);
      break;
    // TODO: On disconnect command, return 0?
    default:
      lwsl_err("Invalid command: %d.\n", pictx->msg.header.cmd);
      return -1;
  }

  return 0;
}

// https://github.com/warmcat/libwebsockets/blob/main/minimal-examples-lowlevel/raw/minimal-raw-audio/audio.c
int callback_picontrol(struct lws *wsi, enum lws_callback_reasons reason,
                       void *user, void *in, size_t len) {
  (void)user;
  PiContext *pictx = (PiContext *)lws_protocol_vh_priv_get(
      lws_get_vhost(wsi), lws_get_protocol(wsi));

  switch (reason) {
    case LWS_CALLBACK_PROTOCOL_INIT:
      lwsl_notice("LWS_CALLBACK_PROTOCOL_INIT\n");
      pictx = lws_protocol_vh_priv_zalloc(
          lws_get_vhost(wsi), lws_get_protocol(wsi), sizeof(*pictx));
      // Create backend
      pictx->backend = pictrl_backend_new();
      if (pictx->backend == NULL) {
        lwsl_err("Unable to create PiControl backend!\n");
        return -1;
      }
      lwsl_user("Using %s backend\n",
                pictrl_backend_name(pictx->backend->type));

      // Get our IP
      char *ip = get_ip_address();
      if (ip == NULL) {
        return 1;
      }
      lwsl_user("Connect at: %s:%d\n", ip, SERVER_PORT);
      free(ip);
      break;
    case LWS_CALLBACK_RAW_ADOPT:
      lwsl_notice("LWS_CALLBACK_RAW_ADOPT (%zu)\n", len);
      break;
    case LWS_CALLBACK_RECEIVE:
      // Surely sizeof(uint8_t) == sizeof(char) always... right?
      pictx->msg = parse_to_pictrl_msg(in, len);
      handle_message(pictx);
      break;
    case LWS_CALLBACK_PROTOCOL_DESTROY:
      lwsl_notice("LWS_CALLBACK_PROTOCOL_DESTROY\n");
      if (pictx->backend != NULL) {
        // TODO: prob some error handling
        lwsl_user("Freeing backend...\n");
        pictrl_backend_free(pictx->backend);
      }
      break;
    default:
      break;
  }

  return 0;
}
