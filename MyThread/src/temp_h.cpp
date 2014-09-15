#include "MyThread.h"
#include <iostream>

using namespace std;

void f() {
	while (1) {
		//cout << "f" << endl;
	}
}

void g() {
	cout << "mein g hun" << endl;
	JOIN(0);
	//cout << "result aa gya: " << *p;
	cout << "here" << endl;
}

void* k(void* a) {
	unsigned long i = 0;
	while (i < 100000) {
		i++;
		cout << "k: " << i << endl;
	}
	return a;
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
