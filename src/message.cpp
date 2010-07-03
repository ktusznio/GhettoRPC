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

Message::Message(const char * src) {
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
	
	if (pArgTypes != NULL) {
		while (*pArgTypes != 0) {
			argTypes.push_back(*pArgTypes);
			pArgTypes++;
		}
	}
}

Message::Message(int t, const char * name, int * pArgTypes, void ** pArgs) {
	
	Message(t, name, pArgTypes);
	
	if (pArgs != NULL) {
		for (int i = 0; i < argTypes.size(); i++) {
			args.push_back(*pArgs);
			pArgs++;
		}
	}
}

void Message::parse() {
	// split message by \n
	
	char * p = strtok(buffer, "\n");
	
	if (p == NULL) {
		printf("Message::parse: could not read message type\n");
		return;
	}
	
	printf("received message of type %s\n", p);
	
	type = atoi(p);
	
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
		printf("message length: %d\n", length);
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
		
		// clear out existing stuff since we're gonna read new stuff in
		// TODO: clear void * from args properly
		argTypes.clear();
		args.clear();
		
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
		
		printf("wrote %d to socket %d\n", (int) wrote, s );
		
		return (int) wrote;
	}
	else {
		// TODO: manually write send failure message
		printf("Message::writeSocket: failed to fill buffer\n");
		return 0;
	}
}

void Message::unmarshall(void ** a) {

	void ** p = a;
	
	for (int i = 0; i < args.size(); i++) {
		
		int iType = (argTypes[i] >> 16) & 0xFF;
		
		switch (iType) {
			case ARG_CHAR:
				*((char *) *p) = *((char *) args[i]);
				break;
			case ARG_SHORT:
				*((short *) *p) = *((short *) args[i]);				
				break;
			case ARG_INT:
				*((int *) *p) = *((int *) args[i]);
				break; 
			case ARG_LONG:
				*((long *) *p) = *((long *) args[i]);
				break;
			case ARG_DOUBLE:
				*((double *) *p) = *((double *) args[i]);
				break;
			case ARG_FLOAT:
				*((float *) *p) = *((float *) args[i]);
				break;
			default:
				printf("rpcCall: invalid arg type %d during unmarshalling\n", iType);
				break;
		}
		
		p++;
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
	
	// write stream to buffer
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
	
	// write stream to buffer
	memset(&buffer, 0, BUFFER_SIZE);
	string str = os.str();
	sprintf(buffer, "%010d\n%s", (int) str.size(), str.c_str());
	
	return 0;	
}

int Message::fillTypeOnly(){
	ostringstream os;
	
	typeToStream(&os);
	
	// write stream to buffer
	memset(&buffer, 0, BUFFER_SIZE);
	string str = os.str();
	sprintf(buffer, "%010d\n%s", (int) str.size(), str.c_str());
	
	return 0;	
}

int Message::fillExecuteFailureBuffer() {
	return 0;
}

int Message::fillTerminateBuffer() {
	return 0;
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
	printf("proc name: %s\n", procName);
	
	p = strtok(NULL, "\n");
	
	return p;
}

char * Message::readArgTypes(char * p) {
	if (p == NULL) {
		printf("Message::readArgTypes: could not read proc name\n");
		return NULL;
	}
	
	// populate argTypes vector
	int argType;
	while ((argType = atoi(p)) != 0) {
		
		printf("read argType %d\n", argType);
		
		argTypes.push_back(argType);
		p = strtok(NULL, "\n");
	}
	
	p = strtok(NULL, "\n"); // skip argType-ending zero

	return p;
}

char * Message::readArgs(char * p) {
	if (p == NULL) {
		printf("Message::readArgs: could not read args\n");
		return NULL;
	}
	
	// populate args vector
	for (int i = 0; i < argTypes.size(); i++) {
		void * pArg;
		int iType = (argTypes[i] >> 16) & 0xFF;
		
		switch (iType) {
			case ARG_CHAR:
				pArg = malloc(sizeof(char));
				sprintf((char *) pArg, "%c", *p);
				break;
			case ARG_SHORT:
				pArg = malloc(sizeof(short));
				*((short *) pArg) = atoi(p);
				break;
			case ARG_INT:
				pArg = malloc(sizeof(int));
				*((int *) pArg) = atoi(p);
				break;
			case ARG_LONG:
				pArg = malloc(sizeof(long));
				*((long *) pArg) = atol(p);
				break;
			case ARG_DOUBLE:
				pArg = malloc(sizeof(double));
				*((double *) pArg) = atof(p);
				break;
			case ARG_FLOAT:
				pArg = malloc(sizeof(float));
				*((float *) pArg) = strtod(p, NULL);
				break;
			default:
				printf("Message::parseExecuteMsg: invalid arg type %d\n", iType);
				return NULL;
				break;
		}
		
		args.push_back(pArg);
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
	for(int i = 0; i <= argTypes.size(); i++) {
		*os << argTypes[i] << endl;
	}
}

void Message::argsToStream(ostringstream * os) {
	
	for (int i = 0; i < args.size(); i++) {
		
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
	printf("Message: (type = %d)\n%s\n", type, buffer);
	printArgTypes();
	printArgs();
}

void Message::printArgTypes() {
	printf("argTypes:\n");
	for (int i = 0; i < argTypes.size(); i++) {
		printf("\t%d: %d\n", i, argTypes.at(i));
	}
}

void Message::printArgs() {
	printf("args:\n");
	for (int i = 0; i < args.size(); i++) {
		
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

// argTypes[i]: io000000 tttttttt llllllll llllllll

/*
 int in, out, type, len;
 
 in = (argTypes[i] >> ARG_INPUT) & 1;
 out = (argTypes[i] >> ARG_OUTPUT) & 1;
 type = (argTypes[i] >> 16) & 0xFF;
 len = argTypes[i] & 0xFFFF;
 
 printf("in=%d, out=%d, type=%d, length=%d\n", in, out, type, len);
 */
