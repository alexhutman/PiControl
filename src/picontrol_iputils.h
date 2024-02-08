#ifndef _PICTRL_IPUTILS_H
#define _PICTRL_IPUTILS_H
#include <netinet/in.h>

// Client struct size
#define PICTRL_CLIENT_SZ (socklen_t)sizeof(sockaddr_in)

typedef struct sockaddr_in sockaddr_in;
typedef struct sockaddr sockaddr;

typedef struct pictrl_client_t {
    sockaddr_in client;  // Client struct
    socklen_t client_sz; 
    char *client_ip;     // Client IP string
    int client_port;     // Client port number
    int connfd;
} pictrl_client_t;

char *iface_ip_address(const char *);
char *get_ip_address();

#endif
