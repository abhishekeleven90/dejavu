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
bool startsWith(const char *a, const char *b);
unsigned int fetchPortNumber();
char* substring(char *string, int position, int length);
bool isValidPort(unsigned int port);
void helperPort();
void helperCreate(bool created, bool & joined);
void helperJoin(bool joined);
void helperHelp();

//-----TCP Functions-------
void UI();
void server();
void client();

//-----Chord Functions----------

//****************Function Definitions*******************
//-------Helper Functions----------
bool startsWith(const char *a, const char *b) {
	if (strncmp(a, b, strlen(b)) == 0) {
		return 0;
	}
	return 1;
}

unsigned int fetchPortNumber() {
	char *portNumber = substring(ui_data, 6, strlen(ui_data) - 5);
	unsigned int port = atoi(portNumber);
	if (isValidPort(port)) {
		return port;
	}
	return 0;
}

bool isValidPort(unsigned int port) {
	if (port == 0 || port > 65535) {
		cout << "port invalid or reserved - port Number did not set" << endl;
		return false;
	}
	return true;
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

void helperPort() {
	server_port = fetchPortNumber();
	if (server_port != 0) {
		cout << "port: set to " << server_port << endl;
	}
}

void helperCreate(bool created, bool & joined) {
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

void helperJoin(bool joined) {
	//assume server add is 127.0.0.1 and server acc to you, initial 5001
	//here act as client, and ping server, to accept, it should give some info
	//as of now the server just notifies to close connection
	if (joined == false) {
		joined = true;
		int clientThreadID = create(client);
		run(clientThreadID);
	} else {
		cout << "Already in a network and joined, joined thread running"
				<< endl;
	}
}

void helperHelp() {
	cout << "Commands supported: " << endl;
	cout << "\thelp\t\t\t\t ==> Provides a list of command and their usage details"
			<< endl;
	cout << "\tport <x>\t\t\t ==> sets the port number (eg- port 1234)" << endl;
	cout << "\tcreate\t\t\t\t ==> creates a ring" << endl;
	cout
			<< "\tjoin <x>\t\t\t ==> joins the ring with x address (eg- join 111.111.111.111:1000)"
			<< endl;
	cout << "\tquit\t\t\t\t ==> shuts down the ring" << endl;
	cout
			<< "\tput <key> <value>\t\t ==> inserts the given <key,value> pair in the ring (eg- put 23 654)"
			<< endl;
	cout
			<< "\tget <key>\t\t\t ==> returns the previously inserted value corresponding to the key(eg- get 23)"
			<< endl << endl;

	cout
			<< "\t---Yes!!! we have implemented BONUS COMMANDS, mentioned them below---"
			<< endl;
	cout << "\tfinger\t\t\t\t ==> prints the list of addresses of nodes on the ring"
			<< endl;
	cout << "\tsuccessor\t\t\t ==> prints the address of the next node on the ring"
			<< endl;
	cout
			<< "\tpredecessor\t\t\t ==> prints the address of the previous node on the ring"
			<< endl;
	cout << "\tdump\t\t\t\t ==> displays all information pertaining to calling node"
			<< endl;
	cout
			<< "\tdumpaddr <address>\t\t ==> displays all information pertaining to node at address (eg - dump 111.111.111.111:1000)"
			<< endl;
	cout << "\tdumpall\t\t\t\t ==> displays all information of all the nodes" << endl;
}

//-----TCP Functions-------
void UI() {
	bool created = false;
	bool joined = false;

	while (1) {
		cout << "------------------------------" << endl;
		cout << ">>>: ";
		fgets(ui_data, sizeof(ui_data), stdin);

		if (strcmp(ui_data, "exit") == 0 || strcmp(ui_data, "EXIT") == 0) {
			break;
		}

		cout << "<<<: " << ui_data << endl;

		if (strcmp(ui_data, "create\n") == 0) {
			helperCreate(created, joined);
		}

		if (strcmp(ui_data, "help\n") == 0) {
			helperHelp();
		}

		else if (startsWith(ui_data, "port") == 0) {
			helperPort();
		}

		else if (strcmp(ui_data, "join\n") == 0) {
			helperJoin(joined);
		}

		else {
			cout
					<< "sorry! seems like you are new here, please type 'help' for list of commands"
					<< endl;
		}

		fflush(stdout);
	}
}

void server() {
	//will take an int let's say the port is random : 5001 --- //TO-DO: remove the comment
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
			<< ntohs(server_addr.sin_port);
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
	cout << "client started" << endl;

	int sock, bytes_recieved;
	struct hostent *host;

	struct sockaddr_in server_addr;

	host = gethostbyname("127.0.0.1");
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
			cout << "harinder jee" << endl;
			//fflush(stdout);
			break;
		}
	}
	cout << "Client end! Server sent to bug off. Or client ran away!" << endl;
}

void otherThread() {
	while (1) {

	}
}

int main() {
	create(UI);
	create(otherThread);
	start();
	return 0;
}
