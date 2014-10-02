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
#include <stdlib.h>
using namespace std;

#ifdef __x86_64__
// code for 64 bit Intel arch

typedef unsigned long address_t;
#define JB_SP 6
#define JB_PC 7
address_t translate_address(address_t addr) {
	address_t ret;
	asm volatile("xor    %%fs:0x30,%0\n"
			"rol    $0x11,%0\n"
			: "=g" (ret)
			: "0" (addr));
	return ret;
}

#else
// code for 32 bit Intel arch

typedef unsigned int address_t;
#define JB_SP 4
#define JB_PC 5
address_t translate_address(address_t addr)
{
	address_t ret;
	asm volatile("xor    %%gs:0x18,%0\n"
			"rol    $0x9,%0\n"
			: "=g" (ret)
			: "0" (addr));
	return ret;
}

#endif

//----------Constants---------
#define TQ 1 //Time Quantum for round-robin scheduling
#define STACK_SIZE 40000
#define N 50 //Number of max threads allowed
//----------Globals---------
enum State {
	NEW, READY, RUNNING, SLEEPING, SUSPENDED, DELETED, TERMINATED, WAITING
};

struct Statistics {
	int threadID;
	State state;
	int numberOfBursts;
	unsigned long totalExecutionTime;
	unsigned long totalRequestedSleepingTime;
	unsigned int averageExecutionTimeQuantum;
	unsigned long averageWaitingTime;
};

struct Timers {
	struct timeval exec_start;
	struct timeval exec_end;
	struct timeval ready_start;
	struct timeval ready_end;
	unsigned int waitingCount;
	unsigned long totalWaitingTime;
	unsigned long sleepEndTime;
};

struct Thread_node {
	Statistics* stats;
	Timers* timers;
	void (*fn)(void);
	void *(*fn_arg)(void*); //used in case of createWithArgs
	void *arg; //used in case of createWithArgs
	void *fn_arg_result; //used in case of createWithArgs
	char* stack;
	list<Thread_node*> threadsWaitingForMe;
};

sigjmp_buf jbuf[N];
list<Thread_node*> newQueue, readyQueue, suspendQueue, deleteQueue, sleepQueue,
		masterList, terminateQueue, waitingQueue;
int lastCreatedThreadID = -1; //global variable to maintain the threadIds
//char master_stack[N][STACK_SIZE];
Thread_node* runningThread;

//***************************FUNCTION DECLARATIONS*********************************
//----------Helper Functions---------
void enque(list<Thread_node*> *l, Thread_node* node);
Thread_node* deque(list<Thread_node*> *l);
void initializeThread(Thread_node* t_node);
void switchThreads();
void checkIfSleepDone(Thread_node* t_node);
unsigned long int translate_address(unsigned long int addr);
void setUp(char *stack, void(*f)(void));
Thread_node* searchInQueue(int threadId, list<Thread_node*> *l);
void printQueue(list<Thread_node*> *l);
bool isValidThreadID(int threadId);
void protector(void);
int createHelper(void(*fn)(void), void *(*fn_arg)(void *), void *arg);
void changeState(Thread_node* node, State state);
uint64_t convertToMillis(timeval time);
uint64_t getTimeDiff(timeval start, timeval end);
Timers* getTimers(int threadID);
void resumeWaitingThreads(Thread_node *t_node);
void moveThread(Thread_node *t_node, State fromState, State toState);
void emptyQueue(list<Thread_node*> *queue);
void printStats(Thread_node* t_node);
void printTime(unsigned long sec);
uint64_t getCurrentTimeMillis();

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
		////cout << "SWITCH: ret_val= " << ret_val << endl;
		if (ret_val == 1) {
			////cout << "Returning from switch : may go inside the function"
			//	<< endl;
			return;
		}
	}

	//Moving readyHead to running state
	if (!readyQueue.empty()) {
		runningThread = deque(&readyQueue);
		//checkIfSleepDone(runningThread);
		changeState(runningThread, RUNNING);
		int runningThreadId = runningThread->stats->threadID;
		//cout << "/----------------" << endl;
		//cout << "switching now to " << runningThreadId << endl;
		//cout << "Ready Queue: ";
		//printQueue(&readyQueue);
		//cout << "----------------/" << endl;
		siglongjmp(jbuf[runningThreadId], 1);
	} else {
		//cout << "no thread to run - readyQueue empty" << endl;
	}
}

void checkIfSleepDone(Thread_node* t_node) {
	if (t_node->stats->state == SLEEPING) {
		uint64_t currentTimeMillis = getCurrentTimeMillis();
		if (t_node->timers->sleepEndTime > currentTimeMillis) {
			moveThread(runningThread, RUNNING, SLEEPING);
		}
	}
}

void setUp(char *stack, void(*f)(void)) {
	unsigned int sp, pc;
	sp = (address_t) stack + STACK_SIZE - sizeof(address_t);
	pc = (address_t) (&protector);
	sigsetjmp(jbuf[lastCreatedThreadID], 1);
	//cout << "/----------------" << endl;
	//cout << "Inside setup" << lastCreatedThreadID << endl;
	//cout << "----------------/" << endl;
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
		//cout << "Inside ifValidThreadID : Invalid threadId" << endl;
		return false;
	}
	return true;
}

void protector(void) {
	void (*fn)(void);
	void *(*fn_arg)(void*);
	void *arg;

	////cout << "Going inside the function from protector" << endl;

	if (runningThread->fn != NULL) {
		//In case of "create(void(*f)(void))"
		fn = runningThread->fn;
		(fn)();
	} else {
		fn_arg = runningThread->fn_arg;
		arg = runningThread->arg;
		runningThread -> fn_arg_result = (fn_arg)(arg);
	}

	moveThread(runningThread, RUNNING, TERMINATED);
}

int createHelper(void(*fn)(void), void *(*fn_arg)(void *) = NULL,
		void *arg = NULL) {

	Thread_node* t_node = new Thread_node;
	if (t_node == NULL) {
		//cout << "Sorry, out of memory, no more thread can be created" << endl;
		return -1;
	}
	if (lastCreatedThreadID == N - 1) {
		//cout << "Reached max allowed limit of threads" << endl;
		return -1;
	}
	char* stack = (char*) malloc(STACK_SIZE);
	if (stack == NULL) {
		//cout << "Sorry, out of memory, no more thread can be created" << endl;
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
	//calculations for waiting time
	if (node -> stats -> state == READY) {
		//Previous state
		gettimeofday(&time, NULL);
		node->timers->ready_end = time;
		node->timers->totalWaitingTime += getTimeDiff(
				node->timers->ready_start, node->timers->ready_end);
		node->timers->waitingCount++;
		node->stats->averageWaitingTime = node->timers->totalWaitingTime
				/ node->timers->waitingCount;
	} else if (state == READY) {
		//New State
		gettimeofday(&time, NULL);
		node->timers->ready_start = time;
	}

	//calculations for execution time
	if (node->stats->state == RUNNING) {
		//Previous State
		gettimeofday(&time, NULL);
		node->timers->exec_end = time;
		node->stats->totalExecutionTime += getTimeDiff(
				node->timers->exec_start, node->timers->exec_end);
		node->stats->numberOfBursts++;
		node->stats->averageExecutionTimeQuantum
				= node->stats->totalExecutionTime / node->stats->numberOfBursts;
	} else if (state == RUNNING) {
		//New State
		gettimeofday(&time, NULL);
		node->timers->exec_start = time;
	}
	node->stats->state = state;
}

uint64_t getTimeDiff(timeval start, timeval end) {
	uint64_t endmillis = convertToMillis(end);
	uint64_t startmillis = convertToMillis(start);
	return endmillis - startmillis;
}

uint64_t convertToMillis(timeval time) {
	return (time.tv_sec * (uint64_t) 1000) + (time.tv_usec / 1000);
}

Timers* getTimers(int threadID) {
	if (!isValidThreadID(threadID)) {
		return NULL;
	}

	Thread_node* t_node = searchInQueue(threadID, &masterList);
	if (t_node == NULL) {
		//cout << "Inside getTimers : thread not found" << endl;
		return NULL;
	}

	return t_node->timers;
}

void resumeWaitingThreads(Thread_node *t_node) {
	while (!(t_node->threadsWaitingForMe).empty()) {
		Thread_node* waitingThread = deque(&(t_node->threadsWaitingForMe));
		if (waitingThread->stats->state == WAITING) {
			//Using "if" check considering the scenario if waiting thread is deleted already
			moveThread(waitingThread, WAITING, READY);
		}
	}
}

void moveThread(Thread_node *t_node, State fromState, State toState) {
	//int threadId = t_node->stats->threadID;
	switch (toState) {
	case RUNNING:
		//cout << "Inside moveThread: toState as RUNNING not supported";
		return;
	case NEW:
		//cout << "Inside moveThread: toState as NEW not supported";
		return;
	case READY:
		enque(&readyQueue, t_node);
		changeState(t_node, READY);
		break;
	case SLEEPING: //sleeping moves it again to readyqueue
		enque(&readyQueue, t_node);
		changeState(t_node, SLEEPING);
		break;
	case SUSPENDED:
		//cout << "/----------------" << endl;
		//cout << "Suspended thread: " << threadId << endl;
		//cout << "----------------/" << endl;
		changeState(t_node, SUSPENDED);
		enque(&suspendQueue, t_node);
		break;
	case WAITING:
		changeState(t_node, WAITING);
		enque(&waitingQueue, t_node);
		break;
	case DELETED:
		//cout << "/----------------" << endl;
		//cout << "Deleted thread: " << threadId << endl;
		//cout << "----------------/" << endl;
		changeState(t_node, DELETED);
		enque(&deleteQueue, t_node);
		resumeWaitingThreads(t_node);
		break;
	case TERMINATED:
		//cout << "/----------------" << endl;
		//cout << "Terminated thread: " << threadId << endl;
		//cout << "----------------/" << endl;
		changeState(t_node, TERMINATED);
		enque(&terminateQueue, t_node);
		resumeWaitingThreads(t_node);
		break;
	}

	switch (fromState) {
	case DELETED:
		//cout << "Inside moveThread: fromState as DELETED not supported";
		break;
	case TERMINATED:
		terminateQueue.remove(t_node);
		break;
	case RUNNING:
		yield(); //Need to yield the thread if fromStatus is RUNNING
		break;
	case READY:
		readyQueue.remove(t_node);
		break;
	case SLEEPING: //sleeping removes from ready queue
		readyQueue.remove(t_node);
		break;
	case SUSPENDED:
		suspendQueue.remove(t_node);
		break;
	case NEW:
		newQueue.remove(t_node);
		break;
	case WAITING:
		waitingQueue.remove(t_node);
		break;
	}
}

void emptyQueue(list<Thread_node*> *queue) {
	while (!(*queue).empty()) {
		deque(queue);
	}
}

void printStats(Thread_node* t_node) {
	Statistics* stats = getStatus(t_node->stats->threadID);
	//cout << "---------------" << endl << endl;
	//cout << "ThreadId: " << stats->threadID << endl;
	//cout << "Current State: " << stats->state << endl;
	//cout << "Number of bursts: " << stats->numberOfBursts << endl;
	//cout << "Total Execution Time: ";
	printTime(stats->totalExecutionTime);
	//cout << "Average Execution Time: ";
	printTime(stats->averageExecutionTimeQuantum);
	//cout << "Average Waiting Time: ";
	printTime(stats->averageWaitingTime);
	//cout << "Total Requested Sleeping Time: ";
	printTime(stats->totalRequestedSleepingTime);
}

void printTime(unsigned long sec) {
	if (sec == 0) {
		//cout << "N/A" << endl;
	} else {
		//cout << sec << " msec" << endl;
	}
}

uint64_t getCurrentTimeMillis() {
	struct timeval currentTime;
	gettimeofday(&currentTime, NULL);
	return convertToMillis(currentTime);
}

//----------Thread Functions---------

/**
 * @brief Creates a no-return-type and no-arg thread.
 * Creates a thread, whose execution starts from
 * function f which has no parameter and no return type; however create returns
 * thread ID of created thread.
 */
int create(void(*f)(void)) {
	return createHelper(f);
}

/**
 * @brief Creates a return-type and arg thread.
 * f is a ptr to a function which
 * takes (void *) and returns (void *). Unlike the threads so far, this thread takes
 * arguments, and instead of uselessly looping forever, returns a value in a (void *).
 * This function returns the id of the thread created.
 */
int createWithArgs(void *(*f)(void *), void *arg) {
	return createHelper(NULL, f, arg);
}

/**
 * @brief A scheduler for our thread library.
 * dispatch() is called after every time quantum finishes.
 * We are simulating a round robin scheduler.
 */
void dispatch(int sig) {
	signal(SIGALRM, dispatch);
	alarm(TQ);
	switchThreads();
}

/**
 * @brief Starts the thread environment.
 * Call in your main method. Moves all the created threads to ready queue & runs the first one.
 * One called,starts running of thread and never return.
 * So at least one of your thread should have infinite loop.
 */
void start() {
	if (newQueue.empty()) {
		//cout << "Inside start: newQueue empty, please create some threads"<< endl;
		return;
	}

	//Moving all new threads to readyQueue
	while (!newQueue.empty()) {
		Thread_node* t_node = newQueue.front();
		moveThread(t_node, NEW, READY);
	}
	dispatch(14);
}

/**
 * @brief Submits particular thread to scheduler for execution.
 * Puts a thread to ready queue from new queue.
 * Could be called from any other thread.
 */
void run(int threadID) {
	if (!isValidThreadID(threadID)) {
		return;
	}

	Thread_node* t_node = searchInQueue(threadID, &newQueue);
	if (t_node == NULL) {
		//cout << "Inside run : Thread not found in the created(new) state"	<< endl;
		return;
	}

	moveThread(t_node, NEW, READY);
}

/**
 * @brief Returns it's own thread ID to the running thread.
 * Called from inside the thread.
 * Should return thread ID of the current thread.
 */
int getID() {
	if (runningThread != NULL) {
		return runningThread->stats->threadID;
	}

	return -1;
}

/**
 * @brief Returns thread Statistics with id as threadID
 * Could be called from any thread.
 * Returns the status of threaded by returning pointer to its statistics
 * Returns NULL of threadID is invalid.
 */
Statistics* getStatus(int threadID) {
	if (!isValidThreadID(threadID)) {
		return NULL;
	}

	Thread_node* t_node = searchInQueue(threadID, &masterList);
	if (t_node == NULL) {
		//cout << "Inside getStatus : thread not found" << endl;
		return NULL;
	}

	return t_node->stats;

}

/**
 * @brief Suspends the thread.
 * Could be called from any thread.
 * Puts the thread in suspended queue.
 * Suspends the thread with ID as threadID until a resume() is not called for the same thread.
 */
void suspend(int threadID) {
	if (!isValidThreadID(threadID)) {
		return;
	}

	if (runningThread != NULL && runningThread->stats->threadID == threadID) {
		moveThread(runningThread, RUNNING, SUSPENDED);
		return;
	}

	// Comes here only if not a running Thread, else should have dispatched
	Thread_node* t_node = searchInQueue(threadID, &readyQueue);
	if (t_node == NULL) {
		//cout << "Inside suspend : thread not found" << endl;
		return;
	}

	moveThread(t_node, READY, SUSPENDED);
}

/**
 * @brief Resumes a suspended thread.
 * Could be called from any other thread.
 * Puts the thread back to ready queue from suspended queue.
 */
void resume(int threadID) {
	if (!isValidThreadID(threadID)) {
		return;
	}

	Thread_node* t_node = searchInQueue(threadID, &suspendQueue);
	if (t_node == NULL) {
		//cout << "Inside resume : Thread not found" << endl;
		return;
	}

	moveThread(t_node, SUSPENDED, READY);
}

/**
 * @brief Deletes a particular thread.
 * Could be called from any other thread.
 * Deleted thread object lives, but it will lose all executing capability.
 */
void deleteThread(int threadID) {
	if (!isValidThreadID(threadID)) {
		return;
	}

	Thread_node* t_node = searchInQueue(threadID, &masterList);
	if (t_node == NULL) {
		//cout << "Inside deleteThread: thread not found" << endl;
		return;
	}

	switch (t_node->stats->state) {
	case DELETED:
		//cout << "Inside deleteThread: thread already deleted" << endl;
		break;
	case READY:
		moveThread(t_node, READY, DELETED);
		break;
	case SLEEPING:
		moveThread(t_node, SLEEPING, DELETED);
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
	case WAITING:
		moveThread(t_node, WAITING, DELETED);
		break;
	case TERMINATED:
		moveThread(t_node, TERMINATED, DELETED);
		break;
	}
}

/**
 * @brief Sleeps the calling thread for sec seconds.
 * Sleep is not a blocking call.
 * Puts the thread into sleeping state, and on completion puts back to ready queue.
 */
void sleep(int sec) {
	alarm(0);
	//Adding the total requested sleeping time
	runningThread->stats->totalRequestedSleepingTime += sec * 1000;

	uint64_t time = getCurrentTimeMillis();
	runningThread->timers->sleepEndTime = time + sec * 1000;
	//cout << "/----------------" << endl;
	//cout << "thread going to sleep for " << sec << " seconds, id: " << runningThread->stats->threadID << endl;
	//cout << "----------------/" << endl;
	bool firstTime = true;
	while (runningThread->timers->sleepEndTime > time) {
		if (!firstTime) {
			runningThread->stats->numberOfBursts--;
		} else {
			firstTime = false;
		}
		moveThread(runningThread, RUNNING, SLEEPING);
		time = getCurrentTimeMillis();
	}
}

/**
 * @brief Calling thread waits till thread with ID threadID returns.
 * Waits till a thread created with the above function returns,
 * and returns the return value of that thread. This
 * function, obviously, waits until that thread is done with.
 */
void *GetThreadResult(int threadID) {
	if (!isValidThreadID(threadID)) {
		return NULL;
	}

	Thread_node* t_node = searchInQueue(threadID, &masterList);
	if (t_node == NULL) {
		//cout << "Inside GetThreadResult: thread not found" << endl;
		return NULL;
	}

	if (t_node->stats->state == RUNNING) {
		//cout	<< "Inside GetThreadResult: Well this is embarrassing, I don't know my result" << endl;
		return NULL;
	}

	if (t_node->fn_arg == NULL) {
		//cout << "Inside GetThreadResult: Dude I don't have a result" << endl;
		return NULL;
	}

	if (t_node->stats->state == DELETED) {
		//cout << "Inside GetThreadResult: thread deleted" << endl;
		return NULL;
	}

	if (t_node->stats->state != TERMINATED) {
		(t_node->threadsWaitingForMe).push_back(runningThread);
		moveThread(runningThread, RUNNING, WAITING);
	}

	return t_node->fn_arg_result;
}

/**
 * @brief Calling thread waits till thread with ID threadID completes.
 * Waits till a thread created with the above function completes,
 * This function, obviously, waits until that thread is done with.
 */
void JOIN(int threadID) {
	if (!isValidThreadID(threadID)) {
		return;
	}

	Thread_node* t_node = searchInQueue(threadID, &masterList);
	if (t_node == NULL) {
		//cout << "Inside GetThreadResult: thread not found" << endl;
		return;
	}

	if (t_node->stats->state == RUNNING) {
		//cout << "Inside JOIN: Well this is embarrassing, how can I join Myself"	<< endl;
		return;
	}

	if (t_node->stats->state == DELETED) {
		//cout << "Inside JOIN: thread deleted" << endl;
		return;
	}

	if (t_node->stats->state != TERMINATED) {
		(t_node->threadsWaitingForMe).push_back(runningThread);
		moveThread(runningThread, RUNNING, WAITING);
	}
}

/** @brief cleans up everything.
 * Stops scheduler, frees all the space allocated, print statistics per thread.
 */
void clean() {
	alarm(0);
	//changing the state of runningThread to have proper stats
	if (runningThread != NULL) {
		changeState(runningThread, READY);
	}

	//all queues empty
	emptyQueue(&newQueue);
	emptyQueue(&readyQueue);
	emptyQueue(&suspendQueue);
	emptyQueue(&deleteQueue);
	emptyQueue(&sleepQueue);
	emptyQueue(&terminateQueue);
	emptyQueue(&waitingQueue);

	while (!masterList.empty()) {
		Thread_node* t_node = masterList.front();
		printStats(t_node);
		free(deque(&masterList));
	}
	exit(0);
}

/** @brief The calling thread yields the CPU.
 * Calling thread passes control to another next thread.
 */
void yield() {
	alarm(0);
	dispatch(14);
}
