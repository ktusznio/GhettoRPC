#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string>

#include "include/binder.h"
#include "include/message.h"
#include "include/connection.h"

using namespace std;

Binder::Binder() {
}


void Binder::printRegistration() {
	
	ostringstream os;
	
	map<string, queue<string>* >::iterator it;
	
	for ( it=registrations.begin() ; it != registrations.end(); it++ )
	{
		os << (*it).first << " => ";
	    
	    queue<string> serverList = *((*it).second);
		
	    while(!serverList.empty())
		{
			os << serverList.front() << ", ";
			serverList.pop();
		}
	    
	    os << endl;
	}

	printf("Registrations:\n%s", os.str().c_str());
}

string Binder::messageToSignature(Message * m) {
	ostringstream os;
	
	os << m->procName << "|";
	
	for(int i = 0; i < m->numArgs; i++ ) {
		os << m->argTypes[i];
	}
	
	return os.str();
}

string Binder::socketToAddress(int s) {
	ostringstream os;
	
	struct sockaddr_in sa;
	socklen_t sa_len = sizeof(sa);
	
	if(getpeername(s, (struct sockaddr *) &sa, &sa_len) < 0) {
		printf("getsockname error, couldn't get port number\n");
	}

	os << inet_ntoa(sa.sin_addr) << "|" << ntohs(sa.sin_port);

	return os.str();
}

void Binder::handleTerminate(int s, Message * m) {	
}

void Binder::handleRegister(int s, Message * m) {
	printf("registering in binder\n");

	string signature = messageToSignature(m);
	printf("Function signature: %s\n", signature.c_str());
	
	string address = socketToAddress(s);
	printf("Server address: %s\n", address.c_str());

	
	//Search for the registration
	map<string, queue<string>* >::iterator it;

	it = registrations.find(signature);
	
	if(it == registrations.end())
	{
		//Need to add function signature
		
		queue<string>* serverList = new queue<string>();
		serverList->push(address);
		registrations.insert( pair<string,queue<string>* >(signature, serverList));
	}
	else
	{
		//Already have a mapping, add server to list
		
		//TODO need to make sure server isn't already on the list
		it->second->push(address);
	}
	
	printRegistration();
	
	//TODO  failure case?
	m->type = M_REGISTER_SUCCESS;
	m->writeSocket(s);
}

void Binder::handleExecute(int s, Message * m) {
	
}

int Binder::run() {
	
	// TODO: break this up so it can be reused in binder

	fd_set master, // master file descriptor list
	read_fds; // temp file descriptor list for select()
	int fdmax; // maximum file descriptor number
	int newfd; // newly accepted socket descriptor
	
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
	
	printf("BINDER_ADDRESS %s\n", inet_ntoa(h_addr));
	
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
	if ((rv = getaddrinfo(hostname, "38447", &hints, &ai)) != 0) {
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
	
	printf("BINDER_PORT %d\n", (int) ntohs(sa.sin_port));
	
	if (p == NULL) {
		// if we got here, it means we didn't get bound
		fprintf(stderr, "selectserver: failed to bind\n");
		exit(2);
	}
	
	freeaddrinfo(ai); // all done with this
	
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
								
							case M_REGISTER:
								handleRegister(i, &req);
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

int main(int argc, char* argv[]) {
	Binder b;
	b.run();
}
