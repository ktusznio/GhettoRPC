#ifndef _server_included
#define _server_included

#define S_F0 "f0"
#define S_F1 "f1"
#define S_F2 "f2"
#define S_F3 "f3"
// TODO: f4-f6?

#include <map>
#include <string>
#include <vector>

#include "message.h"

typedef int (*pProcSkel)(std::vector<int> *, std::vector<void *> *);

class Server {
private:
	void handleExecute(int s, Message * m);
	void handleTerminate(int s, Message * m);

public:
	
	std::map<std::string, pProcSkel> procMap;
	
	Server();
	
	int run();
	
};

#endif