#ifndef _PICTRL_IPUTILS_H
#define _PICTRL_IPUTILS_H

#include <arpa/inet.h>
#include <net/if.h>
#include <string.h>

// Client struct size
#define PICTRL_CLIENT_INIT_SZ (socklen_t)sizeof(struct sockaddr_in)

typedef char pictrl_ip[IFNAMSIZ];

typedef struct pictrl_client_t {
    struct sockaddr_in client; /* Client struct */
    socklen_t client_sz;       /* Client size. This is a compile-time-resolvable value,
                                  but `accept()` modifies it after initialization, so we
                                  can't just store it as a constant. */

    int client_port;           /* Client port number */
    int connfd;
    pictrl_ip client_ip;       /* Client IP string */
} pictrl_client_t;

char *get_ip_address();

static inline pictrl_client_t pictrl_client_new() {
    pictrl_client_t new_pi_client = {
            .client = {},
            .client_sz = PICTRL_CLIENT_INIT_SZ,
            .client_port = -1,
            .connfd = -1
        };
    strncpy(new_pi_client.client_ip, "(no client)", IFNAMSIZ - 1);
    return new_pi_client;
}

static inline void pictrl_client_clear(pictrl_client_t *pi_client) {
    memset(&pi_client->client, 0, pi_client->client_sz);
    pi_client->client_port = -1;
    pi_client->connfd = -1;
    strncpy(pi_client->client_ip, "(no client)", IFNAMSIZ - 1);
}

static inline void pictrl_client_get_ip_and_port(pictrl_client_t *pi_client) {
    // TODO: Clean
    strncpy(pi_client->client_ip, inet_ntoa(pi_client->client.sin_addr), IFNAMSIZ - 1);
    pi_client->client_port = ntohs(pi_client->client.sin_port);
}

#endif
