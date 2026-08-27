#include "data_structures/multithread_pool.h"
#include "data_structures/multithread_queue.h"
#include "keyboard/virtual_keyboard.h"
#include "logging/logger.h"
#include "networking/websocket_protocol.h"
#include "picontrol_config.h"
#include "serialize/protocol.h"

#include <libwebsockets.h>
#include <uv.h>

#include <assert.h>
#include <stdio.h>

#define DESERIALIZER_POOL_SIZE ((size_t)5)
#ifdef PICTRL_XDO
  #define KEYBOARD_BACKEND PICTRL_BACKEND_XDO
#else
  #define KEYBOARD_BACKEND PICTRL_BACKEND_UINPUT
#endif // PICTRL_XDO

static int init_deserializer(void *item, void *user_data) {
  (void)user_data;
  return pictrl_initialize_deserializer((MsgDeserializer *)item);
}

static int destroy_deserializer(void *item, void *user_data) {
  (void)user_data;
  return pictrl_destroy_deserializer((MsgDeserializer *)item);
}

static bool initialize_state(Runtime *state) {
  void *usr_data = NULL;
  const PoolOpts pool_opts = {DESERIALIZER_POOL_SIZE, sizeof(MsgDeserializer), &init_deserializer,
                              &destroy_deserializer, usr_data};

  if (!pictrl_pool_init(&state->deserializer_pool, &pool_opts)) {
    pictrl_log_error("Unable to create deserializer pool\n");
    return false;
  }
  if (!pictrl_queue_init(&state->queue, DESERIALIZER_POOL_SIZE, sizeof(MsgDeserializer *))) {
    pictrl_log_error("Unable to create worker thread's deserializer queue\n");
    pictrl_pool_destroy(&state->deserializer_pool);
    return false;
  }

  state->keyboard = pictrl_keyboard_new(KEYBOARD_BACKEND);
  if (!state->keyboard) {
    pictrl_log_error("Unable to create PiControl keyboard!\n");
    pictrl_queue_destroy(&state->queue);
    pictrl_pool_destroy(&state->deserializer_pool);
    return false;
  }
  pictrl_log_debug("Initialized app state\n");
  return true;
}

static void clean_up_state(Runtime *state) {
  pictrl_queue_close(&state->queue);
  uv_thread_join(&state->writer_thread);

  pictrl_keyboard_free(state->keyboard);
  pictrl_queue_destroy(&state->queue);
  assert(state->deserializer_pool.top == state->deserializer_pool.capacity &&
         "Not all deserializers were put back");
  pictrl_pool_destroy(&state->deserializer_pool);
  pictrl_log_debug("Destroyed app state\n");
}

int main() {
  if (!pictrl_logger_init()) {
    fprintf(stderr, "Could not initialize logger!\n");
    return 1;
  }
  lws_set_log_level(0, NULL);

  Runtime state = {0};
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
