//----------Constants---------
#define M 160	//number of bits

//----------Globals---------
struct nodeHelper {
	char nodeKey[HASH_HEX_BITS];
	char ip[IP_SIZE];
	unsigned int port;
	char ipWithPort[IP_SIZE];
};

struct Node {
	nodeHelper* self;
	nodeHelper* predecessor;
	nodeHelper* successor;

	char fingerStart[M][HASH_HEX_BITS];
	nodeHelper* fingerNode[M];
	map<const char*, const char*, cmp_key> keyMap;
};

//****************Function Declarations*******************
void joinIpWithPort(char* ip, unsigned int port, char* ipWithPort);

char* fetchAddress(char* cmd, int pos);
unsigned int fetchPortNumber(char* string, int pos);

bool isValidAddress(char* addr);
bool isValidPort(unsigned int port);
bool checkIfPartOfNw(Node* node);

void printNotInNetworkErrorMessage();
void printAllFingerTable(Node* node);
void printNodeDetails(Node* node);

void helperHelpNewCmd();

//****************Function Definitions*******************
void joinIpWithPort(char* ip, unsigned int port, char* ipWithPort) {
	char portChar[10];
	intToChar(port, portChar);
	strcpy(ipWithPort, ip);
	strcat(ipWithPort, ":");
	strcat(ipWithPort, portChar);
}

char* fetchAddress(char* cmd, int pos) {
	char* addrWithPort = substring(cmd, pos, strlen(cmd) - pos);
	if (!isValidAddress(addrWithPort)) {
		cout << "fetchAddress: Address not valid" << endl;
		return NULL;
	}
	char* addr = substring(addrWithPort, 0, indexOf(addrWithPort, ':'));
	return addr;
}

unsigned int fetchPortNumber(char* string, int pos) {
	char *portNumber = substring(string, pos, strlen(string) - pos);
	unsigned int port = atoi(portNumber);
	if (isValidPort(port)) {
		return port;
	}
	return 0;
}

bool isValidAddress(char* addr) {
	int dotCount = countOccurence(addr, '.');
	int colonCount = countOccurence(addr, ':');

	if (colonCount == 1 && dotCount == 3) {
		return true;
	}
	return false;
}

bool isValidPort(unsigned int port) {
	if (port == 0 || port > 65535) {
		cout << "isValidPort: port invalid or reserved" << endl;
		return false;
	}
	return true;
}

bool checkIfPartOfNw(Node* node) {
	if (node->self == NULL) {
		printNotInNetworkErrorMessage();
		return false;
	}
	return true;
}

void printNotInNetworkErrorMessage() {
	cout << "Hey!!! I am not yet in the network, try again later" << endl;
}

void printAllFingerTable(Node* node) {
	for (int i = 0; i < M; i++) {
		cout << "start: " << node-> fingerStart[i] << ", ";
		cout << "node: " << node-> fingerNode[i]->nodeKey << endl;
	}
}

void printNodeDetails(Node* node) {
	cout << "self-> " << node->self->ipWithPort << endl;
	cout << "Self Key-> " << node->self->nodeKey << endl;
	cout << "Successor-> " << node->successor->ipWithPort << endl;
	cout << "Predecessor-> " << node->predecessor->ipWithPort << endl;
	cout << "Finger table: " << endl;
	printAllFingerTable(node);
}

void helperHelpNewCmd() {
	cout << endl;
	tab(1);
}
