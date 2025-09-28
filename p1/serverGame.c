#include "serverGame.h"
#include <pthread.h>
#include <stdbool.h>

tPlayer getNextPlayer(tPlayer currentPlayer) {

	tPlayer next;

	if (currentPlayer == player1)
		next = player2;
	else
		next = player1;

	return next;
}

void initDeck(tDeck *deck) {

	deck->numCards = DECK_SIZE;

	for (int i = 0; i < DECK_SIZE; i++) {
		deck->cards[i] = i;
	}
}

void clearDeck(tDeck *deck) {

	// Set number of cards
	deck->numCards = 0;

	for (int i = 0; i < DECK_SIZE; i++) {
		deck->cards[i] = UNSET_CARD;
	}
}

void printSession(tSession *session) {

	printf("\n ------ Session state ------\n");

	// Player 1
	printf("%s [bet:%d; %d chips] Deck:", session->player1Name,
			session->player1Bet, session->player1Stack);
	printFancyDeck(&(session->player1Deck));

	// Player 2
	printf("%s [bet:%d; %d chips] Deck:", session->player2Name,
			session->player2Bet, session->player2Stack);
	printFancyDeck(&(session->player2Deck));

	// Current game deck
	if (DEBUG_PRINT_GAMEDECK) {
		printf("Game deck: ");
		printDeck(&(session->gameDeck));
	}
}

void initSession(tSession *session) {

	clearDeck(&(session->player1Deck));
	session->player1Bet = 0;
	session->player1Stack = INITIAL_STACK;

	clearDeck(&(session->player2Deck));
	session->player2Bet = 0;
	session->player2Stack = INITIAL_STACK;

	initDeck(&(session->gameDeck));
}

unsigned int calculatePoints(tDeck *deck) {

	unsigned int points;

	// Init...
	points = 0;

	for (int i = 0; i < deck->numCards; i++) {

		if (deck->cards[i] % SUIT_SIZE < 9)
			points += (deck->cards[i] % SUIT_SIZE) + 1;
		else
			points += FIGURE_VALUE;
	}

	return points;
}

unsigned int getRandomCard(tDeck* deck) {

	unsigned int card, cardIndex, i;

	// Get a random card
	cardIndex = rand() % deck->numCards;
	card = deck->cards[cardIndex];

	// Remove the gap
	for (i = cardIndex; i < deck->numCards - 1; i++)
		deck->cards[i] = deck->cards[i + 1];

	// Update the number of cards in the deck
	deck->numCards--;
	deck->cards[deck->numCards] = UNSET_CARD;

	return card;
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

int gestionarApuesta(int socketJugador, unsigned int stackJugador,
		unsigned int *betPtr) {



	unsigned int bet;
	bool isValid = FALSE;
	while (!isValid) {
		int rec = recv(socketJugador, &bet, sizeof(bet), 0);
		if (rec <= 0)
			return -1;

		if (bet > 0 && bet <= stackJugador) {
			*betPtr = bet;
			sendCode(socketJugador, TURN_BET_OK);
			isValid = TRUE;
		} else {
			sendCode(socketJugador, TURN_BET);
		}
	}

	return 0;
}

//Hacer la comprovacion de errores
int gameStart(int socketJugador, unsigned int code) {
	sendCode(socketJugador, code);
	return 0;
}

void enviarEstadoJugador(int socket, tDeck *deck, unsigned int code) {
	sendCode(socket, code);
	sendCode(socket, deck->numCards);

	sendCode(socket, calculatePoints(deck));
	//printf("Numero de cartas del jugador: %u\n", deck->numCards);
	send(socket, deck->cards, sizeof(unsigned int) * deck->numCards, 0);

}

void *handleGame(void *args) {
	tSession session;
	unsigned int code;
	unsigned int partidaFinalizada = 0;
	int apuestaJugadores = 0;

	tThreadArgs *threadArgs = (tThreadArgs *) args;
	int socketPlayer1 = threadArgs->socketPlayer1;
	int socketPlayer2 = threadArgs->socketPlayer2;

	printf("Nueva partida creada entre sockets %d y %d\n", socketPlayer1, socketPlayer2);

	char buffer[MAX_MSG_LENGTH];
	memset(buffer, 0, MAX_MSG_LENGTH);
	recvMessage(socketPlayer1, buffer, MAX_MSG_LENGTH - 1);
	printf("Nombre de jugador de jugador 1: %s\n", buffer);
	strncpy(session.player1Name, buffer, STRING_LENGTH - 1);

	memset(buffer, 0, MAX_MSG_LENGTH);
	recvMessage(socketPlayer2, buffer, MAX_MSG_LENGTH - 1);
	printf("Nombre de jugador de jugador 2: %s\n", buffer);
	strncpy(session.player2Name, buffer, STRING_LENGTH - 1);

	initSession(&session);
	printSession(&session);
    send(socketPlayer1, &session.player1Stack, sizeof(session.player1Stack), 0);
    send(socketPlayer2, &session.player2Stack, sizeof(session.player2Stack), 0);
    sendCode(socketPlayer1, TURN_BET);
    sendCode(socketPlayer2, TURN_BET);


	tPlayer jugadorActivo = player1;
		int *socketActivo = &socketPlayer1;
		int *socketPasivo = &socketPlayer2;
	while (partidaFinalizada == 0) {

		
        printf("Stacks iniciales: %s: %d, %s: %d\n", session.player1Name,session.player1Stack, session.player2Name,session.player2Stack);

		gestionarApuesta(*socketActivo, session.player1Stack, &session.player1Bet);
		gestionarApuesta(*socketPasivo, session.player2Stack, &session.player2Bet);
      
		if (session.player1Bet > session.player2Bet) {
            session.player1Bet = session.player2Bet;

		} else if (session.player2Bet > session.player1Bet) {
			session.player2Bet = session.player1Bet;
		}

		enviarEstadoJugador(*socketActivo, &session.player1Deck, TURN_PLAY);

		while (apuestaJugadores < 2) {

			recv(*socketActivo, &code, sizeof(code), 0);

			tDeck *deckActivo = (jugadorActivo == player1) ? &session.player1Deck : &session.player2Deck;
			tDeck *deckPasivo = (jugadorActivo == player1) ? &session.player2Deck : &session.player1Deck;

			if ((code == TURN_PLAY_HIT)) {
				deckActivo->cards[deckActivo->numCards] = getRandomCard( &session.gameDeck);
				deckActivo->numCards++;
				printSession(&session);

				if (calculatePoints(deckActivo) > GOAL_GAME) {
					if (apuestaJugadores == 1) {
						enviarEstadoJugador(*socketActivo, deckActivo, TURN_PLAY_WAIT);
						enviarEstadoJugador(*socketPasivo, deckPasivo, TURN_PLAY_WAIT);
					} else {
						enviarEstadoJugador(*socketActivo, deckActivo, TURN_PLAY_OUT);
						enviarEstadoJugador(*socketPasivo, deckActivo, TURN_PLAY);
					}
					printf("El jugador ha superado los puntos permitidos \n");

					// Cambiar jugador activo
					jugadorActivo = getNextPlayer(jugadorActivo);
					int *tmp = socketActivo;
					socketActivo = socketPasivo;
					socketPasivo = tmp;
					++apuestaJugadores;
				} else {
					enviarEstadoJugador(*socketActivo, deckActivo, TURN_PLAY);
					enviarEstadoJugador(*socketPasivo, deckActivo, TURN_PLAY_WAIT);
				}

			} else if ((code == TURN_PLAY_STAND)) {
				if (apuestaJugadores == 1) {
					enviarEstadoJugador(*socketActivo, deckActivo, TURN_PLAY_WAIT);
					enviarEstadoJugador(*socketPasivo, deckPasivo, TURN_PLAY_WAIT);
				} else {
					enviarEstadoJugador(*socketActivo, deckActivo, TURN_PLAY_WAIT);
					enviarEstadoJugador(*socketPasivo, deckPasivo, TURN_PLAY);
				}
				printf("El jugador se planta.\n");

				// Cambiar jugador activo
				jugadorActivo = getNextPlayer(jugadorActivo);
				int *tmp = socketActivo;
				socketActivo = socketPasivo;
				socketPasivo = tmp;
				++apuestaJugadores;
			}

		}
		printf("Fin de la ronda. Calculando resultados...\n");

		unsigned int puntosJugador1 = calculatePoints(&session.player1Deck);
		unsigned int puntosJugador2 = calculatePoints(&session.player2Deck);

		printf("Puntos %s: %u\n", session.player1Name, puntosJugador1);
		printf("Puntos %s: %u\n", session.player2Name, puntosJugador2);

		if ((puntosJugador1 > GOAL_GAME) && (puntosJugador2 > GOAL_GAME)) {
			printf( "Ambos jugadores han superado los puntos permitidos. Empate.\n");
			enviarEstadoJugador(socketPlayer1, &session.player1Deck, TURN_GAME_WIN);
			enviarEstadoJugador(socketPlayer2, &session.player2Deck, TURN_GAME_WIN);
		} else if ((puntosJugador1 <= GOAL_GAME) && ((puntosJugador1 > puntosJugador2) || (puntosJugador2 > GOAL_GAME))) {

			enviarEstadoJugador(socketPlayer1, &session.player1Deck, TURN_GAME_WIN);
			enviarEstadoJugador(socketPlayer2, &session.player2Deck, TURN_GAME_LOSE);

			session.player1Stack += session.player1Bet;
			session.player2Stack -= session.player2Bet;
			jugadorActivo = getNextPlayer(jugadorActivo);

		} 
        else if ((puntosJugador2 <= GOAL_GAME)&& ((puntosJugador2 > puntosJugador1)|| (puntosJugador1 > GOAL_GAME))) {
			printf("%s gana la partida!\n", session.player2Name);

			enviarEstadoJugador(socketPlayer2, &session.player1Deck, TURN_GAME_WIN);
			enviarEstadoJugador(socketPlayer1, &session.player2Deck, TURN_GAME_LOSE);

			session.player2Stack += session.player2Bet;
			session.player1Stack -= session.player1Bet;
			jugadorActivo = getNextPlayer(jugadorActivo);
		}else if( puntosJugador1 == puntosJugador2){
			printf("Empate!\n");
			enviarEstadoJugador(socketPlayer1, &session.player1Deck, TURN_GAME_WIN);
			enviarEstadoJugador(socketPlayer2, &session.player2Deck, TURN_GAME_WIN);
		}

        apuestaJugadores = 0;
        session.player1Bet = 0;
        session.player2Bet = 0;
        clearDeck(&session.player1Deck);
        clearDeck(&session.player2Deck);
		printf("Stacks finales: %s: %d, %s: %d\n", session.player1Name,session.player1Stack, session.player2Name,session.player2Stack);
        if (session.player1Stack == 0 || session.player2Stack == 0) {
            partidaFinalizada = 1;
             send(socketPlayer1, &session.player1Stack, sizeof(session.player1Stack), 0);
            send(socketPlayer2, &session.player2Stack, sizeof(session.player2Stack), 0);
            sendCode(socketPlayer1, TURN_PLAY_OUT);
            sendCode(socketPlayer2, TURN_PLAY_OUT);
            printf("Un jugador se ha quedado sin fichas. Fin de la partida.\n");
        } else {
            initDeck(&session.gameDeck);
            send(socketPlayer1, &session.player1Stack, sizeof(session.player1Stack), 0);
            send(socketPlayer2, &session.player2Stack, sizeof(session.player2Stack), 0);
            sendCode(socketPlayer1, TURN_BET);
            sendCode(socketPlayer2, TURN_BET);
			//
			jugadorActivo = getNextPlayer(jugadorActivo);
        }
        
	}

	close(socketPlayer1);
	close(socketPlayer2);
	free(threadArgs);

	printf("Partida finalizada\n");
	return NULL;
}

int main(int argc, char *argv[]) {
	int socketfd;
	struct sockaddr_in serverAddress, player1Address;
	unsigned int port, clientLength;
	int socketPlayer1 = -1, socketPlayer2;
	tThreadArgs *threadArgs;
	pthread_t threadID;

	if (argc != 2) {
		fprintf(stderr, "Uso: %s <puerto>\n", argv[0]);
		exit(1);
	}
	socketfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (socketfd < 0)
		showError("Error creando socket");

	memset(&serverAddress, 0, sizeof(serverAddress));
	port = atoi(argv[1]);
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddress.sin_port = htons(port);

	if (bind(socketfd, (struct sockaddr *) &serverAddress,sizeof(serverAddress)) < 0)
		showError("Error en bind");

	listen(socketfd, 10);
	printf("Servidor escuchando en puerto %d...\n", port);

	clientLength = sizeof(player1Address);

	while (1) {
		int newSocket = accept(socketfd, (struct sockaddr *) &player1Address,&clientLength);
		if (newSocket < 0)
			showError("Error en accept");

		printf("Jugador conectado (socket %d)\n", newSocket);

		if (socketPlayer1 == -1) {
			socketPlayer1 = newSocket;

		} else {
			socketPlayer2 = newSocket;

			threadArgs = malloc(sizeof(tThreadArgs));
			threadArgs->socketPlayer1 = socketPlayer1;
			threadArgs->socketPlayer2 = socketPlayer2;

			pthread_create(&threadID, NULL, handleGame, threadArgs);
			pthread_detach(threadID);

			// Reiniciar "jugador en espera"
			socketPlayer1 = -1;
		}
	}

	close(socketfd);
	return 0;
}
