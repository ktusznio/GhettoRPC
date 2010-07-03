#include <sys/socket.h>
#include <vector>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <arpa/inet.h>
#include <string.h>
#include <iostream>

#include "include/rpc.h"
#include "include/message.h"

using namespace std;

Message::~Message() {
	clearArgTypes();
	clearArgs();
}

Message::Message() {
	argTypes = NULL;
	args = NULL;
}

Message::Message(const char * src) {
	argTypes = NULL;
	args = NULL;
	memset(&buffer, 0, BUFFER_SIZE);
	memcpy(&buffer, src, BUFFER_SIZE);
	parse();
}

Message::Message(int t, const char * name, int * pArgTypes) {
	
	type = t;
	
	if (name != NULL) {
		// copy name
		procName = (char *) malloc(strlen(name));
		strcpy(procName, name);	
	}
	
	copyArgTypes(pArgTypes);
	
	args = NULL;
	numArgs = sizeof(*argTypes) - 1;
}

Message::Message(int t, const char * name, int * pArgTypes, void ** pArgs) {
	
	type = t;
	
	if (name != NULL) {
		// copy name
		procName = (char *) malloc(strlen(name));
		strcpy(procName, name);	
	}
	
	copyArgTypes(pArgTypes);
	numArgs = sizeof(*argTypes) - 1;
	copyArgs(pArgs);
}

void Message::copyArgTypes(int * pArgTypes) {
	argTypes = (int *) malloc(sizeof(int) * sizeof(*pArgTypes));
	memcpy(argTypes, pArgTypes, sizeof(int) * sizeof(*pArgTypes));
}

void Message::copyArgs(void ** pArgs) {
	
	if (argTypes == NULL) {
		printf("Message::copyArgs: arg types need to be copied first\n");
		return;
	}
	
	// create space for pointers that will point to copied args
	args = (void **) malloc(numArgs * sizeof(void *));
	
	// now copy each individual argument and set the pointers
	for (int i = 0; i < numArgs; i++) {
		
		int iType = (argTypes[i] >> 16) & 0xFF;
		
		void * q; // the pointer to an arg that will be set
		
		switch (iType) {
			case ARG_CHAR:
				// allocate space for arg and copy it
				q = malloc(sizeof(char));
				memcpy(q, pArgs[i], sizeof(char));
				break;
			case ARG_SHORT:
				q = malloc(sizeof(short));
				memcpy(q, pArgs[i], sizeof(short));
				break;
			case ARG_INT:
				q = malloc(sizeof(int));
				memcpy(q, pArgs[i], sizeof(int));
				break; 
			case ARG_LONG:
				q = malloc(sizeof(long));
				memcpy(q, pArgs[i], sizeof(long));
				break;
			case ARG_DOUBLE:
				q = malloc(sizeof(double));
				memcpy(q, pArgs[i], sizeof(double));
				break;
			case ARG_FLOAT:
				q = malloc(sizeof(float));
				memcpy(q, pArgs[i], sizeof(float));
				break;
			default:
				printf("Message::copyArgs: invalid arg type %d\n", iType);
				break;
		}
		
		// set arg pointer in pointer array
		args[i] = q;
	}
}

void Message::clearArgTypes() {
	free(argTypes);
	argTypes = NULL;
}

void Message::clearArgs() {
	
	if (args != NULL) {
		for (int i = 0; i < numArgs; i++) {
			free(args[i]);
			args[i] = NULL;
		}
		
		free(args);
		args = NULL;		
		numArgs = 0;
	}
}

void Message::parse() {
	
	// split message by \n
	char * p = strtok(buffer, "\n");
	
	if (p == NULL) {
		printf("Message::parse: could not read message type\n");
		return;
	}
	
	type = atoi(p); // set message type
	p = strtok(NULL, "\n"); // advance tokenizer
	
	// handle different message types
	
	int parseResult;
	
	switch (type) {
		case M_REGISTER:
			parseResult = parseRegister(p);
			break;
		case M_EXECUTE:
			parseResult = parseExecute(p);
			break;
		case M_EXECUTE_SUCCESS:
			parseResult = parseExecuteSuccess(p);
			break;
		case M_EXECUTE_FAILURE:
			parseResult = parseExecuteFailure(p);
			break;
		case M_REGISTER_SUCCESS:
		case M_REGISTER_FAILURE:
			parseResult = 0; //TODO
			break;
		case M_TERMINATE:
			parseResult = 0; // nothing to do here
			break;
		default:
			parseResult = -1;
			printf("Message::parse: invalid message type");
			break;
	}
	
	if (parseResult != 0) {
		printf("Message::readSocket: error parsing message: %d\n", parseResult);
	}
	
	// TODO: return error message on parse failure?
}

int Message::readSocket(int s) {

	int nbytes;
	char lenBuffer[11];
	
	// receive message length
	nbytes = recv(s, lenBuffer, 11, 0);
	
	if(nbytes == 11) {
		length = atoi(lenBuffer);
	}
	else {
		printf("Message::readSocket: error reading message length\n");
	}
	
	// receive rest of message
	if((nbytes = recv(s, buffer, length, 0)) <= 0) {
		if (nbytes == 0) {
			// connection closed
			printf("readSocket: socket %d hung up\n", s);
			return nbytes;
		} 
		else {
			printf("readSocket: recv error on socket %d\n", s);
			return nbytes;
		}
	} 
	else {
		// clear out existing stuff and parse new stuff
		clearArgTypes();
		clearArgs();
		parse();
	}
	
	return nbytes;
}

int Message::writeSocket(int s) {
	
	int fillResult;
	
	switch (type) {
		case M_REGISTER:
			fillResult = fillRegisterBuffer();
			break;
		case M_EXECUTE:
			fillResult = fillExecuteBuffer();
			break;
		case M_EXECUTE_SUCCESS:
			fillResult = fillExecuteSuccessBuffer();
			break;
		case M_EXECUTE_FAILURE:
			fillResult = fillExecuteFailureBuffer();
			break;
		case M_TERMINATE:
			fillResult = fillTerminateBuffer();
			break;
		case M_REGISTER_SUCCESS:
		case M_REGISTER_FAILURE:
			fillResult = fillTypeOnly();
			break;		
		default:
			printf("Message::writeSocket: invalid message type\n");
			fillResult = -1;
			break;
	}
	
	if (fillResult == 0) {
		
		printf("Message::writeSocket: writing buffer to socket %d\n%s\n", s, buffer);
		
		size_t wrote = send(s, &buffer, strlen(buffer), 0);
		
		printf("Message::writeSocket: wrote %d to socket %d\n", (int) wrote, s );
		
		return (int) wrote;
	}
	else {
		// TODO: manually write send failure message
		printf("Message::writeSocket: failed to fill buffer\n");
		return 0;
	}
}

void Message::unmarshall(void ** a) {

	for (int i = 0; i < numArgs; i++) {
		
		int iType = (argTypes[i] >> 16) & 0xFF;
		
		switch (iType) {
			case ARG_CHAR:
				*((char *) a[i]) = *((char *) args[i]);
				break;
			case ARG_SHORT:
				*((short *) a[i]) = *((short *) args[i]);				
				break;
			case ARG_INT:
				*((int *) a[i]) = *((int *) args[i]);
				break; 
			case ARG_LONG:
				*((long *) a[i]) = *((long *) args[i]);
				break;
			case ARG_DOUBLE:
				*((double *) a[i]) = *((double *) args[i]);
				break;
			case ARG_FLOAT:
				*((float *) a[i]) = *((float *) args[i]);
				break;
			default:
				printf("rpcCall: invalid arg type %d during unmarshalling\n", iType);
				break;
		}
	}
}

int Message::fillRegisterBuffer() {
	ostringstream os;
	
	typeToStream(&os);
	procNameToStream(&os);
	argTypesToStream(&os);
	
	// write stream to buffer
	memset(&buffer, 0, BUFFER_SIZE);
	string str = os.str();
	sprintf(buffer, "%010d\n%s", (int) str.size(), str.c_str());
	
	return 0;
}

int Message::fillExecuteBuffer() {
	ostringstream os;
	
	typeToStream(&os);
	procNameToStream(&os);
	argTypesToStream(&os);
	argsToStream(&os);
	
	memset(&buffer, 0, BUFFER_SIZE);
	string str = os.str();
	sprintf(buffer, "%010d\n%s", (int) str.size(), str.c_str());
	
	return 0;
}

int Message::fillExecuteSuccessBuffer() {
	ostringstream os;
	
	typeToStream(&os);
	procNameToStream(&os);
	argTypesToStream(&os);
	argsToStream(&os);
	
	memset(&buffer, 0, BUFFER_SIZE);
	string str = os.str();
	sprintf(buffer, "%010d\n%s", (int) str.size(), str.c_str());
	
	return 0;	
}

int Message::fillTypeOnly(){
	ostringstream os;
	
	typeToStream(&os);
	
	memset(&buffer, 0, BUFFER_SIZE);
	string str = os.str();
	sprintf(buffer, "%010d\n%s", (int) str.size(), str.c_str());
	
	return 0;	
}

int Message::fillExecuteFailureBuffer() {
	return 0; // TODO
}

int Message::fillTerminateBuffer() {
	return 0; // TODO
}

int Message::parseRegister(char * p) {
	p = readProcName(p);
	p = readArgTypes(p);
	
	printArgTypes();
	
	return 0;
}

int Message::parseExecute(char * p) {
	p = readProcName(p);
	p = readArgTypes(p);
	p = readArgs(p);
	
	printArgTypes();
	printArgs();
	
	return 0;
}

int Message::parseExecuteSuccess(char * p) {
	p = readProcName(p);
	p = readArgTypes(p);
	p = readArgs(p);
	
	printArgTypes();
	printArgs();
	
	return 0;
}

int Message::parseExecuteFailure(char * p) {
	if (p == NULL) {
		printf("Message::parseExecuteFailure: error reading reason code\n");
		return M_READ_REASONCODE_ERROR;
	}
	
	// read reason code
	reasonCode = atoi(p);
	
	return 0;
}

char * Message::readProcName(char * p) {
	if (p == NULL) {
		printf("Message::readProcName: could not read proc name\n");
		return NULL;
	}
	
	procName = (char *) malloc(strlen(p));
	strcpy(procName, p);
	
	p = strtok(NULL, "\n");
	return p;
}

char * Message::readArgTypes(char * p) {
	if (p == NULL) {
		printf("Message::readArgTypes: could not read proc name\n");
		return NULL;
	}
	
	clearArgTypes();
	
	int argType;
	vector<int> temp;
	
	// read arg types into a vector since we don't know how many there are
	do {
		argType = atoi(p);
		temp.push_back(argType);
		p = strtok(NULL, "\n");
	} while (argType != 0);
	
	// create argTypes array
	argTypes = (int *) malloc(temp.size() * sizeof(int));
	
	for (int i = 0; i < temp.size(); i++) {
		argTypes[i] = temp[i];
	}
	
	numArgs = temp.size() - 1;
	
	temp.clear();
	return p;
}

char * Message::readArgs(char * p) {
	if (p == NULL) {
		printf("Message::readArgs: could not read args\n");
		return NULL;
	}
	
	clearArgs();
	
	args = (void **) malloc(numArgs * sizeof(void *));
	
	// populate args vector
	for (int i = 0; i < numArgs; i++) {
		void * pArg;
		int iType = (argTypes[i] >> 16) & 0xFF;
		
		switch (iType) {
			case ARG_CHAR:
				args[i] = malloc(sizeof(char));
				sprintf((char *) args[i], "%c", *p);
				break;
			case ARG_SHORT:
				args[i] = malloc(sizeof(short));
				*((short *) args[i]) = atoi(p);
				break;
			case ARG_INT:
				args[i] = malloc(sizeof(int));
				*((int *) args[i]) = atoi(p);
				break;
			case ARG_LONG:
				args[i] = malloc(sizeof(long));
				*((long *) args[i]) = atol(p);
				break;
			case ARG_DOUBLE:
				args[i] = malloc(sizeof(double));
				*((double *) args[i]) = atof(p);
				break;
			case ARG_FLOAT:
				args[i] = malloc(sizeof(float));
				*((float *) args[i]) = strtod(p, NULL);
				break;
			default:
				printf("Message::parseExecuteMsg: invalid arg type %d\n", iType);
				return NULL;
				break;
		}
		
		p = strtok(NULL, "\n");
	}
	
	return p;
}

void Message::procNameToStream(ostringstream * os) {
	*os << procName << endl;
}

void Message::typeToStream(ostringstream * os) {
	*os << type << endl;
}

void Message::argTypesToStream(ostringstream * os) {
	for(int i = 0; i <= numArgs; i++) {
		*os << argTypes[i] << endl;
	}
}

void Message::argsToStream(ostringstream * os) {
	
	for (int i = 0; i < numArgs; i++) {
		
		int argType = (argTypes[i] >> 16) & 0xFF;
		
		// TODO: test that all types write correctly
		
		switch (argType) {
			case ARG_CHAR:
				*os << (* ((char *) args[i])) << endl;
				break;
				
			case ARG_SHORT:
				*os << (* ((short *) args[i])) << endl;
				break;
				
			case ARG_INT:
				*os << (* ((int *) args[i])) << endl;
				break;
				
			case ARG_LONG:
				*os << (* ((long *) args[i])) << endl;
				break;
				
			case ARG_DOUBLE:
				*os << (* ((double *) args[i])) << endl;
				break;
				
			case ARG_FLOAT:
				*os << (* ((float *) args[i])) << endl;
				break;
				
			default:
				printf("Message::argsToStream: arg %d has invalid type %d\n", i, argType);
				break;
		}
	}
}

void Message::print() {
	printf("Message: (type = %d)\n", type);
	printArgTypes();
	printArgs();
}

void Message::printArgTypes() {
	printf("argTypes:\n");
	for (int i = 0; i <= numArgs; i++) {
		printf("\t%d: %d\n", i, argTypes[i]);
	}
}

void Message::printArgs() {
	if (args == NULL) {
		printf("args: none\n");
		return;
	}
	
	printf("args:\n");
	
	for (int i = 0; i < numArgs; i++) {
		
		int iType = (argTypes[i] >> 16) & 0xFF;
		
		switch (iType) {
			case ARG_CHAR:
				printf("\t%d: CHAR %c\n", i, *((char *) args[i]));
				break;
			case ARG_SHORT:
				printf("\t%d: SHORT %hd\n", i, *((short *) args[i]));
				break;
			case ARG_INT:
				printf("\t%d: INT %d\n", i, *((int *) args[i]));
				break; 
			case ARG_LONG:
				printf("\t%d: LONG %ld\n", i, *((long *) args[i]));
				break;
			case ARG_DOUBLE:
				printf("\t%d: DOUBLE %f\n", i, *((double *)args[i]));
				break;
			case ARG_FLOAT:
				printf("\t%d: FLOAT %f\n", i, *((float *)args[i]));
				break;
			default:
				printf("Message::parseExecuteMsg: invalid arg type %d\n", iType);
				break;
		}
	}
}

/*
 int in, out, type, len;
 
 in = (argTypes[i] >> ARG_INPUT) & 1;
 out = (argTypes[i] >> ARG_OUTPUT) & 1;
 type = (argTypes[i] >> 16) & 0xFF;
 len = argTypes[i] & 0xFFFF;
 
 printf("in=%d, out=%d, type=%d, length=%d\n", in, out, type, len);
 */
