/*
 * This code uses Jacob Sorber's example at https://www.youtube.com/watch?v=esXw4bdaZkc
 * The plan is to modify it more to obviously perform the requested action of the client
 * (i.e. move the mouse or press keys)
 */
#include "picontrol_common.h"
#include <xdo.h>


int setup_server() {
	int listenfd; 
	struct sockaddr_in servaddr;

	if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		// TODO: for all of the error messages, print the error message associated with errno
		fprintf(stderr, "Error creating socket.\n");
		return -1;
	}

	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(SERVER_PORT);

	if ((bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr))) < 0) {
		fprintf(stderr, "Couldn't bind socket to %" PRIu32 ":%d.\n", INADDR_ANY, SERVER_PORT);
		close(listenfd);
		// TODO: Convert the address to a string
		return -1;
	}

	if ((listen(listenfd, MAX_CONNS)) < 0) {
		fprintf(stderr, "Error listening to the socket.\n");
		close(listenfd);
		return -1;
	}

	return listenfd;
}

void print_err_hex(char *msg) {
	while (*msg) {
		fprintf(stderr, "%02x", (unsigned int) *msg++);
	}
	fprintf(stderr, "\n");
}

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

int main(int argc, char **argv) {
	char *ip = get_ip_address();
	if (ip == NULL) {
		fprintf(stderr, "You are not connected to the internet.\n");
		return -1;
	}

	printf("Connect at: %s\n", ip);
	free(ip);

	int listenfd = setup_server();
	if (listenfd < 0) {
		// We already print the appropriate error message in setup_server()
		return 1;
	}

	const char *display = getenv("DISPLAY");
	xdo_t *xdo = xdo_new(display);
	if (xdo == NULL) {
		printf("Unable to create xdo_t instance\n");
		xdo_free(xdo);
		return 1;
	}

	int connfd, n, i;
	uint_fast8_t cmd, payload_size;
	int_fast8_t relX, relY;
	uint8_t recvline[MAX_BUF+1]; // Receive buffer

	while(1) {
		connfd = accept(listenfd, (struct sockaddr *)NULL, NULL);
		
		// Zero out the receive and internal buffers to ensure that they are null-terminated
		memset(recvline, 0, MAX_BUF);

		// Read the incoming message
		while ((n = read(connfd, recvline, MAX_BUF-1)) > 0) {
			// TODO: maybe decide to make everything big endian for a standard
			cmd = (uint_fast8_t)recvline[0];
			payload_size = (uint_fast8_t)recvline[1];

			// Set the next byte after to 0 in case the last received msg was longer than this one
			*(&recvline[2] + n) = '\0';

			printf("Command (1st byte): 0x%x\n", recvline[0]);

			printf("Payload size (2nd byte): 0x%x\n", recvline[1]);

			printf("Payload (hex): 0x");
			for (i = 0; i < payload_size; i++) {
				printf("%02x", recvline[2 + i]);
			}
			printf("\n");

			switch (cmd) {
				case PI_CTRL_MOUSE_MV:
					// If the payload size is 2 bytes long, we can extract the relative X and Y mouse locations to move by
					if (payload_size == 2) {
						relX = (int_fast8_t)recvline[2];
						relY = (int_fast8_t)recvline[3];
						printf("Moving mouse (%d, %d) relative units.\n\n", relX, relY);

						if (xdo_move_mouse_relative(xdo, relX, relY) != 0) {
							fprintf(stderr, "Mouse was unable to be moved (%d, %d) relative units.\n", relX, relY);
						}
					}
					else {
						fprintf(stderr, "A PI_CTRL_MOUSE_MV message was sent with a %u byte payload instead"
								"of a 2 byte payload.\n", payload_size);
					}
					break;
				case PI_CTRL_KEY_PRESS:
					// Need to send the payload length since UTF-8 chars can be more than 1 byte long
					printf("%.*s|<-\n\n", payload_size, &recvline[2]);
					xdo_enter_text_window(xdo, CURRENTWINDOW, &recvline[2], 40000);
					break;
				default:
					printf("Invalid test. Message is not formatted correctly.\n");
			}

			// TODO: Find a way to break once we know the message is finished.
			// For testing, we'll assume that strlen(recvline) < MAX_BUF
			//break;

			// When we do the above TODO, we need to clear the buffer on the next read
			// memset(recvline, 0, MAX_BUF);
		}

		// They disconnected?
		if (n < 0) {
			fprintf(stderr, "Error reading from socket into buffer.\n");
			return 1;
		}

		/*
		// Close listening socket
		if ((close(listenfd)) < 0) {
			fprintf(stderr, "Error closing the listening socket.\n");
			return 1;
		}
		*/
		if ((close(connfd)) < 0) {
			fprintf(stderr, "Error closing the new socket.\n");
			return 1;
		}

	}

	return 0;
}
