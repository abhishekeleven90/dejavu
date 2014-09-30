#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
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
#include "MyHelper.h"

using namespace std;

//----------Constants---------
#define SECOND 1000000
#define QUEUE_LIMIT 5
#define DATA_SIZE 16384
#define KILO 1024

#define msgJoin 'j'
#define msgDump 'd'
#define msgQuit 'e'
#define msgNodeSucc 's'
#define msgNodePred 'p'
#define msgChangeSucc 'a'
#define msgChangePred 'b'
#define msgKeySucc 'k'
#define msgFinger 'f'
#define msgConnectFailed 'x'
#define msgPut 'i'
#define msgGet 'g'

//----------Globals---------
char ui_data[KILO];
char server_send_data[DATA_SIZE], server_recv_data[DATA_SIZE];
char client_send_data[DATA_SIZE], client_recv_data[DATA_SIZE];

unsigned int server_port = 0;
unsigned int remote_port = 0; // port with which to connect to server
char ip2Join[IP_SIZE]; //used by client to join the server

int serverThreadId;
int serverSock;

int isCreated = false;
int isJoined = false;
int retry_count = 5;

int isFirstJoin = true;

Node* selfNode = new Node;

//****************Function Declarations*******************
//-------Helper Functions----------
void runClientAndWaitForResult(int clientThreadID);

void helperHelp();
void helperPort(char* portCmd);
void helperCreate();
void helperJoin(char* joinCmd);
void helperQuit();
void helperPut(char* putCmd);
void helperGet(char* getCmd);

void helperFinger();
void helperSuccessor();
void helperPredecessor();
void helperDump();
void helperDumpAddr(char* dumpAddrCmd);
void helperDumpAll();

void populateFingerTableSelf();
void fillNodeEntries(struct sockaddr_in server_addr);

void askSuccForFinger();
void askSuccToQuit();

void processQuit(char *data);
void processSucc();
void processPred();
void processFinger(char *data);
void processChangeSucc(char *addr);
void processChangePred(char *addr);
void processKeySucc(char *keyToSearch);
void processGetDump();

void changeSuccAndFixFirstFinger(nodeHelper* succ);

//-----TCP Functions-------
void userInput();
void server();
void client();

//-----CHORD Functions-------
void fixFingers();
nodeHelper* get_SuccFromRemoteNode(nodeHelper* remoteNode);
nodeHelper* get_PredFromRemoteNode(nodeHelper* remoteNode);
void changeSuccOfRemoteNodeToMyself(nodeHelper* remoteNode);
void changePredOfRemoteNodeToMyself(nodeHelper* remoteNode);
nodeHelper* find_successor(char key[]);
nodeHelper* closest_preceding_finger(char key[]);

//latest functions TO-DO remove
nodeHelper* convertToNodeHelper(char *ipWithPort);
nodeHelper* getKeySuccFromRemoteNode(nodeHelper* remoteNode, char key[]);

//****************Function Definitions*******************
//-------Helper Functions----------

void runClientAndWaitForResult(int clientThreadID) {
	client_recv_data[0] = '\0';
	run(clientThreadID);
	while (client_recv_data[0] == '\0')
		; //wait until data is received

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

void helperPort(char* portCmd) {
	if (!checkIfNotPartOfNw(selfNode)) {
		return;
	}

	server_port = fetchPortNumber(portCmd, 6);
	if (server_port != 0) {
		cout << "port: set to " << server_port << endl;
	} else {
		cout << "port Number did not set" << endl;
	}
}

void helperCreate() {
	//create a listening socket here
	if (isCreated) {
		cout << "Already in a network, server thread running" << endl;
		return;
	}

	isCreated = true;
	isJoined = true;
	serverThreadId = create(server);
	run(serverThreadId);
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

	strcpy(ip2Join, addr);
	remote_port = port;

	char remoteIpWithPort[IP_SIZE];
	joinIpWithPort(ip2Join, remote_port, remoteIpWithPort);
	nodeHelper* remoteNodeHelper = convertToNodeHelper(remoteIpWithPort);

	if (isJoined) {
		cout << "Already in a network and joined, joined thread running"
				<< endl;
		return;
	}

	cout << "Creating myself!!" << endl;
	helperCreate();

	//Making the connection
	client_send_data[0] = 'j';
	client_send_data[1] = ':';
	client_send_data[2] = '\0';
	int clientThreadID = create(client);

	runClientAndWaitForResult(clientThreadID);

	if (client_recv_data[0] == 'x') {
		isJoined = false;
		isCreated = false;
		deleteThread(serverThreadId);
		selfNode->self = NULL;
		close(serverSock);
		return;
	}

	cout << "Node joined to server" << endl;

	retry_count = 9999; //Modifying the retry count for all the future connections

	cout << "Asking the known remote node for my actual successor" << endl;
	changeSuccAndFixFirstFinger(
			getKeySuccFromRemoteNode(remoteNodeHelper, selfNode->self->nodeKey));
	cout << "My actual successor is now: " << selfNode->successor->ipWithPort
			<< endl;
	selfNode->predecessor = get_PredFromRemoteNode(selfNode->successor);
	cout << "My actual predecessor is now: "
			<< selfNode->predecessor->ipWithPort << endl;

	cout << "changing succ.pred & pred.succ to me" << endl;
	changeSuccOfRemoteNodeToMyself(selfNode->predecessor);
	changePredOfRemoteNodeToMyself(selfNode->successor);

	int fixFingersID = create(fixFingers);
	run(fixFingersID);
}

void helperQuit() {
	if (!checkIfPartOfNw(selfNode)) {
		return;
	}

	cout
			<< "Asking all the nodes to shut down, Thanks for using chord_DHT, see you again soon :)"
			<< endl;

	char tmp[3] = "e:";
	strcpy(client_send_data, tmp);
	strcat(client_send_data, selfNode->self->ipWithPort);
	askSuccToQuit();
}

void putInMyMap(char* dataVal) {
	char dataValArr[2][KILO];
	split(dataVal, ' ', dataValArr);

	char hexHashKey[HASH_HEX_BITS];
	data2hexHash(dataValArr[0], hexHashKey);

	char dataForMap[KILO];
	memset(dataForMap, '\0', KILO);
	strcpy(dataForMap, dataValArr[0]);
	strcat(dataForMap, "=>");
	strcat(dataForMap, dataValArr[1]);
	strcat(dataForMap, "\0");

	insertInKeyMap(&selfNode->dataValMap, hexHashKey, dataForMap);
}

string getFromMyMap(char* data) {
	char hexHashKey[HASH_HEX_BITS];
	data2hexHash(data, hexHashKey);

	map<const char*, const char*>::iterator it;

	for (map<char*, char*>::iterator it = (selfNode->dataValMap).begin(); it
			!= (selfNode->dataValMap).end(); ++it) {
		cout << it->first << " : " << it->second << '\n';
	}

	if (!isPresentInKeyMap(selfNode->dataValMap, hexHashKey)) {
		cout << "Data not found for key: " << hexHashKey << endl;
		return NULL;
	}

	string dataVal = getFromKeyMap(selfNode->dataValMap, hexHashKey);
	return dataVal;
}

void helperPut(char* putCmd) {
	if (!checkIfPartOfNw(selfNode)) {
		return;
	}
	char dataVal[KILO];
	strcpy(dataVal, substring(putCmd, 5, strlen(putCmd) - 5));

	char dataValArr[2][KILO];

	split(dataVal, ' ', dataValArr);

	char hexHashKey[HASH_HEX_BITS];
	data2hexHash(dataValArr[0], hexHashKey);

	nodeHelper* remoteNode = find_successor(hexHashKey);
	if (strcmp(remoteNode->nodeKey, selfNode->self->nodeKey) == 0) {
		putInMyMap(dataVal);
	}

	else {
		char tmp[3] = "i:";
		strcpy(client_send_data, tmp);
		strcat(client_send_data, dataVal);

		strcpy(ip2Join, remoteNode->ip);
		remote_port = remoteNode->port;

		int clientThreadID = create(client);
		runClientAndWaitForResult(clientThreadID);
	}

	cout << "Data inserted: " << dataVal << " with: " << hexHashKey << ", at: "
			<< remoteNode->ipWithPort << endl;
}

void helperGet(char* getCmd) {
	if (!checkIfPartOfNw(selfNode)) {
		return;
	}

	string dataVal;

	char data[KILO];
	strcpy(data, substring(getCmd, 5, strlen(getCmd) - 5));

	char dataArr[1][KILO];

	split(data, ' ', dataArr);

	char hexHashKey[HASH_HEX_BITS];
	data2hexHash(dataArr[0], hexHashKey);

	nodeHelper* remoteNode = find_successor(hexHashKey);

	if (strcmp(remoteNode->nodeKey, selfNode->self->nodeKey) == 0) {
		dataVal = getFromMyMap(data);
	}

	else {
		char tmp[3] = "g:";
		strcpy(client_send_data, tmp);
		strcat(client_send_data, dataArr[0]);

		strcpy(ip2Join, remoteNode->ip);
		remote_port = remoteNode->port;

		int clientThreadID = create(client);
		runClientAndWaitForResult(clientThreadID);
		//strcpy(dataVal, client_recv_data);
		dataVal = client_recv_data;
	}

	cout << "Data Found: " << hexHashKey << "\t" << dataVal << endl;
}

void helperFinger() {
	if (!checkIfPartOfNw(selfNode)) {
		return;
	}

	char tmp[3] = "f:";
	strcpy(client_send_data, tmp);
	askSuccForFinger();
}

void helperSuccessor() {
	if (!checkIfPartOfNw(selfNode)) {
		return;
	}
	cout << "Successor-> " << selfNode->successor->ipWithPort << endl;
}

void helperPredecessor() {
	if (!checkIfPartOfNw(selfNode)) {
		return;
	}
	cout << "Predecessor-> " << selfNode->predecessor->ipWithPort << endl;
}

void helperDump() {
	if (!checkIfPartOfNw(selfNode)) {
		return;
	}

	printNodeDetails(selfNode);
	//TO-DO: needs to be implemented ::: printing keys stored
}

void helperDumpAddr(char* dumpAddrCmd) {
	if (!checkIfPartOfNw(selfNode)) {
		return;
	}

	char* addr = fetchAddress(dumpAddrCmd, 10);
	if (addr == NULL) {
		return;
	}
	unsigned int port = fetchPortNumber(dumpAddrCmd,
			indexOf(dumpAddrCmd, ':') + 2);

	if (port == 0) {
		//Invalid portNumber
		return;
	}

	strcpy(ip2Join, addr);
	remote_port = port;

	client_send_data[0] = 'd';
	client_send_data[1] = ':';
	client_send_data[2] = '\0';
	int clientThreadID = create(client);

	retry_count = 5; //Modifying the retry count assuming server IP may be incorrect
	runClientAndWaitForResult(clientThreadID);
	retry_count = 9999; //Modifying the retry count for all the future connections

	if (client_recv_data[0] == 'x') { //Server busy or does not exist
		return;
	}

	cout << "Dump Received" << endl;
	cout << client_recv_data << endl; //TO-DO: remove

	//split()
}

void helperDumpAll() {
	if (!checkIfPartOfNw(selfNode)) {
		return;
	}
	//TO-DO: needs to be implemented
}

//populates finger table with all the self entries - only node in the network
void populateFingerTableSelf() {
	for (int i = 0; i < M; i++) {
		selfNode->fingerNode[i] = selfNode->self;
	}
}

void fillNodeEntries(struct sockaddr_in server_addr) {
	nodeHelper* self = new nodeHelper();

	char ip[IP_SIZE];
	getMyIp(ip);

	strcpy(self->ip, ip);

	self->port = getMyPort(serverSock);

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

void askSuccForFinger() {
	strcpy(ip2Join, selfNode->successor->ip);
	remote_port = selfNode->successor->port;

	strcat(client_send_data, selfNode->self->ipWithPort);
	strcat(client_send_data, ",");
	cout << "Inside askSuccForFinger: clientsendData - " << client_send_data
			<< endl;
	int clientThreadID = create(client);
	runClientAndWaitForResult(clientThreadID);
}

void askSuccToQuit() {
	strcpy(ip2Join, selfNode->successor->ip);
	remote_port = selfNode->successor->port;

	cout << "Inside askSuccToQuit: clientsendData - " << client_send_data
			<< endl;
	int clientThreadID = create(client);
	run(clientThreadID);
}

void processJoin() {
	cout << "Client wants to join" << endl;
	if (isFirstJoin) {
		isFirstJoin = false;
		int fixFingersID = create(fixFingers);
		run(fixFingersID);
	}
	server_send_data[0] = 'q';
	server_send_data[1] = '\0';
}

void processSucc() {
	cout << "client wants my successor details" << endl;
	strcpy(server_send_data, selfNode->successor->ipWithPort);
}

void processPred() {
	cout << "client wants my predecessor details" << endl;
	strcpy(server_send_data, selfNode->predecessor->ipWithPort);
}

void processFinger(char *data) {
	char *startAddr = substring(data, 0, indexOf(data, ','));
	cout << "client wants to find the fingers" << endl;

	if (strcmp(startAddr, selfNode->self->ipWithPort) == 0) { // checking if I am the starting node
		cout << "Got all the fingers, printing now: " << endl;
		int occ = countOccurence(data, ',') + 1;
		char addressArr[occ][KILO];
		split(data, ',', addressArr);
		for (int i = 0; i < occ - 1; i++) {
			cout << i << " -> " << addressArr[i] << endl;
		}
	}

	else {
		strcpy(client_send_data, server_recv_data);
		askSuccForFinger();
	}
	server_send_data[0] = 'q';
	server_send_data[1] = '\0';
}

void shutMe() {
	cout << "Shutting myself after 5 sec" << endl;
	sleep(5);
	close(serverSock);
	clean();
}

void processQuit(char *data) {
	cout << "ChordRing shutting down, I need to shut as well" << endl;

	strcat(data, ",");
	char *startAddr = substring(data, 0, indexOf(data, ','));

	if (strcmp(startAddr, selfNode->self->ipWithPort) == 0) { // checking if I am the starting node
		cout << "Quit Request Sent to all the nodes" << endl;
		shutMe();
	} else {
		strcpy(client_send_data, server_recv_data);
		askSuccToQuit();
	}
	shutMe();
}

void processChangeSucc(char *addr) {
	cout << "client wants to change my succ to: " << addr << endl;

	changeSuccAndFixFirstFinger(convertToNodeHelper(addr));

	server_send_data[0] = 'q';
	server_send_data[1] = '\0';
}

void processChangePred(char *addr) {
	cout << "client wants to change my pred to: " << addr << endl;

	selfNode->predecessor = convertToNodeHelper(addr);

	server_send_data[0] = 'q';
	server_send_data[1] = '\0';
}

void processPut(char *dataVal) {
	cout << "client wants to put: " << dataVal << endl;
	putInMyMap(dataVal);

	server_send_data[0] = 'q';
	server_send_data[1] = '\0';
}

void processGet(char *data) {
	cout << "client wants to get val for: " << data << endl;

	char hexHashKey[HASH_HEX_BITS];
	data2hexHash(data, hexHashKey);
	char dataVal[KILO];

	strcpy(dataVal, getFromKeyMap(selfNode->dataValMap, hexHashKey));
	strcpy(server_send_data, dataVal);
}

void processKeySucc(char *keyToSearch) {
	cout << "Client requests for finding Node successor of this key: "
			<< keyToSearch << endl;
	nodeHelper* toReturn = find_successor(keyToSearch);
	strcpy(server_send_data, toReturn->ipWithPort);
}

void processGetDump() {
	cout << "Client wants my dump" << endl;

	strcpy(server_send_data, selfNode->self->ipWithPort);
	strcat(server_send_data, ",");
	strcat(server_send_data, selfNode->successor->ipWithPort);
	strcat(server_send_data, ",");
	strcat(server_send_data, selfNode->predecessor->ipWithPort);

	strcat(server_send_data, "|");

	for (int i = 0; i < M; i++) {
		strcat(server_send_data, selfNode->fingerStart[i]);
		strcat(server_send_data, ",");
	}

	strcat(server_send_data, "|");

	for (int i = 0; i < M; i++) {
		strcat(server_send_data, selfNode->fingerNode[i]->ipWithPort);
		strcat(server_send_data, ",");
	}
}

void changeSuccAndFixFirstFinger(nodeHelper* succ) {
	selfNode->successor = succ;
	selfNode->fingerNode[0] = succ;
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
			helperPut(ui_data);
		}

		else if (strcmp(cmdType, "get") == 0) {
			helperGet(ui_data);
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
			helperDumpAddr(ui_data);
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

	serverSock = sock;

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
		int bytes_received;
		sin_size = sizeof(struct sockaddr_in);
		connected
				= accept(sock, (struct sockaddr*) ((&client_addr)), &sin_size);

		printf("\n I got a connection from (%s , %d)",
				inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

		cout << "closing the connection after communicating" << endl;
		bytes_received = recv(connected, server_recv_data, DATA_SIZE, 0);
		cout << "Data received from client" << endl;
		server_recv_data[bytes_received] = '\0';

		char* type = substring(server_recv_data, 0, 2);
		char* data = substring(server_recv_data, 3, strlen(server_recv_data));

		if (strcmp(type, "e:") == 0) {
			processQuit(data);
		}

		else if (strcmp(type, "j:") == 0) {
			processJoin();
		}

		else if (strcmp(type, "s:") == 0) {
			processSucc();
		}

		else if (strcmp(type, "p:") == 0) {
			processPred();
		}

		else if (strcmp(type, "f:") == 0) {
			processFinger(data);
		}

		else if (strcmp(type, "k:") == 0) {
			processKeySucc(data);
		}

		else if (strcmp(type, "d:") == 0) {
			processGetDump();
		}

		else if (strcmp(type, "a:") == 0) {
			processChangeSucc(data);
		}

		else if (strcmp(type, "b:") == 0) {
			processChangePred(data);
		}

		else if (strcmp(type, "i:") == 0) {
			processPut(data);
		}

		else if (strcmp(type, "g:") == 0) {
			processGet(data);
		}

		send(connected, server_send_data, strlen(server_send_data), 0);
		cout << "Done the required task, closing the connection" << endl;
		close(connected);
	}
	//right now, doesn't reach here
	close(sock);
}

bool connectToServer(int & sock) {
	struct hostent *host;
	struct sockaddr_in server_addr;
	cout << "Inside connect to server: " << ip2Join << endl;
	host = gethostbyname(ip2Join);
	cout << "creating client socket" << endl;
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("Socket");
		exit(1);
	}
	cout << "client socket created" << endl;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(remote_port);
	server_addr.sin_addr = *((struct in_addr *) host->h_addr);
	bzero(&(server_addr.sin_zero), 8);
	cout << "client will try and connect to server" << endl;
	int retriedCount = 0;
	while (connect(sock, (struct sockaddr *) &server_addr,
			sizeof(struct sockaddr)) == -1) {

		//trying again assuming the server is busy
		retriedCount++;
		cout << "server busy --- retrying(" << retriedCount << "/"
				<< retry_count << ")" << endl;
		sleep(1);
		if (retriedCount == retry_count) {
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
		client_recv_data[0] = 'x'; //Inserting this --- to be used in helperJoin
		return;
	}

	cout << "Client socket ID:" << sock << endl;

	send(sock, client_send_data, strlen(client_send_data), 0);

	bytes_recieved = recv(sock, client_recv_data, DATA_SIZE, 0);
	cout << "Data successfully received" << endl;
	client_recv_data[bytes_recieved] = '\0';
	close(sock);
}

//-----------CHORD FUNCTIONS-------
void fixFingers() {
	int fixFingerIndex = 1;

	while (true) {
		if (fixFingerIndex > M - 1) {
			fixFingerIndex = 1;
		}

		char* key = selfNode->fingerStart[fixFingerIndex];
		char* me = selfNode->self->nodeKey;
		char* succKey = selfNode->successor->nodeKey;
		char* predKey = selfNode->successor->nodeKey;

		if (strcmp(key, succKey) == 0 || keyBelongCheck(me, succKey, key)) {
			selfNode->fingerNode[fixFingerIndex] = selfNode->successor;
		}

		else if (strcmp(key, me) == 0 || keyBelongCheck(predKey, me, key)) {
			selfNode->fingerNode[fixFingerIndex] = selfNode->self;
		}

		else {
			selfNode->fingerNode[fixFingerIndex] = find_successor(key);
		}

		fixFingerIndex++;
	}
}

nodeHelper* get_SuccFromRemoteNode(nodeHelper* remoteNode) {
	strcpy(ip2Join, remoteNode->ip);
	remote_port = remoteNode->port;
	client_send_data[0] = 's';
	client_send_data[1] = ':';
	client_send_data[2] = '\0';
	int clientThreadID = create(client);
	runClientAndWaitForResult(clientThreadID);
	cout << "Got the successor from remote node: " << client_recv_data << endl;
	return convertToNodeHelper(client_recv_data);
}

nodeHelper* get_PredFromRemoteNode(nodeHelper* remoteNode) {
	strcpy(ip2Join, remoteNode->ip);
	remote_port = remoteNode->port;
	client_send_data[0] = 'p';
	client_send_data[1] = ':';
	client_send_data[2] = '\0';
	int clientThreadID = create(client);
	runClientAndWaitForResult(clientThreadID);
	cout << "Got the predecessor from remote node: " << client_recv_data
			<< endl;
	return convertToNodeHelper(client_recv_data);
}

void changeSuccOfRemoteNodeToMyself(nodeHelper* remoteNode) {
	strcpy(ip2Join, remoteNode->ip);
	remote_port = remoteNode->port;

	char tmp[3] = "a:";
	strcpy(client_send_data, tmp);
	strcat(client_send_data, selfNode->self->ipWithPort);

	int clientThreadID = create(client);
	runClientAndWaitForResult(clientThreadID);
	cout << "Changed the successor of remote node to myself" << endl;
}

void changePredOfRemoteNodeToMyself(nodeHelper* remoteNode) {
	strcpy(ip2Join, remoteNode->ip);
	remote_port = remoteNode->port;

	char tmp[3] = "b:";
	strcpy(client_send_data, tmp);
	strcat(client_send_data, selfNode->self->ipWithPort);

	int clientThreadID = create(client);
	runClientAndWaitForResult(clientThreadID);
	cout << "Changed the predecessor of remote node to myself" << endl;
}

nodeHelper* find_successor(char key[]) {
	char* nodeKey = selfNode->self->nodeKey;
	char* succKey = selfNode->successor->nodeKey;

	if (strcmp(key, succKey) == 0 || keyBelongCheck(nodeKey, succKey, key)) {
		return selfNode->successor;
	} else {
		nodeHelper* closestPrecedingNode = closest_preceding_finger(key);
		if (strcmp(closestPrecedingNode->nodeKey, selfNode->self->nodeKey) == 0) {
			return selfNode->self;
		} else {
			return getKeySuccFromRemoteNode(closestPrecedingNode, key);
		}
	}
}

nodeHelper* closest_preceding_finger(char key[]) {
	for (int i = M - 1; i >= 0; i--) {
		char* fingerNodeId = selfNode->fingerNode[i]->nodeKey;
		if (keyBelongCheck(selfNode->self->nodeKey, key, fingerNodeId)) {
			return selfNode->fingerNode[i];
		}
	}
	return selfNode->self;
}

nodeHelper* getKeySuccFromRemoteNode(nodeHelper* remoteNode, char key[]) {

	strcpy(ip2Join, remoteNode->ip);
	remote_port = remoteNode->port;
	char tmp[] = "k:";
	strcpy(client_send_data, tmp); //we will be acting as client to send data
	strcat(client_send_data, key);
	cout << "Sending this data to remote node: " << client_send_data << endl;
	int clientThreadID = create(client);
	runClientAndWaitForResult(clientThreadID);

	cout << "Data received from remote node " << client_recv_data << endl;
	//now make a nodeHelper and return based on ipWithPort received
	return convertToNodeHelper(client_recv_data);

}

nodeHelper* convertToNodeHelper(char *ipWithPort) {
	nodeHelper* toReturn = new nodeHelper;

	strcpy(toReturn->ipWithPort, ipWithPort);
	char* ipAddr = substring(ipWithPort, 0, indexOf(ipWithPort, ':'));
	char* portString = substring(ipWithPort, indexOf(ipWithPort, ':') + 2,
			strlen(ipWithPort));
	unsigned int portNum = atoi(portString);

	strcpy(toReturn->ip, ipAddr);
	toReturn->port = portNum;

	char hexHash[HASH_HEX_BITS];
	data2hexHash(toReturn->ipWithPort, hexHash);
	strcpy(toReturn->nodeKey, hexHash);

	return toReturn;
}

//-----------MAIN---------------

int main() {
	create(userInput);
	start();
	return 0;

	/*putInMyMap("123 kapil");
	 getFromMyMap("123");*/

	/*char ip[IP_SIZE];
	 getMyIp(ip);
	 cout << ip;*/
}
