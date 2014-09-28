#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <map>
#include <openssl/evp.h>
#include <math.h>
#include "MyThread.h"
#include "MyHash.h"
#include "MyString.h"

using namespace std;

//----------Constants---------
#define SECOND 1000000
#define QUEUE_LIMIT 5
#define RETRY_COUNT 5

#define M 160	//number of bits
//----------Globals---------
char ui_data[1024];
char server_send_data[1024], server_recv_data[1024];
char client_send_data[1024], client_recv_data[1024];

unsigned int server_port = 0;
unsigned int client_port = 0; // port with which to connect to server
char* ip2Join; //used by client to join the server

int created = false;
int joined = false;

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

Node* selfNode = new Node;

//****************Function Declarations*******************
//-------Helper Functions----------
void joinIpWithPort(char* ip, unsigned int port, char* ipWithPort);

char* fetchAddress(char* cmd, int pos);
unsigned int fetchPortNumber(char* string, int pos);

bool isValidAddress(char* addr);
bool isValidPort(unsigned int port);

void printNotInNetworkErrorMessage();
bool checkIfPartOfNw();

void helperHelpNewCmd();

void helperHelp();
void helperPort(char* portCmd);
void helperCreate();
void helperJoin(char* joinCmd);
void helperQuit();
void helperPut();
void helperGet();

void helperFinger();
void helperSuccessor();
void helperPredecessor();
void helperDump();
void helperDumpAddr();
void helperDumpAll();

void populateFingerTableSelf();
void printAllFingerTable();

void fillNodeEntries(struct sockaddr_in server_addr);

//-----TCP Functions-------
void userInput();
void server();
void client();

//-----CHORD Functions-------
void get_SuccFromRemoteNode(nodeHelper* remoteNode);
nodeHelper* find_successor(char key[HASH_HEX_BITS]);
nodeHelper* closest_preceding_finger(char key[HASH_HEX_BITS]);

//****************Function Definitions*******************
//-------Helper Functions----------

void joinIpWithPort(char* ip, unsigned int port, char* ipWithPort) {
	char portChar[10];
	intToChar(port, portChar);
	strcpy(ipWithPort, ip);
	strcat(ipWithPort, ":");
	strcat(ipWithPort, portChar);
}

void helperHelpNewCmd() {
	cout << endl;
	tab(1);
}

void helperHelp() {
	cout << "Commands supported: " << endl;

	helperHelpNewCmd();
	cout << "help";
	tab(4);
	cout << "==> Provides a list of command and their usage details";

	helperHelpNewCmd();
	cout << "port <x>";
	tab(3);
	cout << "==> sets the port number";
	cout << " (eg- port 1234)";

	helperHelpNewCmd();
	cout << "create";
	tab(4);
	cout << "==> creates a ring";

	helperHelpNewCmd();
	cout << "join <x>";
	tab(3);
	cout << "==> joins the ring with x address";
	cout << " (eg- join 111.111.111.111:1000)";

	helperHelpNewCmd();
	cout << "quit";
	tab(4);
	cout << "==> shuts down the ring";

	helperHelpNewCmd();
	cout << "put <key> <value>";
	tab(2);
	cout << "==> inserts the given <key,value> pair in the ring ";
	cout << " (eg- put 23 654)";

	helperHelpNewCmd();
	cout << "get <key>";
	tab(3);
	cout << "==> returns the value corresponding to the key";
	cout << " (eg- get 23)";

	cout << endl;

	//Bonus Commands
	helperHelpNewCmd();
	cout << "-Yes!!! we have implemented BONUS COMMANDS, mentioned them below-";

	helperHelpNewCmd();
	cout << "finger";
	tab(4);
	cout << "==> prints the list of addresses of nodes on the ring";

	helperHelpNewCmd();
	cout << "successor";
	tab(3);
	cout << "==> prints the address of the next node on the ring";

	helperHelpNewCmd();
	cout << "predecessor";
	tab(3);
	cout << "==> prints the address of the previous node on the ring";

	helperHelpNewCmd();
	cout << "dump";
	tab(4);
	cout << "==> displays all information pertaining to calling node";

	helperHelpNewCmd();
	cout << "dumpaddr <address>";
	tab(2);
	cout << "==> displays all information pertaining to node at address";
	cout << " (eg - dump 111.111.111.111:1000)";

	helperHelpNewCmd();
	cout << "dumpall";
	tab(4);
	cout << "==> displays all information of all the nodes";

	cout << endl;
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

bool isValidAddress(char* addr) {
	int dotCount = countOccurence(addr, '.');
	int colonCount = countOccurence(addr, ':');

	if (colonCount == 1 && dotCount == 3) {
		return true;
	}
	return false;
}

unsigned int fetchPortNumber(char* string, int pos) {
	char *portNumber = substring(string, pos, strlen(string) - pos);
	unsigned int port = atoi(portNumber);
	if (isValidPort(port)) {
		return port;
	}
	return 0;
}

bool isValidPort(unsigned int port) {
	if (port == 0 || port > 65535) {
		cout << "isValidPort: port invalid or reserved" << endl;
		return false;
	}
	return true;
}

void helperPort(char* portCmd) {
	server_port = fetchPortNumber(portCmd, 6);
	if (server_port != 0) {
		cout << "port: set to " << server_port << endl;
	} else {
		cout << "port Number did not set" << endl;
	}
}

void helperCreate() {
	//create a listening socket here
	if (created == false) {
		created = true;
		joined = true;
		int serverId = create(server);
		run(serverId);
	} else {
		cout << "Already in a network, server thread running" << endl;
	}
}

void helperJoin(char* joinCmd) {
	char* addr = fetchAddress(joinCmd, 6);
	if (addr == NULL) {
		return;
	}
	unsigned int port = fetchPortNumber(joinCmd, indexOf(joinCmd, ':') + 2);

	if (port == 0) {
		//Invalid portNumber
		return;
	}

	ip2Join = addr;
	client_port = port;

	if (joined == false) {
		helperCreate(); //The node needs to be created as a server as well
		client_send_data[0] = 'q';
		client_send_data[1] = ':';
		client_send_data[2] = '\0';
		int clientThreadID = create(client);
		run(clientThreadID);
	} else {
		cout << "Already in a network and joined, joined thread running"
				<< endl;
	}
}

void printNotInNetworkErrorMessage() {
	cout << "Hey!!! I am not yet in the network, try again later" << endl;
}

bool checkIfPartOfNw() {
	if (selfNode->self == NULL) {
		printNotInNetworkErrorMessage();
		return false;
	}
	return true;
}

void helperSuccessor() {
	if (!checkIfPartOfNw()) {
		return;
	}
	cout << "Successor-> " << selfNode->successor->ipWithPort << endl;
}

void helperPredecessor() {
	if (!checkIfPartOfNw()) {
		return;
	}
	cout << "Predecessor-> " << selfNode->predecessor->ipWithPort << endl;
}

void printNodeDetails(Node* node) {
	cout << "self-> " << node->self->ipWithPort << endl;
	cout << "Self Key-> " << node->self->nodeKey << endl;
	cout << "Successor-> " << node->successor->ipWithPort << endl;
	cout << "Predecessor-> " << node->predecessor->ipWithPort << endl;
	cout << "Finger table: " << endl;
	printAllFingerTable();
}

void helperDump() {
	if (!checkIfPartOfNw()) {
		return;
	}
	printNodeDetails(selfNode);
}

void helperDumpAll() {
	if (!checkIfPartOfNw()) {
		return;
	}
	//TO-DO: needs to be implemented

	/*nodeHelper* tmpNode = new nodeHelper;
	 strcpy(tmpNode->ip, "0.0.0.0");
	 tmpNode->port = 5000;
	 get_SuccFromRemoteNode(tmpNode);*/
}

void helperDumpAddr() {
	if (!checkIfPartOfNw()) {
		return;
	}
	//TO-DO: needs to be implemented
}

void askSuccForFinger() {
	strcat(client_send_data, selfNode->self->ipWithPort);
	int clientThreadID = create(client);
	client_recv_data[0] = '\0';
	run(clientThreadID);
	while (client_recv_data[0] == '\0')
		; //wait until data is received

}
void helperFinger() {
	if (!checkIfPartOfNw()) {
		return;
	}

	ip2Join = selfNode->successor->ip;
	client_port = selfNode->successor->port;

	char tmp[3] = "f:";
	strcpy(client_send_data, tmp);
	askSuccForFinger();
}

void helperPut() {
	if (!checkIfPartOfNw()) {
		return;
	}
	//TO-DO: needs to be implemented
}

void helperGet() {
	if (!checkIfPartOfNw()) {
		return;
	}
	//TO-DO: needs to be implemented
}

void helperQuit() {
	if (!checkIfPartOfNw()) {
		return;
	}
	cout << "Thanks for using chord_DHT, see you again soon :)" << endl;
	clean(); //cleaning all the threads & exiting
	//TO-DO: needs to be implemented
}

//populates finger table with all the self entries - only node in the network
void populateFingerTableSelf() {
	for (int i = 0; i < M; i++) {
		selfNode->fingerNode[i] = selfNode->self;
	}
}

void printAllFingerTable() {
	for (int i = 0; i < M; i++) {
		cout << "start: " << selfNode-> fingerStart[i] << ", ";
		cout << "node: " << selfNode-> fingerNode[i]->nodeKey << endl;
	}
}

void fillNodeEntries(struct sockaddr_in server_addr) {
	nodeHelper* self = new nodeHelper();

	strcpy(self->ip, inet_ntoa(server_addr.sin_addr));
	self->port = ntohs(server_addr.sin_port);

	char ipWithPort[IP_SIZE];
	joinIpWithPort(self->ip, self->port, ipWithPort);
	strcpy(self->ipWithPort, ipWithPort);

	char hexHash[HASH_HEX_BITS];
	data2hexHash(self->ipWithPort, hexHash);
	strcpy(self->nodeKey, hexHash);

	selfNode->self = self;
	selfNode->successor = self;
	selfNode->predecessor = self;

	//Fill finger table
	int index = 39;
	for (int i = 0; i < M; i++) {
		if (i != 0 && i % 4 == 0) {
			index--;
		}
		char token[] = "0000000000000000000000000000000000000000";
		int tmp = pow(2, i % 4);
		token[index] = (char) (tmp + 48);
		char hexVal[HASH_HEX_BITS];
		hexAddition(self-> nodeKey, token, hexVal, strlen(self->nodeKey));
		strcpy(selfNode->fingerStart[i], hexVal);
	}
	populateFingerTableSelf();
}

//-----TCP Functions-------
void userInput() {
	while (1) {
		cout << "------------------------------" << endl;

		cout << ">>>: ";
		fgets(ui_data, sizeof(ui_data), stdin);

		cout << "<<<: " << ui_data << endl;

		char* cmdType = substring(ui_data, 0, indexOf(ui_data, ' '));

		if (strcmp(cmdType, "help") == 0) {
			helperHelp();
		}

		else if (strcmp(cmdType, "port") == 0) {
			helperPort(ui_data);
		}

		else if (strcmp(cmdType, "create") == 0) {
			helperCreate();
		}

		else if (strcmp(cmdType, "join") == 0) {
			helperJoin(ui_data);
		}

		else if (strcmp(cmdType, "quit") == 0) {
			helperQuit();
		}

		else if (strcmp(cmdType, "put") == 0) {
			helperPut();
		}

		else if (strcmp(cmdType, "get") == 0) {
			helperGet();
		}

		else if (strcmp(cmdType, "finger") == 0) {
			helperFinger();
		}

		else if (strcmp(cmdType, "successor") == 0) {
			helperSuccessor();
		}

		else if (strcmp(cmdType, "predecessor") == 0) {
			helperPredecessor();
		}

		else if (strcmp(cmdType, "dump") == 0) {
			helperDump();
		}

		else if (strcmp(cmdType, "dumpaddr") == 0) {
			helperDumpAddr();
		}

		else if (strcmp(cmdType, "dumpall") == 0) {
			helperDumpAll();
		}

		else {
			cout
					<< "sorry!!! It seems like you are new here, please type 'help' for list of commands"
					<< endl;
		}

		fflush(stdout);
	}
}

void server() {
	int sock, connected, bytes_recieved, trueint = 1;

	struct sockaddr_in server_addr, client_addr;
	unsigned int sin_size;

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("Socket");
		exit(1);
	}

	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &trueint, sizeof(int)) == -1) {
		perror("Setsockopt");
		exit(1);
	}

	server_addr.sin_family = AF_INET;
	if (server_port != 0) { //Let the server choose the port itself if not supplied externally
		server_addr.sin_port = htons(server_port);
	}
	server_addr.sin_addr.s_addr = INADDR_ANY;

	bzero(&(server_addr.sin_zero), 8);

	if (bind(sock, (struct sockaddr *) ((&server_addr)),
			sizeof(struct sockaddr)) == -1) {
		perror("Unable to bind");
		exit(1);
	}
	if (listen(sock, QUEUE_LIMIT) == -1) {
		perror("Listen");
		exit(1);
	}
	fillNodeEntries(server_addr);

	cout << "Starting to listen on: " << selfNode->self->ipWithPort << endl;
	cout << ">>>: ";
	fflush(stdout);
	while (1) {
		int bytes_recieved;
		sin_size = sizeof(struct sockaddr_in);
		connected
				= accept(sock, (struct sockaddr*) ((&client_addr)), &sin_size);

		printf("\n I got a connection from (%s , %d)",
				inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

		cout << "closing the connection after communicating" << endl;
		bytes_recieved = recv(connected, server_recv_data, 1024, 0);
		cout << "Data received from client" << endl;
		server_recv_data[bytes_recieved] = '\0';

		char* type = substring(server_recv_data, 0, 2);
		char* addrs = substring(server_recv_data, 3, strlen(server_recv_data));

		if (strcmp(type, "q:") == 0) { //"quit"
			cout << "client wants to disconnect" << endl;
			server_send_data[0] = 'q';
			server_send_data[1] = '\0';
		}

		else if (strcmp(type, "s:") == 0) { //"succ"
			cout << "client wants my successor details" << endl;
			strcpy(server_send_data, selfNode->successor->ipWithPort);
		}

		else if (strcmp(type, "f:") == 0) { //"finger"
			char* startAddr = substring(addrs, 0, indexOf(addrs, ',') + 1);
			cout << startAddr << endl; //TO-DO : remove the cout -- keep it until finger fully tested
			cout << "client wants to find the fingers" << endl;

			if (strcmp(startAddr, selfNode->self->ipWithPort) == 0) { // checking if I am the starting node
				cout << "Got all the fingers, printing now: " << endl;
				int occ = countOccurence(addrs, ',') + 1;
				char addressArr[occ][IP_SIZE];
				split(addrs, ',', addressArr);
				for (int i = 0; i < occ; i++) {
					cout << i << " -> " << addressArr[i] << endl;
				}
			} else {
				//TO-DO : needs to be tested
				strcpy(client_send_data, server_recv_data);
				strcat(client_send_data, ",");
				askSuccForFinger();
			}
			server_send_data[0] = 'q';
			server_send_data[1] = '\0';
		}

		send(connected, server_send_data, strlen(server_send_data), 0);
		cout << "Done the required task, closing" << endl;
		close(connected);
		cout << "closed" << endl;
	}
	//right now, doesn't reach here
	close(sock);
}

bool connectToServer(int & sock) {
	struct hostent *host;
	struct sockaddr_in server_addr;
	host = gethostbyname(ip2Join);
	cout << "creating client socket" << endl;
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("Socket");
		exit(1);
	}
	cout << "client socket created" << endl;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(client_port);
	server_addr.sin_addr = *((struct in_addr *) host->h_addr);
	bzero(&(server_addr.sin_zero), 8);
	cout << "client will try and connect to server" << endl;
	int retriedCount = 0;
	while (connect(sock, (struct sockaddr *) &server_addr,
			sizeof(struct sockaddr)) == -1) {

		//trying again assuming the server is busy
		retriedCount++;
		cout << "server busy --- retrying(" << retriedCount << "/"
				<< RETRY_COUNT << ")" << endl;
		sleep(1);
		if (retriedCount == RETRY_COUNT) {
			cout
					<< "server is not up or not responding, terminating client...please try again"
					<< endl;
			close(sock);
			return false;
		}
	}
	cout << "client connected to server\n" << endl;
	return true;
}

void client() {
	cout << "---client started---" << endl;

	int sock, bytes_recieved;

	if (!connectToServer(sock)) {
		return;
	}

	cout << "Client socket ID:" << sock << endl;

	send(sock, client_send_data, strlen(client_send_data), 0);

	bytes_recieved = recv(sock, client_recv_data, 1024, 0);
	cout << "Data successfully received" << endl;
	client_recv_data[bytes_recieved] = '\0';
	close(sock);
}

//-----------CHORD FUNCTIONS-------
void get_SuccFromRemoteNode(nodeHelper* remoteNode) {
	ip2Join = remoteNode->ip;
	client_port = remoteNode->port;
	client_send_data[0] = 's';
	client_send_data[1] = ':';
	client_send_data[2] = '\0';
	int clientThreadID = create(client);
	client_recv_data[0] = '\0';
	run(clientThreadID);
	while (client_recv_data[0] == '\0')
		; //wait until data is received
	cout << client_recv_data << endl;
}

nodeHelper* find_successor(char key[HASH_HEX_BITS]) {
	char* nodeKey = selfNode->self->nodeKey;
	char* succKey = selfNode->successor->nodeKey;

	if (strcmp(key, nodeKey) > 0 && strcmp(key, succKey) <= 0) {
		return selfNode->successor;
	} else {
		nodeHelper* closestPrecedingNode = closest_preceding_finger(key);
		if (strcmp(closestPrecedingNode->nodeKey, selfNode->self->nodeKey) == 0) {
			return selfNode->self;
		}
		//TO-DO: find successor on remote machine(closestPrecedingNode)
	}
}

nodeHelper* closest_preceding_finger(char key[HASH_HEX_BITS]) {
	for (int i = M - 1; i >= 0; i--) {
		char* fingerNodeId = selfNode->fingerNode[i]->nodeKey;
		if (strcmp(fingerNodeId, selfNode->self->nodeKey) > 0 && strcmp(
				fingerNodeId, key) < 0) {
			return selfNode->fingerNode[i];
		}
	}
	return selfNode->self;
}

//-----------MAIN---------------

int main() {
	create(userInput);
	start();
	return 0;
}
