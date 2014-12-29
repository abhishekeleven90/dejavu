#include <stdio.h>
#include <unistd.h>
#include "MyThread.h"

void f();

void f()
{
  int i=0; int arr[100];
  while(1){
    printf("tid :%d in f: %d\n",getID(),i++);
    usleep(90000);
  }
}



//1 - Stress create and stress the scheduler

int main()
{
  for(int i=0; i<10000; i++) {
       create(f);
  }
  start();
  printf("All children created\n");
  while(1) {
	
  }
}
