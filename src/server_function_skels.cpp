#include "include/server_function_skels.h"
#include "include/server_functions.h"
#include <stdio.h>
#include <string.h>
//#include <vector>

//using namespace std;

int f0_Skel(int *argTypes, void **args) {

  *(int *)args[0] = f0(*(int *)args[1], *(int *)args[2]);
  return 0;
}

int f1_Skel(int *argTypes, void **args) {

  *((long *)*args) = f1( *((char *)(*(args + 1))), 
		        *((short *)(*(args + 2))),
		        *((int *)(*(args + 3))),
		        *((long *)(*(args + 4))) );

  return 0;
}

int f2_Skel(int *argTypes, void **args) {

  //*args = f2( *((float *)(*(args + 1))),
	//	*((double *)(*(args + 2))) );
  return 0;
}

int f3_Skel(int *argTypes, void **args) {

  f3((long *)(*args));
  return 0;
}

/*
int f0_Skel(vector<int> * argTypes, vector<void *> * args) {
	
	printf("setting %d to %d + %d\n",
		*((int *)args->at(0)),
		*((int *)args->at(1)),
		*((int *)args->at(2)));
	
	*((int *)args->at(0)) = f0(*((int *)args->at(1)), *((int *)args->at(2)));
	
	printf("exiting f0_Skel\n");
	
	return 0;
}

int f1_Skel(vector<int> * argTypes, vector<void *> * args) {
	
	*( (long *) args->at(0) ) = f1( *( (char *) args->at(1)), 
						  *((short *) args->at(2)),
						  *((int *) args->at(3)),
						  *((long *) args->at(4)));
	
	return 0;
}

int f2_Skel(vector<int> * argTypes, vector<void *> * args) {
	
	//args->at(0) = (void *) f2( *( (float *) args->at(1)),
	//		   *( (double *) args->at(2)));
	
	
	printf("f2 not implemented yet.\n");
	
	return 0;
}

int f3_Skel(vector<int> * argTypes, vector<void *> * args) {
	
	//f3((long *)(*args));
	f3(args);
	return 0;
}
*/