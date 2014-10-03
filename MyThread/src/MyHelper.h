//----------Constants---------
#define M 160	//number of bits
#define SECOND 1000000
#define QUEUE_LIMIT 5

#define DATA_SIZE_TOO_LARGE 131072
#define DATA_SIZE_LARGE 16384
#define DATA_SIZE_KILO 1024

#define NODE_DELIM_CHAR ';'
#define NODE_DELIM_STR ";"

//----------Globals---------
char GLOBAL_ARR[M][DATA_SIZE_LARGE];
char FINGER_ARR[M][DATA_SIZE_KILO];
char KEY_VALUES[M][DATA_SIZE_KILO];

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

Node* helperNode = new Node;
Node* selfNode = new Node;

//****************Function Declarations*******************
void joinIpWithPort(char* ip, unsigned int port, char* ipWithPort);

char* fetchAddress(char* cmd, int pos);
unsigned int fetchPortNumber(char* string, int pos);

bool isValidAddress(char* addr);
bool isValidPort(unsigned int port);
bool checkIfPartOfNw(Node* node);

void printNotInNetworkErrorMessage();
void printInNetworkErrorMessage();
void printAllFingerTable(Node* node, int isUnique);
void printDataValMap(Node* node);
void printNodeDetails(Node* node);
void printDump(char* dumpData, int isUnique);

void helperHelpNewCmd();

void getMyIp(char* ip);
int getMyPort(int mySock);

nodeHelper* convertToNodeHelper(char *ipWithPort);

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

void printAllFingerTable(Node* node, int isUnique) {
	for (int i = 0; i < M; i++) {
		char* nodeFinger = node-> fingerNode[i]->ipWithPort;

		if (isUnique && strcmp(nodeFinger, node->self->ipWithPort) == 0) {
			continue; //not printing self finger entries
		}

		cout << i << ": " << "\t";
		cout << node-> fingerStart[i] << ", ";
		cout << nodeFinger << endl;
	}
}

void printDataValMap(Node* node) {
	map<char*, char*>::iterator it;

	for (map<char*, char*>::iterator it = (node->dataValMap).begin(); it
			!= (node->dataValMap).end(); ++it) {
		cout << it->first << " : " << it->second << '\n';
	}
}

void printNodeDetails(Node* node, int isUnique) {
	cout << "Node-> " << node->self->ipWithPort << endl;
	cout << "Node Key-> " << node->self->nodeKey << endl;
	cout << "Successor-> " << node->successor->ipWithPort << endl;
	cout << "Predecessor-> " << node->predecessor->ipWithPort << endl;
	cout << "Finger table: " << endl;
	printAllFingerTable(node, isUnique);
	printDataValMap(node);
}

void printDump(char* dumpData, int isUnique) {
	for (int i = 0; i < M; i++) {
		memset(GLOBAL_ARR[i], 0, sizeof GLOBAL_ARR[i]);
	}
	for (int i = 0; i < M; i++) {
		memset(FINGER_ARR[i], 0, sizeof FINGER_ARR[i]);
	}
	for (int i = 0; i < M; i++) {
		memset(KEY_VALUES[i], 0, sizeof KEY_VALUES[i]);
	}

	split(dumpData, '|', GLOBAL_ARR);

	split(GLOBAL_ARR[1], ',', FINGER_ARR);

	for (int i = 0; i < M; i++) {
		strcpy(helperNode->fingerStart[i], FINGER_ARR[i]);
	}

	split(GLOBAL_ARR[2], ',', FINGER_ARR);

	for (int i = 0; i < M; i++) {
		char* tmp = FINGER_ARR[i];
		helperNode->fingerNode[i] = convertToNodeHelper(tmp);
	}

	char ipWithPort[3][DATA_SIZE_KILO];
	split(GLOBAL_ARR[0], ',', ipWithPort);

	split(GLOBAL_ARR[3], ',', KEY_VALUES);

	helperNode->self = convertToNodeHelper(ipWithPort[0]);
	helperNode->successor = convertToNodeHelper(ipWithPort[1]);
	helperNode->predecessor = convertToNodeHelper(ipWithPort[2]);

	printNodeDetails(helperNode, isUnique);

	int occ = countOccurence(GLOBAL_ARR[3], ',');
	for (int i = 0; i < occ; i++) {
		cout << KEY_VALUES[i] << endl;
	}
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
			if (strcmp(ifa->ifa_name, "eth0") == 0 || strcmp(ifa->ifa_name,
					"wlan0") == 0) {
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
	} else {
		; // handle error
	}
	return 0;
}

nodeHelper* convertToNodeHelper(char *ipWithPort) {
	nodeHelper* toReturn = new nodeHelper;

	//cout << "inside convertToNodeHelper: ipWithPort " << ipWithPort << endl;
	strcpy(toReturn->ipWithPort, ipWithPort);
	char* ipAddr = substring(ipWithPort, 0, indexOf(ipWithPort, ':'));
	char* portString = substring(ipWithPort, indexOf(ipWithPort, ':') + 2,
			strlen(ipWithPort));
	//cout << "inside convertToNodeHelper: " << portString << endl;
	unsigned int portNum = atoi(portString);

	strcpy(toReturn->ip, ipAddr);
	toReturn->port = portNum;

	char hexHash[HASH_HEX_BITS];
	data2hexHash(toReturn->ipWithPort, hexHash);
	strcpy(toReturn->nodeKey, hexHash);

	return toReturn;
}
