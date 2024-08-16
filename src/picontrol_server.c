#include <errno.h>
#include <inttypes.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include <libwebsockets.h>

#include "backend/picontrol_backend.h"
#include "data_structures/ring_buffer.h"
#include "logging/log_utils.h"
#include "networking/iputils.h"
#include "networking/websocket_protocol.h"
#include "picontrol_config.h"

static int picontrol_listen(struct lws_context *context);

void interrupt_handler(int signum);

volatile sig_atomic_t should_exit = false;

const struct lws_protocols protocols[] = {
    {
        .name = "picontrol",
        .callback = &callback_picontrol,
        .per_session_data_size = 0,
        .rx_buffer_size = 0,
        .id = 1 // First iteration of the protocol (ignored by lws)
    },
    LWS_PROTOCOL_LIST_TERM
};


int main() {
  // Get our IP
  char *ip = get_ip_address();
  if (ip == NULL) {
    return 1;
  }
  pictrl_log("Connect at: %s:%d\n", ip, SERVER_PORT);
  free(ip);

  const struct lws_context_creation_info info = {
      .port = SERVER_PORT,
      .protocols = protocols,
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

static int picontrol_listen(struct lws_context *context) {
  // Set SIGINT and SIGTERM handlers
  struct sigaction old_sigint_handler, old_sigterm_handler;
  struct sigaction new_sigint_handler = {.sa_handler = &interrupt_handler,
                                         .sa_flags = 0};
  struct sigaction new_sigterm_handler = {.sa_handler = &interrupt_handler,
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

void interrupt_handler(int signum) {
  (void)signum;  // To shut compiler up about unused var
  should_exit = true;
}
