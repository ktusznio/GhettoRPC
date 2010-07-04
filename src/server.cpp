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
#include <map>

#include "include/server.h"
#include "include/server_function_skels.h"
#include "include/rpc.h"

using namespace std;

int main(int argc, char* argv[]) {
	
	int argTypes[4];
	argTypes[0] = (1 << ARG_OUTPUT) | (ARG_LONG << 16) | 255;
	argTypes[1] = (1 << ARG_INPUT) | (ARG_INT << 16);
	argTypes[2] = (1 << ARG_INPUT) | (ARG_INT << 16);
	argTypes[3] = 0;
	
	rpcRegister("f0", argTypes, f0_Skel);
	
	rpcExecute();
}
