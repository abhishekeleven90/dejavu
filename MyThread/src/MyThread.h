#include<constants.h>

class MyThread {

private:
	Statistics statistics;
	static int counter = 0;
	MyThread() {
	}

public:
	static int create(void(*f)(void)) {
	}

	static int getID() {
	}

	static void dispatch(int sig);
	static void start();
	static void run(int threadID);
	static void suspend(int threadID);
	static void resume(int threadID);
	static void yield();
	//static void delete(int threadID);
	static void sleep(int sec);
	static struct statistics* getStatus(int threadID);
	static int createWithArgs(void *(*f)(void *), void *arg);
	static void clean();
	static void JOIN(int threadID);
	static void *GetThreadResult(int threadID);
};
