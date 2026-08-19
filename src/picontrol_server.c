#include "data_structures/multithread_pool.h"
#include "data_structures/multithread_queue.h"
#include "networking/websocket_protocol.h"
#include "serialize/protocol.h"
#include "picontrol_config.h"

#include <libwebsockets.h>
#include <uv.h>

#include <assert.h>

#define DESERIALIZER_POOL_SIZE ((size_t)5)

int main() {
  int logs = LLL_USER | LLL_ERR | LLL_WARN | LLL_NOTICE;
  lws_set_log_level(logs, NULL);

  pictrl_app_runtime_t state = {0};
  pictrl_pool_init(&state.deserializer_pool, DESERIALIZER_POOL_SIZE, sizeof(PiCtrlMsgDeserializer));
  pictrl_queue_init(&state.queue, DESERIALIZER_POOL_SIZE, sizeof(PiCtrlMsgDeserializer *));
  for (size_t idx = 0; idx < state.deserializer_pool.top; idx++) {
      // Unsafe since the pool is unlocked but we aren't using it yet
      initialize_deserializer(state.deserializer_pool.pool[idx]);
  }

  struct lws_context *ws_context = NULL;
  struct lws_context_creation_info info = {
      .port = SERVER_PORT,
      .protocols = protocols,
      .options = LWS_SERVER_OPTION_FALLBACK_TO_APPLY_LISTEN_ACCEPT_CONFIG
               | LWS_SERVER_OPTION_LIBUV,
      .gid = -1,
      .uid = -1,
      .pcontext = &ws_context,
      .user = &state
  };

  state.backend = pictrl_backend_new();
  if (!state.backend) {
    lwsl_err("Unable to create PiControl backend!\n");
    return 1;
  }
  lwsl_user("Using %s backend\n",
            pictrl_backend_name(state.backend->type));

  ws_context = lws_create_context(&info);
  if (!ws_context) {
    lwsl_err("Failed to create LWS context\n");
    return 1;
  }

  uv_thread_create(&state.writer_thread, &keyboard_writer_thread, &state);

  lws_service(ws_context, 0);

  const PiCtrlMsgDeserializer *poison_pill = NULL;
  pictrl_queue_push(&state.queue, poison_pill); // Stop queue from waiting for work
  uv_thread_join(&state.writer_thread);

  pictrl_queue_destroy(&state.queue);
  assert(state.deserializer_pool.top == state.deserializer_pool.capacity && "Not all deserializers were put back");
  for (size_t idx = 0; idx < state.deserializer_pool.top; idx++) {
      // Unsafe since the pool is unlocked but we aren't using it anymore
      destroy_deserializer(state.deserializer_pool.pool[idx]);
  }
  pictrl_pool_destroy(&state.deserializer_pool);
  pictrl_backend_free(state.backend);
  if (ws_context) lws_context_destroy(ws_context);
  return 0;
}
