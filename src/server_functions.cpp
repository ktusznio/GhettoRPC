#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <vector>

using namespace std;

/* return: OUT; a, b: IN */
int f0(int a, int b) {

  return a + b;
}   


/* returns: OUT; a, b, c, d: IN */
long f1(char a, short b, int c, long d) {

  return a + b * c - d;
}


/* return string is the concatenation of the integer 
   part of the float and the interger part of the double
   return: OUT string; a, b: IN */
/*
char* f2(float a, double b) {

 float ai;
 double bi;
 char *str1;
 char *str2;

 a = modff(a, &ai);
 b = modf(b, &bi);

 str1 = (char *)malloc(100);
 str2 = (char *)malloc(100);

 char temp[100];
 char* temp2;

 memset(temp, 0, 100);

 temp2 = lltostr((long long)ai, temp+99);
 str1 = strcpy(str1, temp2);
 temp2 = lltostr((long long)bi, temp+99);
 str2 = strcpy(str2, temp2);

 str1 = strcat(str1, str2);

 free(str2);

 return str1;
}
*/

/* 
 * bubble sort
 * the first element in the array indicates the size of the array
 * a: INOUT array 
 */
/*
void f3(vector<void *> * a) {
	
	int len = *( (int *) a->at(0) );
	int i, j, k;
	
	for (i = 0; i < len; i++) {
		for (j = len - 1; j > i; j--) {
			
			if (*( (long *) a->at(j) ) > *( (long *) a->at(j -1) )) {
				k = *( (long *) a->at(j) );
				*( (long *) a->at(j) ) = *( (long *) a->at(j - 1) );
				*( (long *) a->at(j - 1) ) = k;
			}
		}
	}
}
*/

void f3(long a[]) {

  int len = a[0];
  int i, j, k;

  for (i = 0; i < len; i++) {
    for (j = len - 1; j > i; j--) {
      if (a[j] > a[j - 1]) {
	k = a[j];
	a[j] = a[j - 1];
	a[j - 1] = k;
      }
    }
  }
}

/*
 * print file named by a 
 * a: IN array
 */

void f4(char a[]) {

  /* print file a to a printer */
}


/* return: OUT; a, b: IN */
int f5(int a, int b) {

  	sleep(5);
  	return a + b;
}   

/* return: OUT; a, b: IN */
int f6(int a, int b) {

  return a + b;
}   

/* return: OUT; a, b: IN */
int f7(int a, int b) {

  return a - b;
}

