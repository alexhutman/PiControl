#define _GNU_SOURCE

#include "keyboard/virtual_keyboard.h"
#include "shared/ipc/socket.h"
#include "shared/logging/logger.h"
#include "shared/model/protocol.h"

#include <fcntl.h>
#include <linux/limits.h>
#include <linux/uinput.h>
#include <pwd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define EXPECTED_SERVER_PATH "/usr/local/bin/picontrol_server"
#define SOCK_CONN_NUM        1
#ifdef PICTRL_XDO
  #define KEYBOARD_BACKEND PICTRL_BACKEND_XDO
#else
  #define KEYBOARD_BACKEND PICTRL_BACKEND_UINPUT
#endif // PICTRL_XDO

/*
typedef struct {
    char *path;
    mode_t perms;
} Dir;

static const mode_t public_mask = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;           // == 0755
static const mode_t socket_mask = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH; // == 0666

// Create intermediate directories needed as well
static bool get_socket_path(char *dst, size_t sz) {
    uid_t uid = getuid();
    const struct passwd *pass = getpwuid(uid);
    if (!pass || !pass->pw_dir) {
        pictrl_log_error("Couldn't read user's home directory\n");
        return false;
    }

    const Dir subdirs[] = {
        {".local",                 public_mask},
        {".local/state",           public_mask}, // Fallback $XDG_STATE_HOME dir
        {".local/state/picontrol", S_IRWXU},
    };
    char runtime_dir[PATH_MAX];
    for (size_t i = 0; i < PICTRL_SIZE(subdirs); i++) {
        snprintf(runtime_dir, sizeof(runtime_dir), "%s/%s", pass->pw_dir, subdirs[i].path);
        if (mkdir(runtime_dir, subdirs[i].perms) < 0) {
            int err = errno;
            if (err != EEXIST) {
                pictrl_log_error("Could not create runtime directory '%s' (%s)\n", runtime_dir, strerror(err));
                return false;
            }
        }
    }
    chmod(runtime_dir, S_IRWXU);

    int written = snprintf(dst, sz, "%s/helper.sock", runtime_dir);
    return written > 0 && (size_t)written < sz;
}
*/

static const mode_t socket_mask = S_IRUSR; // == 0400

static bool verify_peer_binary(pid_t pid) {
    char exe_link[25];
    snprintf(exe_link, sizeof(exe_link), "/proc/%d/exe", pid);

    char real_path[PATH_MAX];
    ssize_t len = readlink(exe_link, real_path, sizeof(real_path) - 1);
    if (len == -1) {
        int err = errno;
        pictrl_log_error("Couldn't inspect process' executable path (%s)\n", strerror(err));
        return false;
    }
    real_path[len] = '\0';

    return strcmp(real_path, EXPECTED_SERVER_PATH) == 0;
}

// TODO: Use abstract socket? (not portable... more info @ `man unix`)
static int create_socket(const char *socket_path) {
    struct sockaddr_un addr = {
        .sun_family = AF_UNIX,
    };
    int written = snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", socket_path);
    if (written < 0 || (size_t)written >= sizeof(addr.sun_path)) {
        pictrl_log_error("Socket path too long\n");
        return -1;
    }

    int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    unlink(socket_path);
    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        int err = errno;
        pictrl_log_critical("Socket bind failed (%s)\n", strerror(err));
        close(server_fd);
        return -1;
    }
    chmod(socket_path, socket_mask); // TODO: More error handling on everything
    pictrl_log_info("Socket created @ %s\n", socket_path);

    return server_fd;
}

static bool is_connection_allowed(int client_fd) {
    struct ucred cr;
    socklen_t len = sizeof(struct ucred);

    if (getsockopt(client_fd, SOL_SOCKET, SO_PEERCRED, &cr, &len) < 0) {
        int err = errno;
        pictrl_log_warn("Couldn't read socket options (%s)\n", strerror(err));
        return false;
    }

    if (cr.uid != geteuid()) {
        pictrl_log_warn("Blocked unauthorized UID (%d)\n", cr.uid);
        return false;
    }

    if (!verify_peer_binary(cr.pid)) {
        pictrl_log_warn("Blocked unauthorized PID (%d)\n", cr.pid);
        return false;
    }

    return true;
}

static int handle_message(Keyboard *keyboard, Message *msg) {
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

static bool read_msg(int server_fd, Message *msg) {
    // TODO: could have partial reads
    if (read(server_fd, &msg->header.cmd, sizeof(msg->header.cmd)) == -1)
        return false;
    if (read(server_fd, &msg->header.payload_size, sizeof(msg->header.payload_size)) == -1)
        return false;
    if (read(server_fd, msg->payload, msg->header.payload_size) == -1)
        return false;
    return true;
}

static void process_messages(int server_fd, Keyboard *keyboard) {
    int client_fd;
    while ((client_fd = accept(server_fd, NULL, NULL)) >= 0) {
        if (!is_connection_allowed(client_fd)) {
            pictrl_log_error("Client did not pass security checks. Disconnecting.\n");
            close(client_fd);
            continue;
        }
        shutdown(client_fd, SHUT_WR); // TODO: error checks
        pictrl_log_info("Server connected to socket\n");

        Message msg;
        while (read_msg(client_fd, &msg)) {
            if (!pictrl_validate_message(&msg))
                continue;
            handle_message(keyboard, &msg);
        }

        close(client_fd);
        pictrl_log_info("Server disconnected from socket\n");
    }
}

int main(void) {
    if (!pictrl_logger_init()) {
      fprintf(stderr, "Could not initialize logger!\n");
      return 1;
    }

    char socket_path[PATH_MAX];
    if (!get_socket_path(socket_path, sizeof(socket_path))) {
        pictrl_log_critical("Could not resolve socket's path in the runtime directory\n");
        pictrl_logger_destroy();
        return EXIT_FAILURE;
    }

    Keyboard *keyboard = pictrl_keyboard_new(KEYBOARD_BACKEND);
    if (!keyboard) {
      pictrl_log_critical("Unable to create PiControl keyboard\n");
      pictrl_logger_destroy();
      return EXIT_FAILURE;
    }

    int server_fd = create_socket(socket_path);
    if (server_fd < 0) {
        pictrl_log_critical("Unable to create socket\n");
        pictrl_keyboard_free(keyboard);
        pictrl_logger_destroy();
        return EXIT_FAILURE;
    }

    int listen_ret = listen(server_fd, SOCK_CONN_NUM);
    if (listen_ret < 0) {
        pictrl_log_critical("Unable to listen to socket\n");
        pictrl_keyboard_free(keyboard);
        pictrl_logger_destroy();
        close(server_fd);
        unlink(socket_path);
        return EXIT_FAILURE;
    }
    pictrl_log_info("Listening on %s...\n", socket_path);

    process_messages(server_fd, keyboard);

    close(server_fd);
    unlink(socket_path);
    pictrl_log_info("Closed socket at %s\n", socket_path);

    pictrl_log_info("Shutting down...\n");
    pictrl_keyboard_free(keyboard); // TODO: Should probably make sure this is successful
    pictrl_logger_destroy();
    return EXIT_SUCCESS;
}
