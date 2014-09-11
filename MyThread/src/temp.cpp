//============================================================================
// Name        : temp.cpp
// Author      : abhishek agarwal
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================
#include <iostream>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <setjmp.h>
#include <unistd.h>
#include <signal.h>

#define JB_SP 6
#define JB_PC 7
#define STACK_SIZE 30000
#define QUANTA 4
#define N 1
#define MAX 2

using namespace std; //useless till now

char master_stack[MAX][STACK_SIZE];

char stack1[STACK_SIZE];
char stack2[STACK_SIZE];

typedef unsigned long address_t; //64bit address
sigjmp_buf jbuf[N];
int curThread = 0;

void switchThreads() {
	//printf("here here\n");
	//static int curThread = 0;
	printf("switchThreads calleeeeeeed: %d\n",curThread);
	int ret_val = sigsetjmp(jbuf[curThread], 1);
	printf("SWITCH: ret_val=%d\n", ret_val);
	if (ret_val == 1) {
		return;
	}
	if(curThread==1)
		curThread = 0;
	else
		curThread = 1;
	//curThread = 1 - curThread;
	printf("switching now to %d\n",curThread);
	siglongjmp(jbuf[curThread], 1);
}


void dispatch(int sig)
{
	printf("dispatch\n");
	//static int times=0;
	//times++;
	//struct sigaction act;
	//act.sa_handler = dispatch;
	//sigaction (SIGALRM, & act, 0);
	signal(SIGALRM, dispatch);
	alarm (QUANTA);
	//printf("should come here\n");
	if(sig==-1)
	{
		siglongjmp(jbuf[0], 1);
		return;//useless
	}
	switchThreads();
}

void f() {
	//int i = 0;
	//int f_flag=1;
	while (1) {
		//++i;
		//printf("f \n");
		//sleep(1);
		//if (i % 1000 == 0) {
		//	printf("in f (%d)\n", i);
		//}
		//	printf("f: switching\n");
		//	switchThreads();
		//}
		//if(i==1000)
			//break;
		//usleep( QUANTA);
	}
}

void g() {

	//int i = 0;
	while (1) {
		//++i;
		//printf("g \n");
		//sleep(1);
		//if (i % 10000 == 0) {
					//printf("in g (%d)\n", i);
		//		}
		//if (i % 3 == 0) {
			//printf("g: switching\n");
			//switchThreads();
		//}
		//sleep(1);
		//if(i%1000==0)
			//		break;
		//usleep( QUANTA);
	}

}


unsigned long int translate_address(unsigned long int addr)
{
    unsigned long int ret;
    asm volatile("movq %%fs:0x30,%0\n"

        : "=r" (ret));
    ret=ret^addr;
    ret = (ret << 17) | ( ret >> (64-17) );
    return ret;
}

void setup() {
	unsigned int sp, pc;

	//sp = (address_t) stack1 + STACK_SIZE - sizeof(address_t);
	sp = (address_t) master_stack[0] + STACK_SIZE - sizeof(address_t);
	pc = (address_t) f;
	sigsetjmp(jbuf[0], 1);
	(jbuf[0]->__jmpbuf)[JB_SP] = translate_address(sp);
	(jbuf[0]->__jmpbuf)[JB_PC] = translate_address(pc);
	sigemptyset(&jbuf[0]->__saved_mask);//empty saved signal mask

	//sp = (address_t) stack2 + STACK_SIZE - sizeof(address_t);
	sp = (address_t) master_stack[1] + STACK_SIZE - sizeof(address_t);
	pc = (address_t) g;
	sigsetjmp(jbuf[1], 1);
	(jbuf[1]->__jmpbuf)[JB_SP] = translate_address(sp);
	(jbuf[1]->__jmpbuf)[JB_PC] = translate_address(pc);
	sigemptyset(&jbuf[1]->__saved_mask);//empty saved signal mask

}

int create(void (*f)(void));



int main77() {
	setup();
	//ualarm (10000, 0);
	//printf("main2\n");
	//pre_dispatch();
	dispatch(-1);
	//alarm(0);
	//dispatch(14);
	//siglongjmp(jbuf[0], 1);
	return 0;
}
