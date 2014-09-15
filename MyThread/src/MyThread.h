#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <time.h>
#include <sys/time.h>
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
#define TQ 1			//Time Quantum for round-robin scheduling
#define STACK_SIZE 4096
#define N 50				//Number of max threads allowed
//----------Globals---------
typedef enum {
	NEW, READY, RUNNING, SLEEPING, SUSPENDED, DELETED, TERMINATED
} State;

typedef struct {
	int threadID;
	State state;
	int numberOfBursts;
	unsigned long totalExecutionTime;
	unsigned long totalRequestedSleepingTime;
	unsigned int averageExecutionTimeQuantum;
	unsigned long averageWaitingTime;
} Statistics;

typedef struct {
	struct timeval exec_start;
	struct timeval exec_end;
	struct timeval ready_start;
	struct timeval ready_end;
	unsigned int waitingCount;
	unsigned long totalWaitingTime;
} Timers;

typedef struct {
	Statistics* stats;
	Timers* timers;
	void (*fn)(void);
	void *(*fn_arg)(void*); //used in case of createWithArgs
	void *arg; //used in case of createWithArgs
	void *fn_arg_result; //used in case of createWithArgs
	char* stack;
} Thread_node;

sigjmp_buf jbuf[N];
list<Thread_node*> newQueue, readyQueue, suspendQueue, deleteQueue, sleepQueue,
		masterList, terminateQueue;
int lastCreatedThreadID = -1; //global variable to maintain the threadIds

//char master_stack[N][STACK_SIZE];
typedef unsigned long address_t; //64bit address
Thread_node* runningThread;

//***************************FUNCTION DECLARATIONS*********************************

//----------Helper Functions---------
void enque(list<Thread_node*> *l, Thread_node* node);
Thread_node* deque(list<Thread_node*> *l);
void initializeThread(Thread_node* t_node);
void switchThreads();
unsigned long int translate_address(unsigned long int addr);
void setUp(char *stack, void(*f)(void));
Thread_node* searchInQueue(int threadId, list<Thread_node*> *l);
void printQueue(list<Thread_node*> *l);
bool isValidThreadID(int threadId);
void c(void);
int createHelper(void(*fn)(void), void *(*fn_arg)(void *), void *arg);
void changeState(Thread_node* node, State state);
uint64_t getTimeDiff(timeval start, timeval end);
void protector(void);
int switchThreadsHelper();

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

	Timers* timers = new Timers;
	t_node -> timers = timers;
	timers -> totalWaitingTime = 0;
	timers -> waitingCount = 0;

	enque(&newQueue, t_node); //Adding to the new queue
	enque(&masterList, t_node); //Adding to the master list
}

int switchThreadsHelper(list<Thread_node*> *targetQueue, State state) {
	changeState(runningThread, state);
	enque(targetQueue, runningThread);
	cout << "printing the target queue: " << endl;
	//printQueue(targetQueue);
	int ret_val = sigsetjmp(jbuf[runningThread->stats->threadID], 1);
	cout << "SWITCH HELPER: ret_val= " << ret_val << endl;
	//runningThread = NULL;
	return ret_val;
}

void switchThreads() {
	//Moving current running thread to readyQueue
	if (runningThread != NULL) {
		if (switchThreadsHelper(&readyQueue, READY) == 1) {
			//TO-DO write some reason here
			return;
		}
	}

	//Moving readyHead to running state
	if (!readyQueue.empty()) {
		runningThread = deque(&readyQueue);
		changeState(runningThread, RUNNING);
		int runningThreadId = runningThread->stats->threadID;
		cout << "switching now to " << runningThreadId << endl;
		siglongjmp(jbuf[runningThreadId], 1);
	} else {
		printQueue(&readyQueue);
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
	pc = (address_t) (&protector);
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
	if (l == NULL)
		return;
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

void protector(void) {
	void (*fn)(void);
	void *(*fn_arg)(void*);
	void *arg;
	if (runningThread->fn != NULL) {
		//In case of "create(void(*f)(void))"
		fn = runningThread->fn;
		(fn)();
	} else {
		fn_arg = runningThread->fn_arg;
		arg = runningThread->arg;
		runningThread -> fn_arg_result = (fn_arg)(arg);
	}
	/*changeState(runningThread, TERMINATED);
	 enque(&terminateQueue, runningThread);
	 runningThread = NULL;
	 alarm(0);
	 dispatch(-1);*/
	alarm(0); //Cancels the alarm, first thing
	if (switchThreadsHelper(&terminateQueue, TERMINATED) == 1) {
		//TO-DO write some reason here
		return;
	}
	dispatch(-1);
}

int createHelper(void(*fn)(void), void *(*fn_arg)(void *) = NULL,
		void *arg = NULL) {
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

	t_node -> fn = fn;
	t_node -> fn_arg = fn_arg; //In case of createWithArgs
	t_node -> arg = arg; //In case of createWithArgs
	t_node -> stack = stack;

	setUp(stack, fn);
	return lastCreatedThreadID;
}

void changeState(Thread_node* node, State state) {
	struct timeval time;

	if (node -> stats -> state == READY) {
		gettimeofday(&time, NULL);
		node->timers->ready_end = time;
		node->timers->totalWaitingTime += getTimeDiff(
				node->timers->ready_start, node->timers->ready_end);
		node->timers->waitingCount++;
		node->stats->averageWaitingTime = node->timers->totalWaitingTime
				/ node->timers->waitingCount;
	} else if (state == READY) {
		gettimeofday(&time, NULL);
		node->timers->ready_start = time;
	}

	if (node->stats-> state == RUNNING) {
		gettimeofday(&time, NULL);
		node->timers->exec_end = time;
		node->stats->totalExecutionTime += getTimeDiff(
				node->timers->exec_start, node->timers->exec_end);
		node->stats->numberOfBursts++;
		node->stats->averageExecutionTimeQuantum
				= node->stats->totalExecutionTime / node->stats->numberOfBursts;
	} else if (state == RUNNING) {
		gettimeofday(&time, NULL);
		node-> timers->exec_start = time;
	}
	node->stats->state = state;
}

uint64_t getTimeDiff(timeval start, timeval end) {
	uint64_t endmillis = (end.tv_sec * (uint64_t) 1000) + (end.tv_usec / 1000);
	uint64_t startmillis = (start.tv_sec * (uint64_t) 1000) + (start.tv_usec
			/ 1000);
	return endmillis - startmillis;
}

Timers* getTimers(int threadID) {
	if (!isValidThreadID(threadID)) {
		return NULL;
	}
	Thread_node* t_node = NULL;
	t_node = searchInQueue(threadID, &masterList);
	if (t_node != NULL) {
		return t_node->timers;
	} else {
		cout << "Inside getTimers : thread not found" << endl;
		return NULL;
	}
}

//----------Thread Functions---------
int create(void(*f)(void)) {
	return createHelper(f);
}

int createWithArgs(void *(*f)(void *), void *arg) {
	return createHelper(NULL, f, arg);
}

void dispatch(int sig) {
	signal(SIGALRM, dispatch);
	alarm(TQ);
	if (sig == -1) {
		if (!readyQueue.empty()) {
			runningThread = deque(&readyQueue);
			changeState(runningThread, RUNNING);
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
		changeState(t_node, READY);
		enque(&readyQueue, t_node);
	}

	dispatch(-1);
}

void run(int threadID) {
	Thread_node* t_node = NULL;
	if (isValidThreadID(threadID)) {
		t_node = searchInQueue(threadID, &newQueue);
		if (t_node == NULL) {
			cout << "Inside run : Thread not found" << endl;
		} else {
			newQueue.remove(t_node);
			enque(&readyQueue, t_node);
			changeState(t_node, READY);
		}
	}
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
	t_node = searchInQueue(threadID, &masterList);
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
			/*runningThread = NULL;
			 alarm(0); //Cancels the alarm
			 changeState(t_node, SUSPENDED);
			 enque(&suspendQueue, t_node);
			 dispatch(-1);*/
			alarm(0); //Cancels the alarm, first thing
			if (switchThreadsHelper(&suspendQueue, SUSPENDED) == 1) {
				//TO-DO write some reason here
				return;
			}
			dispatch(-1);
		}

		// Comes here only if not a running Thread, else should have dispatched
		t_node = searchInQueue(threadID, &readyQueue);

		if (t_node != NULL) {
			readyQueue.remove(t_node);
			changeState(t_node, SUSPENDED);
			enque(&suspendQueue, t_node);
		}
		if (t_node == NULL) {
			cout << "Inside suspend : thread not found" << endl;
		}
	}
}

void resume(int threadID) {
	Thread_node* t_node = NULL;
	if (isValidThreadID(threadID)) {
		t_node = searchInQueue(threadID, &suspendQueue);
		if (t_node == NULL) {
			cout << "Inside resume : Thread not found" << endl;
		} else {
			suspendQueue.remove(t_node);
			enque(&readyQueue, t_node);
			changeState(t_node, READY);
		}
	}
}

void deleteThread(int threadID) {
	Thread_node* t_node = NULL;
	if (isValidThreadID(threadID)) {
		t_node = searchInQueue(threadID, &masterList);
		if (t_node == NULL) {
			cout << "Inside deleteThread: thread not found" << endl;
		} else {
			switch (t_node->stats->state) {
			case DELETED:
				cout << "Inside deleteThread: thread already deleted" << endl;
				break;
			case SLEEPING:
				cout << "PASS...TO-DO" << endl;
				break;
			case READY:
				readyQueue.remove(t_node);
				changeState(t_node, DELETED);
				enque(&deleteQueue, t_node);
				break;
			case RUNNING:
				/*runningThread = NULL;
				 alarm(0); //Cancels the alarm
				 changeState(t_node, DELETED);
				 enque(&deleteQueue, t_node);
				 dispatch(-1);*/

				alarm(0); //Cancels the alarm, first thing
				if (switchThreadsHelper(&deleteQueue, DELETED) == 1) {
					//TO-DO write some reason here
					return;
				}
				dispatch(-1);

			case NEW:
				newQueue.remove(t_node);
				changeState(t_node, DELETED);
				enque(&deleteQueue, t_node);
				break;
			case SUSPENDED:
				suspendQueue.remove(t_node);
				changeState(t_node, DELETED);
				enque(&deleteQueue, t_node);
				break;
			}
		}
	}
}

void *GetThreadResult(int threadID) {
	if (!isValidThreadID(threadID)) {
		return NULL;
	}

	Thread_node* t_node = searchInQueue(threadID, &masterList);
	if (t_node == NULL) {
		cout << "Inside GetThreadResult: thread not found" << endl;
	} else {
		if (t_node->stats->state == DELETED) {
			cout << "Inside GetThreadResult: thread deleted" << endl;
			return NULL;
		}

		sigsetjmp(jbuf[runningThread->stats->threadID], 1);
		while (t_node->stats->state != TERMINATED) {
			alarm(0);
			changeState(runningThread, READY);
			enque(&readyQueue, runningThread);
			dispatch(-1);
		}
	}
	return t_node->fn_arg_result;
}

void yield() //to be called from the current runningThread
{

	alarm(0); //Cancels the alarm, first thing
	if (switchThreadsHelper(&readyQueue, READY) == 1) {
		//TO-DO write some reason here
		return;
	}
	dispatch(-1);
}

