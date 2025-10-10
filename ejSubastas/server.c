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
#include <math.h>

// Maximum message length
#define MAX_MSG_LENGTH 256

// Maximum number of connections
#define MAX_SALAS 10
#define MAX_CONNECTIONS 2*MAX_SALAS 



//DEFAULTS MESSAGES
const char *MSG_WELCOME = "BIENVENIDO. Este es el servidor de subastas.  \n";
const char *MSG_REJECTED = "El servidor esta ocupado. Intentelo mas tarde.  \n";
const char *MSG_WIN = "Has ganado la subasta!\n";
const char *MSG_LOSE = "Has perdido la subasta.\n";
const char *MSG_EUAL = "Empate en la subasta.\n";

//Case Code
const short int TO_SEND=111;
const short int REJECTED=222;

// Function for thread processing
void *threadProcessing(void *arg);


//Estructuras
struct producto{
	char nombre;
	float precio;
	int puja_ini;
};

struct sala{
	struct producto p;
	int id;
};

struct sala salas[MAX_SALAS];


// Thread parameter
struct ThreadArgs{
	int c1;
	int c2;
	struct sala *s;
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
void recvDouble(int socket, double *num){
	if ((recv(socket, num, sizeof(double), 0)) < 0){
		showError("Error while receiving double");
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
	unsigned int c1,c2;
	struct sala *s;
	struct producto p;
	char name1[20];
	char name2[20];
	double puja1, puja2;


	// Detach resources
	pthread_detach(pthread_self()) ;

	// Get client socket from thread param
	c1 = ((struct ThreadArgs *) threadArgs)->c1;
	c2 = ((struct ThreadArgs *) threadArgs)->c2;
	s = ((struct ThreadArgs *) threadArgs)->s;
	p = s->p;

	sendMSG(c1, MSG_WELCOME);
	sendOption(c1, (short int *)&TO_SEND);

	sendMSG(c2, MSG_WELCOME);
	sendOption(c2,(short int *)&TO_SEND);

	memset(name1,0,20);
	memset(name2,0,20);	
	recvMSG(c1, name1);	
	clean_string(name1);
	recvMSG(c2, name2);
	clean_string(name2);

	//Send product info
	sprintf(message, "Nombre primer pujante %s \n Nombre segundo pujante %s \n \n PRODUCTO A SUBASTA: %c \n Valor: %.2f $ \n Puja inicial: %d $\n",name1,name2,p.nombre,p.precio,p.puja_ini);
	sendMSG(c1, message);
	sendMSG(c2, message);

	//Recibimos pujas
	recvDouble(c1, (double*)&puja1);
	recvDouble(c2, (double*)&puja2);

	// Send message back to client
	double max_puja= (puja1 > puja2) ? puja1 : puja2;
	if(max_puja<p.puja_ini){
		sendMSG(c1, MSG_LOSE);
		sendMSG(c2, MSG_LOSE);
	}else if(puja1>puja2){
		sendMSG(c1, MSG_WIN);
		sendMSG(c2, MSG_LOSE);
	}else if(puja2>puja1){
		sendMSG(c2, MSG_WIN);
		sendMSG(c1, MSG_LOSE);
	}else{
		sendMSG(c1, MSG_EUAL);
		sendMSG(c2, MSG_EUAL);
	}


	close(c1);
	close(c2);
	free(threadArgs) ;
	return (NULL) ;
}

char p_nombres[10]={'Z','X','C','V','B','N','M','L','K','J'};
float p_precios[10]={10.0,20.0,30.0,40.0,50.0,60.0,70.0,80.0,90.0,100.0};

struct producto createProductos(){
	struct producto p;
	int aleatorio = rand() % 10; // Número aleatorio entre 0 y 9
	p.nombre = p_nombres[aleatorio];
	aleatorio = rand() % 10; // Número aleatorio entre 0 y 9
	p.precio = p_precios[aleatorio];
	p.puja_ini = p.precio-p.precio*0.1;
	return p;
}

void initSalas(struct sala salas[]){
	for(int i=0;i<MAX_SALAS;i++){
		salas[i].id = i;
		salas[i].p = createProductos();
	}
}


int main(int argc, char *argv[]){

	int serverSocket;
	int c1,c2;
	unsigned short listeningPort;
	pthread_t threadID;
	struct ThreadArgs *threadArgs;
	
	initSalas(salas);

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
	for(int i=0;i<MAX_SALAS;i++){

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
		threadArgs->s = &salas[i];

		// Create a new thread
		if (pthread_create(&threadID, NULL, threadProcessing, (void *) threadArgs) != 0){
			showError("pthread_create() failed");
		}
	}

	while(1){
		c1 = acceptConnection(serverSocket);
		sendMSG(c1, MSG_REJECTED);
		sendOption(c1, (short int *)&REJECTED);
		close(c1);
	}

	close(serverSocket);
	return 0;

}





