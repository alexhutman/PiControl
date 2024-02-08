/*
 * This code uses Jacob Sorber's example at https://www.youtube.com/watch?v=esXw4bdaZkc
 * The plan is to modify it more to obviously perform the requested action of the client
 * (i.e. move the mouse or press keys)
 */

#include <arpa/inet.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

#include <xdo.h>

#include "picontrol_common.h"
#include "picontrol_iputils.h"
#include "data_structures/ring_buffer.h"
#include "logging/log_utils.h"


#ifndef PICONTROL_ERR_EXIT_RET
#define PICONTROL_ERR_EXIT_RET(ret, msg...) \
    fprintf(stderr, msg);    \
    return ret
#endif

#ifndef PICONTROL_ERR_EXIT
#define PICONTROL_ERR_EXIT(msg...) PICONTROL_ERR_EXIT_RET(1, msg)
#endif

// Delay between xdo keystrokes in microseconds
#define XDO_KEYSTROKE_DELAY (useconds_t)10000

//static const char *pictrl_fifo_path = "/tmp/pictrl_fifo";

static inline PiCtrlHeader pictrl_rb_get_header(pictrl_rb_t *rb);
static inline PiCtrlMouseCoord pictrl_rb_get_mouse_coords(pictrl_rb_t *rb);


int setup_server() {
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        // TODO: for all of the error messages, print the error message associated with errno
        PICONTROL_ERR_EXIT_RET(-1, "Error creating socket.\n");
    }

    sockaddr_in servaddr = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = htonl(INADDR_ANY),
        .sin_port = htons(SERVER_PORT),
    };

    if ((bind(listenfd, (sockaddr *)&servaddr, sizeof(servaddr))) < 0) {
        // TODO: Convert the address to a string
        close(listenfd);
        PICONTROL_ERR_EXIT_RET(-1, "Couldn't bind socket to %" PRIu32 ":%d.\n", INADDR_ANY, SERVER_PORT);
    }

    if ((listen(listenfd, MAX_CONNS)) < 0) {
        close(listenfd);
        PICONTROL_ERR_EXIT_RET(-1, "Error listening to the socket.\n");
    }

    return listenfd;
}

xdo_t *create_xdo() {
    const char *display = getenv("DISPLAY");
    return xdo_new(display);
}

void print_err_hex(char *msg) {
    while (*msg) {
        fprintf(stderr, "%02x", (unsigned int) *msg++);
    }
    fprintf(stderr, "\n");
}

void handle_mouse_move(pictrl_rb_t *rb, xdo_t *xdo) {
    // extract the relative X and Y mouse locations to move by
    const PiCtrlMouseCoord coords = pictrl_rb_get_mouse_coords(rb);
#ifdef PI_CTRL_DEBUG
    printf("Moving mouse (%d, %d) relative units.\n\n", coords.x, coords.y);
#endif

    if (xdo_move_mouse_relative(xdo, coords.x, coords.y) != 0) {
        fprintf(stderr, "Mouse was unable to be moved (%d, %d) relative units.\n", coords.x, coords.y);
    }
}

void handle_text(pictrl_rb_t *rb, xdo_t *xdo) {
    // `xdo_enter_text_window` expects a null-terminated string, there are more efficient approaches but this works
    static char text[MAX_BUF];
    pictrl_rb_copy(rb, text);

    // TODO: Make some rb method for this
    text[rb->data_length + 1] = 0;

    xdo_enter_text_window(xdo, CURRENTWINDOW, text, XDO_KEYSTROKE_DELAY); // TODO: what if sizeof(char) != sizeof(uint8_t)?

    // TODO: Make this a method...
    const size_t data_start_abs_idx = rb->data_start - rb->buffer_start;
    const size_t new_data_start_idx = (data_start_abs_idx + rb->data_length) % rb->num_bytes;
    rb->data_start = rb->buffer_start + new_data_start_idx;
    rb->data_length = 0;
}

void handle_keysym(pictrl_rb_t *rb, xdo_t *xdo) {
    // `xdo_send_keysequence_window` expects a null-terminated string, there are more efficient approaches but this works
    static char keysym[MAX_BUF];
    pictrl_rb_copy(rb, keysym);

    // TODO: Make some rb method for this
    keysym[rb->data_length + 1] = 0;

    xdo_send_keysequence_window(xdo, CURRENTWINDOW, keysym, XDO_KEYSTROKE_DELAY);

    // TODO: Make this a method...
    const size_t data_start_abs_idx = rb->data_start - rb->buffer_start;
    const size_t new_data_start_idx = (data_start_abs_idx + rb->data_length) % rb->num_bytes;
    rb->data_start = rb->buffer_start + new_data_start_idx;
    rb->data_length = 0;
}

// Assumes that rb->data_start is pointing at the beginning of the header in the ring buffer already
static inline PiCtrlHeader pictrl_rb_get_header(pictrl_rb_t *rb) {
    const PiCtrlHeader ret = {
        .command = (PiCtrlCmd)pictrl_rb_get(rb, 0),
        .payload_size = (size_t)pictrl_rb_get(rb, 1)
    };

    const size_t data_start_abs_idx = rb->data_start - rb->buffer_start;
    const size_t new_data_start_idx = (data_start_abs_idx + 2) % rb->num_bytes;
    rb->data_start = rb->buffer_start + new_data_start_idx;

    rb->data_length -= 2;
    return ret;
}

static inline PiCtrlMouseCoord pictrl_rb_get_mouse_coords(pictrl_rb_t *rb) {
    const PiCtrlMouseCoord ret = {
        .x = (int)pictrl_rb_get(rb, 0),
        .y = (int)pictrl_rb_get(rb, 1)
    };

    const size_t data_start_abs_idx = rb->data_start - rb->buffer_start;
    const size_t new_data_start_idx = (data_start_abs_idx + 2) % rb->num_bytes;
    rb->data_start = rb->buffer_start + new_data_start_idx;

    rb->data_length -= 2;
    return ret;
}

int handle_connection(pictrl_client_t *pi_client, pictrl_rb_t *rb, xdo_t *xdo) {
    // TODO:
    // 1. Read (blocking) header (currently only `payload_size` + `cmd`) from ring buffer
    //   a. Serialize to a struct?
    // 2. Convert `payload_size` to size_t
    // 3. Read `payload_size` bytes to a local buffer
    // 4. Handle `cmd` as we currently do, just use the local buffer from previous step

    
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
        printf("Command (1st byte): %x\n", header.command);

        printf("Payload size (2nd byte): %zu\n", header.payload_size);

        printf("Payload (hex): 0x");
        for (size_t i = 0; i < header.payload_size; i++) {
            printf("%02x", pictrl_rb_get(rb, i+2));
        }
        printf("\n");
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
                printf("Invalid test. Message is not formatted correctly.\n");
        }
    }


    if (n < 0) {
        PICONTROL_ERR_EXIT("Error reading from socket into buffer.\n");
    }
    return 0;
}


int picontrol_listen(int listenfd) {
    /////////////////////////////////////////////////////////////////
    xdo_t *xdo = create_xdo();
    if (xdo == NULL) {
        PICONTROL_ERR_EXIT("Unable to create xdo_t instance\n");
    }
#ifdef PI_CTRL_DEBUG
    printf("Acquired new xdo instance\n");
#endif

    pictrl_client_t pi_client = {
        .client = {},
        .client_sz = PICTRL_CLIENT_SZ,
        .client_ip = NULL,
        .client_port = -1,
        .connfd = -1
    };


    /*
    if (mkfifo(pictrl_fifo_path, 0600) == -1) {
        // TODO: handle
        printf("Whoopsies\n");
    }
    int fifo_read_fd = open(pictrl_fifo_path, O_WRONLY),
        fifo_write_fd = open(pictrl_fifo_path, O_RDONLY);
    */

    pictrl_rb_t recv_buf; // Receive ring buffer
    pictrl_rb_init(&recv_buf, MAX_BUF);

    int ret = 0;
    while(1) {
        pi_client.connfd = accept(listenfd, (sockaddr *)&pi_client.client, &pi_client.client_sz);
        if (pi_client.connfd < 0) {
            PICONTROL_ERR_EXIT("Error accepting new connection.\n");
        }
        pi_client.client_ip = inet_ntoa(pi_client.client.sin_addr);
        pi_client.client_port = ntohs(pi_client.client.sin_port);
        printf("Client at %s:%d connected.\n", pi_client.client_ip, pi_client.client_port);
        

        ret = handle_connection(&pi_client, &recv_buf, xdo);

        // Close connection
        if ((close(pi_client.connfd)) < 0) {
            fprintf(stderr, "Error closing the new socket.\n");
            break;
        }

#ifdef PI_CTRL_DEBUG
        printf("Closed connection file descriptor %d\n", pi_client.connfd);
#endif
        printf("Client at %s:%d disconnected.\n", pi_client.client_ip, pi_client.client_port);
        pi_client.connfd = -1;

        if (ret != 0) {
            fprintf(stderr, "Error handling the connection.\n");
            break;
        }

        // Clear out the buffer on each new connection
        pictrl_rb_clear(&recv_buf);
    }

    pictrl_rb_destroy(&recv_buf);
    return 0;
}

int main() {
    char *ip = get_ip_address();
    if (ip == NULL) {
        PICONTROL_ERR_EXIT("You are not connected to the internet.\n");
    }

    printf("Connect at: %s\n", ip);
    free(ip);

    int listenfd = setup_server();
    if (listenfd < 0) {
        // We already print the appropriate error message in setup_server()
        return 1;
    }
#ifdef PI_CTRL_DEBUG
    printf("Acquired listen socket on file descriptor %d\n", listenfd);
#endif


    int listen_ret = picontrol_listen(listenfd);
    if (listen_ret < 0) {
        PICONTROL_ERR_EXIT_RET(listen_ret, "Error occurred while listening...\n");
    }
#ifdef PI_CTRL_DEBUG
    printf("Finished listening on file descriptor %d\n", listenfd);
#endif

    // Close listening socket
    if ((close(listenfd)) < 0) {
        PICONTROL_ERR_EXIT("Error closing the listening socket.\n");
    }
#ifdef PI_CTRL_DEBUG
    printf("Closed listen socket on file descriptor %d\n", listenfd);
    printf("Exiting with code %d\n", listen_ret);
#endif

    return listen_ret;
}
