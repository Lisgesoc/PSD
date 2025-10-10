#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

// Maximum message length
#define MAX_MSG_LENGTH 256

// Maximum number of connections
#define MAX_CONNECTIONS 5

//DEFAULTS MESSAGES
#define W_LENTH 28
#define S_LENTH 16
#define H_LENTH 17
const char *MSG_WELCOME = "Welcome to the Echo server\n";
const char *MSG_SEND = "Send message : ";
const char *MSG_HEAR = "Some one said : ";
const char *MSG_CLOSE = "%close%";

//Case Code
const short int TO_SEND=111;
const short int TO_HEAR=222;
const short int CLOSE=000;

// Function for thread processing
void *threadProcessing(void *arg);



// Thread parameter
struct ThreadArgs{
	int c1;
	int c2;
};

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
void sendOption(int socket, short int *option){
	if (send(socket, option, sizeof(short int), 0) != sizeof(short int)){
		showError("Error while sending option to client");
	}
}



//
void changeOption(short int option[]){
	if(option[0] == TO_SEND){
		option[0] = TO_HEAR;
		option[1] = TO_SEND;
	}
	else{
		option[0] = TO_SEND;
		option[1] = TO_HEAR;
	}
}


/**
 * Create, bind and listen
 */
int createBindListenSocket (unsigned short port){

	int socketId;
	struct sockaddr_in echoServAddr;

		if ((socketId = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
			showError("Error while creating a socket") ;

		// Set server address
		memset(&echoServAddr, 0, sizeof(echoServAddr));
		echoServAddr.sin_family = AF_INET;
		echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY);
		echoServAddr.sin_port = htons(port);

		// Bind
		if (bind(socketId, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0)
			showError ("Error while binding");

		if (listen(socketId, MAX_CONNECTIONS) < 0)
			showError("Error while listening") ;

	return socketId;
}

/**
 * Accept connection
 */
int acceptConnection (int socketServer){

	int clientSocket;
	struct sockaddr_in clientAddress;
	unsigned int clientAddressLength;

		// Get length of client address
		clientAddressLength = sizeof(clientAddress);

		// Accept
		if ((clientSocket = accept(socketServer, (struct sockaddr *) &clientAddress, &clientAddressLength)) < 0)
			showError("Error while accepting connection");

		printf("Connection established with client: %s\n", inet_ntoa(clientAddress.sin_addr));

	return clientSocket;
}


void clean_string(char *str) {
    int i = strlen(str) - 1;
    while (i >= 0 && (str[i] == ' ' || str[i] == '\n' || str[i] == '\r')) {
        str[i] = '\0';
        i--;
    }
}

/**
 * Thread processing function
 */
void *threadProcessing(void *threadArgs){

	char message[MAX_MSG_LENGTH];
	unsigned int c1,c2, habla, escucha;
	short int option[2];
	short int *ptr ;


	// Detach resources
	pthread_detach(pthread_self()) ;

	// Get client socket from thread param
	c1 = ((struct ThreadArgs *) threadArgs)->c1;
	c2 = ((struct ThreadArgs *) threadArgs)->c2;

	sendMSG(c1, MSG_WELCOME);
	option[0] = TO_SEND;
	ptr = &option[0];
	sendOption(c1, ptr);
	habla = c1;

	sendMSG(c2, MSG_WELCOME);
	option[1] = TO_HEAR;
	ptr = &option[1];
	sendOption(c2, ptr);
	escucha = c2;

		

	while(1){	
		// Clear buffer
		memset (message, 0, MAX_MSG_LENGTH);

		// Receive message
		recvMSG(habla, message);
		clean_string(message);

		if (strcmp(message, MSG_CLOSE) == 0) {
            printf("Sala cerrada por mensaje 'close%%'. Cerrando conexiones...\n");
			sendMSG(escucha, message);
			ptr=(short int *)&CLOSE;
			sendOption(c1, ptr);
			sendOption(c2, ptr);
            break;
        }

		printf ("Message received -> %sThread ID:%lu [before sleep]\n", message, (unsigned long) pthread_self());
		printf ("Thread ID:%lu [after sleep]\n", (unsigned long) pthread_self());

		// Send message back to client
		sendMSG(escucha, message);

		changeOption(option);
		ptr = &option[0];
		sendOption(c1, ptr);
		ptr = &option[1];
		sendOption(c2, ptr);

		if(option[0] == TO_SEND){
			habla = c1;
			escucha = c2;
		}
		else{
			habla = c2;
			escucha = c1;
		}
	}
	close(c1);
	close(c2);
	free(threadArgs) ;

	return (NULL) ;
}




int main(int argc, char *argv[]){

	int serverSocket;
	int c1,c2;
	unsigned short listeningPort;
	pthread_t threadID;
	struct ThreadArgs *threadArgs;

		// Check arguments
		if (argc != 2){
			fprintf(stderr, "Usage: %s port\n", argv[0]);
			exit(1);
		}

		// Get the port
		listeningPort = atoi(argv[1]);

		// Create a socket (also bind and listen)
		serverSocket = createBindListenSocket (listeningPort);

		// Infinite loop...
		while(1){

			// Establish connection with a client
			c1 = acceptConnection(serverSocket);
			c2 = acceptConnection(serverSocket);

			// Allocate memory
			if ((threadArgs = (struct ThreadArgs *) malloc(sizeof(struct ThreadArgs))) == NULL){
				showError("Error while allocating memory");
			}

			// Set socket to the thread's parameter structure
			threadArgs->c1 = c1;
			threadArgs->c2 = c2;

			// Create a new thread
			if (pthread_create(&threadID, NULL, threadProcessing, (void *) threadArgs) != 0){
				showError("pthread_create() failed");
			}
		}
}





