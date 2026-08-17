#include "networking/websocket_protocol.h"
#include "picontrol_config.h"

#include <libwebsockets.h>

int main() {
  int logs = LLL_USER | LLL_ERR | LLL_WARN | LLL_NOTICE;
  lws_set_log_level(logs, NULL);

  struct lws_context *ws_context = NULL;
  struct lws_context_creation_info info = {
      .port = SERVER_PORT,
      .protocols = protocols,
      .options = LWS_SERVER_OPTION_FALLBACK_TO_APPLY_LISTEN_ACCEPT_CONFIG
               | LWS_SERVER_OPTION_LIBUV,
      .gid = -1,
      .uid = -1,
      .pcontext = &ws_context
  };

  ws_context = lws_create_context(&info);
  if (!ws_context) {
    lwsl_err("Failed to create LWS context\n");
    return 1;
  }

  lws_service(ws_context, 0);

  if (ws_context) {
    lws_context_destroy(ws_context);
  }
  return 0;
}
