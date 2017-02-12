//GVSU CIS 457 - Data Communications
//Lab 3
//Troy Veldhuizen
//Due 1/27/17
//tcpserver.c
 
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

struct clientinfo{
	struct sockaddr_in addr;
	int socket;
};

void* recieve_message(void *arg){
		int run = 1;
		struct clientinfo ci = *(struct clientinfo *)arg;
		int clientsocket = ci.socket;
		char line[5000];
		// e is the number of bytes recieved
		while(run == 1){
		int e = recv(clientsocket,line,5000,0);
		if(e < 0){
			printf("Error recieving\n");
			return 1;
		}
		printf("\nGot from the client: %s\n", line);
		}
	
}


int main(int argc, char **argv){
	int sockfd = socket(AF_INET, SOCK_STREAM,0);

	struct sockaddr_in serveraddr, clientaddr;
	serveraddr.sin_family = AF_INET;
	// ask for port number
	printf("What port do you want to listen to? \n");
	int port;
	int err = scanf("%d", &port);
	if(err < 0){
		printf("error getting port\n");
	}
	serveraddr.sin_port = htons(port); //had to match
	serveraddr.sin_addr.s_addr = INADDR_ANY;

	int e = bind(sockfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
	
	//check for error
	if(e < 0){
		printf("There was an error binding the address\n");
	}
	
	listen(sockfd,10);

	// does not handle multiple clients concurrently

	int len = sizeof(clientaddr);
	//blocking call: waits for connection
	int clientsocket = accept(sockfd,(struct sockaddr*)&clientaddr,&len);
	
	struct clientinfo ci;
	ci.socket = clientsocket;
	ci.addr = clientaddr;
	
	// pthreads share the same memory and global variables.
	// solved by semephores and locking

	pthread_t childrec;
	pthread_create(&childrec, NULL, recieve_message, &ci);
	pthread_detach(childrec); // will clean up thread when function ends
	
	char message[5000];

	printf("Enter to chat with client: ");	
	
	while (fgets(message, 5000, stdin) != NULL) {

		send(clientsocket, message,strlen(message)+1,0);
		
	}

	close(clientsocket);
	// one socket to listen to incomeing
	// pass off to one socket for specific client	
}	
