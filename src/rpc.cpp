#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <sstream>
#include <map>

#include "include/rpc.h"
#include "include/message.h"
#include "include/connection.h"
#include "include/handlers.h"

#define HOST_NAME_SIZE      255
#define SOCKET_ERROR        -1

// TODO: remove hard-coded ports
#define BINDER_PORT "38447"
#define SERVER_PORT "58181"

using namespace std;

// globals
string serverAddr; 
int serverPort;
int serverSocket;

map<string, function> procMap;

int rpcCall(const char * name, int * argTypes, void ** args) {
	
	// TODO: connect to binder to get server info
	
	// connect to server
	Connection conn("127.0.0.1", SERVER_PORT);
	int iSocket = conn.create();
	
	// send message
	Message m(M_EXECUTE, name, argTypes, args);	
	int wrote = m.writeSocket(iSocket);
	
	// read response
	m.readSocket(iSocket);
	
	close(iSocket);
	
	// unmarshall and set results
	m.unmarshall(args);
	
	return 0;
}

int rpcRegister(const char* name, int* argTypes, function f) {
	
	Connection conn("127.0.0.1", BINDER_PORT);
	int iSocket = conn.create();
	
	procMap[name] = f;
	
	// send message	
	// TODO: send over serverAddr, serverPort
	
	printf("serverAddr=%s, serverPort=%d\n", serverAddr.c_str(), serverPort);
	
	Message m(M_REGISTER, name, argTypes);
	printf("in rpcRegister...\n");
	m.print();
	int wrote = m.writeSocket(iSocket);
	
	// read response
	m.readSocket(iSocket);
		
	if(m.type == M_REGISTER_SUCCESS)
	{
		printf("Registration successful\n");
	}
	else
	{
		printf("Registration failed\n");
	}
	
	close(iSocket);
	
	// unmarshall and set results
	//m.unmarshall(args);
	
	return 0;
}

int rpcExecute() {
	
	// get hostname, resolve it, and print server address
	const char * envServerAddr = getenv("HOSTNAME");
	
	if(envServerAddr == NULL) {
		printf("HOSTNAME not set, setting to localhost\n");
		envServerAddr = "localhost";
	}
	
	struct hostent *host;
	struct in_addr h_addr;
	
	if ((host = gethostbyname(envServerAddr)) == NULL) {
		fprintf(stderr, "(mini) nslookup failed on '%s'\n", envServerAddr);
		return -1;
	}
	h_addr.s_addr = *((unsigned long *) host->h_addr_list[0]);
	
	serverAddr = inet_ntoa(h_addr);
	printf("SERVER_ADDRESS %s\n", serverAddr.c_str());
	
	// bind socket
	
	struct addrinfo hints, *ai, *p;
	int yes = 1;
	int rv;
	
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	if ((rv = getaddrinfo(envServerAddr, INADDR_ANY, &hints, &ai)) != 0) {
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
	
	serverPort = (int) ntohs(sa.sin_port);
	printf("SERVER_PORT %d\n", serverPort);
	
	if (p == NULL) {
		// if we got here, it means we didn't get bound
		fprintf(stderr, "selectserver: failed to bind\n");
		exit(2);
	}
	
	freeaddrinfo(ai); // all done with this
	
	fd_set master; // master file descriptor list
	fd_set read_fds; // temp file descriptor list for select()
	int fdmax; // maximum file descriptor number
	int newfd; // newly accepted socket descriptor
	
	FD_ZERO(&master);
	FD_ZERO(&read_fds);
	
	// establish listen queue
    if(listen(serverSocket, 10) == SOCKET_ERROR) {
        printf("\nCould not listen\n");
        return -1;
    }
	
	// add the socket to the master set
	FD_SET(serverSocket, &master);
	
	// keep track of the biggest file descriptor
	fdmax = serverSocket;
	
	char buffer[BUFFER_SIZE];
	int clientSocket;
	
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

int rpcTerminate() {
	// TODO
	return 0;
}
