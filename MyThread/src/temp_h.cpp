#include "MyThread.h"
#include <iostream>
#define SECOND 1000000

using namespace std;


void ff() {

	while (1) {
		cout << "ff" << endl;
		usleep(SECOND);
	}
}


void f() {
	GetThreadResult(0);
	cout << "f" << endl;
	//usleep(SECOND);
	//JOIN(2);
	//sleep(5);
	//cout << "f khatam" << endl;
	//suspend(1);
	while (1) {
		//printStats(searchInQueue(0, &masterList));
		cout << "f." << endl;
		usleep(SECOND);

	}
}

void g() {
	create(ff);
	run(2);
	while (1) {
		cout << "g" << endl;
		//int* p = (int *) GetThreadResult(2);
		//	cout << "inside g, printing the result: " << *p << endl;
		//cout << "reached here in g" << endl;
		usleep(SECOND);
		//printStats(searchInQueue(0, &masterList));
		//deleteThread(2);
	}
}




void* k(void* a) {
	int i=0;
run(3);
	while (1) {
		cout << "K" << endl;
		usleep(SECOND);
		//printStats(searchInQueue(1, &masterList));
		//sleep(1);
		//clean();
		//return a;
		//resume(1);
	}
}

int main() {
	cout << "reached" << endl;
	cout << create(f) << endl;
	cout << create(g) << endl;

	int a = 5;
	int *p = &a;
	cout << createWithArgs(k, p) << endl;
	//clean();
	//cout << getStatus(2) -> state << endl;

	start();
	cout << "hereeeeee in main";
	return 0;
}
