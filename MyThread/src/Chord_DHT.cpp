#include "MyThread.h"
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

using namespace std;

//----------Constants---------
#define SECOND 1000000
#define QUEUE_LIMIT 5

//----------Globals---------
char ui_data[1024];
char server_send_data[1024], server_recv_data[1024];
char client_send_data[1024], client_recv_data[1024];
unsigned int server_port = 0;

struct Node {
	Node* predecessor;

};

//****************Function Declarations*******************
//-------Helper Functions----------
void tab(int count);
void helperHelpNewCmd();
void helperHelp();
bool startsWith(const char *a, const char *b);
char* fetchAddress(char* cmd, int pos);
bool isValidAddress(char* addr);
unsigned int fetchPortNumber(char* string, int pos);
bool isValidPort(unsigned int port);
int indexOf(char* string, char of);
char* substring(char *string, int position, int length);
void helperPort();
void helperCreate(bool created, bool & joined);
void helperJoin(bool joined);
bool isValidJoinCmd();

//-----TCP Functions-------
void UI();
void server();
void client();

//-----Chord Functions----------

//****************Function Definitions*******************
//-------Helper Functions----------
void tab(int count) {
	for (int i = 0; i < count; i++) {
		cout << "\t";
	}
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

bool startsWith(const char *a, const char *b) {
	if (strncmp(a, b, strlen(b)) == 0) {
		return 0;
	}
	return 1;
}

char* fetchAddress(char* cmd, int pos) {
	char* addr = substring(cmd, pos, strlen(cmd) - pos);
	if (!isValidAddress(addr)) {
		cout << "fetchAddress: Address not valid" << endl;
		return NULL;
	}
	return addr;
}

bool isValidAddress(char* addr) {
	int len = strlen(addr);
	int dotCount = 0;
	int colonCount = 0;

	for (int i = 0; i < len; i++) {
		if (addr[i] == ':') {
			colonCount++;
		} else if (addr[i] == '.') {
			dotCount++;
		}
	}

	if (colonCount == 1 && dotCount == 3) {
		return true;
	}
	return false;
}

unsigned int fetchPortNumber(char* string, int pos) {
	char *portNumber = substring(string, pos, strlen(string) - pos + 1);
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

int indexOf(char* string, char of) {
	int len = strlen(string);
	for (int i = 0; i < len; i++) {
		if (string[i] == of) {
			return i;
		}
	}
	return len-1;
}

char* substring(char *string, int position, int length) {
	char *pointer;
	int c;

	pointer = (char*) malloc(length + 1);

	if (pointer == NULL) {
		printf("Inside substring : Unable to allocate memory.\n");
		exit(EXIT_FAILURE);
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

void helperPort(char* portCmd) {
	server_port = fetchPortNumber(portCmd, 6);
	if (server_port != 0) {
		cout << "port: set to " << server_port << endl;
	} else {
		cout << "port Number did not set" << endl;
	}
}

void helperCreate(int& created, int& joined) {
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

void helperJoin(char* joinCmd, bool joined) {
	char* addr = fetchAddress(joinCmd, 6);
	if (addr == NULL) {
		return;
	}
	unsigned int port = fetchPortNumber(addr, indexOf(addr, ':') + 2);

	if (port == 0) {
		//Invalid portNumber
		return;
	}

	if (joined == false) {
		joined = true;
		int clientThreadID = create(client);
		run(clientThreadID);
	} else {
		cout << "Already in a network and joined, joined thread running"
				<< endl;
	}
}

bool checkJoinUsage(char* cmd) {
	return true;
}

//-----TCP Functions-------
void UI() {
	int created = false;
	int joined = false;

	while (1) {
		cout << "------------------------------" << endl;
		cout << ">>>: ";
		fgets(ui_data, sizeof(ui_data), stdin);

		cout << "<<<: " << ui_data << endl;

		char* cmdType = substring(ui_data, 0, indexOf(ui_data, ' '));

		if (strcmp(cmdType, "exit") == 0 || strcmp(cmdType, "EXIT") == 0) {
			break;
		}

		else if (strcmp(cmdType, "create") == 0) {
			helperCreate(created, joined);
		}

		else if (strcmp(cmdType, "help") == 0) {
			helperHelp();
		}

		else if (strcmp(cmdType, "port") == 0) {
			helperPort(ui_data);
		}

		else if (strcmp(cmdType, "join") == 0) {
			helperJoin(ui_data, joined);
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

	if (bind(sock, (struct sockaddr *) &server_addr, sizeof(struct sockaddr))
			== -1) {
		perror("Unable to bind");
		exit(1);
	}

	if (listen(sock, QUEUE_LIMIT) == -1) {
		perror("Listen");
		exit(1);
	}

	cout << "Starting to listen on: " << inet_ntoa(server_addr.sin_addr) << ":"
			<< ntohs(server_addr.sin_port) << endl;
	fflush(stdout);

	while (1) {
		sin_size = sizeof(struct sockaddr_in);
		connected = accept(sock, (struct sockaddr *) &client_addr, &sin_size);

		printf("\n I got a connection from (%s , %d)",
				inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

		cout << "closing the connection after communicating" << endl;

		server_send_data[0] = 'q';
		server_send_data[1] = '\0';

		send(connected, server_send_data, strlen(server_send_data), 0);
		close(connected);
		cout << "closed";
	}
	//right now, doesn't reach here
	close(sock);
}

void client() {
	cout << "---client started---" << endl;

	int sock, bytes_recieved;
	struct hostent *host;
	struct sockaddr_in server_addr;

	cout << "creating client socket" << endl;

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("Socket");
		exit(1);
	}
	cout << "client socket created" << endl;

	server_addr.sin_family = AF_INET;
	//server_addr.sin_port = htons(server_port); //TO-DO: might have to change here --- port thing
	server_addr.sin_addr = *((struct in_addr *) host->h_addr);
	bzero(&(server_addr.sin_zero), 8);

	cout << "client will try and connect to server" << endl;

	while (connect(sock, (struct sockaddr *) &server_addr,
			sizeof(struct sockaddr)) == -1) {
		//At times the server is not up.
		cout << "in some bad loop" << endl;
	}

	cout << "client connected to server\n" << endl;

	while (1) {
		cout << "Inside client loop" << endl;
		cout << "Receive data from server" << endl;
		cout << "Client socket ID:" << sock << endl;

		bytes_recieved = recv(sock, client_recv_data, 1024, 0);
		cout << "Data successfully received" << endl;
		client_recv_data[bytes_recieved] = '\0';
		if (strcmp(client_recv_data, "q") == 0 || strcmp(client_recv_data, "Q")
				== 0) {
			close(sock);
			break;
		} else {
			printf("\nRecieved data = %s ", client_recv_data);
		}

		printf("\nSEND (q or Q to quit) : ");
		fgets(client_send_data, sizeof(client_send_data), stdin);

		if (strcmp(client_send_data, "q") != 0 && strcmp(client_send_data, "Q")
				!= 0)
			send(sock, client_send_data, strlen(client_send_data), 0);

		else {
			send(sock, client_send_data, strlen(client_send_data), 0);
			close(sock);
			//fflush(stdout);
			break;
		}
	}
	cout << "Client end! Server sent to bug off. Or client ran away!" << endl;
}

int main() {
	create(UI);
	start();
	return 0;
}
