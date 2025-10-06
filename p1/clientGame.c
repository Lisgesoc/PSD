#include "clientGame.h"
#include <stdbool.h>

unsigned int readBet() {

	int isValid, bet = 0;
	tString enteredMove;

	// While player does not enter a correct bet...
	do {

		// Init...
		bzero(enteredMove, STRING_LENGTH);
		isValid = TRUE;

		printf("Enter a value:");
		fgets(enteredMove, STRING_LENGTH - 1, stdin);
		enteredMove[strlen(enteredMove) - 1] = 0;

		// Check if each character is a digit
		for (int i = 0; i < strlen(enteredMove) && isValid; i++)
			if (!isdigit(enteredMove[i]))
				isValid = FALSE;

		// Entered move is not a number
		if (!isValid)
			printf(
					"Entered value is not correct. It must be a number greater than 0\n");
		else
			bet = atoi(enteredMove);

	} while (!isValid);

	printf("\n");

	return ((unsigned int) bet);
}

unsigned int readOption() {

	unsigned int bet;

	do {
		printf("What is your move? Press %d to hit a card and %d to stand\n",
				TURN_PLAY_HIT, TURN_PLAY_STAND);
		bet = readBet();
		if ((bet != TURN_PLAY_HIT) && (bet != TURN_PLAY_STAND))
			printf("Wrong option!\n");
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
		printf("Posición de array Cartas del jugador Activo: ");
		for (unsigned int i = 0; i < numCartas; i++) {
			printf("%u ", cartas[i]);
		}
		
		printf("\n");
		printf("\n");
		printf("↑↑ Estadisticas mesa jugador Activo. ↑↑\n");
		printf("\n");
	}
	return code;
}

int main(int argc, char *argv[]) {

	int socketfd; /** Socket descriptor */
	unsigned int port; /** Port number (server) */
	struct sockaddr_in server_address; /** Server address structure */
	char* serverIP; /** Server IP */
	unsigned int endOfGame = 0; /** Flag to control the end of the game */
	tString playerName; /** Name of the player */
	unsigned int code; /** Code */
	unsigned int stack;

	// Check arguments!
	if (argc != 3) {
		fprintf(stderr, "ERROR wrong number of arguments\n");
		fprintf(stderr, "Usage:\n$>%s serverIP port\n", argv[0]);
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
	if (connect(socketfd, (struct sockaddr *) &server_address,
			sizeof(server_address)) < 0)
		showError("ERROR while establishing connection");

	printf("Introduce tu nombre: ");
	memset(buffer, 0, MAX_MSG_LENGTH);
	fgets(buffer, MAX_MSG_LENGTH - 1, stdin);
	buffer[strcspn(buffer, "\n")] = 0;
	strncpy(playerName, buffer, STRING_LENGTH - 1);
	playerName[STRING_LENGTH - 1] = '\0';
	sendMessage(socketfd, buffer);

	while (endOfGame == 0) {
		
		recv(socketfd, &stack, sizeof(stack), 0);
		recv(socketfd, &code, sizeof(code), 0);
		if (code == TURN_BET) {
			printf("\n");
			printf("------Empieza una nueva ronda!------\n");
			printf("Tu stack actual es: %u\n", stack);
			bool validBet = false;

			while (!validBet) {
				printf("Introduce tu apuesta:\n ");
				unsigned int bet = readBet();
				send(socketfd, &bet, sizeof(bet), 0);

				memset(&code, 0, sizeof(code));
				recv(socketfd, &code, sizeof(code), 0);

				if (code == TURN_BET_OK) {
					
					printf("------Apuesta realizada.------\n");
					printf("\n");
					printf("\n");
					validBet = true;
					memset(&code, 0, sizeof(code));

					unsigned int newCode = TURN_PLAY;
					while ((newCode != TURN_GAME_LOSE) && (newCode != TURN_GAME_WIN)) {
					
						newCode = recibirEstadoJugador(socketfd);
						switch (newCode) {
						case TURN_PLAY:						
							
							unsigned int option = readOption();
							sendCode(socketfd, option);
							
							break;
						case TURN_PLAY_WAIT:
							printf("Mesa del otro jugador...\n");
							break;
						case TURN_PLAY_OUT:
							printf("Has superado los puntos permitidos. \n");
							break;
						case TURN_GAME_LOSE:
							printf("\n");
							printf(".------Has perdido la mano.------. \n");
							printf("\n");
							break;
						case TURN_PLAY_RIVAL_DONE:
        					printf("Tu rival ha terminado su turno. Ahora es tu turno.\n");
        					break;
						case TURN_GAME_WIN:
							printf("\n");
							printf("------Has ganado la mano.------ \n");
							printf("\n");
							break;
						default:
							printf("Código inesperado: %u\n", newCode);
							break;
						}
					}
					printf("------La ronda ha terminado.------\n");

				} else if (code == TURN_BET) {
					printf("Apuesta incorrecta, intenta de nuevo.\n");
				} else {
					printf("Código inesperado: %u\n", code);

				}
			}
		} else if (code == TURN_PLAY_OUT) {
			printf("------Partida finalizada.------ \n");
			endOfGame = 1;
		}

	}

	// Close socket
	close(socketfd);

}
