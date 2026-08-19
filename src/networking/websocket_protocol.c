#include "networking/websocket_protocol.h"

#include "backend/picontrol_backend.h"
#include "data_structures/multithread_queue.h"
#include "model/protocol.h"
#include "networking/iputils.h"
#include "picontrol_config.h"
#include "serialize/protocol.h"

#include <libwebsockets.h>

#include <assert.h>
#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

sig_atomic_t kb_thread_interrupted = false;

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

void keyboard_writer_thread(void *arg) {
    pictrl_app_runtime_t *state = (pictrl_app_runtime_t *)arg;
    PiCtrlMsgDeserializer *des = NULL;

    while (!kb_thread_interrupted) {
        pictrl_queue_pop(&state->queue, &des); // blocks
        lwsl_debug("[Writer Thread]: Processing queue item @%p\n", des);

        if (deserialize_network_data(des) < 0) {
            lwsl_err("[Writer Thread]: Couldn't deserialize message\n");
            goto cleanup;
        } else {
            lwsl_debug("[Writer Thread]: Deserialized PiControlMsg\n");
        }

        if (!validate_pictrl_message(&des->out.msg)) {
            lwsl_err("[Writer Thread]: Invalid message\n");
            goto cleanup;
        } else {
            lwsl_debug("[Writer Thread]: Validated PiControlMsg\n");
        }

        if (handle_message(state->backend, &des->out.msg) < 0) {
            lwsl_err("[Writer Thread]: Couldn't type message\n");
            goto cleanup;
        } else {
            lwsl_debug("[Writer Thread]: PiControlMsg typed\n");
        }

cleanup:
        pictrl_pool_checkin(&state->deserializer_pool, des);
        lwsl_debug("[Writer Thread]: Pushed queue item @%p back to pool\n", des);
    }
    lwsl_debug("[Writer Thread]: Shutting down\n");
}

static bool initialize_session_data(struct lws *wsi, SessionData *pss) {
    lws_get_peer_simple(wsi, pss->client_ip, sizeof(pss->client_ip));
    return true;
}

static int receive_data(struct lws *wsi, pictrl_app_runtime_t *state, SessionData *pss, void *in, size_t len) {
    if (lws_is_first_fragment(wsi)) {
        assert(pss->cur_deserializer == NULL && "Current deserializer was not null on first fragment!");
        pss->cur_deserializer = pictrl_pool_checkout(&state->deserializer_pool);
        assert(pss->cur_deserializer != NULL && "Couldn't pull a deserializer from the pool!");
        lwsl_debug("Using deserializer @%p\n", pss->cur_deserializer);
        pss->cur_deserializer->in.rx_buffered_bytes = 0;
    }

    PiCtrlMsgDeserializer *des = pss->cur_deserializer;
    assert(des->in.rx_buffer != NULL && "Deserializer's rx_buffer is null");

    if ((des->in.rx_buffered_bytes + len) > MAX_PICTRL_MSG_SIZE) {
        lwsl_err("Incoming data exceeds expected boundaries. Dropping connection.\n"); // TODO: Drop connection?
        return -1;
    }

    // Copy as much as we receive into rx_buffer
    memcpy(&des->in.rx_buffer[des->in.rx_buffered_bytes], in, len);
    des->in.rx_buffered_bytes += len;

    const size_t remaining = lws_remaining_packet_payload(wsi);
    const int is_final = lws_is_final_fragment(wsi);

    if (remaining == 0 && is_final) {
        lwsl_debug("Received final fragment. Sending serialized message @%p to typing thread...\n", pss->cur_deserializer);
        pictrl_queue_push(&state->queue, &pss->cur_deserializer); // TODO: Check if this failed?
        pss->cur_deserializer = NULL;
    } else {
        lwsl_debug("Received fragment slice. Waiting for the remaining %zu pieces...\n", remaining);
    }
    return 0;
}

// https://github.com/warmcat/libwebsockets/blob/main/minimal-examples-lowlevel/raw/minimal-raw-audio/audio.c
int callback_picontrol(struct lws *wsi, enum lws_callback_reasons reason,
                       void *user, void *in, size_t len) {
  SessionData *pss = (SessionData *)user;

  switch (reason) {
    case LWS_CALLBACK_PROTOCOL_INIT: {
      // Get our IP
      char *ip = get_ip_address();
      if (!ip) {
        return -2;
      }
      lwsl_user("Connect at: %s:%d\n", ip, SERVER_PORT);
      free(ip);
      break;
    }
    case LWS_CALLBACK_ESTABLISHED: {
      if (!pss) break;
      if (!initialize_session_data(wsi, pss)) return -1;

      lwsl_debug("Initialized session data\n");
      lwsl_notice("[CONN] + Established | IP: %s\n", pss->client_ip);
      break;
    }
    case LWS_CALLBACK_RECEIVE: {
      if (!pss || !in || len == 0) break;
      pictrl_app_runtime_t *app_state = (pictrl_app_runtime_t *)lws_context_user(lws_get_context(wsi));
      receive_data(wsi, app_state, pss, in, len);
      break;
    }
    case LWS_CALLBACK_CLOSED: {
      if (!pss) break;
      pictrl_app_runtime_t *app_state = (pictrl_app_runtime_t *)lws_context_user(lws_get_context(wsi));
      if (pss->cur_deserializer) pictrl_pool_checkin(&app_state->deserializer_pool, pss->cur_deserializer);
      lwsl_notice("[CONN] - Disconnected | IP: %s\n", pss->client_ip);
      break;
    }
    default:
      break;
  }

  return 0;
}

const struct lws_protocols protocols[] = {
    {
        .name = "picontrol",
        .callback = &callback_picontrol,
        .per_session_data_size = sizeof(SessionData),
        .rx_buffer_size = 0,
        .id = 1  // First iteration of the protocol (ignored by lws)
    },
    LWS_PROTOCOL_LIST_TERM
};
