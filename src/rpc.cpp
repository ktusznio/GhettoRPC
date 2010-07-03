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
#define PORT_NUM "50605"

using namespace std;

int rpcCall(const char * name, int * argTypes, void ** args) {
	
	// set up socket connection
	Connection conn("127.0.0.1", PORT_NUM);
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

int rpcRegister(char* name, int* argTypes, function f) {
	return 0;
}

int rpcExecute() {
	return 0;
}

int rpcTerminate() {
	return 0;
}
