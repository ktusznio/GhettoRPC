#ifndef _handlers_included
#define _handlers_included

#include "message.h"

void handleTerminate(int s, Message * m);
void handleExecute(int s, Message * m);

#endif