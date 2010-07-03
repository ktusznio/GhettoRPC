/*
 * rpc.h
 *
 * This file defines all of the rpc related infomation.
 */
 
#define ARG_CHAR    1
#define ARG_SHORT   2
#define ARG_INT     3
#define ARG_LONG    4
#define ARG_DOUBLE  5
#define ARG_FLOAT   6

#define ARG_INPUT   31
#define ARG_OUTPUT  30

typedef int (*function)(int *, void **);

int rpcCall(const char* name, int* argTypes, void** args);
int rpcRegister(const char* name, int* argTypes, function f);
int rpcExecute();
int rpcTerminate();