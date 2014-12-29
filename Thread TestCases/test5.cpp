#include <stdio.h>
#include <unistd.h>
#include "mythread.h"

void f()
{
  int i=0;
  double check[10000];
  while(1){
    printf("tid :%d in f: %d\n",getID(),i++);
    int tid = create(f);
    run(tid);
    usleep(90000);

  }
}



//1-Tests thread stack

int main()
{
    int i=0;
  create(f);
  start();
  printf("All children created\n");
  while(1) {
  
  }
}
