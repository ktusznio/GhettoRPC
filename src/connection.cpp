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

#include "include/connection.h"

Connection::Connection(const char * addr, const char * p) {
	
	// TODO: get rid of hard-coded address and port
	
	// read server address
	/*
	const char * envServerAddr = getenv("SERVER_ADDRESS");
	if(envServerAddr == NULL) {
		printf("SERVER_ADDRESS not set, setting to localhost\n");
		envServerAddr = "localhost";
	}
	*/
	
	
	// read server port
	/*
	 const char * envServerPort = getenv("SERVER_PORT");
	 
	 if(envServerPort == NULL)
	 {
	 printf("SERVER_PORT not set, setting to 50957\n");
	 envServerPort = "57825";
	 }
	 
	 int hostPort = atoi(envServerPort);
	 */
	
	strcpy(hostname, addr);
	strcpy(port, p);
	
	//printf("SERVER_ADDRESS %s\nSERVER_PORT %d\n", hostname, port);
}

// 
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

int Connection::createListener() {
	
	// get hostname, resolve it, and print server address
	
	const char * envServerAddr = getenv("HOSTNAME");
	
	if(envServerAddr == NULL) {
		printf("HOSTNAME not set, setting to localhost\n");
		envServerAddr = "localhost";
	}
	
	char hostname[255];
	
	strcpy(hostname, envServerAddr);
	
	struct hostent *host;
	struct in_addr h_addr;
	
	if ((host = gethostbyname(hostname)) == NULL) {
		fprintf(stderr, "(mini) nslookup failed on '%s'\n", hostname);
		return -1;
	}
	h_addr.s_addr = *((unsigned long *) host->h_addr_list[0]);
	
	printf("SERVER_ADDRESS %s\n", inet_ntoa(h_addr));
	
	// bind socket
	
	int serverSocket;
	
	FD_ZERO(&master);
	FD_ZERO(&read_fds);
	
	struct addrinfo hints, *ai, *p;
	int yes = 1;
	int rv;
	
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	
	if ((rv = getaddrinfo(hostname, INADDR_ANY, &hints, &ai)) != 0) {
		printf("getaddrinfo failed: %s\n", gai_strerror(rv));
		return -1;
	}
	
	for(p = ai; p != NULL; p = p->ai_next) {
    	serverSocket = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (serverSocket < 0) { 
			continue;
		}
		
		// avoid "address already in use" error message
		setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
		
		if (bind(serverSocket, p->ai_addr, p->ai_addrlen) < 0) {
			close(serverSocket);
			continue;
		}
		
		break;
	}
	
	// print port number
	
	struct sockaddr_in sa;
	socklen_t sa_len = sizeof(sa);
	
	if(getsockname(serverSocket, (struct sockaddr *) &sa, &sa_len) < 0) {
		printf("getsockname error, couldn't get port number\n");
		return -1;
	}
	
	printf("SERVER_PORT %d\n", (int) ntohs(sa.sin_port));
	
	if (p == NULL) {
		// if we got here, it means we didn't get bound
		fprintf(stderr, "selectserver: failed to bind\n");
		exit(2);
	}
	
	freeaddrinfo(ai); // all done with this
	
	return serverSocket;
}

void Connection::listen(int serverSocket) {
	
	// establish listen queue
    if(listen(serverSocket, 10) == SOCKET_ERROR) {
        printf("\nCould not listen\n");
        return -1;
    }
	
	// add the socket to the master set
	FD_SET(serverSocket, &master);
	
	// keep track of the biggest file descriptor
	int fdmax = serverSocket;
	
	int newfd; // newly accepted socket descriptor
	struct sockaddr_storage remoteaddr; // client address
	socklen_t addrlen;
	
	for(;;) {
		read_fds = master; // copy it
		
		if(select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
			printf("select failed\n");
			return -1; 
		}
		
		// run through existing conns looking for data to read
		for(int i = 0; i <= fdmax; i++) {
			if(FD_ISSET(i, &read_fds)) {
				// found a conn
				if (i == serverSocket) {
					// handle new connections
					addrlen = sizeof remoteaddr;
					
					newfd = accept(serverSocket,
								   (struct sockaddr *)&remoteaddr,
								   &addrlen);
					
					if(newfd == 1) {
						printf("accept failed\n");
						return -1;
					}
					else {
						FD_SET(newfd, &master);
						if(newfd > fdmax) {
							fdmax = newfd;
						}
					}
				}
				else {
					// handle data from a client
					Message req;
					int result = req.readSocket(i);
					
					if(result <= 0) {
                        close(i);
                        FD_CLR(i, &master); // remove from master set
					} 
					else {
						
						// TODO: threading
						
						switch (req.type) {
							case M_EXECUTE:
								handleExecute(i, &req);
								break;
								
							case M_TERMINATE:
								handleTerminate(i, &req);
								break;
								
							default:
								printf("Invalid message type %d\n", req.type);
								break;
						}
						
						// close socket
						if(close(i) == SOCKET_ERROR) {
							printf("\nCould not close socket\n");
							return -1;
						}
						
						FD_CLR(i, &master);							
					}
				}
			}
		} 
	}
	
	// shouldn't reach this!
	close(serverSocket);
	return 0;
}
