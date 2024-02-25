#include <arpa/inet.h>
#include <errno.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netdb.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "logging/log_utils.h"

#define LOOPBACK_UP_RUNNING (IFF_UP|IFF_RUNNING|IFF_LOOPBACK)
#define UP_RUNNING          (IFF_UP|IFF_RUNNING)

static inline bool non_loopback_up_and_running(struct ifaddrs *interface) {
    return (interface->ifa_flags & LOOPBACK_UP_RUNNING) == UP_RUNNING;
}

static inline bool is_addr_valid(struct sockaddr *addr) {
    return (addr != NULL)
        && (addr->sa_family == AF_INET || addr->sa_family == AF_INET6);
}

// Return first existent, non-loopback, data-receiving interface's IP
// Adapted from https://stackoverflow.com/a/12883978
char *get_ip_address() {
    struct ifaddrs *interfaces;
    if (getifaddrs(&interfaces) != 0) {
        pictrl_log_error("Error retrieving network interfaces: %s\n", strerror(errno));
        return NULL;
    }

    struct ifaddrs *iface = NULL;
    for (struct ifaddrs *interface = interfaces; interface != NULL; interface = interface->ifa_next) {
        if (non_loopback_up_and_running(interface)) {
            if (!is_addr_valid(interface->ifa_addr)) {
                continue;
            }

            iface = interface;
            if (iface->ifa_addr->sa_family == AF_INET) {
                // We prefer IPv4 interfaces
                break;
            }
        }
    }
    if (iface == NULL) {
        pictrl_log_critical("You seem to not be connected to the internet!\n");
        return NULL;
    }
    pictrl_log_debug("Interface: %s\n", iface->ifa_name);

    char *ip = NULL;
    const socklen_t len = sizeof(struct sockaddr);
    switch (iface->ifa_addr->sa_family) {
        case AF_INET:
            ip = malloc(INET_ADDRSTRLEN);  // TODO: err handling
            getnameinfo(iface->ifa_addr, len, ip, INET_ADDRSTRLEN, NULL, 0, NI_NUMERICHOST);
            break;
        case AF_INET6:
            ip = malloc(INET6_ADDRSTRLEN); // TODO: err handling
            getnameinfo(iface->ifa_addr, len, ip, INET6_ADDRSTRLEN, NULL, 0, NI_NUMERICHOST);
            break;
        default:
            pictrl_log_critical("Unknown interface type.\n");
    }
    freeifaddrs(interfaces);
    return ip;
}
