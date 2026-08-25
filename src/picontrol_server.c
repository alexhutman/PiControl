#include "data_structures/multithread_pool.h"
#include "data_structures/multithread_queue.h"
#include "logging/logger.h"
#include "networking/websocket_protocol.h"
#include "picontrol_config.h"
#include "serialize/protocol.h"

#include <libwebsockets.h>
#include <uv.h>

#include <assert.h>
#include <stdio.h>

#define DESERIALIZER_POOL_SIZE ((size_t)5)

static int init_deserializer(void *item, void *user_data) {
  (void)user_data;
  return pictrl_initialize_deserializer((PiCtrlMsgDeserializer *)item);
}

static int destroy_deserializer(void *item, void *user_data) {
  (void)user_data;
  return pictrl_destroy_deserializer((PiCtrlMsgDeserializer *)item);
}

static bool initialize_state(pictrl_app_runtime_t *state) {
  void *usr_data = NULL;
  const pictrl_pool_opts pool_opts = {DESERIALIZER_POOL_SIZE, sizeof(PiCtrlMsgDeserializer),
                                      &init_deserializer, &destroy_deserializer, usr_data};

  if (!pictrl_pool_init(&state->deserializer_pool, &pool_opts)) {
    pictrl_log_error("Unable to create deserializer pool\n");
    return false;
  }
  if (!pictrl_queue_init(&state->queue, DESERIALIZER_POOL_SIZE, sizeof(PiCtrlMsgDeserializer *))) {
    pictrl_log_error("Unable to create worker thread's deserializer queue\n");
    pictrl_pool_destroy(&state->deserializer_pool);
    return false;
  }

  state->backend = pictrl_backend_new();
  if (!state->backend) {
    pictrl_log_error("Unable to create PiControl backend!\n");
    pictrl_queue_destroy(&state->queue);
    pictrl_pool_destroy(&state->deserializer_pool);
    return false;
  }
  pictrl_log_info("Initialized app state. Using %s backend\n",
                  pictrl_backend_name(state->backend->type));
  return true;
}

static void clean_up_state(pictrl_app_runtime_t *state) {
  pictrl_queue_close(&state->queue);
  uv_thread_join(&state->writer_thread);

  pictrl_backend_free(state->backend);
  pictrl_queue_destroy(&state->queue);
  assert(state->deserializer_pool.top == state->deserializer_pool.capacity &&
         "Not all deserializers were put back");
  pictrl_pool_destroy(&state->deserializer_pool);
  pictrl_log_info("Destroyed app state\n");
}

int main() {
  if (!pictrl_logger_init()) {
    fprintf(stderr, "Could not initialize logger!\n");
    return 1;
  }
  lws_set_log_level(0, NULL);

  pictrl_app_runtime_t state = {0};
  if (!initialize_state(&state)) {
    pictrl_log_critical("Failed to initialize app state\n");
    pictrl_logger_destroy();
    return 1;
  }

  struct lws_context *ws_context = NULL;
  struct lws_context_creation_info info = {
      .port = SERVER_PORT,
      .protocols = protocols,
      .options = LWS_SERVER_OPTION_FALLBACK_TO_APPLY_LISTEN_ACCEPT_CONFIG | LWS_SERVER_OPTION_LIBUV,
      .gid = -1,
      .uid = -1,
      .pcontext = &ws_context,
      .user = &state};

  ws_context = lws_create_context(&info);
  if (!ws_context) {
    pictrl_log_error("Failed to create LWS context\n");
    clean_up_state(&state);
    pictrl_logger_destroy();
    return 1;
  }

  uv_thread_create(&state.writer_thread, &keyboard_writer_thread, &state);

  lws_service(ws_context, 0);

  clean_up_state(&state);
  pictrl_logger_destroy();
  return 0;
}
