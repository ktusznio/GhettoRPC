#include <map>

#include "include/handlers.h"
#include "include/rpc.h"

using namespace std;

extern map<string, function> procMap;

void handleTerminate(int s, Message * m) {
	// TODO
}

void handleExecute(int s, Message * m) {
	
	printf("EXECUTE %s\n", m->procName);
	
	int ret;
	int cmp;
	function pSkel;
	map<string, function>::iterator it;
	
	it = procMap.find(m->procName);
	
	if (it != procMap.end()) {
		
		pSkel = it->second;
		ret = (*pSkel)(m->argTypes, m->args);
		
		printf("skel return: %d\n", ret);
		printf("output arg set to: %d\n", *(int *)m->args[0]);
		
		if (ret == 0) {
			m->type = M_EXECUTE_SUCCESS;
			m->writeSocket(s);
		}
		else {
			// TODO: return error message
		}
	}
	else {
		printf("Server::handleExecute: proc %s not found on server\n", m->procName);
	}
	
	// TODO: test rest of server functions
}
