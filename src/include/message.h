#ifndef _message_included
#define _message_included

#include <string>
#include <vector>
#include <sstream>

#define BUFFER_SIZE         1024

#define M_REGISTER			1
#define M_EXECUTE			2
#define M_EXECUTE_SUCCESS	3
#define M_EXECUTE_FAILURE	4
#define M_TERMINATE			5
#define M_REGISTER_SUCCESS	6
#define M_REGISTER_FAILURE	7

#define M_READ_PROCNAME_ERROR	1
#define M_READ_ARGTYPES_ERROR	2
#define M_READ_ARGS_ERROR		3
#define M_READ_REASONCODE_ERROR	4
#define M_READ_LENGTH_ERROR		5
#define M_READ_TYPE_ERROR		6

class Message {
private:
	void parse();
	int parseRegister(char * p);
	int parseExecute(char * p);
	int parseExecuteSuccess(char * p);
	int parseExecuteFailure(char * p);
	int parseTerminate(char * p);
	
	int fillTypeOnly();
	int fillRegisterBuffer();
	int fillExecuteBuffer();
	int fillExecuteSuccessBuffer();
	int fillExecuteFailureBuffer();
	int fillTerminateBuffer();
	
	char * readProcName(char * p);
	char * readArgTypes(char * p);
	char * readArgs(char * p);
	
	void procNameToStream(std::ostringstream * os);
	void typeToStream(std::ostringstream * os);
	void argTypesToStream(std::ostringstream * os);
	void argsToStream(std::ostringstream * os);
	
public:
	int length;
	int type;
	char buffer[BUFFER_SIZE];
	
	// register
	char * ip;
	
	// execute
	char * procName;
	std::vector<int> argTypes;
	std::vector<void *> args;
	
	// execute failure
	int reasonCode;
	
	Message(){};
	Message(const char * src);
	Message(int t, const char * name, int * argTypes);
	Message(int t, const char * name, int * argTypes, void ** args);
	
	
	int readSocket(int s);
	int writeSocket(int s);
	
	void unmarshall(void ** a);
	
	void print();
	void printArgTypes();
	void printArgs();
};

// TODO: (optional) subclass different message types

#endif
