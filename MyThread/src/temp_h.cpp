#include "MyThread.h"
#include <iostream>

using namespace std;

void f() {
	long i = 0;
	while (1) {
		i++;
		//cout << "aah ha1" << endl;
		if (i % 1000 == 0) {
			cout << "in f: " << i << endl;
		}
		if (i % 100000 == 0) {

			yield();
		}
	}
}

void g() {
	long i = 0;

	while (1) {
		//cout << "aah ha2" << endl;
		if (i % 100000 == 0) {
			i++;
			cout << "mein g hun" << endl;
		}

		//int *p = (int*) GetThreadResult(1);
		//cout << "result aa gya: " << *p;
	}
}

void* k(void* a) {
	long i = 0;

	while (1) {
		//cout << "aah ha13" << endl;
		if (i % 100000 == 0) {
			i++;
			cout << "mein k hun" << endl;
		}

	}
}

int main() {
	cout << "reached";
	cout << create(f) << endl;
	cout << create(g) << endl;
	int a = 5;
	int *p = &a;
	cout << createWithArgs(k, p) << endl;

	//cout << getStatus(2) -> state << endl;

	start();
	return 0;
}
