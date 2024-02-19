#include <arpa/inet.h>
#include <inttypes.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>

#include <xdo.h>

#include "data_structures/ring_buffer.h"
#include "logging/log_utils.h"
#include "networking/iputils.h"
#include "picontrol_common.h"


// Delay between xdo keystrokes in microseconds
#define XDO_KEYSTROKE_DELAY (useconds_t)10000

static int setup_server();
static int picontrol_listen(int fd);
static int handle_connection(pictrl_client_t *client, pictrl_rb_t *rb, xdo_t *xdo);
static void handle_mouse_move(pictrl_rb_t *rb, xdo_t *xdo);
static void handle_text(pictrl_rb_t *rb, xdo_t *xdo);
static void handle_keysym(pictrl_rb_t *rb, xdo_t *xdo);
static xdo_t *create_xdo();
static inline PiCtrlHeader pictrl_rb_get_header(pictrl_rb_t *rb);
static inline PiCtrlMouseCoord pictrl_rb_get_mouse_coords(pictrl_rb_t *rb);

void interrupt_handler(int signum);

volatile sig_atomic_t should_exit = false;

int main() {
    char *ip = get_ip_address();
    if (ip == NULL) {
        pictrl_log_critical("You are not connected to the internet.\n");
        return 1;
    }
    pictrl_log("Connect at: %s:%d\n", ip, SERVER_PORT);
    free(ip);

    int listenfd = setup_server();
    if (listenfd < 0) {
        // We already print the appropriate error message in setup_server()
        return 1;
    }
    pictrl_log_debug("Acquired listen socket on file descriptor %d\n", listenfd);

    int listen_ret = picontrol_listen(listenfd);
    if (listen_ret < 0) {
        pictrl_log_critical("Error occurred while listening...\n");
    }
    pictrl_log_debug("Finished listening on file descriptor %d\n", listenfd);

    // Close listening socket
    if ((close(listenfd)) < 0) {
        pictrl_log_error("Error closing the listening socket.\n");
    }
    pictrl_log_debug("Closed listen socket on file descriptor %d\n", listenfd);
    pictrl_log_debug("Exiting with code %d\n", listen_ret);
    return listen_ret;
}

static int setup_server() {
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        // TODO: for all of the error messages, print the error message associated with errno
        pictrl_log_error("Error creating socket.\n");
        return -1;
    }

    sockaddr_in servaddr = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = htonl(INADDR_ANY),
        .sin_port = htons(SERVER_PORT),
    };

    if ((bind(listenfd, (sockaddr *)&servaddr, sizeof(servaddr))) < 0) {
        // TODO: Convert the address to a string
        close(listenfd);
        pictrl_log_error("Couldn't bind socket to %" PRIu32 ":%d.\n", INADDR_ANY, SERVER_PORT);
        return -1;
    }

    if ((listen(listenfd, MAX_CONNS)) < 0) {
        close(listenfd);
        pictrl_log_error("Error listening to the socket.\n");
        return -1;
    }

    return listenfd;
}

static int picontrol_listen(int listenfd) {
    xdo_t *xdo = create_xdo();
    if (xdo == NULL) {
        pictrl_log_error("Unable to create xdo_t instance\n");
        return -1;
    }
    pictrl_log_debug("Acquired new xdo instance\n");

    pictrl_client_t pi_client = {
        .client = {},
        .client_sz = PICTRL_CLIENT_SZ,
        .client_ip = NULL,
        .client_port = -1,
        .connfd = -1
    };

    pictrl_rb_t recv_buf; // Receive ring buffer
    pictrl_rb_init(&recv_buf, MAX_BUF);

    struct sigaction old_sigint_handler;
    struct sigaction new_sigint_handler = {
        .sa_handler = &interrupt_handler,
        .sa_flags = 0
    };
    sigemptyset(&new_sigint_handler.sa_mask);

    int ret = -1;
    sigaction(SIGINT, &new_sigint_handler, &old_sigint_handler);
    while(!should_exit) {
        // Accept connection
        pi_client.connfd = accept(listenfd, (sockaddr *)&pi_client.client, &pi_client.client_sz);
        if (pi_client.connfd < 0) {
            pictrl_log_error("Error accepting new connection.\n");
            break;
        }
        pi_client.client_ip = inet_ntoa(pi_client.client.sin_addr);
        pi_client.client_port = ntohs(pi_client.client.sin_port);
        pictrl_log_info("Client at %s:%d connected.\n", pi_client.client_ip, pi_client.client_port);

        // Handle connection
        ret = handle_connection(&pi_client, &recv_buf, xdo);

        // Close connection
        if ((close(pi_client.connfd)) < 0) {
            pictrl_log_error("Error closing the new socket.\n");
            break;
        }

        pictrl_log_debug("Closed connection file descriptor %d\n", pi_client.connfd);
        pictrl_log_info("Client at %s:%d disconnected.\n", pi_client.client_ip, pi_client.client_port);
        pi_client.connfd = -1;

        if (ret != 0) {
            pictrl_log_error("Error handling the connection.\n");
            break;
        }

        // Clear out the buffer on each new connection
        pictrl_rb_clear(&recv_buf);
        pictrl_log_debug("Cleared ring buffer\n");
    }
    sigaction(SIGINT, &old_sigint_handler, NULL);

    pictrl_log_debug("Destroying ring buffer\n");
    pictrl_rb_destroy(&recv_buf);

    pictrl_log_debug("Freeing xdo\n");
    xdo_free(xdo);
    return ret;
}

static int handle_connection(pictrl_client_t *pi_client, pictrl_rb_t *rb, xdo_t *xdo) {
    ssize_t n;
    // TODO: Block on this write, since we can't do anything unless we have the command and payload_size
    while ((n = pictrl_rb_write(pi_client->connfd, 2, rb)) > 0) { // 2 for the `cmd` and `payload_size`
        // TODO: make everything big endian to align with network byte order
        const PiCtrlHeader header = pictrl_rb_get_header(rb);
        // TODO: validate header (i.e. payload size is expected, given command)
        // TODO: block until this write finishes
        n = pictrl_rb_write(pi_client->connfd, header.payload_size, rb);
        if (n <= 0) {
            break;
        }

#ifdef PI_CTRL_DEBUG
        pictrl_log_debug("Command (1st byte): %x\n"
                         "Payload size (2nd byte): %zu\n"
                         "Payload: ",
                         header.command,
                         header.payload_size);
        for (size_t i = 0; i < header.payload_size; i++) {
            pictrl_log("%c", pictrl_rb_get(rb, i));
        }
        pictrl_log(" (0x");
        for (size_t i = 0; i < header.payload_size; i++) {
            pictrl_log("%02x", pictrl_rb_get(rb, i));
        }
        pictrl_log(")\n");
#endif

        // Handle command
        switch (header.command) {
            case PI_CTRL_MOUSE_MV:
                handle_mouse_move(rb, xdo);
                break;
            case PI_CTRL_TEXT:
                handle_text(rb, xdo);
                break;
            case PI_CTRL_KEYSYM:
                handle_keysym(rb, xdo);
                break;
            // TODO: On disconnect command, return 0?
            default:
                pictrl_log_error("Invalid test. Message is not formatted correctly.\n");
        }
    }

    if (n < 0) {
        pictrl_log_error("Error reading from socket into buffer.\n");
        return -1;
    }
    return 0;
}

static void handle_mouse_move(pictrl_rb_t *rb, xdo_t *xdo) {
    // extract the relative X and Y mouse locations to move by
    const PiCtrlMouseCoord coords = pictrl_rb_get_mouse_coords(rb);

    pictrl_log_debug("Moving mouse (%d, %d) relative units.\n\n", coords.x, coords.y);
    if (xdo_move_mouse_relative(xdo, coords.x, coords.y) != 0) {
        pictrl_log_warn("Mouse was unable to be moved (%d, %d) relative units.\n", coords.x, coords.y);
    }
}

static void handle_text(pictrl_rb_t *rb, xdo_t *xdo) {
    // `xdo_enter_text_window` expects a null-terminated string, there are more efficient approaches but this works
    static char text[MAX_BUF];
    pictrl_rb_copy(rb, text);

    text[rb->num_items] = 0;

    xdo_enter_text_window(xdo, CURRENTWINDOW, text, XDO_KEYSTROKE_DELAY); // TODO: what if sizeof(char) != sizeof(uint8_t)?

    const size_t new_data_start_idx = (rb->data_start + rb->num_items) % rb->capacity;
    rb->data_start = new_data_start_idx;
    rb->num_items = 0;
}

static void handle_keysym(pictrl_rb_t *rb, xdo_t *xdo) {
    // `xdo_send_keysequence_window` expects a null-terminated string, there are more efficient approaches but this works
    static char keysym[MAX_BUF];
    pictrl_rb_copy(rb, keysym);

    keysym[rb->num_items] = 0;

    xdo_send_keysequence_window(xdo, CURRENTWINDOW, keysym, XDO_KEYSTROKE_DELAY);

    const size_t new_data_start_idx = (rb->data_start + rb->num_items) % rb->capacity;
    rb->data_start = new_data_start_idx;
    rb->num_items = 0;
}

static xdo_t *create_xdo() {
    const char *display = getenv("DISPLAY");
    return xdo_new(display);
}

// Assumes that rb->data_start is pointing at the beginning of the header in the ring buffer already
static inline PiCtrlHeader pictrl_rb_get_header(pictrl_rb_t *rb) {
    const PiCtrlHeader ret = {
        .command = (PiCtrlCmd)pictrl_rb_get(rb, 0),
        .payload_size = (size_t)pictrl_rb_get(rb, 1)
    };

    const size_t new_data_start_idx = (rb->data_start + 2) % rb->capacity;
    rb->data_start = new_data_start_idx;

    rb->num_items -= 2;
    return ret;
}

static inline PiCtrlMouseCoord pictrl_rb_get_mouse_coords(pictrl_rb_t *rb) {
    const PiCtrlMouseCoord ret = {
        .x = (int)pictrl_rb_get(rb, 0),
        .y = (int)pictrl_rb_get(rb, 1)
    };

    const size_t new_data_start_idx = (rb->data_start + 2) % rb->capacity;
    rb->data_start = new_data_start_idx;

    rb->num_items -= 2;
    return ret;
}

void interrupt_handler(int signum) {
    (void)signum; // To shut compiler up about unused var
    should_exit = true;
}
