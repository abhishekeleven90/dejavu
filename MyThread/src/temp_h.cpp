#include "MyThread.h"
#include <iostream>

using namespace std;

void f() {
	while (1) {
		cout << "inside fff : ";
		cout << getID() << endl;
	}

}

void g() {
	while (1) {
		cout << "inside ggg" << endl;
	}

}

void k() {
	while (1) {
		cout << "inside kkkkkkkkkkkkk" << endl;

	}

}

int main() {

	cout << create(f) << endl;
	cout << create(g) << endl;
	cout << create(k) << endl;

	cout << getStatus(3) -> state << endl;

	start();
	return 0;
}
