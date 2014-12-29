#include <stdio.h>
#include <cctype>
#include <cstring>
#include <unistd.h>
#include "threadlib.h"

void f();
void g();
void h();

void f()
{
  int i=0;
  while(1){
    printf("tid :%d in f: %d\n",getID(),i++);
    usleep(90000);

  }
}
void g()
{
  int i=0;
  while(1){
    printf("tid :%d in g: %d\n",getID(),i++);
    usleep(90000);
  }
}

void h(void *s)
{
    int i=0;
    char *ptr = (char *) s;
    printf("tid :%d in h: %d and string is %s\n",getID(),i++,(char *)s);
    for(int i=0; i<strlen(ptr); i++) {
	if(islower(*(ptr+i))) toupper(*(ptr+i));
    }
}



int main()
{
  int i=0;
  char str[] = "QuickPowPow";
  //create(f);
  //create(g);
  createWithArgs(h, str);
  start();
  printf("All children created\n");
  while(1)
  {
  }
}
