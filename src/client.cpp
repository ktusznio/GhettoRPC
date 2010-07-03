#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <iostream>
#include <string>

#include "include/client.h"
#include "include/message.h"
#include "include/rpc.h"

using namespace std;

int Client::run() {
	int a = 5;
	int b = 10;
	int count = 3;
	long ret;
	int argTypes[count + 1];
	void **args;
	
	// argTypes[i]:
	// io000000 tttttttt llllllll llllllll
	
	argTypes[0] = (1 << ARG_OUTPUT) | (ARG_LONG << 16) | 255;
	argTypes[1] = (1 << ARG_INPUT) | (ARG_INT << 16);
	argTypes[2] = (1 << ARG_INPUT) | (ARG_INT << 16);
	argTypes[3] = 0;
	
	args = (void **)malloc(count * sizeof(void *));
	args[0] = (void *)&ret;
	args[1] = (void *)&a;
	args[2] = (void *)&b;
	
	int s = rpcCall("f0", argTypes, args);
	
	printf("rpcCall returned %d\n", s);
	
	printf("a=%d, b=%d, ret=%ld\n", a, b, ret);
	
	return 0;
}

int main(int argc, char* argv[]) {

	Client c;
	c.run();
	
}