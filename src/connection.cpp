#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <sstream>

#include "include/connection.h"

using namespace std;

Connection::Connection(const char * addr, const char * p) {
	strcpy(hostname, addr);
	strcpy(port, p);
}

int Connection::create() {
	
	// get server address info
	
	int rv;
	
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	
	if((rv = getaddrinfo(hostname, port, &hints, &servInfo)) != 0)
	{
		printf("error getaddrinfo: %s\n", gai_strerror(rv));
		return -1;
	}

	struct addrinfo * p;
	
	// create the socket and connect
	
	for(p = servInfo; p != NULL; p = p->ai_next) {
		if ((iSocket = socket(p->ai_family, p->ai_socktype,
							  p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}
		
		if (connect(iSocket, p->ai_addr, p->ai_addrlen) == -1) {
			close(iSocket);
			perror("client: connect");
			continue;
		}
		
		break;
	}
	
	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}
	
	return iSocket;	
}
