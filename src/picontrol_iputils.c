#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

// Adapted from https://stackoverflow.com/a/2283541/6708303
char *iface_ip_address(const char *interface) {
    int fd = socket(AF_INET, SOCK_DGRAM, 0);;
    struct ifreq ifr;

    /* I want to get an IPv4 IP address */
    ifr.ifr_addr.sa_family = AF_INET;

    /* I want IP address attached to <interface> */
    strncpy(ifr.ifr_name, interface, IFNAMSIZ-1);

    int ip_res = ioctl(fd, SIOCGIFADDR, &ifr);
    int close_res = close(fd);

    // Interface doesn't exist or close failed
    if (ip_res < 0 || close_res < 0) {
        return NULL;
    }

    char *ip = (char *) malloc(IFNAMSIZ);
    strncpy(ip, inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr), IFNAMSIZ-1);

    return ip;
}


// Return first existent (non-loopback) interface's IP
// Adapted from https://stackoverflow.com/a/45796495/6708303
char *get_ip_address() {
    struct if_nameindex *if_nidxs, *intf;
    char *ip;

    if_nidxs = if_nameindex();
    if ( if_nidxs != NULL )
    {
        for (intf = if_nidxs; intf->if_index != 0 || intf->if_name != NULL; intf++)
        {
            if (strncmp(intf->if_name, "lo", 3) && (ip = iface_ip_address(intf->if_name))) {
                printf("Interface: %s\n", intf->if_name);
                if_freenameindex(if_nidxs);
                return ip;
            }
        }

        if_freenameindex(if_nidxs);
    }
    return NULL;
}
