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
#define MSG_JOIN "j:"
#define MSG_DUMP "d:"
#define MSG_DUMP_ALL "u:"
#define MSG_QUIT "e:"
#define MSG_NODE_SUCC "s:"
#define MSG_NODE_PRED "p:"
#define MSG_CHNG_SUCC "a:"
#define MSG_CHNG_PRED "b:"
#define MSG_KEY_SUCC "k:"
#define MSG_FINGER "f:"
#define MSG_FIX_FINGER "o:"
#define MSG_PUT "i:"
#define MSG_GET "g:"
#define MSG_ACK "m:"
#define MSG_FINGER_ACK "1:"

#define SERVER_BUSY 'x'
#define SERVER_DIFF_RING "c:"

//----------Globals---------
char ui_data[DATA_SIZE_KILO];
char server_send_data[DATA_SIZE_KILO], server_recv_data[DATA_SIZE_KILO];
char client_send_data[DATA_SIZE_KILO], client_recv_data[DATA_SIZE_KILO];
char ff_client_send_data[DATA_SIZE_KILO], ff_client_recv_data[DATA_SIZE_KILO];
char dd_client_send_data[DATA_SIZE_KILO], dd_client_recv_data[DATA_SIZE_KILO];

unsigned int server_port = 0;
unsigned int remote_port = 0; // port with which to connect to server
char ip2Join[IP_SIZE]; //used by client to join the server

int serverThreadId;
int serverSock;

int isCreated = false;
int isJoined = false;
int retry_count = 5;

int isNodeJoined = false;
int isMeCreator = false;

char creatorIP[IP_SIZE];

int fixFingerCount = 0;

//****************Function Declarations*******************
//-------Helper Functions----------
void runClientAndWaitForResult(int clientThreadID);

void helperHelp();
void helperPort(char* portCmd);
void helperClear();
void helperKeys();
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

void connectToRemoteNode(char* ip, unsigned int port);

void processQuit(char *data);
void processSucc();
void processPred();

void processFinger(char *data);
void processChangeSucc(char *addr);
void processChangePred(char *addr);
void processKeySucc(char *keyToSearch);
void processDump();

void changeSuccAndFixFirstFinger(nodeHelper* succ);

void shutMe();

//-----TCP Functions-------
void userInput();
void server();
void client();

//-----CHORD Functions-------
void askSuccToFixFinger();
void fixFingers();
nodeHelper* get_SuccFromRemoteNode(nodeHelper* remoteNode);
nodeHelper* get_PredFromRemoteNode(nodeHelper* remoteNode);
void changeSuccOfRemoteNodeToMyself(nodeHelper* remoteNode);
void changePredOfRemoteNodeToMyself(nodeHelper* remoteNode);
nodeHelper* find_successor(char key[]);
nodeHelper* closest_preceding_finger(char key[]);

void distributeKeys(nodeHelper *myPred);

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
	cout << "self";
	tab(4);
	cout << "==> prints my ip with port";

	helperHelpNewCmd();
	cout << "clear";
	tab(4);
	cout << "==> clears my screen";

	helperHelpNewCmd();
	cout << "mykeys";
	tab(4);
	cout << "==> prints my key values";

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
	cout << "==> displays information pertaining to node at address";
	cout << " (eg - dumpaddr 111.111.111.111:1000)";

	helperHelpNewCmd();
	cout << "dumpall";
	tab(4);
	cout << "==> displays information of all the nodes";

	cout << endl;
}

void helperSelf() {
	cout << "I am " << selfNode->self->ipWithPort << endl;
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

void helperClear() {
	system("clear");
}

void helperMyKeys() {
	if (!checkIfPartOfNw(selfNode)) {
		return;
	}
	printDataValMap(selfNode);
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
	strcpy(client_send_data, MSG_JOIN);
	int clientThreadID = create(client);
	runClientAndWaitForResult(clientThreadID);

	if (client_recv_data[0] == SERVER_BUSY) {
		isJoined = false;
		isCreated = false;
		deleteThread(serverThreadId);
		selfNode->self = NULL;
		close(serverSock);
		return;
	}
	strcpy(creatorIP, client_recv_data);
	cout << "Joined chord network with creatorIp: " << creatorIP << endl;
	retry_count = 9999; //Modifying the retry count for all the future connections

	cout << "Asking the known remote node for my actual successor" << endl;
	changeSuccAndFixFirstFinger(
			getKeySuccFromRemoteNode(remoteNodeHelper, selfNode->self->nodeKey));

	cout << "My actual successor is now: " << selfNode->successor->ipWithPort
			<< endl;

	selfNode->predecessor = get_PredFromRemoteNode(selfNode->successor);
	cout << "My actual predecessor is now: "
			<< selfNode->predecessor->ipWithPort << endl;

	cout << "Changing succ.pred & pred.succ to me, please wait---" << endl;

	changeSuccOfRemoteNodeToMyself(selfNode->predecessor);
	changePredOfRemoteNodeToMyself(selfNode->successor);
}

void putInMyMap(char* dataVal) {

	char dataValArr[2][DATA_SIZE_KILO];
	split(dataVal, ' ', dataValArr);

	char* hexHashKey = (char *) malloc(sizeof(char) * 41);
	data2hexHash(dataValArr[0], hexHashKey);

	char* dataForMap = (char *) malloc(sizeof(char) * 1024);

	strcpy(dataForMap, dataValArr[0]);
	strcat(dataForMap, " ");//changed from =>to ' '
	strcat(dataForMap, dataValArr[1]);
	strcat(dataForMap, "\0");

	insertInKeyMap(&selfNode->dataValMap, hexHashKey, dataForMap);

}

char* getFromMyMap(char* data) {

	char hexHashKey[HASH_HEX_BITS];
	data2hexHash(data, hexHashKey);

	map<const char*, const char*>::iterator it;

	if (!isPresentInKeyMap((selfNode->dataValMap), hexHashKey)) {
		cout << "Data not found for key: " << hexHashKey << endl;
		char* toReturn = (char *) malloc(sizeof(char) * 50);
		strcpy(toReturn, "DATA NOT FOUND!!!");
		//return "DATA NOT FOUND!!!";
		return toReturn;
	}
	return getFromKeyMap(selfNode->dataValMap, hexHashKey);
}

void helperPut(char* putCmd) {
	if (!checkIfPartOfNw(selfNode)) {
		return;
	}

	char *dataVal = (char *) malloc(sizeof(char) * 1024);
	char *hexHashKey = (char *) malloc(sizeof(char) * 41);
	strcpy(dataVal, substring(putCmd, 5, strlen(putCmd) - 5));

	char dataValArr[2][DATA_SIZE_KILO];

	split(dataVal, ' ', dataValArr);

	//char hexHashKey[HASH_HEX_BITS];
	data2hexHash(dataValArr[0], hexHashKey);

	nodeHelper* remoteNode = find_successor(hexHashKey);
	if (strcmp(remoteNode->nodeKey, selfNode->self->nodeKey) == 0) {
		putInMyMap(dataVal);
	}

	else {
		strcpy(client_send_data, MSG_PUT);
		strcat(client_send_data, dataVal);

		connectToRemoteNode(remoteNode->ip, remoteNode->port);
	}

	cout << "Data inserted: " << dataVal << " with: " << hexHashKey << ", at: "
			<< remoteNode->ipWithPort << endl;

}

void helperGet(char* getCmd) {
	if (!checkIfPartOfNw(selfNode)) {
		return;
	}
	char dataVal[1024];

	char data[DATA_SIZE_KILO];
	strcpy(data, substring(getCmd, 5, strlen(getCmd) - 5));

	char dataArr[1][DATA_SIZE_KILO];

	split(data, ' ', dataArr);

	char hexHashKey[HASH_HEX_BITS];
	data2hexHash(dataArr[0], hexHashKey);

	nodeHelper* remoteNode = find_successor(hexHashKey);

	if (strcmp(remoteNode->nodeKey, selfNode->self->nodeKey) == 0) {
		strcpy(dataVal, getFromMyMap(data));
	}

	else {
		strcpy(client_send_data, MSG_GET);
		strcat(client_send_data, dataArr[0]);

		connectToRemoteNode(remoteNode->ip, remoteNode->port);
		strcpy(dataVal, client_recv_data);
	}

	if (strcmp(dataVal, "DATA NOT FOUND!!!") == 0) {
		cout << dataVal << endl;
		return;
	}

	cout << "Data Found: " << hexHashKey << "\t" << dataVal << endl;
}

void helperFinger() {
	if (!checkIfPartOfNw(selfNode)) {
		return;
	}

	strcpy(client_send_data, MSG_FINGER);

	int i = 0;
	cout << i++ << " -> " << selfNode->self->ipWithPort << endl;

	nodeHelper* remoteNode = selfNode->successor;
	while (strcmp(remoteNode->ipWithPort, selfNode->self->ipWithPort) != 0) {
		cout << i++ << " -> " << remoteNode->ipWithPort << endl;
		connectToRemoteNode(remoteNode->ip, remoteNode->port);

		remoteNode = convertToNodeHelper(client_recv_data);
	}

	cout << "Done with printing all the fingers" << endl;
}

void helperQuit() {
	if (!checkIfPartOfNw(selfNode)) {
		return;
	}

	cout
			<< "Asking all the nodes to shut down, Thanks for using chord_DHT, see you again soon :)"
			<< endl;

	strcpy(client_send_data, MSG_QUIT);
	strcat(client_send_data, selfNode->self->ipWithPort);

	nodeHelper* remoteNode = selfNode->successor;
	while (strcmp(remoteNode->ipWithPort, selfNode->self->ipWithPort) != 0) {
		connectToRemoteNode(remoteNode->ip, remoteNode->port);
		remoteNode = convertToNodeHelper(client_recv_data);
	}

	cout << "Got ack from all the nodes" << endl;
	shutMe();
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
}

void getAndPrintDump(char *addr, unsigned int port) {
	strcpy(client_send_data, MSG_DUMP);

	retry_count = 5; //Modifying the retry count assuming server IP may be incorrect
	connectToRemoteNode(addr, port);
	retry_count = 9999; //Modifying the retry count for all the future connections

	if (client_recv_data[0] == SERVER_BUSY) { //Server busy or does not exist
		return;
	} else if (strcmp(client_recv_data, SERVER_DIFF_RING) == 0) { //Server not in the same chord network
		cout << "Sorry, server unreachable, may belong to diff network" << endl;
		return;
	}

	cout << "Dump Received" << endl;

	printDump(client_recv_data);
}

void helperDumpAddr(char* dumpAddrCmd) {
	if (!checkIfPartOfNw(selfNode)) {
		return;
	}

	char* addr;
	addr = fetchAddress(dumpAddrCmd, 10);

	if (addr == NULL) {
		return;
	}

	unsigned int port = fetchPortNumber(dumpAddrCmd,
			indexOf(dumpAddrCmd, ':') + 2);

	if (port == 0) {
		//Invalid portNumber
		return;
	}
	getAndPrintDump(addr, port);
}

void helperDumpAll() {
	if (!checkIfPartOfNw(selfNode)) {
		return;
	}

	strcpy(client_send_data, MSG_DUMP_ALL);

	int i = 0;
	cout << i++ << " :: " << endl;
	printNodeDetails(selfNode, false);

	nodeHelper* remoteNode = selfNode->successor;
	while (strcmp(remoteNode->ipWithPort, selfNode->self->ipWithPort) != 0) {

		connectToRemoteNode(remoteNode->ip, remoteNode->port);
		//findSucc
		int startIndex = indexOf(client_recv_data, '?') + 2;
		int lenSucc = strlen(client_recv_data) - startIndex + 1;
		char* succ = substring(client_recv_data, startIndex, lenSucc);

		cout << i++ << " :: " << endl;
		printDump(substring(client_recv_data, 0, startIndex - 2));

		remoteNode = convertToNodeHelper(succ);
	}
	cout << "Done with printing dump of all the nodes" << endl;
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
	memset(ip, 0, sizeof ip);
	getMyIp(ip);

	if (strlen(ip) == 0) {
		strcpy(ip, "127.0.0.1");
	}

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

	if (isMeCreator) {
		strcpy(creatorIP, selfNode->self->ipWithPort);
	}
}

void connectToRemoteNode(char* ip, unsigned int port) {
	memset(client_recv_data, 0, sizeof client_recv_data);
	strcpy(ip2Join, ip);
	remote_port = port;

	//Appending creatorIp in the request in the request
	strcat(client_send_data, "?");
	strcat(client_send_data, creatorIP);

	//cout << "Inside connectToRemoteNode: clientsendData - " << client_send_data
	//<< endl;
	int clientThreadID = create(client);
	runClientAndWaitForResult(clientThreadID);
}

void processJoin() {
	//cout << "Client wants to join" << endl;
	if (isMeCreator && !isNodeJoined) {
		isNodeJoined = true;
		askSuccToFixFinger();
	}
	strcpy(server_send_data, creatorIP);
}

void processSucc() {
	//cout << "Client wants my successor details" << endl;
	strcpy(server_send_data, selfNode->successor->ipWithPort);
}

void processPred() {
	//cout << "Client wants my predecessor details" << endl;
	strcpy(server_send_data, selfNode->predecessor->ipWithPort);
}

void processFinger(char *data) {
	//cout << "Client wants to find the fingers" << endl;
	strcpy(server_send_data, selfNode->successor->ipWithPort);
}

void processQuit(char *data) {
	//cout << "ChordRing shutting down, I need to shut as well" << endl;
	strcpy(server_send_data, selfNode->successor->ipWithPort);
}

void processFixFinger() {
	askSuccToFixFinger();
	strcpy(server_send_data, MSG_FINGER_ACK);
}

void processChangeSucc(char *addr) {
	//cout << "Client wants to change my succ to: " << addr << endl;
	changeSuccAndFixFirstFinger(convertToNodeHelper(addr));
	strcpy(server_send_data, MSG_ACK);
}

void processChangePred(char *addr) {
	//cout << "Client wants to change my pred to: " << addr << endl;
	selfNode->predecessor = convertToNodeHelper(addr);
	distributeKeys(selfNode->predecessor);

	strcpy(server_send_data, MSG_ACK);
}

void processPut(char *dataVal) {
	//cout << "Client wants to put: " << dataVal << endl;
	putInMyMap(dataVal);

	strcpy(server_send_data, MSG_ACK);
}

void processGet(char *data) {
	//cout << "Client wants to get val for: " << data << endl;

	char hexHashKey[HASH_HEX_BITS];
	data2hexHash(data, hexHashKey);
	char dataVal[DATA_SIZE_KILO];

	strcpy(dataVal, getFromMyMap(data));
	strcpy(server_send_data, dataVal);
}

void processKeySucc(char *keyToSearch) {
	/*cout << "Client requests for finding Node successor of this key: "
	 << keyToSearch << endl;*/
	nodeHelper* toReturn = find_successor(keyToSearch);
	strcpy(server_send_data, toReturn->ipWithPort);
}

void processDump() {
	//cout << "Client wants my dump" << endl;

	strcpy(server_send_data, selfNode->self->ipWithPort);
	strcat(server_send_data, ",");
	strcat(server_send_data, selfNode->successor->ipWithPort);
	strcat(server_send_data, ",");
	strcat(server_send_data, selfNode->predecessor->ipWithPort);

	//------Not sending my finger details---

	/*strcat(server_send_data, "|");

	 for (int i = 0; i < M; i++) {
	 strcat(server_send_data, selfNode->fingerStart[i]);
	 strcat(server_send_data, ",");
	 }

	 strcat(server_send_data, "|");

	 for (int i = 0; i < M; i++) {
	 char* nodeIpWithPort = selfNode->fingerNode[i]->ipWithPort;
	 strcat(server_send_data, nodeIpWithPort);
	 strcat(server_send_data, ",");
	 }*/

	strcat(server_send_data, "|");

	map<char*, char*>::iterator it;

	for (map<char*, char*>::iterator it = (selfNode->dataValMap).begin(); it
			!= (selfNode->dataValMap).end(); ++it) {
		strcat(server_send_data, it->first);
		strcat(server_send_data, ":");
		strcat(server_send_data, it->second);
		strcat(server_send_data, ",");
	}
}

void processDumpAll() {
	//cout << "Received DumpAll request" << endl;
	processDump();
	//Adding successor ipWithPort, to be used by client to find my successors dump
	strcat(server_send_data, "?");
	strcat(server_send_data, selfNode->successor->ipWithPort);
}

void changeSuccAndFixFirstFinger(nodeHelper* succ) {
	selfNode->successor = succ;
	selfNode->fingerNode[0] = succ;
}

void shutMe() {
	cout << "Shutting myself after 5 sec" << endl;
	sleep(5);
	close(serverSock);
	clean();
}

//-----TCP Functions-------
void userInput() {
	helperClear();

	while (1) {
		cout << "\n------------------------------" << endl;

		cout << ">>>: ";
		fgets(ui_data, sizeof(ui_data), stdin);

		cout << "<<<: " << ui_data << endl;

		char* cmdType = substring(ui_data, 0, indexOf(ui_data, ' '));

		if (strcmp(cmdType, "help") == 0) {
			helperHelp();
		}

		else if (strcmp(cmdType, "self") == 0) {
			helperSelf();
		}

		else if (strcmp(cmdType, "clear") == 0) {
			helperClear();
		}

		else if (strcmp(cmdType, "mykeys") == 0) {
			helperMyKeys();
		}

		else if (strcmp(cmdType, "port") == 0) {
			helperPort(ui_data);
		}

		else if (strcmp(cmdType, "create") == 0) {
			isMeCreator = true;
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
					<< "Sorry!!! It seems like you are new here, please type 'help' for list of commands"
					<< endl;
		}

		fflush(stdout);
	}
}

void server() {
	int sock, connected, trueint = 1;

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

		/*cout << "I got a connection from" << inet_ntoa(client_addr.sin_addr)
		 << ntohs(client_addr.sin_port) << endl;*/

		bytes_received = recv(connected, server_recv_data, DATA_SIZE_KILO, 0);
		server_recv_data[bytes_received] = '\0';

		char* type = substring(server_recv_data, 0, 2);
		char* data = substring(server_recv_data, 3, strlen(server_recv_data));

		char dataValArr[2][DATA_SIZE_KILO];
		split(data, '?', dataValArr);

		//cout << "Got request from: " << dataValArr[1] << endl;

		char* reqData = dataValArr[0];
		if (strcmp(type, MSG_QUIT) == 0) {
			processQuit(reqData);
		}

		else if (strcmp(type, MSG_FIX_FINGER) == 0) {
			processFixFinger();
		}

		else if (strcmp(type, MSG_JOIN) == 0) {
			processJoin();
		}

		else if (strcmp(type, MSG_NODE_SUCC) == 0) {
			processSucc();
		}

		else if (strcmp(type, MSG_NODE_PRED) == 0) {
			processPred();
		}

		else if (strcmp(type, MSG_FINGER) == 0) {
			processFinger(reqData);
		}

		else if (strcmp(type, MSG_KEY_SUCC) == 0) {
			processKeySucc(reqData);
		}

		else if (strcmp(type, MSG_DUMP) == 0) {
			if (strcmp(creatorIP, dataValArr[1]) == 0) { //checking if belonging to same chord network
				processDump();
			} else {
				strcpy(server_send_data, SERVER_DIFF_RING);
			}
		}

		else if (strcmp(type, MSG_DUMP_ALL) == 0) {
			processDumpAll();
		}

		else if (strcmp(type, MSG_CHNG_SUCC) == 0) {
			processChangeSucc(reqData);
		}

		else if (strcmp(type, MSG_CHNG_PRED) == 0) {
			processChangePred(reqData);
		}

		else if (strcmp(type, MSG_PUT) == 0) {
			processPut(reqData);
		}

		else if (strcmp(type, MSG_GET) == 0) {
			processGet(reqData);
		}

		send(connected, server_send_data, strlen(server_send_data), 0);
		//cout << "Done the required task, closing the connection" << endl;
		//cout << "------------------------------\n>>>:";
		fflush(stdout);//may be fatal, adding for UI
		close(connected);

		if (strcmp(type, MSG_QUIT) == 0) {
			shutMe();
		}
	}
	//right now, doesn't reach here
	close(sock);
}

bool connectToServer(int & sock) {
	struct hostent *host;
	struct sockaddr_in server_addr;
	/*cout << "Inside connect to server: " << ip2Join << ":" << remote_port
	 << endl;*/
	host = gethostbyname(ip2Join);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("Socket");
		exit(1);
	}
	//cout << "Client socket created" << endl;
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr = *((struct in_addr *) host->h_addr);
	server_addr.sin_port = htons(remote_port);
	bzero(&(server_addr.sin_zero), 8);

	int retriedCount = 0;
	while (connect(sock, (struct sockaddr *) &server_addr,
			sizeof(struct sockaddr)) == -1) {

		//trying again assuming the server is busy
		retriedCount++;
		cout << "Server busy --- retrying(" << retriedCount << "/"
				<< retry_count << ")" << endl;
		sleep(1);
		if (retriedCount == retry_count) {
			cout
					<< "Server is not up or not responding, terminating client...please try again"
					<< endl;
			close(sock);
			return false;
		}
	}
	//cout << "Client successfully connected to server" << endl;
	return true;
}

void client() {
	//cout << "\n------------------------------" << endl;
	//cout << "Client started" << endl;

	int sock, bytes_recieved;

	if (!connectToServer(sock)) {
		client_recv_data[0] = SERVER_BUSY; //Inserting this --- to be used in helperJoin
		return;
	}

	//cout << "Client socket ID:" << sock << endl;

	send(sock, client_send_data, strlen(client_send_data), 0);

	bytes_recieved = recv(sock, client_recv_data, DATA_SIZE_KILO, 0);
	//cout << "Data successfully received" << endl;
	client_recv_data[bytes_recieved] = '\0';

	close(sock);
}

void fingersClient() {
	sleep(5);
	int sock, bytes_recieved;

	if (!connectToServer(sock)) {
		ff_client_recv_data[0] = SERVER_BUSY; //Inserting this --- to be used in helperJoin
		return;
	}

	send(sock, ff_client_send_data, strlen(ff_client_send_data), 0);

	bytes_recieved = recv(sock, ff_client_recv_data, DATA_SIZE_KILO, 0);
	ff_client_recv_data[bytes_recieved] = '\0';
	close(sock);
}

void distributeClient() {
	sleep(5);
	int sock, bytes_recieved;

	if (!connectToServer(sock)) {
		dd_client_recv_data[0] = SERVER_BUSY; //Inserting this --- to be used in helperJoin
		return;
	}

	send(sock, dd_client_send_data, strlen(dd_client_send_data), 0);

	bytes_recieved = recv(sock, dd_client_recv_data, DATA_SIZE_KILO, 0);
	dd_client_recv_data[bytes_recieved] = '\0';
	close(sock);
}

//-----------CHORD FUNCTIONS-------
void askSuccToFixFinger() {
	//sleep(2);
	fixFingers(); //fixing my finger table

	strcpy(ff_client_send_data, MSG_FIX_FINGER);

	strcpy(ip2Join, selfNode->successor->ip);
	remote_port = selfNode->successor->port;

	int clientThreadId = create(fingersClient);
	run(clientThreadId); //Non blocking call for remoteSucc fixFnger
}

void fixFingers() {
	if (fixFingerCount % 10 != 0) {
		fixFingerCount++;
		return;
	}

	fixFingerCount++;
	//cout << "stabilizing--- ";
	for (int fixFingerIndex = 1; fixFingerIndex < M; fixFingerIndex++) {
		char* key = selfNode->fingerStart[fixFingerIndex];
		char* me = selfNode->self->nodeKey;
		char* succKey = selfNode->successor->nodeKey;
		char* predKey = selfNode->predecessor->nodeKey;

		if (strcmp(key, succKey) == 0 || keyBelongCheck(me, succKey, key)) {
			selfNode->fingerNode[fixFingerIndex] = selfNode->successor;
		}

		else if (strcmp(key, me) == 0 || keyBelongCheck(predKey, me, key)) {
			selfNode->fingerNode[fixFingerIndex] = selfNode->self;
		}

		else {
			selfNode->fingerNode[fixFingerIndex] = find_successor(key);
		}
	}
	//cout << "stabilized" << endl;
}

//I am going to distributeKeys to my predecessor
void distributeKeys(nodeHelper* myPred) {
	map<char*, char*>::iterator it;
	for (map<char*, char*>::iterator it = (selfNode->dataValMap).begin(); it
			!= (selfNode->dataValMap).end(); ++it) {
		//cout << it->first << " : " << it->second << '\n';
		if (keyBelongCheck(selfNode->predecessor->nodeKey,
				selfNode->self->nodeKey, it->first) or strcmp(it->first,
				selfNode->self->nodeKey) == 0) {

		} else {
			char *dataVal = (char *) malloc(sizeof(char) * 1024);
			strcpy(dataVal, it->second);

			strcpy(dd_client_send_data, MSG_PUT);
			strcat(dd_client_send_data, dataVal);

			strcpy(ip2Join, myPred->ip);
			remote_port = myPred->port;

			int clientThreadId = create(distributeClient);
			run(clientThreadId); //Non blocking call for distributekeys

			cout << "key-val pair " << dataVal << " transferring to pred"
					<< endl;
			selfNode->dataValMap.erase(it->first);//last line
			sleep(5);
		}
	}
}

nodeHelper* get_SuccFromRemoteNode(nodeHelper* remoteNode) {
	strcpy(client_send_data, MSG_NODE_SUCC);

	connectToRemoteNode(remoteNode->ip, remoteNode->port);
	//cout << "Got the successor from remote node: " << client_recv_data << endl;
	return convertToNodeHelper(client_recv_data);
}

nodeHelper* get_PredFromRemoteNode(nodeHelper* remoteNode) {
	strcpy(client_send_data, MSG_NODE_PRED);

	connectToRemoteNode(remoteNode->ip, remoteNode->port);
	//cout << "Got the predecessor from remote node: " << client_recv_data
	//<< endl;
	return convertToNodeHelper(client_recv_data);
}

void changeSuccOfRemoteNodeToMyself(nodeHelper* remoteNode) {
	strcpy(client_send_data, MSG_CHNG_SUCC);
	strcat(client_send_data, selfNode->self->ipWithPort);

	connectToRemoteNode(remoteNode->ip, remoteNode->port);
	cout << "Changed the successor of remote node to myself" << endl;
}

void changePredOfRemoteNodeToMyself(nodeHelper* remoteNode) {
	strcpy(client_send_data, MSG_CHNG_PRED);
	strcat(client_send_data, selfNode->self->ipWithPort);

	connectToRemoteNode(remoteNode->ip, remoteNode->port);
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
	strcpy(client_send_data, MSG_KEY_SUCC); //we will be acting as client to send data
	strcat(client_send_data, key);

	if (strcmp(remoteNode->ipWithPort, selfNode->self->ipWithPort) == 0) { //no need to connect to remote node, if it is me
		return selfNode->self;
	}
	connectToRemoteNode(remoteNode->ip, remoteNode->port);

	//cout << "Data received from remote node " << client_recv_data << endl;
	return convertToNodeHelper(client_recv_data);
}

//-----------MAIN---------------

int main() {
	create(userInput);
	start();
	return 0;
}


//----------Constants---------
#define HASH_BYTES 20
#define HASH_HEX_BITS 41

//----------Globals---------

struct cmp_str {
	bool operator()(char const *a, char const *b) {
		return strcmp(a, b) < 0;
	}
};

typedef map<char*, char*, cmp_str> keyMap;

//****************Function Declarations*******************
void insertInKeyMap(keyMap* myMap, char *hexHashKey, char *data);
bool isPresentInKeyMap(keyMap myMap, char *key);
char* getFromKeyMap(keyMap myMap, char *key);
void printHashKey(unsigned char* key, int len);
unsigned int data2hexHash(const char* dataToHash, char* hexHash);
void getHashInHex(unsigned char* key, char* tempValue, int len);
int convert(char item);
void hexAddition(char* hexOne, char* hexTwo, char* hexSum, int len);
unsigned int hash(const char *mode, const char* dataToHash,
		unsigned char* outHash);

//****************Function Definitions*******************

void insertInKeyMap(keyMap* myMap, char *hexHashKey, char *data) {
	(*myMap)[hexHashKey] = data;
	//(*myMap).insert(keyMap::value_type(hexHashKey, data));
}

bool isPresentInKeyMap(keyMap myMap, char *key) {
	keyMap::iterator iter = myMap.find(key);
	if (iter != myMap.end()) {
		return true;
	}
	return false;
}

//use this function to getFromMap only if 'isPresentInMap == true'
char* getFromKeyMap(keyMap myMap, char *key) {
	keyMap::iterator iter = myMap.find(key);
	return (*iter).second;
}

//prints the hash key in readable format, (requires len)
void printHashKey(unsigned char* key, int len) {
	for (int i = 0; i < len; i++) {
		printf("%02x", key[i]);
	}
}

unsigned int data2hexHash(const char* dataToHash, char* hexHash) {
	unsigned char outHash[HASH_BYTES];

	int len = hash("SHA1", dataToHash, outHash);
	if (len == -1) {
		return len;
	}
	getHashInHex(outHash, hexHash, HASH_BYTES);
	return strlen(hexHash);
}

//convert from unsigned char* 20 bytes to char* 40 hex digits
void getHashInHex(unsigned char* key, char* tempValue, int len) {
	for (int i = 0; i < len; ++i)
		sprintf(tempValue + 2 * i, "%02x", (unsigned char) key[i]);
}

int convert(char item) {
	switch (item) {
	case 'a':
		return 10;
		break;
	case 'b':
		return 11;
		break;
	case 'c':
		return 12;
		break;
	case 'd':
		return 13;
		break;
	case 'e':
		return 14;
		break;
	case 'f':
		return 15;
		break;
	}
	return (int) (item - 48);
}

//adds two hashes in hex and stores result in third
void hexAddition(char* hexOne, char* hexTwo, char* hexSum, int len) {
	char hexArr[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a',
			'b', 'c', 'd', 'e', 'f' };
	int carry = 0;
	int temp, i;
	for (i = len - 1; i >= 0; i--) {
		// convert to decimal and add both array values
		temp = convert(hexOne[i]) + convert(hexTwo[i]) + carry;
		// add values and if they are greater than F add 1 to next value
		carry = temp / 16;
		temp %= 16;
		hexSum[i] = hexArr[temp];
	}
	hexSum[len] = '\0';
}

//hash of the given data in given mode, hash is stored in outHash
unsigned int hash(const char *mode, const char* dataToHash,
		unsigned char* outHash) {
	unsigned int md_len = -1;
	size_t dataSize = strlen(dataToHash);
	OpenSSL_add_all_digests();
	const EVP_MD *md = EVP_get_digestbyname(mode);
	if (NULL != md) {
		EVP_MD_CTX mdctx;
		EVP_MD_CTX_init(&mdctx);
		EVP_DigestInit_ex(&mdctx, md, NULL);
		EVP_DigestUpdate(&mdctx, dataToHash, dataSize);
		EVP_DigestFinal_ex(&mdctx, outHash, &md_len);
		EVP_MD_CTX_cleanup(&mdctx);
	}
	return md_len;
}

bool keyBelongCheck(char startKey[HASH_HEX_BITS], char endKey[HASH_HEX_BITS],
		char searchKey[HASH_HEX_BITS]) {
	char min[] = "0000000000000000000000000000000000000000";
	char max[] = "ffffffffffffffffffffffffffffffffffffffff";

	if (strcmp(startKey, endKey) < 0) {
		if (strcmp(searchKey, startKey) > 0 && strcmp(searchKey, endKey) < 0)
			return true;
		else
			return false;
	} else //that means either startKey==endKey or  startKey > endKey
	{
		bool ans1 = false;
		bool ans2 = false;
		//case 1
		if (strcmp(startKey, max) < 0) {
			if (strcmp(searchKey, startKey) > 0 && strcmp(searchKey, max) <= 0)
				ans1 = true;
		}
		//case 2
		if (strcmp(min, endKey) < 0) {
			if (strcmp(searchKey, min) >= 0 && strcmp(searchKey, endKey) < 0)
				ans2 = true;
		}
		return ans1 || ans2;
	}
}
//----------Constants---------
#define M 160	//number of bits
#define SECOND 1000000
#define QUEUE_LIMIT 5

#define DATA_SIZE_KILO 1024

#define NODE_DELIM_CHAR ';'
#define NODE_DELIM_STR ";"

//----------Globals---------
char GLOBAL_ARR[M][DATA_SIZE_KILO];
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
void printAllFingerTable(Node* node);
void printDataValMap(Node* node);
void printNodeDetails(Node* node, int isPrintFinger);
void printDump(char* dumpData);

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

void printAllFingerTable(Node* node) {
	for (int i = 0; i < M; i++) {
		char* nodeFinger = node-> fingerNode[i]->ipWithPort;

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

void printNodeDetails(Node* node, int isPrintFinger = true) {
	cout << "Node-> " << node->self->ipWithPort << endl;
	cout << "Node Key-> " << node->self->nodeKey << endl;
	cout << "Successor-> " << node->successor->ipWithPort << endl;
	cout << "Predecessor-> " << node->predecessor->ipWithPort << endl;

	if (isPrintFinger) {
		cout << "Finger table: " << endl;
		printAllFingerTable(node);
	}
	printDataValMap(node);
}

void printDump(char* dumpData) {
	for (int i = 0; i < M; i++) {
		memset(GLOBAL_ARR[i], 0, sizeof GLOBAL_ARR[i]);
	}
	/*for (int i = 0; i < M; i++) {
	 memset(FINGER_ARR[i], 0, sizeof FINGER_ARR[i]);
	 }*/
	for (int i = 0; i < M; i++) {
		memset(KEY_VALUES[i], 0, sizeof KEY_VALUES[i]);
	}

	split(dumpData, '|', GLOBAL_ARR);

	/*	split(GLOBAL_ARR[1], ',', FINGER_ARR);

	 for (int i = 0; i < M; i++) {
	 strcpy(helperNode->fingerStart[i], FINGER_ARR[i]);
	 }

	 split(GLOBAL_ARR[2], ',', FINGER_ARR);

	 for (int i = 0; i < M; i++) {
	 char* tmp = FINGER_ARR[i];
	 helperNode->fingerNode[i] = convertToNodeHelper(tmp);
	 }*/

	char ipWithPort[3][DATA_SIZE_KILO];
	split(GLOBAL_ARR[0], ',', ipWithPort);

	split(GLOBAL_ARR[1], ',', KEY_VALUES);

	helperNode->self = convertToNodeHelper(ipWithPort[0]);
	helperNode->successor = convertToNodeHelper(ipWithPort[1]);
	helperNode->predecessor = convertToNodeHelper(ipWithPort[2]);

	printNodeDetails(helperNode, false);

	int occ = countOccurence(GLOBAL_ARR[1], ',');
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
//----------Constants---------
#define IP_SIZE 40

//****************Function Declarations*******************
void intToChar(int intToChng, char* charToRet);
void tab(int count);
bool startsWith(const char *a, const char *b);
char* substring(char *string, int position, int length);
int countOccurence(char* string, char splitter);
void split(char* string, char splitter, char splittedArr[][1024]);
void split(char* string, char splitter, char splittedArr[][16384]);
int indexOf(char* string, char of);

//****************Function Definitions*******************
void intToChar(int intToChng, char* charToRet) {
	size_t size = sizeof(charToRet);
	snprintf(charToRet, size, "%d", intToChng);
}

void tab(int count) {
	for (int i = 0; i < count; i++) {
		cout << "\t";
	}
}

bool startsWith(const char *a, const char *b) {
	if (strncmp(a, b, strlen(b)) == 0) {
		return 0;
	}
	return 1;
}

char* substring(char *string, int position, int length) {
	char *pointer;
	int c;

	pointer = (char*) malloc(length + 1);

	if (pointer == NULL) {
		printf("Inside substring : Unable to allocate memory.\n");
		exit( EXIT_FAILURE);
	}

	for (c = 0; c < position - 1; c++) {
		string++;
	}

	for (c = 0; c < length; c++) {
		*(pointer + c) = *string;
		string++;
	}

	*(pointer + c) = '\0';
	return pointer;
}

int countOccurence(char* string, char splitter) {
	int len = strlen(string);
	int count = 0;
	for (int i = 0; i < len; i++) {
		if (string[i] == splitter) {
			count++;
		}
	}
	return count;
}

void split(char* string, char splitter, char splittedArr[][1024]) {
	int len = strlen(string);
	char tmp[len];
	memset(tmp, 0, strlen(tmp));
	int j = 0; //pointer for tmp string
	int k = 0; //pointer for strings in splittedArr
	for (int i = 0; i < len; i++) {
		if (string[i] != splitter) {
			tmp[j] = string[i];
			j++;
		} else {
			tmp[j] = '\0';
			strcpy(splittedArr[k], tmp);
			j = 0;
			memset(tmp, 0, strlen(tmp));
			k++;
		}
	}
	if (string[len - 1] != splitter) { //Adding this check to get the last array
		tmp[j] = '\0';
		strcpy(splittedArr[k], tmp);
	}
}

void split(char* string, char splitter, char splittedArr[][16384]) {
	int len = strlen(string);
	char tmp[len];
	memset(tmp, 0, strlen(tmp));
	int j = 0; //pointer for tmp string
	int k = 0; //pointer for strings in splittedArr
	for (int i = 0; i < len; i++) {
		if (string[i] != splitter) {
			tmp[j] = string[i];
			j++;
		} else {
			tmp[j] = '\0';
			strcpy(splittedArr[k], tmp);
			j = 0;
			memset(tmp, 0, strlen(tmp));
			k++;
		}
	}
	if (string[len - 1] != splitter) { //Adding this check to get the last array
		tmp[j] = '\0';
		strcpy(splittedArr[k], tmp);
	}
}

int indexOf(char* string, char of) {
	int len = strlen(string);
	for (int i = 0; i < len; i++) {
		if (string[i] == of) {
			return i;
		}
	}
	return len - 1;
}
	toReturn->port = portNum;

	char hexHash[HASH_HEX_BITS];
	data2hexHash(toReturn->ipWithPort, hexHash);
	strcpy(toReturn->nodeKey, hexHash);

	return toReturn;
}

#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <setjmp.h>
#include <signal.h>
#include <list>
#include <iostream>
#include <malloc.h>
#include <unistd.h>
#include <stdlib.h>
using namespace std;

#ifdef __x86_64__
// code for 64 bit Intel arch

typedef unsigned long address_t;
#define JB_SP 6
#define JB_PC 7
address_t translate_address(address_t addr) {
	address_t ret;
	asm volatile("xor    %%fs:0x30,%0\n"
			"rol    $0x11,%0\n"
			: "=g" (ret)
			: "0" (addr));
	return ret;
}

#else
// code for 32 bit Intel arch

typedef unsigned int address_t;
#define JB_SP 4
#define JB_PC 5
address_t translate_address(address_t addr)
{
	address_t ret;
	asm volatile("xor    %%gs:0x18,%0\n"
			"rol    $0x9,%0\n"
			: "=g" (ret)
			: "0" (addr));
	return ret;
}

#endif

//----------Constants---------
#define TQ 1 //Time Quantum for round-robin scheduling
#define STACK_SIZE 40000
#define N 50 //Number of max threads allowed
//----------Globals---------
enum State {
	NEW, READY, RUNNING, SLEEPING, SUSPENDED, DELETED, TERMINATED, WAITING
};

struct Statistics {
	int threadID;
	State state;
	int numberOfBursts;
	unsigned long totalExecutionTime;
	unsigned long totalRequestedSleepingTime;
	unsigned int averageExecutionTimeQuantum;
	unsigned long averageWaitingTime;
};

struct Timers {
	struct timeval exec_start;
	struct timeval exec_end;
	struct timeval ready_start;
	struct timeval ready_end;
	unsigned int waitingCount;
	unsigned long totalWaitingTime;
	unsigned long sleepEndTime;
};

struct Thread_node {
	Statistics* stats;
	Timers* timers;
	void (*fn)(void);
	void *(*fn_arg)(void*); //used in case of createWithArgs
	void *arg; //used in case of createWithArgs
	void *fn_arg_result; //used in case of createWithArgs
	char* stack;
	list<Thread_node*> threadsWaitingForMe;
};

sigjmp_buf jbuf[N];
list<Thread_node*> newQueue, readyQueue, suspendQueue, deleteQueue, sleepQueue,
		masterList, terminateQueue, waitingQueue;
int lastCreatedThreadID = -1; //global variable to maintain the threadIds
//char master_stack[N][STACK_SIZE];
Thread_node* runningThread;

//***************************FUNCTION DECLARATIONS*********************************
//----------Helper Functions---------
void enque(list<Thread_node*> *l, Thread_node* node);
Thread_node* deque(list<Thread_node*> *l);
void initializeThread(Thread_node* t_node);
void switchThreads();
void checkIfSleepDone(Thread_node* t_node);
unsigned long int translate_address(unsigned long int addr);
void setUp(char *stack, void(*f)(void));
Thread_node* searchInQueue(int threadId, list<Thread_node*> *l);
void printQueue(list<Thread_node*> *l);
bool isValidThreadID(int threadId);
void protector(void);
int createHelper(void(*fn)(void), void *(*fn_arg)(void *), void *arg);
void changeState(Thread_node* node, State state);
uint64_t convertToMillis(timeval time);
uint64_t getTimeDiff(timeval start, timeval end);
Timers* getTimers(int threadID);
void resumeWaitingThreads(Thread_node *t_node);
void moveThread(Thread_node *t_node, State fromState, State toState);
void emptyQueue(list<Thread_node*> *queue);
void printStats(Thread_node* t_node);
void printTime(unsigned long sec);
uint64_t getCurrentTimeMillis();

//----------Thread Functions---------
int create(void(*f)(void));
int getID();
void dispatch(int sig);
void start();
void run(int threadID);
void suspend(int threadID);
void resume(int threadID);
void yield();
void deleteThread(int threadID);
void sleep(int sec);
Statistics* getStatus(int threadID);
int createWithArgs(void *(*f)(void *), void *arg);
void clean();
void JOIN(int threadID);
void *GetThreadResult(int threadID);

//*******************************FUNCTION DEFINITIONS************************
//Helper Functions
void enque(list<Thread_node*> *l, Thread_node* node) {
	(*l).push_back(node);
}

Thread_node* deque(list<Thread_node*> *l) {
	Thread_node* node = (*l).front();
	(*l).pop_front();
	return node;
}

void initializeThread(Thread_node* t_node) {
	lastCreatedThreadID++;
	Statistics* stats = new Statistics;
	t_node -> stats = stats;
	stats -> state = NEW;
	stats -> threadID = lastCreatedThreadID;
	stats -> numberOfBursts = 0;
	stats -> totalExecutionTime = 0;
	stats -> totalRequestedSleepingTime = 0;
	stats -> averageExecutionTimeQuantum = 0;
	stats -> averageWaitingTime = 0;
	Timers* timers = new Timers;
	t_node -> timers = timers;
	timers -> totalWaitingTime = 0;
	timers -> waitingCount = 0;
	enque(&newQueue, t_node); //Adding to the new queue
	enque(&masterList, t_node); //Adding to the master list
}

void switchThreads() {
	if (runningThread != NULL) {
		if (runningThread->stats->state == RUNNING) {
			//shall come here only if TQ expires
			changeState(runningThread, READY);
			enque(&readyQueue, runningThread);
		}

		int ret_val = sigsetjmp(jbuf[runningThread->stats->threadID], 1);
		////cout << "SWITCH: ret_val= " << ret_val << endl;
		if (ret_val == 1) {
			////cout << "Returning from switch : may go inside the function"
			//	<< endl;
			return;
		}
	}

	//Moving readyHead to running state
	if (!readyQueue.empty()) {
		runningThread = deque(&readyQueue);
		//checkIfSleepDone(runningThread);
		changeState(runningThread, RUNNING);
		int runningThreadId = runningThread->stats->threadID;
		//cout << "/----------------" << endl;
		//cout << "switching now to " << runningThreadId << endl;
		//cout << "Ready Queue: ";
		//printQueue(&readyQueue);
		//cout << "----------------/" << endl;
		siglongjmp(jbuf[runningThreadId], 1);
	} else {
		//cout << "no thread to run - readyQueue empty" << endl;
	}
}

void checkIfSleepDone(Thread_node* t_node) {
	if (t_node->stats->state == SLEEPING) {
		uint64_t currentTimeMillis = getCurrentTimeMillis();
		if (t_node->timers->sleepEndTime > currentTimeMillis) {
			moveThread(runningThread, RUNNING, SLEEPING);
		}
	}
}

void setUp(char *stack, void(*f)(void)) {
	unsigned int sp, pc;
	sp = (address_t) stack + STACK_SIZE - sizeof(address_t);
	pc = (address_t) (&protector);
	sigsetjmp(jbuf[lastCreatedThreadID], 1);
	//cout << "/----------------" << endl;
	//cout << "Inside setup" << lastCreatedThreadID << endl;
	//cout << "----------------/" << endl;
	(jbuf[lastCreatedThreadID]->__jmpbuf)[JB_SP] = translate_address(sp);
	(jbuf[lastCreatedThreadID]->__jmpbuf)[JB_PC] = translate_address(pc);
	sigemptyset(&jbuf[lastCreatedThreadID]->__saved_mask); //empty saved signal mask
}

Thread_node* searchInQueue(int threadId, list<Thread_node*> *l) {
	for (list<Thread_node*>::iterator it = (*l).begin(); it != (*l).end(); it++) {
		if ((*it)->stats->threadID == threadId) {
			return *it;
		}
	}
	return NULL;
}

void printQueue(list<Thread_node*> *l) {
	for (list<Thread_node*>::iterator it = (*l).begin(); it != (*l).end(); it++) {
		cout << (*it)->stats->threadID << ", ";
	}
	cout << endl;
}

bool isValidThreadID(int threadId) {
	if (threadId > lastCreatedThreadID) {
		//cout << "Inside ifValidThreadID : Invalid threadId" << endl;
		return false;
	}
	return true;
}

void protector(void) {
	void (*fn)(void);
	void *(*fn_arg)(void*);
	void *arg;

	////cout << "Going inside the function from protector" << endl;

	if (runningThread->fn != NULL) {
		//In case of "create(void(*f)(void))"
		fn = runningThread->fn;
		(fn)();
	} else {
		fn_arg = runningThread->fn_arg;
		arg = runningThread->arg;
		runningThread -> fn_arg_result = (fn_arg)(arg);
	}

	moveThread(runningThread, RUNNING, TERMINATED);
}

int createHelper(void(*fn)(void), void *(*fn_arg)(void *) = NULL,
		void *arg = NULL) {

	Thread_node* t_node = new Thread_node;
	if (t_node == NULL) {
		//cout << "Sorry, out of memory, no more thread can be created" << endl;
		return -1;
	}
	if (lastCreatedThreadID == N - 1) {
		//cout << "Reached max allowed limit of threads" << endl;
		return -1;
	}
	char* stack = (char*) malloc(STACK_SIZE);
	if (stack == NULL) {
		//cout << "Sorry, out of memory, no more thread can be created" << endl;
		return -1;
	}
	initializeThread(t_node);
	t_node -> fn = fn;
	t_node -> fn_arg = fn_arg; //In case of createWithArgs
	t_node -> arg = arg; //In case of createWithArgs
	t_node -> stack = stack;
	setUp(stack, fn);
	return lastCreatedThreadID;
}

void changeState(Thread_node* node, State state) {
	struct timeval time;
	//calculations for waiting time
	if (node -> stats -> state == READY) {
		//Previous state
		gettimeofday(&time, NULL);
		node->timers->ready_end = time;
		node->timers->totalWaitingTime += getTimeDiff(
				node->timers->ready_start, node->timers->ready_end);
		node->timers->waitingCount++;
		node->stats->averageWaitingTime = node->timers->totalWaitingTime
				/ node->timers->waitingCount;
	} else if (state == READY) {
		//New State
		gettimeofday(&time, NULL);
		node->timers->ready_start = time;
	}

	//calculations for execution time
	if (node->stats->state == RUNNING) {
		//Previous State
		gettimeofday(&time, NULL);
		node->timers->exec_end = time;
		node->stats->totalExecutionTime += getTimeDiff(
				node->timers->exec_start, node->timers->exec_end);
		node->stats->numberOfBursts++;
		node->stats->averageExecutionTimeQuantum
				= node->stats->totalExecutionTime / node->stats->numberOfBursts;
	} else if (state == RUNNING) {
		//New State
		gettimeofday(&time, NULL);
		node->timers->exec_start = time;
	}
	node->stats->state = state;
}

uint64_t getTimeDiff(timeval start, timeval end) {
	uint64_t endmillis = convertToMillis(end);
	uint64_t startmillis = convertToMillis(start);
	return endmillis - startmillis;
}

uint64_t convertToMillis(timeval time) {
	return (time.tv_sec * (uint64_t) 1000) + (time.tv_usec / 1000);
}

Timers* getTimers(int threadID) {
	if (!isValidThreadID(threadID)) {
		return NULL;
	}

	Thread_node* t_node = searchInQueue(threadID, &masterList);
	if (t_node == NULL) {
		//cout << "Inside getTimers : thread not found" << endl;
		return NULL;
	}

	return t_node->timers;
}

void resumeWaitingThreads(Thread_node *t_node) {
	while (!(t_node->threadsWaitingForMe).empty()) {
		Thread_node* waitingThread = deque(&(t_node->threadsWaitingForMe));
		if (waitingThread->stats->state == WAITING) {
			//Using "if" check considering the scenario if waiting thread is deleted already
			moveThread(waitingThread, WAITING, READY);
		}
	}
}

void moveThread(Thread_node *t_node, State fromState, State toState) {
	//int threadId = t_node->stats->threadID;
	switch (toState) {
	case RUNNING:
		//cout << "Inside moveThread: toState as RUNNING not supported";
		return;
	case NEW:
		//cout << "Inside moveThread: toState as NEW not supported";
		return;
	case READY:
		enque(&readyQueue, t_node);
		changeState(t_node, READY);
		break;
	case SLEEPING: //sleeping moves it again to readyqueue
		enque(&readyQueue, t_node);
		changeState(t_node, SLEEPING);
		break;
	case SUSPENDED:
		//cout << "/----------------" << endl;
		//cout << "Suspended thread: " << threadId << endl;
		//cout << "----------------/" << endl;
		changeState(t_node, SUSPENDED);
		enque(&suspendQueue, t_node);
		break;
	case WAITING:
		changeState(t_node, WAITING);
		enque(&waitingQueue, t_node);
		break;
	case DELETED:
		//cout << "/----------------" << endl;
		//cout << "Deleted thread: " << threadId << endl;
		//cout << "----------------/" << endl;
		changeState(t_node, DELETED);
		enque(&deleteQueue, t_node);
		resumeWaitingThreads(t_node);
		break;
	case TERMINATED:
		//cout << "/----------------" << endl;
		//cout << "Terminated thread: " << threadId << endl;
		//cout << "----------------/" << endl;
		changeState(t_node, TERMINATED);
		enque(&terminateQueue, t_node);
		resumeWaitingThreads(t_node);
		break;
	}

	switch (fromState) {
	case DELETED:
		//cout << "Inside moveThread: fromState as DELETED not supported";
		break;
	case TERMINATED:
		terminateQueue.remove(t_node);
		break;
	case RUNNING:
		yield(); //Need to yield the thread if fromStatus is RUNNING
		break;
	case READY:
		readyQueue.remove(t_node);
		break;
	case SLEEPING: //sleeping removes from ready queue
		readyQueue.remove(t_node);
		break;
	case SUSPENDED:
		suspendQueue.remove(t_node);
		break;
	case NEW:
		newQueue.remove(t_node);
		break;
	case WAITING:
		waitingQueue.remove(t_node);
		break;
	}
}

void emptyQueue(list<Thread_node*> *queue) {
	while (!(*queue).empty()) {
		deque(queue);
	}
}

void printStats(Thread_node* t_node) {
	Statistics* stats = getStatus(t_node->stats->threadID);
	//cout << "---------------" << endl << endl;
	//cout << "ThreadId: " << stats->threadID << endl;
	//cout << "Current State: " << stats->state << endl;
	//cout << "Number of bursts: " << stats->numberOfBursts << endl;
	//cout << "Total Execution Time: ";
	printTime(stats->totalExecutionTime);
	//cout << "Average Execution Time: ";
	printTime(stats->averageExecutionTimeQuantum);
	//cout << "Average Waiting Time: ";
	printTime(stats->averageWaitingTime);
	//cout << "Total Requested Sleeping Time: ";
	printTime(stats->totalRequestedSleepingTime);
}

void printTime(unsigned long sec) {
	if (sec == 0) {
		//cout << "N/A" << endl;
	} else {
		//cout << sec << " msec" << endl;
	}
}

uint64_t getCurrentTimeMillis() {
	struct timeval currentTime;
	gettimeofday(&currentTime, NULL);
	return convertToMillis(currentTime);
}

//----------Thread Functions---------

/**
 * @brief Creates a no-return-type and no-arg thread.
 * Creates a thread, whose execution starts from
 * function f which has no parameter and no return type; however create returns
 * thread ID of created thread.
 */
int create(void(*f)(void)) {
	return createHelper(f);
}

/**
 * @brief Creates a return-type and arg thread.
 * f is a ptr to a function which
 * takes (void *) and returns (void *). Unlike the threads so far, this thread takes
 * arguments, and instead of uselessly looping forever, returns a value in a (void *).
 * This function returns the id of the thread created.
 */
int createWithArgs(void *(*f)(void *), void *arg) {
	return createHelper(NULL, f, arg);
}

/**
 * @brief A scheduler for our thread library.
 * dispatch() is called after every time quantum finishes.
 * We are simulating a round robin scheduler.
 */
void dispatch(int sig) {
	signal(SIGALRM, dispatch);
	alarm(TQ);
	switchThreads();
}

/**
 * @brief Starts the thread environment.
 * Call in your main method. Moves all the created threads to ready queue & runs the first one.
 * One called,starts running of thread and never return.
 * So at least one of your thread should have infinite loop.
 */
void start() {
	if (newQueue.empty()) {
		//cout << "Inside start: newQueue empty, please create some threads"<< endl;
		return;
	}

	//Moving all new threads to readyQueue
	while (!newQueue.empty()) {
		Thread_node* t_node = newQueue.front();
		moveThread(t_node, NEW, READY);
	}
	dispatch(14);
}

/**
 * @brief Submits particular thread to scheduler for execution.
 * Puts a thread to ready queue from new queue.
 * Could be called from any other thread.
 */
void run(int threadID) {
	if (!isValidThreadID(threadID)) {
		return;
	}

	Thread_node* t_node = searchInQueue(threadID, &newQueue);
	if (t_node == NULL) {
		//cout << "Inside run : Thread not found in the created(new) state"	<< endl;
		return;
	}

	moveThread(t_node, NEW, READY);
}

/**
 * @brief Returns it's own thread ID to the running thread.
 * Called from inside the thread.
 * Should return thread ID of the current thread.
 */
int getID() {
	if (runningThread != NULL) {
		return runningThread->stats->threadID;
	}

	return -1;
}

/**
 * @brief Returns thread Statistics with id as threadID
 * Could be called from any thread.
 * Returns the status of threaded by returning pointer to its statistics
 * Returns NULL of threadID is invalid.
 */
Statistics* getStatus(int threadID) {
	if (!isValidThreadID(threadID)) {
		return NULL;
	}

	Thread_node* t_node = searchInQueue(threadID, &masterList);
	if (t_node == NULL) {
		//cout << "Inside getStatus : thread not found" << endl;
		return NULL;
	}

	return t_node->stats;

}

/**
 * @brief Suspends the thread.
 * Could be called from any thread.
 * Puts the thread in suspended queue.
 * Suspends the thread with ID as threadID until a resume() is not called for the same thread.
 */
void suspend(int threadID) {
	if (!isValidThreadID(threadID)) {
		return;
	}

	if (runningThread != NULL && runningThread->stats->threadID == threadID) {
		moveThread(runningThread, RUNNING, SUSPENDED);
		return;
	}

	// Comes here only if not a running Thread, else should have dispatched
	Thread_node* t_node = searchInQueue(threadID, &readyQueue);
	if (t_node == NULL) {
		//cout << "Inside suspend : thread not found" << endl;
		return;
	}

	moveThread(t_node, READY, SUSPENDED);
}

/**
 * @brief Resumes a suspended thread.
 * Could be called from any other thread.
 * Puts the thread back to ready queue from suspended queue.
 */
void resume(int threadID) {
	if (!isValidThreadID(threadID)) {
		return;
	}

	Thread_node* t_node = searchInQueue(threadID, &suspendQueue);
	if (t_node == NULL) {
		//cout << "Inside resume : Thread not found" << endl;
		return;
	}

	moveThread(t_node, SUSPENDED, READY);
}

/**
 * @brief Deletes a particular thread.
 * Could be called from any other thread.
 * Deleted thread object lives, but it will lose all executing capability.
 */
void deleteThread(int threadID) {
	if (!isValidThreadID(threadID)) {
		return;
	}

	Thread_node* t_node = searchInQueue(threadID, &masterList);
	if (t_node == NULL) {
		//cout << "Inside deleteThread: thread not found" << endl;
		return;
	}

	switch (t_node->stats->state) {
	case DELETED:
		//cout << "Inside deleteThread: thread already deleted" << endl;
		break;
	case READY:
		moveThread(t_node, READY, DELETED);
		break;
	case SLEEPING:
		moveThread(t_node, SLEEPING, DELETED);
		break;
	case RUNNING:
		moveThread(t_node, RUNNING, DELETED);
		break;
	case NEW:
		moveThread(t_node, NEW, DELETED);
		break;
	case SUSPENDED:
		moveThread(t_node, SUSPENDED, DELETED);
		break;
	case WAITING:
		moveThread(t_node, WAITING, DELETED);
		break;
	case TERMINATED:
		moveThread(t_node, TERMINATED, DELETED);
		break;
	}
}

/**
 * @brief Sleeps the calling thread for sec seconds.
 * Sleep is not a blocking call.
 * Puts the thread into sleeping state, and on completion puts back to ready queue.
 */
void sleep(int sec) {
	alarm(0);
	//Adding the total requested sleeping time
	runningThread->stats->totalRequestedSleepingTime += sec * 1000;

	uint64_t time = getCurrentTimeMillis();
	runningThread->timers->sleepEndTime = time + sec * 1000;
	//cout << "/----------------" << endl;
	//cout << "thread going to sleep for " << sec << " seconds, id: " << runningThread->stats->threadID << endl;
	//cout << "----------------/" << endl;
	bool firstTime = true;
	while (runningThread->timers->sleepEndTime > time) {
		if (!firstTime) {
			runningThread->stats->numberOfBursts--;
		} else {
			firstTime = false;
		}
		moveThread(runningThread, RUNNING, SLEEPING);
		time = getCurrentTimeMillis();
	}
}

/**
 * @brief Calling thread waits till thread with ID threadID returns.
 * Waits till a thread created with the above function returns,
 * and returns the return value of that thread. This
 * function, obviously, waits until that thread is done with.
 */
void *GetThreadResult(int threadID) {
	if (!isValidThreadID(threadID)) {
		return NULL;
	}

	Thread_node* t_node = searchInQueue(threadID, &masterList);
	if (t_node == NULL) {
		//cout << "Inside GetThreadResult: thread not found" << endl;
		return NULL;
	}

	if (t_node->stats->state == RUNNING) {
		//cout	<< "Inside GetThreadResult: Well this is embarrassing, I don't know my result" << endl;
		return NULL;
	}

	if (t_node->fn_arg == NULL) {
		//cout << "Inside GetThreadResult: Dude I don't have a result" << endl;
		return NULL;
	}

	if (t_node->stats->state == DELETED) {
		//cout << "Inside GetThreadResult: thread deleted" << endl;
		return NULL;
	}

	if (t_node->stats->state != TERMINATED) {
		(t_node->threadsWaitingForMe).push_back(runningThread);
		moveThread(runningThread, RUNNING, WAITING);
	}

	return t_node->fn_arg_result;
}

/**
 * @brief Calling thread waits till thread with ID threadID completes.
 * Waits till a thread created with the above function completes,
 * This function, obviously, waits until that thread is done with.
 */
void JOIN(int threadID) {
	if (!isValidThreadID(threadID)) {
		return;
	}

	Thread_node* t_node = searchInQueue(threadID, &masterList);
	if (t_node == NULL) {
		//cout << "Inside GetThreadResult: thread not found" << endl;
		return;
	}

	if (t_node->stats->state == RUNNING) {
		//cout << "Inside JOIN: Well this is embarrassing, how can I join Myself"	<< endl;
		return;
	}

	if (t_node->stats->state == DELETED) {
		//cout << "Inside JOIN: thread deleted" << endl;
		return;
	}

	if (t_node->stats->state != TERMINATED) {
		(t_node->threadsWaitingForMe).push_back(runningThread);
		moveThread(runningThread, RUNNING, WAITING);
	}
}

/** @brief cleans up everything.
 * Stops scheduler, frees all the space allocated, print statistics per thread.
 */
void clean() {
	alarm(0);
	//changing the state of runningThread to have proper stats
	if (runningThread != NULL) {
		changeState(runningThread, READY);
	}

	//all queues empty
	emptyQueue(&newQueue);
	emptyQueue(&readyQueue);
	emptyQueue(&suspendQueue);
	emptyQueue(&deleteQueue);
	emptyQueue(&sleepQueue);
	emptyQueue(&terminateQueue);
	emptyQueue(&waitingQueue);

	while (!masterList.empty()) {
		Thread_node* t_node = masterList.front();
		printStats(t_node);
		free(deque(&masterList));
	}
	exit(0);
}

/** @brief The calling thread yields the CPU.
 * Calling thread passes control to another next thread.
 */
void yield() {
	alarm(0);
	dispatch(14);
}
