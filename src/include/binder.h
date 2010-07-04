#ifndef _binder_included
#define _binder_included

#include <map>
#include <queue>
#include <string>

#include "message.h"

class Binder {
private:
	//key: function signature , value: socket number
	std::map<std::string, std::queue<std::string>* > registrations;
	
	std::string messageToSignature(Message * m);
	std::string socketToAddress(int s);
	
	
	void printRegistration(); //for debugging
	
	void handleExecute(int s, Message * m);
	void handleTerminate(int s, Message * m);
	void handleRegister(int s, Message * m);
	
public:
	
	Binder();
	int run();
	
};

#endif
