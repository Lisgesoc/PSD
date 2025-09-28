#include "clientGame.h"
#include <stdbool.h>

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
void *sendCode(int socket, unsigned int value) {
    int tam = send(socket, &value, sizeof(value), 0);

    if (tam < 0) {
        perror("Error al enviar unsigned int");
        return NULL;
    }
    return NULL;
}

unsigned int recibirEstadoJugador(int socket) {
    unsigned int code;
    unsigned int numCartas;
	unsigned int puntos; 

    memset(&code, 0, sizeof(code));
    recv(socket, &code, sizeof(code), 0);

    memset(&numCartas, 0, sizeof(numCartas));
    recv(socket, &numCartas, sizeof(numCartas), 0);

	memset(&puntos, 0, sizeof(puntos));
    recv(socket, &puntos, sizeof(puntos), 0);

    printf("Numero de cartas del jugador: %u\n", numCartas);
	printf("Numero de puntos del jugador: %u\n", puntos);

    if (numCartas > 0) {
        unsigned int cartas[DECK_SIZE];
        int bytesEsperados = sizeof(unsigned int) * numCartas;
		memset(cartas, 0, sizeof(cartas));
        int bytesRecibidos = recv(socket, cartas, bytesEsperados, 0);
    	 if (bytesRecibidos != bytesEsperados) {
              perror("Error al recibir las cartas del jugador");
            return code;
        }
        printf("Cartas del jugador: ");
        for (unsigned int i = 0; i < numCartas; i++) {
            printf("%u ", cartas[i]);
        }
        printf("\n");
	}
	return code;
}

int main(int argc, char *argv[]){

	int socketfd;						/** Socket descriptor */
	unsigned int port;					/** Port number (server) */
	struct sockaddr_in server_address;	/** Server address structure */
	char* serverIP;						/** Server IP */
	unsigned int endOfGame =0;			/** Flag to control the end of the game */
	tString playerName;					/** Name of the player */
	unsigned int code;					/** Code */
	unsigned int stack;

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

		
		char buffer[MAX_MSG_LENGTH];
		

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
		buffer[strcspn(buffer, "\n")] = 0; 
        strncpy(playerName, buffer, STRING_LENGTH-1); 
        playerName[STRING_LENGTH-1] = '\0'; 
		sendMessage(socketfd, buffer);	

		
	
    	recv(socketfd, &code, sizeof(code), 0); //Recibir code
    	recv(socketfd, &stack, sizeof(stack), 0); //Recibir stack

   		printf("Tu stack: %u\n", stack);
		while(endOfGame == 0){
   		 	printf("Empieza una nueva ronda!\n");
   		 	printf("Tu stack actual es: %u\n", stack);
		bool validBet = false;
   		
    	while (!validBet) {
        	unsigned int bet = readBet();
        	send(socketfd, &bet, sizeof(bet), 0);
			memset(&code, 0, sizeof(code));
    		recv(socketfd, &code, sizeof(code), 0);
		
       		if (code == TURN_BET_OK) {
           		printf("Apuesta realizada.\n");
           		validBet = true; 
			
		   		memset(&code, 0, sizeof(code));
			
					unsigned int newCode =TURN_PLAY;
					while((newCode != TURN_GAME_LOSE )&& (newCode != TURN_GAME_WIN ) ){
						printf("Esperando recivir estado...\n");
						printf("nnnnnnnn codigo es: %u \n", newCode);
						newCode = recibirEstadoJugador(socketfd);
						printf("EL codigo es: %u \n", newCode);
						printf(" \n");
						printf(" \n");
                 
						if (newCode == TURN_PLAY){
						
							printf("Jugador activo.\n");
							
							unsigned int option = readOption();
							sendCode(socketfd, option);
							
						}
						else if (newCode == TURN_PLAY_WAIT){
							printf("Jugador pasivo.\n");
						}
						else if(newCode ==TURN_PLAY_OUT){
							printf("Has superado los puntos permitidos. \n");
						}
						else if(newCode ==TURN_GAME_LOSE){
							printf("Has perdido la partida. \n");
						
						}
						else if(newCode ==TURN_GAME_WIN){
							printf("Has ganado la partida. \n");
							
						}
					}
					printf("La ronda ha terminad00000000o.\n");
						
					
        		} else if (code == TURN_BET) {
            		printf("Apuesta incorrecta, intenta de nuevo.\n");
        		} else {
            		printf("Código inesperado: %u\n", code);
            	
        	}
    	}
	}

		// Close socket
		close(socketfd);

}