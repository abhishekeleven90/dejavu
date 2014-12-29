#include <stdio.h>
#include <unistd.h>
#include "MyThread.h"

void f()
{
  int i=0;
  while(1){
    printf("tid :%d in f: %d\n",getID(),i++);
    usleep(90000);

  }
}

void h()
{
  int i=0;
  while(1){
    printf("tid :%d in h: %d\n",getID(),i++);
    usleep(90000);
  }
}

void g()
{
  int i=0;
  int flag = 0;
  int tid = create(h);
  while(1){
    printf("tid :%d in g: %d\n",getID(),i++);
    if(i> 20 && !flag)
	{
		run(tid);
		flag = 1;
	}
    usleep(90000);
  }
}

//1 - Thread creating thread 

int main()
{
    int i=0;
  create(f);
  create(g);
  start();
  printf("All children created\n");
  while(1)
  {
  }
}
