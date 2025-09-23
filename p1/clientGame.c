#include "clientGame.h"


unsigned int readBet (){

	int isValid, bet=0;
	tString enteredMove;
 
		// While player does not enter a correct bet...
		do{

			// Init...
			bzero (enteredMove, STRING_LENGTH);
			isValid = TRUE;

			printf ("Enter a value:");
			fgets(enteredMove, STRING_LENGTH-1, stdin);
			enteredMove[strlen(enteredMove)-1] = 0;

			// Check if each character is a digit
			for (int i=0; i<strlen(enteredMove) && isValid; i++)
				if (!isdigit(enteredMove[i]))
					isValid = FALSE;

			// Entered move is not a number
			if (!isValid)
				printf ("Entered value is not correct. It must be a number greater than 0\n");
			else
				bet = atoi (enteredMove);

		}while (!isValid);

		printf ("\n");

	return ((unsigned int) bet);
}

unsigned int readOption (){

	unsigned int bet;

		do{		
			printf ("What is your move? Press %d to hit a card and %d to stand\n", TURN_PLAY_HIT, TURN_PLAY_STAND);
			bet = readBet();
			if ((bet != TURN_PLAY_HIT) && (bet != TURN_PLAY_STAND))
				printf ("Wrong option!\n");			
		} while ((bet != TURN_PLAY_HIT) && (bet != TURN_PLAY_STAND));

	return bet;
}


int sendMessage(int socket, const char *msg) {
    int len = strlen(msg);
    int tam = send(socket, &len, sizeof(len), 0);
    int mens = send(socket, msg, len, 0);

    if (tam < 0 || mens < 0) {
        perror("Error al enviar mensaje");
        return -1;
    }
    return 0;
}

int recvMessage(int socket, char *buffer, size_t bufferSize) {
	int len;
    int rec = recv(socket, &len, sizeof(len), 0);  
    if (rec <= 0) {
        perror("Error al recibir tamaño de mensaje");
        return -1;
    }

    int recMsg = recv(socket, buffer, len, 0);  
    if (recMsg <= 0) {
        perror("Error al recibir mensaje");
        return -1;
    }

    buffer[recMsg] = '\0';  
    return 0;
}

int main(int argc, char *argv[]){

	int socketfd;						/** Socket descriptor */
	unsigned int port;					/** Port number (server) */
	struct sockaddr_in server_address;	/** Server address structure */
	char* serverIP;						/** Server IP */
	unsigned int endOfGame;				/** Flag to control the end of the game */
	tString playerName;					/** Name of the player */
	unsigned int code;					/** Code */


		// Check arguments!
		if (argc != 3){
			fprintf(stderr,"ERROR wrong number of arguments\n");
			fprintf(stderr,"Usage:\n$>%s serverIP port\n", argv[0]);
			exit(0);
		}

		// Get the server address
		serverIP = argv[1];

		// Get the port
		port = atoi(argv[2]);

		/*TODO*/
		
		char buffer[MAX_MSG_LENGTH];
		ssize_t msgLength;

		// Create socket
		socketfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

		// Check if the socket has been successfully created
		if (socketfd < 0)
			showError("ERROR while creating the socket");

		// Fill server address structure
		memset(&server_address, 0, sizeof(server_address));
		server_address.sin_family = AF_INET;
		server_address.sin_addr.s_addr = inet_addr(serverIP);
		server_address.sin_port = htons(port);

		// Connect with server
		if (connect(socketfd, (struct sockaddr *) &server_address, sizeof(server_address)) < 0)
			showError("ERROR while establishing connection");

		printf("Introduce tu nombre: ");
		memset(buffer, 0, MAX_MSG_LENGTH);
		fgets(buffer, MAX_MSG_LENGTH-1, stdin);
		sendMessage(socketfd, buffer);	



		while(strcmp(buffer, "exit") != 0){
		
			unsigned int code;
			memset(buffer, 0, MAX_MSG_LENGTH);
			recv(socketfd, &code, sizeof(code), 0);
			printf("Código recibido: %u\n", code);
			recv(socketfd, &code, sizeof(code), 0);
			printf("Código recibido: %u\n", code);; 
			printf("Introduce apuesta: "); 
			fgets(buffer, MAX_MSG_LENGTH-1, stdin);
			sendMessage(socketfd, buffer);	

	
		}
		// Close socket
		close(socketfd);

}
