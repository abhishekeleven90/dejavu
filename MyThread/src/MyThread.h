#include <stdio.h>
#include <setjmp.h>
#include <signal.h>
#include <list>
#include <iostream>

using namespace std;

//----------Constants---------
#define JB_SP 6
#define JB_PC 7
#define TQ 1				//Time Quantum for round-robin scheduling
#define STACK_SIZE 4096
#define N 50				//Number of max threads allowed

//----------Globals---------
typedef enum {
	NEW, RUNNING, READY, SLEEPING, SUSPENDED, DELETED
}State;

typedef struct {
	int threadId;
	State state;
	int numberOfBursts;
	unsigned long totalExecutionTime;
	unsigned long totalRequestedSleepingTime;
	int averageExecutionTimeQuantum;
	unsigned long averageWaitingTime;
}Statistics;

typedef struct {
	Statistics* stats;
	void (*func)(void);
}Thread_node;

sigjmp_buf jbuf[N];
list<Thread_node*> readyQueue, suspendedQueue;
int currentId = 0;			//global variable to maintain the threadIds

//***************************FUNCTION DECLARATIONS*********************************

//----------Helper Functions---------
void enque(list<Thread_node*> *l, Thread_node** node);
void deque(list<Thread_node*> *l);

//----------Thread Functions---------
int create(void (*f)(void));
int getID();
void dispatch(int sig);
void start();
void run(int threadID);
void suspend(int threadID);
void resume(int threadID);
void yield();
void deleteThread(int threadID);
void sleep(int sec);
Statistics* getStatus(int threadID);
int createWithArgs(void *(*f)(void *), void *arg);
void clean();
void JOIN(int threadID);
void *GetThreadResult(int threadID);


//*******************************FUNCTION DEFINITIONS************************
//Helper Functions
void enque(list<Thread_node*> l, Thread_node* node) {
	l.push_back(node);
}

Thread_node* deque(list<Thread_node*> l) {
	Thread_node* node = l.front();
	l.pop_front();
	return node;
}

void initializeThreadStats(Thread_node* t_node) {
	Statistics* stats = new Statistics;
	t_node -> stats = stats;
	stats -> state = NEW;
	stats -> threadId = currentId;
	stats -> numberOfBursts = 0;
	stats -> totalExecutionTime = 0;
	stats -> totalRequestedSleepingTime = 0;
	stats -> averageExecutionTimeQuantum = 0;
	stats -> averageWaitingTime = 0;
}

//----------Thread Functions---------
int create(void (*f)(void)) {
	Thread_node* t_node = new Thread_node;

	if(t_node == NULL){
		cout << "Sorry, out of memory, no more thread can be created";
		return -1;
	}

	initializeThreadStats(t_node);

	return 0;
}

//char stack1[STACK_SIZE];
//char stack2[STACK_SIZE];
//typedef unsigned long address_t; //64bit address


/*void switchThreads() {
	static int curThread = 0;
	int ret_val = sigsetjmp(jbuf[curThread], 1);
	printf("SWITCH: ret_val=%d\n", ret_val);
	if (ret_val == 1) {
		return;
	}
	curThread = 1 - curThread;
	siglongjmp(jbuf[curThread], 1);
}

void f() {
	int i = 0;
	//int f_flag=1;
	while (1) {
		++i;
		printf("in f (%d)\n", i);
		if (i % 3 == 0) {
			printf("f: switching\n");
			switchThreads();
		}
		if(i%10==0)
			break;
	}
}

void g() {

	int i = 0;
	while (1) {
		++i;
		printf("in g (%d)\n", i);
		if (i % 3 == 0) {
			printf("g: switching\n");
			switchThreads();
		}
		if(i%10==0)
					break;
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
	sp = (address_t) stack1 + STACK_SIZE - sizeof(address_t);
	pc = (address_t) f;
	sigsetjmp(jbuf[0], 1);
	(jbuf[0]->__jmpbuf)[JB_SP] = translate_address(sp);
	(jbuf[0]->__jmpbuf)[JB_PC] = translate_address(pc);
	sigemptyset(&jbuf[0]->__saved_mask);//empty saved signal mask

	sp = (address_t) stack2 + STACK_SIZE - sizeof(address_t);
	pc = (address_t) g;
	sigsetjmp(jbuf[1], 1);
	(jbuf[1]->__jmpbuf)[JB_SP] = translate_address(sp);
	(jbuf[1]->__jmpbuf)[JB_PC] = translate_address(pc);
	sigemptyset(&jbuf[1]->__saved_mask);//empty saved signal mask

}

int create(void (*f)(void));

int main() {
	setup();
	siglongjmp(jbuf[0], 1);
	return 0;
}*/
