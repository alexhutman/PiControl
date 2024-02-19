#include <arpa/inet.h>
#include <net/if.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "logging/log_utils.h"

// Adapted from https://stackoverflow.com/a/2283541/6708303
char *iface_ip_address(const char *interface) {
    const int fd = socket(AF_INET, SOCK_DGRAM, 0);
    struct ifreq ifr = {
        .ifr_addr = {
            .sa_family = AF_INET // IPv4
        }
    };
    strncpy(ifr.ifr_name, interface, IFNAMSIZ-1); // IP address attached to <interface>

    if (ioctl(fd, SIOCGIFADDR, &ifr) < 0) {
        // Interface doesn't exist
        return NULL;
    }
    if (close(fd) < 0) {
        return NULL;
    }

    char *ip = (char *) malloc(IFNAMSIZ);
    strncpy(ip, inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr), IFNAMSIZ-1);
    return ip;
}


// Return first existent (non-loopback) interface's IP
// Adapted from https://stackoverflow.com/a/45796495/6708303
char *get_ip_address() {
    struct if_nameindex *if_nidxs = if_nameindex();
    if (if_nidxs == NULL) {
        return NULL;
    }

    char *ip = NULL;
    for (struct if_nameindex *intf = if_nidxs; !(intf->if_index == 0 && intf->if_name == NULL); intf++) {
        if (strncmp(intf->if_name, "lo", 3)) {
            pictrl_log_debug("Interface: %s\n", intf->if_name);
            ip = iface_ip_address(intf->if_name);
        }
    }
    if_freenameindex(if_nidxs);
    return ip;
}
