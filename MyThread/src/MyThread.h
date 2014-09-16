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
#define TQ 2 //Time Quantum for round-robin scheduling
#define STACK_SIZE 4096
#define N 50 //Number of max threads allowed
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
void protector(void);
int createHelper(void(*fn)(void), void *(*fn_arg)(void *), void *arg);
void changeState(Thread_node* node, State state);
uint64_t getTimeDiff(timeval start, timeval end);
void moveThread(Thread_node *t_node, State fromState, State toState);

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
void switchThreads() {
	if (runningThread != NULL) {
		if (runningThread->stats->state == RUNNING) {
			//shall come here only if TQ expires
			changeState(runningThread, READY);
			enque(&readyQueue, runningThread);
		}

		int ret_val = sigsetjmp(jbuf[runningThread->stats->threadID], 1);
		//cout << "SWITCH: ret_val= " << ret_val << endl;
		if (ret_val == 1) {
			cout << "Returning from switch : may go inside the function"
					<< endl;
			return;
		}
	}

	//Moving readyHead to running state
	if (!readyQueue.empty()) {
		runningThread = deque(&readyQueue);
		changeState(runningThread, RUNNING);
		int runningThreadId = runningThread->stats->threadID;
		cout << "switching now to " << runningThreadId << endl;
		cout << "Ready Queue: ";
		printQueue(&readyQueue);
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
	pc = (address_t) (&protector);
	sigsetjmp(jbuf[lastCreatedThreadID], 1);
	cout << "inside setup";
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
void protector(void) {
	void (*fn)(void);
	void *(*fn_arg)(void*);
	void *arg;

	cout << "Going inside the function from protector" << endl;

	if (runningThread->fn != NULL) {
		//In case of "create(void(*f)(void))"
		fn = runningThread->fn;
		(fn)();
	} else {
		fn_arg = runningThread->fn_arg;
		arg = runningThread->arg;
		runningThread -> fn_arg_result = (fn_arg)(arg);
	}

	cout << "Inside protector: terminated with id: "
			<< runningThread->stats->threadID << endl;

	moveThread(runningThread, RUNNING, TERMINATED);
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

	Thread_node* t_node = searchInQueue(threadID, &masterList);
	if (t_node == NULL) {
		cout << "Inside getTimers : thread not found" << endl;
		return NULL;
	}

	return t_node->timers;
}

void moveThread(Thread_node *t_node, State fromState, State toState) {
	switch (toState) {
	case RUNNING:
		cout << "This method does not support making toState as RUNNING";
		return;
	case READY:
		enque(&readyQueue, t_node);
		changeState(t_node, READY);
		break;
	case SUSPENDED:
		changeState(t_node, SUSPENDED);
		enque(&suspendQueue, t_node);
		break;
	case DELETED:
		changeState(t_node, DELETED);
		enque(&deleteQueue, t_node);
		break;
	case TERMINATED:
			changeState(t_node, TERMINATED);
			enque(&terminateQueue, t_node);
			break;
	}

	switch (fromState) {
	case RUNNING:
		yield(); //Need to yield the thread if the fromStatus is RUNNING
		break;
	case READY:
		readyQueue.remove(t_node);
		break;
	case SUSPENDED:
		suspendQueue.remove(t_node);
		break;
	case NEW:
		newQueue.remove(t_node);
		break;
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
	switchThreads();
}

/***Moves all the created threads to ready queue & runs the first one***/
void start() {
	if (newQueue.empty()) {
		cout << "Inside start: newQueue empty, please create some threads";
		return;
	}

	//Moving all new threads to readyQueue
	while (!newQueue.empty()) {
		Thread_node* t_node = newQueue.front();
		moveThread(t_node, NEW, READY);
	}
	dispatch(14);
}
void run(int threadID) {
	if (!isValidThreadID(threadID)) {
		return;
	}

	Thread_node* t_node = searchInQueue(threadID, &newQueue);
	if (t_node == NULL) {
		cout << "Inside run : Thread not found in the created(new) state"
				<< endl;
		return;
	}

	moveThread(t_node, NEW, READY);
}

int getID() {
	if (runningThread != NULL) {
		return runningThread->stats->threadID;
	}

	return -1;
}

Statistics* getStatus(int threadID) {
	if (!isValidThreadID(threadID)) {
		return NULL;
	}

	Thread_node* t_node = searchInQueue(threadID, &masterList);
	if (t_node == NULL) {
		cout << "Inside getStatus : thread not found" << endl;
		return NULL;
	}

	return t_node->stats;

}
void suspend(int threadID) {
	if (!isValidThreadID(threadID)) {
		return;
	}

	if (runningThread != NULL && runningThread->stats->threadID == threadID) {
		moveThread(runningThread, RUNNING, SUSPENDED);
	}

	// Comes here only if not a running Thread, else should have dispatched
	Thread_node* t_node = searchInQueue(threadID, &readyQueue);
	if (t_node == NULL) {
		cout << "Inside suspend : thread not found" << endl;
	}

	moveThread(t_node, READY, SUSPENDED);
}

void resume(int threadID) {
	if (!isValidThreadID(threadID)) {
		return;
	}

	Thread_node* t_node = searchInQueue(threadID, &suspendQueue);
	if (t_node == NULL) {
		cout << "Inside resume : Thread not found" << endl;
		return;
	}

	moveThread(t_node, SUSPENDED, READY);
}

void deleteThread(int threadID) {
	if (!isValidThreadID(threadID)) {
		return;
	}

	Thread_node* t_node = searchInQueue(threadID, &masterList);
	if (t_node == NULL) {
		cout << "Inside deleteThread: thread not found" << endl;
		return;
	}

	switch (t_node->stats->state) {
	case DELETED:
		cout << "Inside deleteThread: thread already deleted" << endl;
		break;
	case READY:
		moveThread(t_node, READY, DELETED);
		break;
	case RUNNING:
		moveThread(t_node, RUNNING, DELETED);
		break;
	case NEW:
		moveThread(t_node, NEW, DELETED);
		break;
	case SUSPENDED:
		moveThread(t_node, SUSPENDED, DELETED);
		break;
	}
}

void *GetThreadResult(int threadID) {
	if (!isValidThreadID(threadID)) {
		return NULL;
	}

	Thread_node* t_node = searchInQueue(threadID, &masterList);
	if (t_node == NULL) {
		cout << "Inside GetThreadResult: thread not found" << endl;
		return NULL;
	}

	if (t_node->stats->state == RUNNING) {
		cout
				<< "Inside GetThreadResult: Well this is embarrassing, I don't know my result"
				<< endl;
		return NULL;
	}

	if (t_node->fn_arg == NULL) {
		cout << "Inside GetThreadResult: Dude I don't have a result" << endl;
		return NULL;
	}

	while (t_node->stats->state != TERMINATED) {
		//Using Delete check here to cover the scenario - if thread deleted before terminating
		if (t_node->stats->state == DELETED) {
			cout << "Inside GetThreadResult: thread deleted" << endl;
			return NULL;
		}

		moveThread(runningThread, RUNNING, READY); //Moving currentThread to Ready queue until t_node is terminated
	}
	return t_node->fn_arg_result;
}

void JOIN(int threadID) {
	if (!isValidThreadID(threadID)) {
		return;
	}

	Thread_node* t_node = searchInQueue(threadID, &masterList);
	if (t_node == NULL) {
		cout << "Inside GetThreadResult: thread not found" << endl;
		return;
	}

	while (t_node->stats->state != TERMINATED) {
		//Using Delete check here to cover the scenario - if thread deleted before terminating
		if (t_node->stats->state == DELETED) {
			cout << "Inside GetThreadResult: thread deleted" << endl;
			return;
		}

		moveThread(runningThread, RUNNING, READY); //Moving currentThread to Ready queue until t_node is terminated
	}
}

void yield() {
	alarm(0);
	dispatch(14);
}
