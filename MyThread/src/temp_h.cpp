#include "MyThread.h"
#include <iostream>

using namespace std;

void f() {
	while (1) {
	}

}

void g() {
	while (1) {
	}

}

int main() {
	cout << create(f) << endl;
	cout << create(f) << endl;
	cout << create(g) << endl;
	cout << create(g) << endl;
	start();
	return 0;
}
