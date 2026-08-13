#include "networking/websocket_protocol.h"

#include "backend/picontrol_backend.h"
#include "logging/log_utils.h"
#include "model/protocol.h"
#include "networking/iputils.h"
#include "picontrol_config.h"
#include "serialize/protocol.h"

#include <libwebsockets.h>

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

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
      lwsl_err("Invalid command: %d.\n", msg->header.cmd);
      return -1;
  }

  return 0;
}

static PerVHostData* initialize_vhost_data(struct lws *wsi) {
    PerVHostData *vhd = lws_protocol_vh_priv_zalloc(lws_get_vhost(wsi),
                                                    lws_get_protocol(wsi),
                                                    sizeof(*vhd));
    if (!vhd) return NULL;

    vhd->backend = pictrl_backend_new();
    if (!vhd->backend) {
      lwsl_err("Unable to create PiControl backend!\n");
      return NULL;
    }
    lwsl_user("Using %s backend\n",
              pictrl_backend_name(vhd->backend->type));
    return vhd;
}

static int initialize_session_data(SessionData *pss) {
    return initialize_deserializer(&pss->des);
}

static int destroy_session_data(SessionData *pss) {
    return destroy_deserializer(&pss->des);
}

static int destroy_vhost_data(PerVHostData *vhd) {
    if (!vhd) return -1;
    if (vhd->backend) {
      // TODO: prob some error handling
      pictrl_log_debug("Freeing backend...\n");
      pictrl_backend_free(vhd->backend);
      vhd->backend = NULL;
    }
    return 0;
}

static int receive_data(struct lws *wsi, pictrl_backend *backend, PiCtrlMsgDeserializer *des, void *in, size_t len) {
    if (!des) {
        lwsl_err("Per-session user struct is null\n");
        return -1;
    }
    if (!des->in.rx_buffer) {
        lwsl_err("Per-session rx_buffer is null\n");
        return -2;
    }
    if ((des->in.rx_buffered_bytes + len) > MAX_PICTRL_MSG_SIZE) {
        lwsl_err("Incoming data exceeds expected boundaries. Dropping connection.\n");
        return -3;
    }

    if (lws_is_first_fragment(wsi)) {
        des->in.rx_buffered_bytes = 0;
    }

    // Copy as much as we receive into rx_buffer
    memcpy(&des->in.rx_buffer[des->in.rx_buffered_bytes], in, len);
    des->in.rx_buffered_bytes += len;

    const size_t remaining = lws_remaining_packet_payload(wsi);
    const int is_final = lws_is_final_fragment(wsi);

    if (remaining == 0 && is_final) {
        // End of transmission
        if (deserialize_network_data(des) < 0) return -4;
        pictrl_log_debug("Deserialized PiControlMsg\n");

        if (!validate_pictrl_message(&des->out.msg)) return -5;
        pictrl_log_debug("Validated PiControlMsg\n");

        if (handle_message(backend, &des->out.msg) < 0) return -6;
        pictrl_log_debug("PiControlMsg handled\n");
    } else {
        pictrl_log_debug("Received fragment slice. Waiting for the remaining %zu pieces...\n", remaining);
    }
    return 0;
}

// https://github.com/warmcat/libwebsockets/blob/main/minimal-examples-lowlevel/raw/minimal-raw-audio/audio.c
int callback_picontrol(struct lws *wsi, enum lws_callback_reasons reason,
                       void *user, void *in, size_t len) {
  SessionData *pss = (SessionData *)user;
  PerVHostData *vhd = (PerVHostData *)lws_protocol_vh_priv_get(
      lws_get_vhost(wsi), lws_get_protocol(wsi));

  switch (reason) {
    case LWS_CALLBACK_PROTOCOL_INIT:
      vhd = initialize_vhost_data(wsi);
      if (!vhd) return -1;
      pictrl_log_debug("Initialized vhost data\n");

      // Get our IP
      char *ip = get_ip_address();
      if (!ip) {
        return -2;
      }
      lwsl_user("Connect at: %s:%d\n", ip, SERVER_PORT);
      free(ip);
      break;
    case LWS_CALLBACK_ESTABLISHED: {
      if (!pss) break;
      lws_get_peer_simple(wsi, pss->client_ip, sizeof(pss->client_ip));
      lwsl_notice("[CONN] + Established | IP: %s\n", pss->client_ip);

      int ret = initialize_session_data(pss);
      if (ret < 0) return ret;
      pictrl_log_debug("Initialized session data\n");
      break;
    }
    case LWS_CALLBACK_RECEIVE:
      if (!pss) break;
      receive_data(wsi, vhd->backend, &pss->des, in, len);
      break;
    case LWS_CALLBACK_CLOSED:
      if (!pss) break;
      destroy_session_data(pss);
      pictrl_log_debug("Destroyed session data\n");
      lwsl_notice("[CONN] - Disconnected | IP: %s\n", pss->client_ip);
      break;
    case LWS_CALLBACK_PROTOCOL_DESTROY:
      destroy_vhost_data(vhd);
      pictrl_log_debug("Destroyed vhost data\n");
      break;
    default:
      break;
  }

  return 0;
}
