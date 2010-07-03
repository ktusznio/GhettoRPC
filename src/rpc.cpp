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

#include "include/rpc.h"
#include "include/message.h"
#include "include/connection.h"

#define HOST_NAME_SIZE      255
#define SOCKET_ERROR        -1

// TODO: get rid of PORT_NUM
#define PORT_NUM "38447"
#define	SERVER_PORT	"53797"

using namespace std;

int connectToBinder() {
	Connection conn("127.0.0.1", PORT_NUM);
	return conn.create();
}

int rpcCall(const char * name, int * argTypes, void ** args) {
	
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
	
	int iSocket = connectToBinder();
	
	//TODO map skeleton function to server procedure
	//
	//
	
	// send message
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
	return 0;
}

int rpcTerminate() {
	return 0;
}
