#include "MyThread.h"
#include <iostream>

using namespace std;

void f() {
	while (1) {
		//cout << "ffff " << endl;
		/*cout << getStatus(getID()) -> averageExecutionTimeQuantum << endl;
		cout << getStatus(getID()) -> averageWaitingTime << endl;
		cout << getStatus(getID()) -> numberOfBursts << endl;
		cout << getStatus(getID()) -> totalExecutionTime << endl;
		cout << getTimers(getID()) -> totalWaitingTime << endl;
		cout << getTimers(getID()) -> waitingCount << endl;*/
	}
}

void g() {
		/*
		 cout << getStatus(2) -> state << endl;
		 cout << "ready: ";
		 printQueue(&readyQueue);
		 cout << "terminate: ";
		 printQueue(&terminateQueue);
		 */
		cout << "mein g hun" << endl;
		int* p = (int *)GetThreadResult(2);
		cout << "printing inside g: " << *p;
		/*cout << getStatus(0) -> averageExecutionTimeQuantum << endl;
		cout << getStatus(0) -> averageWaitingTime << endl;
		cout << getStatus(0) -> numberOfBursts << endl;
		cout << getStatus(0) -> totalExecutionTime << endl;
		cout << getTimers(0) -> totalWaitingTime << endl;
		cout << getTimers(0) -> waitingCount << endl;
		cout << "g ki values" << endl;
		cout << getStatus(getID()) -> averageExecutionTimeQuantum << endl;
		cout << getStatus(getID()) -> averageWaitingTime << endl;
		cout << getStatus(getID()) -> numberOfBursts << endl;
		cout << getStatus(getID()) -> totalExecutionTime << endl;
		cout << getTimers(getID()) -> totalWaitingTime << endl;
		cout << getTimers(getID()) -> waitingCount << endl;*/
}

void* k(void* a) {
	unsigned long i = 0;
	while (i<600000) {
		i++;
		//if(i%1000000 == 0){
			//cout << "k: ";
			//cout << getStatus(getID()) -> numberOfBursts << endl;
		//}

		int* p = (int *) a;
		//cout << "I am an arg, I am alive & my value is :" << *p << endl;
		//cout << "inside kkkkkkkkkkkkk" << endl;
		/*cout << getStatus(getID()) -> averageExecutionTimeQuantum << endl;
		cout << getStatus(getID()) -> averageWaitingTime << endl;
		cout << getStatus(getID()) -> numberOfBursts << endl;
		cout << getStatus(getID()) -> totalExecutionTime << endl;
		cout << getTimers(getID()) -> totalWaitingTime << endl;
		cout << getTimers(getID()) -> waitingCount << endl;*/
	}
	return a;
}

int main() {
	cout << "reached";
	int a = 5;
		int *p = &a;
	cout << create(f) << endl;
	cout << create(g) << endl;
	cout << createWithArgs(k, p) << endl;



	//cout << getStatus(2) -> state << endl;

	start();
	return 0;
}
