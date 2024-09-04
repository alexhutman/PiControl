#include <libwebsockets.h>
#include <signal.h>

#include "backend/picontrol_backend.h"
#include "networking/websocket_protocol.h"

static int picontrol_listen(struct lws_context *context);

volatile sig_atomic_t should_exit = false;

const struct lws_protocols protocols[] = {
    {
        .name = "picontrol",
        .callback = &callback_picontrol,
        .per_session_data_size = 0,
        .rx_buffer_size = 0,
        .id = 1  // First iteration of the protocol (ignored by lws)
    },
    LWS_PROTOCOL_LIST_TERM};

int main() {
  int logs = LLL_USER | LLL_ERR | LLL_WARN | LLL_NOTICE;
  lws_set_log_level(logs, NULL);

  const struct lws_context_creation_info info = {
      .port = SERVER_PORT,
      .protocols = protocols,
      .options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT,
      .ssl_cert_filepath = "certs/picontrol.pem",
      .ssl_private_key_filepath = "certs/picontrol.key",
      .gid = -1,
      .uid = -1,
  };
  struct lws_context *ws_context = lws_create_context(&info);
  if (ws_context == NULL) {
    lwsl_err("lws init failed\n");
    return 1;
  }

  int ret = picontrol_listen(ws_context);

  lws_context_destroy(ws_context);
  return ret;
}

void interrupt_handler(int signum) {
  (void)signum;  // To shut compiler up about unused var
  lwsl_debug("SIGINT received. Shutting down...\n");
  should_exit = true;
}

void term_handler(int signum) {
  (void)signum;
  lwsl_debug("SIGTERM received. Shutting down...\n");
  should_exit = true;
}

static int picontrol_listen(struct lws_context *context) {
  // Set SIGINT and SIGTERM handlers
  struct sigaction old_sigint_handler, old_sigterm_handler;
  struct sigaction new_sigint_handler = {.sa_handler = &interrupt_handler,
                                         .sa_flags = 0};
  struct sigaction new_sigterm_handler = {.sa_handler = &term_handler,
                                          .sa_flags = 0};
  sigemptyset(&new_sigint_handler.sa_mask);
  sigemptyset(&new_sigterm_handler.sa_mask);
  sigaction(SIGINT, &new_sigint_handler, &old_sigint_handler);
  sigaction(SIGTERM, &new_sigterm_handler, &old_sigterm_handler);

  int n = 0;
  while (n >= 0 && !should_exit) {
    n = lws_service(context, 0);
  }
  // Restore old signal handlers
  sigaction(SIGINT, &old_sigint_handler, NULL);
  sigaction(SIGTERM, &old_sigterm_handler, NULL);

  return 0;
}
