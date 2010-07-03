#ifndef _connection_included
#define _connection_included

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define HOST_NAME_SIZE      255
#define SOCKET_ERROR        -1

class Connection {
	
public:
	int iSocket;
	char hostname[HOST_NAME_SIZE];
	char port[6];
	struct addrinfo hints;
	struct addrinfo * servInfo;
	
	Connection(const char * addr, const char * port);
	
	int create();
};

#endif