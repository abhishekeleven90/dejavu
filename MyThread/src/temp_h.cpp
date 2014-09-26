/*
#include <string.h>
#include <iostream>
#include <stdio.h>

#include <stdlib.h>
using namespace std;

#include "MyThread.h"
 #include <iostream>
 #define SECOND 1000000

 using namespace std;

 void f() {
 while (1) {
 cout << "f" << endl;
 usleep(SECOND);
 }
 }

 void g() {
 while (1) {
 cout << "g" << endl;
 usleep(SECOND);
 }
 }

 void* k(void* a) {
 while (1) {
 cout << "K" << endl;
 usleep(SECOND);
 //clean();
 }
 void* p;
 return p;
 }


 int main() {
 create(f);
 create(g);

 int a = 5;
 int *p = &a;
 createWithArgs(k, p);

 start();
 return 0;
 }




int main() {
	while (1) {
		char ui_data[1024];
		fgets(ui_data, sizeof(ui_data), stdin);
		cout << ui_data << endl;
		cout << "outside" << endl;
		if (strcmp(ui_data, "create\n") == 0) {
			cout << "inside";
		}
	}
	return 0;
}

*/
