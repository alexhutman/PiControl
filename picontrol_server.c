/*
 * This code uses Jacob Sorber's example at https://www.youtube.com/watch?v=esXw4bdaZkc
 * The plan is to modify it more to obviously perform the requested action of the client
 * (i.e. move the mouse or press keys)
 */
#include "picontrol_common.h"


int main(int argc, char **argv) {
	int listenfd, connfd, n;
	struct sockaddr_in servaddr;
	uint8_t buff[MAX_BUF+1];
	uint8_t recvline[MAX_BUF+1];

	if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		// TODO: for all of the error messages, print the error message associated with errno
		fprintf(stderr, "Error creating socket.\n");
		return 1;
	}

	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(SERVER_PORT);

	if ((bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr))) < 0) {
		fprintf(stderr, "Couldn't bind socket to %" PRIu32 ":%d.\n", INADDR_ANY, SERVER_PORT);
		// TODO: Convert the address to a string
		return 1;
	}

	if ((listen(listenfd, MAX_CONNS)) < 0) {
		fprintf(stderr, "Error listening to the socket.\n");
		return 1;
	}

	//while(1) {
		struct sockaddr_in addr;
		socklen_t addr_len;

		connfd = accept(listenfd, (struct sockaddr *)NULL, NULL);
		
		// Zero out the receive buffer to ensure that it is null-terminated
		memset(recvline, 0, MAX_BUF);

		// Read the incoming message
		while ((n = read(connfd, recvline, MAX_BUF-1)) > 0) {
			int cmd_size = (int)recvline[0];

			printf("First byte: %x\n", recvline[0]);
			printf("Second byte (char): %c\n", recvline[1]);
			printf("Size: %d bytes = %d chars\n", cmd_size, (int)(cmd_size/sizeof(uint8_t)));
			printf("%.*s|<-\n", cmd_size, &recvline[1]);
			// TODO: Find a way to break once we know the message is finished.
			// For testing, we'll assume that strlen(recvline) < MAX_BUF
			break;

			// When we do the above TODO, we need to clear the buffer on the next read
			// memset(recvline, 0, MAX_BUF);
		}

		if (n < 0) {
			fprintf(stderr, "Error reading from socket into buffer.\n");
			return 1;
		}

		// TODO: when we need to send responses, we'll do them here
		// snprintf((char *)buff, sizeof(buff), "RESPONSE_GOES_HERE");
		// write(connfd, (char *)buff, strlen((char *)buff));
		if ((close(listenfd)) < 0) {
			fprintf(stderr, "Error closing the listening socket.\n");
			return 1;
		}
		if ((close(connfd)) < 0) {
			fprintf(stderr, "Error closing the new socket.\n");
			return 1;
		}

	//}

	return 0;
}
