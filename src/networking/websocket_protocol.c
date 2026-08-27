#include "networking/websocket_protocol.h"

#include "data_structures/queue.h"
#include "keyboard/virtual_keyboard.h"
#include "logging/logger.h"
#include "model/protocol.h"
#include "networking/iputils.h"
#include "picontrol_config.h"
#include "serialize/protocol.h"

#include <libwebsockets.h>

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static int handle_message(Keyboard *keyboard, Message *msg) {
  // Handle command
  switch (msg->header.cmd) {
  case PI_CTRL_MOUSE_MV:
    pictrl_handle_mouse_move(keyboard, msg);
    break;
  case PI_CTRL_MOUSE_CLICK:
    pictrl_handle_mouse_click(keyboard, msg);
    break;
  case PI_CTRL_TEXT:
    pictrl_handle_text(keyboard, msg);
    break;
  case PI_CTRL_KEYSYM:
    pictrl_handle_keysym(keyboard, msg);
    break;
  // TODO: On disconnect command, return 0?
  default:
    pictrl_log_error("Invalid command: %d.\n", msg->header.cmd);
    return -1;
  }

  return 0;
}

void keyboard_writer_thread(void *arg) {
  Runtime *state = (Runtime *)arg;
  MsgDeserializer *des = NULL;

  while (pictrl_queue_pop(&state->queue, &des)) {
    pictrl_log_debug("[Writer Thread]: Processing queue item @%p\n", des);

    if (pictrl_deserialize_network_data(des) < 0) {
      pictrl_log_error("[Writer Thread]: Couldn't deserialize message\n");
      goto cleanup;
    } else {
      pictrl_log_debug("[Writer Thread]: Deserialized PiControlMsg\n");
    }

    if (!pictrl_validate_message(&des->out.msg)) {
      pictrl_log_error("[Writer Thread]: Invalid message\n");
      goto cleanup;
    } else {
      pictrl_log_debug("[Writer Thread]: Validated PiControlMsg\n");
    }

    if (handle_message(state->keyboard, &des->out.msg) < 0) {
      pictrl_log_error("[Writer Thread]: Couldn't type message\n");
      goto cleanup;
    } else {
      pictrl_log_debug("[Writer Thread]: PiControlMsg typed\n");
    }

  cleanup:
    pictrl_pool_checkin(&state->deserializer_pool, des);
    pictrl_log_debug("[Writer Thread]: Pushed queue item @%p back to pool\n", des);
  }
  pictrl_log_debug("[Writer Thread]: Queue is empty and closed. Exiting\n");
}

static bool initialize_session_data(struct lws *wsi, SessionData *pss) {
  lws_get_peer_simple(wsi, pss->client_ip, sizeof(pss->client_ip));
  return true;
}

static int receive_data(struct lws *wsi, Runtime *state, SessionData *pss, void *in, size_t len) {
  if (lws_is_first_fragment(wsi)) {
    assert(pss->cur_deserializer == NULL && "Current deserializer was not null on first fragment!");
    pss->cur_deserializer = pictrl_pool_checkout(&state->deserializer_pool);
    assert(pss->cur_deserializer != NULL && "Couldn't pull a deserializer from the pool!");
    pictrl_log_debug("Using deserializer @%p\n", pss->cur_deserializer);
    pss->cur_deserializer->in.rx_buffered_bytes = 0;
  }

  MsgDeserializer *des = pss->cur_deserializer;
  assert(des->in.rx_buffer != NULL && "Deserializer's rx_buffer is null");

  if ((des->in.rx_buffered_bytes + len) > MAX_PICTRL_MSG_SIZE) {
    pictrl_log_error(
        "Incoming data exceeds expected boundaries. Dropping connection.\n"); // TODO: Drop
                                                                              // connection?
    return -1;
  }

  // Copy as much as we receive into rx_buffer
  memcpy(&des->in.rx_buffer[des->in.rx_buffered_bytes], in, len);
  des->in.rx_buffered_bytes += len;

  const size_t remaining = lws_remaining_packet_payload(wsi);
  const int is_final = lws_is_final_fragment(wsi);

  if (remaining == 0 && is_final) {
    pictrl_log_debug(
        "Received final fragment. Sending serialized message @%p to typing thread...\n",
        pss->cur_deserializer);
    pictrl_queue_push(&state->queue, &pss->cur_deserializer); // TODO: Check if this failed?
    pss->cur_deserializer = NULL;
  } else {
    pictrl_log_debug("Received fragment slice. Waiting for the remaining %zu pieces...\n",
                     remaining);
  }
  return 0;
}

// https://github.com/warmcat/libwebsockets/blob/main/minimal-examples-lowlevel/raw/minimal-raw-audio/audio.c
int callback_picontrol(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in,
                       size_t len) {
  SessionData *pss = (SessionData *)user;

  switch (reason) {
  case LWS_CALLBACK_PROTOCOL_INIT: {
    // Get our IP
    char *ip = pictrl_get_ip_address();
    if (!ip) {
      return -2;
    }
    pictrl_log_info("Connect at: %s:%d\n", ip, SERVER_PORT);
    free(ip);
    break;
  }
  case LWS_CALLBACK_ESTABLISHED: {
    if (!pss)
      break;
    if (!initialize_session_data(wsi, pss))
      return -1;

    pictrl_log_debug("Initialized session data\n");
    pictrl_log_warn("[CONN] + Established | IP: %s\n", pss->client_ip);
    break;
  }
  case LWS_CALLBACK_RECEIVE: {
    if (!pss || !in || len == 0)
      break;
    Runtime *app_state = (Runtime *)lws_context_user(lws_get_context(wsi));
    receive_data(wsi, app_state, pss, in, len);
    break;
  }
  case LWS_CALLBACK_CLOSED: {
    if (!pss)
      break;
    Runtime *app_state = (Runtime *)lws_context_user(lws_get_context(wsi));
    if (pss->cur_deserializer)
      pictrl_pool_checkin(&app_state->deserializer_pool, pss->cur_deserializer);
    pictrl_log_warn("[CONN] - Disconnected | IP: %s\n", pss->client_ip);
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
        .id = 1 // First iteration of the protocol (ignored by lws)
    },
    LWS_PROTOCOL_LIST_TERM};
