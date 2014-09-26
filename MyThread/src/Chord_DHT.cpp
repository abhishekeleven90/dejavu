#include "MyThread.h"
#include <iostream>
#include <string.h>
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


#define SECOND 1000000

using namespace std;

void server();
void client();

void UI() {
	char ui_data [1024];
	bool created=false;
	bool joined=false;

	while(1)
	{
		cout<<"------------------------------"<<endl;
		cout << ">>>: ";
		gets(ui_data);
	
		if (strcmp(ui_data , "exit") == 0 || strcmp(ui_data , "EXIT") == 0)
			break;
	
		cout <<"<<<: " << ui_data << endl;
	
		if(strcmp(ui_data , "create")==0)
		{
			//create a listening socket here
			if(created==false)
			{
				created=true;
				joined=true;
				int serverId = create(server);
				run(serverId);
			}
			else
			{
				cout << "Already in a network, server thread running" <<endl;
			}

		}
		if(strcmp(ui_data , "join")==0)
		{
			//assume server add is 127.0.0.1 and server acc to you, initial 5001
			//here act as client, and ping server, to accept, it should give some info
			//as of now the server just notifies to close connection
			if(joined==false)
			{
				joined=true;
				int clientThreadID = create(client);
				run(clientThreadID);
			}
			else
			{
				cout << "Already in a network and joined, joined thread running" <<endl;
			}

		}
		fflush(stdout);	
	}
}


void server()
{
		//will take an int let's say the port is random : 5001
		int sock, connected, bytes_recieved , trueint = 1;  
    	char send_data [1024] , recv_data[1024];       

        struct sockaddr_in server_addr,client_addr;    
        unsigned int sin_size;
        
        if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
            perror("Socket");
            exit(1);
        }

        if (setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&trueint,sizeof(int)) == -1) {
            perror("Setsockopt");
            exit(1);
        }
        
        server_addr.sin_family = AF_INET;         
        server_addr.sin_port = htons(5001);     
        server_addr.sin_addr.s_addr = INADDR_ANY;

        bzero(&(server_addr.sin_zero),8); 

        if (bind(sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr))
                                                                       == -1) {
            perror("Unable to bind");
            exit(1);
        }

        if (listen(sock, 5) == -1) {
            perror("Listen");
            exit(1);
        }
		
		cout<<"TCPServer Waiting for client on port 5001"<<endl;
        fflush(stdout);
        while(1)
        {  

            sin_size = sizeof(struct sockaddr_in);

            connected = accept(sock, (struct sockaddr *)&client_addr,&sin_size);

            printf("\n I got a connection from (%s , %d)",
                   inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));

            cout << "closing the connection after communicating"<<endl;
			

			send_data[0]='q';
			send_data[1]='\0';

			send(connected, send_data,strlen(send_data), 0);

            close(connected);

            cout<<"closed";

		}

		//right now, doesn't reach here
		close(sock);
}

void client()
{
	
	    cout <<"client started"<<endl;

	    int sock, bytes_recieved;  
        char send_data[1024],recv_data[1024];
        struct hostent *host;

        struct sockaddr_in server_addr;  

        host = gethostbyname("127.0.0.1");

        cout<<"creating client socket"<<endl;

        if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
            perror("Socket");
            exit(1);
        }

        cout<<"client socket created"<<endl;

        server_addr.sin_family = AF_INET;     
        server_addr.sin_port = htons(5001);   
        server_addr.sin_addr = *((struct in_addr *)host->h_addr);
        bzero(&(server_addr.sin_zero),8); 

        cout<<"client will try and connect to server"<<endl;

        while (connect(sock, (struct sockaddr *)&server_addr,
                    sizeof(struct sockaddr)) == -1) 
        {
        	//At times the server is not up.
        }

        cout<<"client connected to server\n"<<endl;

		while(1)
		{
		
		  cout<<"Inside client loop"<<endl;
		  cout<<"Receive data from server"<<endl;
		  cout <<"Client socket ID:"<<sock<<endl;
		  
          bytes_recieved=recv(sock,recv_data,1024,0);

          cout<<"Data successfully received"<<endl;

          recv_data[bytes_recieved] = '\0';
 
          if (strcmp(recv_data , "q") == 0 || strcmp(recv_data , "Q") == 0)
          {
           close(sock);
           break;
          }

          else
          {
           	printf("\nRecieved data = %s " , recv_data);

       	  }
           
           printf("\nSEND (q or Q to quit) : ");
           gets(send_data);
           
          if (strcmp(send_data , "q") != 0 && strcmp(send_data , "Q") != 0)
           send(sock,send_data,strlen(send_data), 0); 

          else
          {
           send(sock,send_data,strlen(send_data), 0);   
           close(sock);
           cout << "harinder jee"<<endl;
			//fflush(stdout);
           break;
          }

		}
		cout<<"Client end! Server sent to bug off. Or client ran away!"<<endl;
}

void otherThread()
{
		while(1)
		{


		}
}

int main() {
	create(UI);
	create(otherThread);
	start();
	return 0;
}
