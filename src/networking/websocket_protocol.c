#include <stddef.h>

#include <libwebsockets.h>

#include "backend/picontrol_backend.h"
#include "logging/log_utils.h"
#include "networking/websocket_protocol.h"
#include "serialize/protocol.h"

typedef struct {
    pictrl_backend *backend;
} PiContext;


static int handle_message(pictrl_backend *backend, RawPiCtrlMessage *msg) {
  // Handle command
  switch (msg->header.cmd) {
    case PI_CTRL_MOUSE_MV:
      handle_mouse_move(backend, msg);
      break;
    case PI_CTRL_MOUSE_CLICK:
      handle_mouse_click(backend, msg);
      break;
    case PI_CTRL_TEXT:
      handle_text(backend, msg);
      break;
    case PI_CTRL_KEYSYM:
      handle_keysym(backend, msg);
      break;
    // TODO: On disconnect command, return 0?
    default:
      pictrl_log_error("Invalid command: %d.\n", msg->header.cmd);
  }

  return 0;
}

// https://github.com/warmcat/libwebsockets/blob/main/minimal-examples-lowlevel/raw/minimal-raw-audio/audio.c
int callback_picontrol(struct lws *wsi, enum lws_callback_reasons reason,
        void *user, void *in, size_t len) {
    PiContext *pictx = (PiContext *)lws_protocol_vh_priv_get(lws_get_vhost(wsi), lws_get_protocol(wsi));

    switch (reason) {
        case LWS_CALLBACK_PROTOCOL_INIT:
            lwsl_notice("LWS_CALLBACK_PROTOCOL_INIT\n");
            pictx = lws_protocol_vh_priv_zalloc(lws_get_vhost(wsi),
				lws_get_protocol(wsi), sizeof(*pictx));
            // Create backend
            pictx->backend = pictrl_backend_new();
            if (pictx->backend == NULL) {
                pictrl_log_error("Unable to create PiControl backend!\n");
                return -1;
            }
            pictrl_log_debug("Using %s backend\n", pictrl_backend_name(pictx->backend->type));
            break;
        case LWS_CALLBACK_RAW_RX:
            // Surely sizeof(uint8_t) == sizeof(char) always... right?
            lwsl_notice("LWS_CALLBACK_RAW_RX (%zu)\n", len);
            RawPiCtrlMessage msg = parse_to_pictrl_msg(in, len);
            handle_message(pictx->backend, &msg);
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
