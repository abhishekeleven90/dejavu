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
	keyMap dataValMap;
};

//****************Function Declarations*******************
void joinIpWithPort(char* ip, unsigned int port, char* ipWithPort);

char* fetchAddress(char* cmd, int pos);
unsigned int fetchPortNumber(char* string, int pos);

bool isValidAddress(char* addr);
bool isValidPort(unsigned int port);
bool checkIfPartOfNw(Node* node);

void printNotInNetworkErrorMessage();
void printInNetworkErrorMessage();
void printAllFingerTable(Node* node);
void printDataValMap(Node* node);
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

bool checkIfNotPartOfNw(Node* node) {
	if (node->self != NULL) {
		printInNetworkErrorMessage();
		return false;
	}
	return true;
}

void printNotInNetworkErrorMessage() {
	cout << "Hey!!! I am not yet in the network, try again later" << endl;
}

void printInNetworkErrorMessage() {
	cout << "Hey!!! I am in the network, try again later" << endl;
}

void printAllFingerTable(Node* node) {
	for (int i = 0; i < M; i++) {
		cout << "start: " << node-> fingerStart[i] << ", ";
		cout << "node: " << node-> fingerNode[i]->ipWithPort << endl;
	}
}

void printDataValMap(Node* node) {
	map<char*, char*>::iterator it;

	for (map<char*, char*>::iterator it = (node->dataValMap).begin(); it
			!= (node->dataValMap).end(); ++it) {
		cout << it->first << " : " << it->second << '\n';
	}
}

void printNodeDetails(Node* node) {
	cout << "self-> " << node->self->ipWithPort << endl;
	cout << "Self Key-> " << node->self->nodeKey << endl;
	cout << "Successor-> " << node->successor->ipWithPort << endl;
	cout << "Predecessor-> " << node->predecessor->ipWithPort << endl;
	cout << "Finger table: " << endl;
	printAllFingerTable(node);
	printDataValMap(node);
}

void helperHelpNewCmd() {
	cout << endl;
	tab(1);
}

void getMyIp(char* ip) {
	struct ifaddrs * ifAddrStruct = NULL;
	struct ifaddrs * ifa = NULL;
	void * tmpAddrPtr = NULL;

	getifaddrs(&ifAddrStruct);

	for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
		if (!ifa->ifa_addr) {
			continue;
		}
		if (ifa->ifa_addr->sa_family == AF_INET) { // check it is IP4
			// is a valid IP4 Address
			tmpAddrPtr = &((struct sockaddr_in *) ifa->ifa_addr)->sin_addr;
			char addressBuffer[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
			//printf("%s IP Address %s\n", ifa->ifa_name, addressBuffer);
			if (strcmp(ifa->ifa_name, "eth0") == 0) {
				strcpy(ip, addressBuffer);
			}
		} else if (ifa->ifa_addr->sa_family == AF_INET6) { // check it is IP6
			// is a valid IP6 Address
			tmpAddrPtr = &((struct sockaddr_in6 *) ifa->ifa_addr)->sin6_addr;
			char addressBuffer[INET6_ADDRSTRLEN];
			inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);
			//printf("%s IP Address %s\n", ifa->ifa_name, addressBuffer);
		}
	}

	if (ifAddrStruct != NULL) {
		freeifaddrs(ifAddrStruct);
	}
}

int getMyPort(int mySock) {
	struct sockaddr_in sin;
	socklen_t addrlen = sizeof(sin);
	if (getsockname(mySock, (struct sockaddr *) &sin, &addrlen) == 0
			&& sin.sin_family == AF_INET && addrlen == sizeof(sin)) {
		int local_port = ntohs(sin.sin_port);
		return local_port;
	} else
		; // handle error
}
