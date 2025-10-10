#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <arpa/inet.h>
#include <stdbool.h>

#define MAX_MSG_LENGTH 256		// Maximum length of the message

//DEFAULTS MESSAGES
const char *MSG_SEND = "Envia tu puja : ";
const char *MSG_SEND_NAME = "Envia tu nombre : ";
const char *MSG_CLOSE = "El servidor ha cerrado la sala.\n";


const short int TO_SEND=111;
const short int REJECTED=222;

/** Function that shows an error message */
void showError(const char *errorMsg){
    perror(errorMsg);
    exit(0);
}

//Send and receive functions
void sendMSG(int socket, const char *msg){
	short int *ptr ;
	short int length = strlen(msg);
	ptr = &length;
	send(socket, ptr, sizeof(short int), 0);
	if (send(socket, msg, *ptr, 0) != length){
		showError("Error while sending message to client");
	}
}
void recvMSG(int socket, char *msg){
	memset (msg, 0, MAX_MSG_LENGTH);
	short int *length=malloc(sizeof(short int));
	recv(socket,length,sizeof(short int),0);
	if ((recv(socket, msg, *length, 0)) < 0){
		showError("Error while receiving");
	}
	msg[*length] = '\0';
	free(length);
}
void recvOption(int socket, short int *option){
	if ((recv(socket, option, sizeof(short int), 0)) < 0){
		showError("Error while receiving option");
	}
}
void sendDouble(int socket, double *num){
	if (send(socket, num, sizeof(double), 0) != sizeof(double)){
		showError("Error while sending double to client");
	}
}

int main(int argc, char *argv[]){

    int socketfd;						/** Socket descriptor */
    unsigned int port;					/** Port number */
    struct sockaddr_in server_address;	/** Server address structure */
    char* serverIP;						/** Server IP */
    char message [MAX_MSG_LENGTH];		/** Message sent to the server side */
	short int option;


		// Check arguments!
		if (argc < 3){
			fprintf(stderr,"ERROR wrong number of arguments\n");
		   	fprintf(stderr,"Usage:\n$>%s host port\n", argv[0]);
		   	exit(0);
		}

		// Get the port
		port = atoi(argv[2]);

		// Create socket
		socketfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

		// Check if the socket has been successfully created
		if (socketfd < 0)
			showError("ERROR while creating the socket");

		// Get the server address
		serverIP = argv[1];

		// Fill server address structure
		memset(&server_address, 0, sizeof(server_address));
		server_address.sin_family = AF_INET;
		server_address.sin_addr.s_addr = inet_addr(serverIP);
		server_address.sin_port = htons(port);

		// Connect with server
		if (connect(socketfd, (struct sockaddr *) &server_address, sizeof(server_address)) < 0){
			showError("ERROR while establishing connection");
		}

		// Read welcome message and option
		recvMSG(socketfd, message);
		printf("%s\n",message);
		recvOption(socketfd, &option);

		double *puja=malloc(sizeof(double));
		memset (message, 0, MAX_MSG_LENGTH);
		switch(option){
			case 111:
				//Nombre
				printf("%s\n",MSG_SEND_NAME);
				fgets(message, MAX_MSG_LENGTH-1, stdin);
				sendMSG(socketfd, message);
				memset (message, 0, MAX_MSG_LENGTH);
				//Puja
				recvMSG(socketfd, message);
				printf("%s\n",message);
				printf("%s",MSG_SEND);
				fgets(message, MAX_MSG_LENGTH-1, stdin);
				*puja = atof(message);
				sendDouble(socketfd, puja);
				recvMSG(socketfd, message);
				printf("%s\n",message);
				break;
			case 222:
				break;
			default:
				showError("Error: unknown option");
				break;
		}

		// Close socket
		close(socketfd);
		free(puja);

	return 0;
}
