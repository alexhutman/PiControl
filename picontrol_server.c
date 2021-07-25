/*
 * This code uses Jacob Sorber's example at https://www.youtube.com/watch?v=esXw4bdaZkc
 * The plan is to modify it more to obviously perform the requested action of the client
 * (i.e. move the mouse or press keys)
 */

// Includes
#include <arpa/inet.h>
#include <fcntl.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <xdo.h>

#include "picontrol_common.h"
#include "picontrol_iputils.h"


#ifndef PICONTROL_ERR_EXIT_RET
#define PICONTROL_ERR_EXIT_RET(ret, msg...) \
	fprintf(stderr, msg);    \
	return ret
#endif

#ifndef PICONTROL_ERR_EXIT
#define PICONTROL_ERR_EXIT(msg...) PICONTROL_ERR_EXIT_RET(1, msg)
#endif


int setup_server() {
	int listenfd; 
	struct sockaddr_in servaddr;

	if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		// TODO: for all of the error messages, print the error message associated with errno
		PICONTROL_ERR_EXIT_RET(-1, "Error creating socket.\n");
	}

	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(SERVER_PORT);

	if ((bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr))) < 0) {
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

void print_err_hex(char *msg) {
	while (*msg) {
		fprintf(stderr, "%02x", (unsigned int) *msg++);
	}
	fprintf(stderr, "\n");
}

int main(int argc, char **argv) {
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

	const char *display = getenv("DISPLAY");
	xdo_t *xdo = xdo_new(display);
	if (xdo == NULL) {
		xdo_free(xdo);
		PICONTROL_ERR_EXIT("Unable to create xdo_t instance\n");
	}

	int connfd, n, i;

	struct sockaddr_in client;                                   // Client struct
	socklen_t client_sz = (socklen_t)sizeof(struct sockaddr_in); // Client struct size
	char *client_ip;                                             // Client IP string
	int client_port;                                             // Client port number

	uint_fast8_t cmd, payload_size;
	int_fast8_t relX, relY;
	uint8_t recvline[MAX_BUF+1]; // Receive buffer

	while(1) {
		connfd = accept(listenfd, (struct sockaddr *)&client, &client_sz);
		client_ip = inet_ntoa(client.sin_addr);
		client_port = ntohs(client.sin_port);
		printf("Client at %s:%d connected.\n", client_ip, client_port);
		
		// Zero out the receive and internal buffers to ensure that they are null-terminated
		memset(recvline, 0, MAX_BUF);

		// Read the incoming message
		while ((n = read(connfd, recvline, MAX_BUF-1)) > 0) {
			// TODO: maybe decide to make everything big endian for a standard
			cmd = (uint_fast8_t)recvline[0];
			payload_size = (uint_fast8_t)recvline[1];

			// Set the next byte after to 0 in case the last received msg was longer than this one
			*(&recvline[2] + n) = '\0';

#ifdef PI_CTRL_DEBUG
			printf("Command (1st byte): 0x%x\n", recvline[0]);

			printf("Payload size (2nd byte): 0x%x\n", recvline[1]);

			printf("Payload (hex): 0x");
			for (i = 0; i < payload_size; i++) {
				printf("%02x", recvline[2 + i]);
			}
			printf("\n");
#endif

			switch (cmd) {
				case PI_CTRL_MOUSE_MV:
					// If the payload size is 2 bytes long, we can extract the relative X and Y mouse locations to move by
					if (payload_size == 2) {
						relX = (int_fast8_t)recvline[2];
						relY = (int_fast8_t)recvline[3];
#ifdef PI_CTRL_DEBUG
						printf("Moving mouse (%d, %d) relative units.\n\n", relX, relY);
#endif

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
#ifdef PI_CTRL_DEBUG
					printf("%.*s|<-\n\n", payload_size, &recvline[2]);
#endif
					xdo_enter_text_window(xdo, CURRENTWINDOW, &recvline[2], 10000);
					break;
				case PI_CTRL_KEYSYM:
#ifdef PI_CTRL_DEBUG
					printf("%.*s|<-\n\n", payload_size, &recvline[2]);
#endif
					xdo_send_keysequence_window(xdo, CURRENTWINDOW, &recvline[2], 10000);
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

		printf("Client at %s:%d disconnected.\n", client_ip, client_port);

		if (n < 0) {
			PICONTROL_ERR_EXIT("Error reading from socket into buffer.\n");
		}

		/*
		// Close listening socket
		if ((close(listenfd)) < 0) {
			PICONTROL_ERR_EXIT("Error closing the listening socket.\n");
		}
		*/
		if ((close(connfd)) < 0) {
			PICONTROL_ERR_EXIT("Error closing the new socket.\n");
		}

	}

	return 0;
}
