#include <list>
#include <iostream>
using namespace std;

typedef struct {

	int a;

} node;

void enque(list<node*> l, node* n) {
	l.push_back(n);
}

void deque(list<node*> l) {
 l.pop_front();
 }

int main() {

	list<node*> jj;
	node* n = new node;
	n->a = 9;

	enque(jj, n);

	n->a = 5;
	enque(jj, n);
	list<node*>::iterator i;
	for(i=jj.begin(); i != jj.end(); ++i) cout << (*i) << " ";
	cout << endl;

	cout << jj.size();
	deque(jj);
	n = jj.front();
	cout << jj.size();

	return 0;
}
