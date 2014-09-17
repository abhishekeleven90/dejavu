#include "MyThread.h"
#include <iostream>
#define SECOND 1000000

using namespace std;

void f() {
	while (1) {
		cout << "f" << endl;
		JOIN(2);
		usleep(SECOND);
	}
}

void g() {
	while (1) {
		cout << "mein g hun" << endl;
		int* p = (int *) GetThreadResult(2);
		cout << "inside g, printing the result: " << *p << endl;
		cout << "reached here in g" << endl;
		usleep(SECOND);
	}
}

void* k(void* a) {
	//while (1) {
	cout << "K" << endl;
	usleep(SECOND);
	return a;
	//}
}

int main() {
	cout << "reached" << endl;
	cout << create(f) << endl;
	cout << create(g) << endl;
	int a = 5;
	int *p = &a;
	cout << createWithArgs(k, p) << endl;

	//cout << getStatus(2) -> state << endl;

	start();
	cout << "hereeeeee in main";
	return 0;
}
