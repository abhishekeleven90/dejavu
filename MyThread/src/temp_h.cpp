#include "MyThread.h"
#include <iostream>
#define SECOND 1000000

using namespace std;

void f() {
		cout << "f" << endl;
		//JOIN(2);
		sleep(5);
		cout << "f khatam" << endl;
		while(1){
			printStats(searchInQueue(0, &masterList));
			cout << "???" << endl;
			usleep(SECOND);
		}
}

void g() {
	while (1) {
		cout << "mein g hun" << endl;
		//int* p = (int *) GetThreadResult(2);
	//	cout << "inside g, printing the result: " << *p << endl;
		//cout << "reached here in g" << endl;
		usleep(SECOND);
		printStats(searchInQueue(0, &masterList));
	}
}

void* k(void* a) {
	while (1) {
	cout << "K" << endl;
	usleep(SECOND);
	//sleep(1);
	//clean();
	//return a;
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
