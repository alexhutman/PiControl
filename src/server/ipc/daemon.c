#define _GNU_SOURCE

#include "ipc/daemon.h"

#include "shared/ipc/socket.h"
#include "shared/logging/logger.h"
#include "shared/serde/protocol.h"

#include <fcntl.h>
#include <linux/limits.h>
#include <pwd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>

/*
static const char sock_homedir_subpath[] = ".local/state/picontrol/helper.sock";

static bool get_socket_path(char *dst, size_t sz) {
    uid_t uid = getuid();
    const struct passwd *pass = getpwuid(uid);
    if (!pass || !pass->pw_dir) {
        pictrl_log_error("Couldn't read user's home directory\n");
        return false;
    }

    int written = snprintf(dst, sz, "%s/%s", pass->pw_dir, sock_homedir_subpath);
    return written > 0 && (size_t)written < sz;
}
*/

static int create_socket(const char *socket_path) {
    struct sockaddr_un addr = {
        .sun_family = AF_UNIX,
    };

    int written = snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", socket_path);
    if (written < 0 || (size_t)written >= sizeof(addr.sun_path)) {
        // addr.sun_path can only be 108 bytes...
        pictrl_log_error("Socket path too long\n");
        return -1;
    }

    int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    unlink(socket_path);
    if (connect(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        int err = errno;
        if (err == ENOENT) {
            pictrl_log_error("The socket file does not exist. Is the keyboard daemon running?\n");
        } else if (err == ECONNREFUSED) {
            pictrl_log_error("The socket file exists, but nothing is listening to it...\n");
        } else {
            pictrl_log_error("Socket bind failed: %s\n", strerror(err));
        }
        close(server_fd);
        return -1;
    }

    return server_fd;
}

bool open_daemon_socket(Daemon *daemon) {
    char socket_path[PATH_MAX];
    if (!get_socket_path(socket_path, sizeof(socket_path)))
        return false;

    int fd = create_socket(socket_path);
    if (fd < 0)
        return false;

    daemon->fd = fd;
    return true;
}

void close_daemon_socket(Daemon *daemon) {
    if (shutdown(daemon->fd, SHUT_WR) == -1) {
        const int err = errno;
        if (err != ENOTCONN) { // ENOTCONN -> remote side closed first (usually...)
            pictrl_log_warn("Couldn't shut down keyboard daemon socket: %s\n", strerror(err));
        }
    }
    if (close(daemon->fd) == -1) {
        const int err = errno;
        pictrl_log_error("Couldn't close keyboard daemon socket: %s\n", strerror(err));
    }
    daemon->fd = -1;
}

bool send_msg_to_daemon(Daemon *daemon, MsgDeserializer *ser) {
    pictrl_serialize_network_data(ser);

    const ssize_t written = write(daemon->fd, ser->in.rx_buffer, ser->in.rx_buffered_bytes);
    if (written == -1) {
        const int err = errno;
        pictrl_log_error("Couldn't write to keyboard daemon: %s\n", strerror(err));
        return false;
    }
    return (size_t)written == ser->in.rx_buffered_bytes;
}
