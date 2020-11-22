/*
 * This code uses Jacob Sorber's example at https://www.youtube.com/watch?v=esXw4bdaZkc
 * The plan is to modify it more to obviously perform the requested action of the client
 * (i.e. move the mouse or press keys)
 */
#include "picontrol_common.h"


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


int main(int argc, char **argv) {
	int listenfd = setup_server();
	if (listenfd < 0) {
		// We already print the appropriate error message in setup_server()
		return 1;
	}

	int connfd, n;
	// uint8_t buff[MAX_BUF+1];
	uint8_t recvline[MAX_BUF+1];


	//while(1) {
	connfd = accept(listenfd, (struct sockaddr *)NULL, NULL);
	
	// Zero out the receive buffer to ensure that it is null-terminated
	memset(recvline, 0, MAX_BUF);

	// Read the incoming message
	while ((n = read(connfd, recvline, MAX_BUF-1)) > 0) {
		int msg_size = (int)recvline[0];
		int cmd = (int)recvline[1];

		printf("Message size (1st byte): 0x%x\n", recvline[0]);
		printf("Command (2nd byte): 0x%x\n", recvline[1]);
		switch (cmd) {
			case PI_CTRL_SET_NAME:
				printf("Size: %d bytes = %d ASCII chars\n", msg_size, (int)(msg_size/sizeof(uint8_t)));
				printf("%.*s|<-\n", msg_size, &recvline[2]);
				break;
			default:
				printf("Invalid test. Message is not formatted correctly.\n");
		}

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
