#include <stdio.h>
#include <setjmp.h>
#include <signal.h>
#include <list>
#include <iostream>
#include <malloc.h>
#include <unistd.h>

using namespace std;

//----------Constants---------
#define JB_SP 6
#define JB_PC 7
#define TQ 2				//Time Quantum for round-robin scheduling
#define STACK_SIZE 4096
#define N 50				//Number of max threads allowed
//----------Globals---------
typedef enum {
	NEW, READY, RUNNING, SLEEPING, SUSPENDED, DELETED
} State;

typedef struct {
	int threadID;
	State state;
	int numberOfBursts;
	unsigned long totalExecutionTime;
	unsigned long totalRequestedSleepingTime;
	int averageExecutionTimeQuantum;
	unsigned long averageWaitingTime;
} Statistics;

typedef struct {
	Statistics* stats;
	void (*func)(void);
	char* stack;
} Thread_node;

sigjmp_buf jbuf[N];
list<Thread_node*> newQueue, readyQueue, suspendQueue, deleteQueue;
int lastCreatedThreadID = -1; //global variable to maintain the threadIds

//char master_stack[N][STACK_SIZE];
typedef unsigned long address_t; //64bit address
Thread_node* runningThread;

//***************************FUNCTION DECLARATIONS*********************************

//----------Helper Functions---------
void enque(list<Thread_node*> *l, Thread_node* node);
Thread_node* deque(list<Thread_node*> *l);
void switchThreads();

//----------Thread Functions---------
int create(void(*f)(void));
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
void enque(list<Thread_node*> *l, Thread_node* node) {
	(*l).push_back(node);
}

Thread_node* deque(list<Thread_node*> *l) {
	Thread_node* node = (*l).front();
	(*l).pop_front();
	return node;
}

void initializeThread(Thread_node* t_node) {
	lastCreatedThreadID++;
	Statistics* stats = new Statistics;
	t_node -> stats = stats;
	stats -> state = NEW;
	stats -> threadID = lastCreatedThreadID;
	stats -> numberOfBursts = 0;
	stats -> totalExecutionTime = 0;
	stats -> totalRequestedSleepingTime = 0;
	stats -> averageExecutionTimeQuantum = 0;
	stats -> averageWaitingTime = 0;

	enque(&newQueue, t_node); //Adding to the new queue
	//curThread++; //increasing the threadId
}

void switchThreads() {
	//Moving current running thread to readyQueue
	if (runningThread != NULL) {
		runningThread->stats->state = READY;
		enque(&readyQueue, runningThread);
		int ret_val = sigsetjmp(jbuf[runningThread->stats->threadID], 1);
		cout << "SWITCH: ret_val= " << ret_val << endl;
		if (ret_val == 1) {
			return;
		}
	}

	//Moving readyHead to running state
	if (!readyQueue.empty()) {
		runningThread = deque(&readyQueue);
		runningThread -> stats -> state = RUNNING;
		int runningThreadId = runningThread->stats->threadID;
		cout << "switching now to " << runningThreadId << endl;
		siglongjmp(jbuf[runningThreadId], 1);
	} else {
		cout << "no thread to run - readyQueue empty" << endl;
	}
}

unsigned long int translate_address(unsigned long int addr) {
	unsigned long int retAddr;
	asm volatile("movq %%fs:0x30,%0\n" : "=r" (retAddr));
	retAddr = retAddr ^ addr;
	retAddr = (retAddr << 17) | (retAddr >> (64 - 17));
	return retAddr;
}

void setUp(char *stack, void(*f)(void)) {
	unsigned int sp, pc;
	sp = (address_t) stack + STACK_SIZE - sizeof(address_t);
	pc = (address_t) (f);
	sigsetjmp(jbuf[lastCreatedThreadID], 1);
	(jbuf[lastCreatedThreadID]->__jmpbuf)[JB_SP] = translate_address(sp);
	(jbuf[lastCreatedThreadID]->__jmpbuf)[JB_PC] = translate_address(pc);
	sigemptyset(&jbuf[lastCreatedThreadID]->__saved_mask); //empty saved signal mask
}

Thread_node* searchInQueue(int threadId, list<Thread_node*> *l) {
	for (list<Thread_node*>::iterator it = (*l).begin(); it != (*l).end(); it++) {
		if ((*it)->stats->threadID == threadId) {
			return *it;
		}
	}
	return NULL;
}

void printQueue(list<Thread_node*> *l) {
	for (list<Thread_node*>::iterator it = (*l).begin(); it != (*l).end(); it++) {
		cout << (*it)->stats->threadID << ", ";
	}
	cout << endl;
}

bool isValidThreadID(int threadId) {
	if (threadId > lastCreatedThreadID) {
		cout << "Inside ifValidThreadID : Invalid threadId" << endl;
		return false;
	}
	return true;
}

//----------Thread Functions---------
int create(void(*f)(void)) {
	Thread_node* t_node = new Thread_node;
	if (t_node == NULL) {
		cout << "Sorry, out of memory, no more thread can be created" << endl;
		return -1;
	}

	if (lastCreatedThreadID == N) {
		cout << "Reached max allowed limit of threads" << endl;
		return -1;
	}

	char* stack = (char*) malloc(STACK_SIZE);
	if (stack == NULL) {
		cout << "Sorry, out of memory, no more thread can be created" << endl;
		return -1;
	}

	initializeThread(t_node);
	t_node -> func = f;
	t_node -> stack = stack;

	setUp(stack, f);
	return lastCreatedThreadID;
}

void dispatch(int sig) {
	signal(SIGALRM, dispatch);
	alarm(TQ);
	if (sig == -1) {
		if (!readyQueue.empty()) {
			runningThread = deque(&readyQueue);
			runningThread->stats->state = RUNNING;
			siglongjmp(jbuf[runningThread->stats->threadID], 1);
		} else {
			cout << "No threads in readyQueue" << endl;
		}
	}
	switchThreads();
}

/***Moves all the created threads to ready queue & runs the first one***/
void start() {
	//Moving all new threads to readyQueue
	while (!newQueue.empty()) {
		Thread_node* t_node = deque(&newQueue);
		t_node->stats->state = READY;
		enque(&readyQueue, t_node);
	}

	dispatch(-1);
}

void run(int threadId) {
	Thread_node* t_node = NULL;
	//Checking if the threadId is valid
	if (threadId > lastCreatedThreadID) {
		cout << "Inside run : Invalid threadId" << endl;
		return;
	}
	t_node = searchInQueue(threadId, &newQueue);
	if (t_node != NULL) {
		newQueue.remove(t_node);
	} else {
		t_node = searchInQueue(threadId, &suspendQueue);
		if (t_node != NULL) {
			suspendQueue.remove(t_node);
		}
	}

	if (t_node != NULL) {
		enque(&readyQueue, t_node);
		t_node->stats->state = READY;
	} else {
		cout << "Inside run : Thread not found" << endl;
	}

	printQueue(&newQueue);
	printQueue(&readyQueue);
}

int getID() {
	if (runningThread != NULL) {
		return runningThread->stats->threadID;
	} else {
		return -1;
	}
}

Statistics* getStatus(int threadID) {
	if (!isValidThreadID(threadID)) {
		return NULL;
	}
	Thread_node* t_node = NULL;
	if (runningThread != NULL && runningThread->stats->threadID == threadID) {
		t_node = runningThread;
	}
	if (t_node == NULL) {
		t_node = searchInQueue(threadID, &newQueue);
	}
	if (t_node == NULL) {
		t_node = searchInQueue(threadID, &readyQueue);
	}
	if (t_node == NULL) {
		t_node = searchInQueue(threadID, &suspendQueue);
	}
	if (t_node == NULL) {
		t_node = searchInQueue(threadID, &deleteQueue);
	}
	if (t_node != NULL) {
		return t_node->stats;
	} else {
		cout << "Inside getStatus : thread not found" << endl;
		return NULL;
	}
}

void suspend(int threadID) {
	Thread_node* t_node = NULL;
	if (isValidThreadID(threadID)) {
		if (runningThread != NULL && runningThread->stats->threadID == threadID) {
			t_node = runningThread;
			runningThread = NULL;
			alarm(0); //Cancels the alarm
			t_node -> stats -> state = SUSPENDED;
			enque(&suspendQueue, t_node);
			dispatch(-1);
		}

		//Comes here only if not a running Thread, else should have dispatched
		t_node = searchInQueue(threadID, &readyQueue);

		if (t_node != NULL) {
			readyQueue.remove(t_node);
			t_node -> stats -> state = SUSPENDED;
			enque(&suspendQueue, t_node);
		}
		if (t_node == NULL) {
			cout << "Inside suspend : thread not found" << endl;
		}
	}
}
