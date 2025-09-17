#include "serverGame.h"
#include <pthread.h>

tPlayer getNextPlayer (tPlayer currentPlayer){

	tPlayer next;

		if (currentPlayer == player1)
			next = player2;
		else
			next = player1;

	return next;
}

void initDeck (tDeck *deck){

	deck->numCards = DECK_SIZE; 

	for (int i=0; i<DECK_SIZE; i++){
		deck->cards[i] = i;
	}
}

void clearDeck (tDeck *deck){

	// Set number of cards
	deck->numCards = 0;

	for (int i=0; i<DECK_SIZE; i++){
		deck->cards[i] = UNSET_CARD;
	}
}

void printSession (tSession *session){

		printf ("\n ------ Session state ------\n");

		// Player 1
		printf ("%s [bet:%d; %d chips] Deck:", session->player1Name, session->player1Bet, session->player1Stack);
		printDeck (&(session->player1Deck));

		// Player 2
		printf ("%s [bet:%d; %d chips] Deck:", session->player2Name, session->player2Bet, session->player2Stack);
		printDeck (&(session->player2Deck));

		// Current game deck
		if (DEBUG_PRINT_GAMEDECK){
			printf ("Game deck: ");
			printDeck (&(session->gameDeck));
		}
}

void initSession (tSession *session){

	clearDeck (&(session->player1Deck));
	session->player1Bet = 0;
	session->player1Stack = INITIAL_STACK;

	clearDeck (&(session->player2Deck));
	session->player2Bet = 0;
	session->player2Stack = INITIAL_STACK;

	initDeck (&(session->gameDeck));
}

unsigned int calculatePoints (tDeck *deck){

	unsigned int points;

		// Init...
		points = 0;

		for (int i=0; i<deck->numCards; i++){

			if (deck->cards[i] % SUIT_SIZE < 9)
				points += (deck->cards[i] % SUIT_SIZE) + 1;
			else
				points += FIGURE_VALUE;
		}

	return points;
}

unsigned int getRandomCard (tDeck* deck){

	unsigned int card, cardIndex, i;

		// Get a random card
		cardIndex = rand() % deck->numCards;
		card = deck->cards[cardIndex];

		// Remove the gap
		for (i=cardIndex; i<deck->numCards-1; i++)
			deck->cards[i] = deck->cards[i+1];

		// Update the number of cards in the deck
		deck->numCards--;
		deck->cards[deck->numCards] = UNSET_CARD;

	return card;
}

/** Función que maneja una partida entre 2 jugadores */
void *handleGame(void *args) {
    tThreadArgs *threadArgs = (tThreadArgs *) args;
    int socketPlayer1 = threadArgs->socketPlayer1;
    int socketPlayer2 = threadArgs->socketPlayer2;

    // Crear sesión
    tSession session;
    initSession(&session);

    printf("Nueva partida creada entre sockets %d y %d\n", socketPlayer1, socketPlayer2);

	//Logica del juego
    send(socketPlayer1, "Bienvenido jugador 1\n", 22, 0);
    send(socketPlayer2, "Bienvenido jugador 2\n", 22, 0);

    // Bucle de juego (simplificado)
    char buffer[MAX_MSG_LENGTH];
    while (1) {
        memset(buffer, 0, MAX_MSG_LENGTH);
        int len = recv(socketPlayer1, buffer, MAX_MSG_LENGTH-1, 0);
        if (len <= 0 || strcmp(buffer,"exit")==0) break;
        send(socketPlayer2, buffer, strlen(buffer), 0);

        memset(buffer, 0, MAX_MSG_LENGTH);
        len = recv(socketPlayer2, buffer, MAX_MSG_LENGTH-1, 0);
        if (len <= 0 || strcmp(buffer,"exit")==0) break;
        send(socketPlayer1, buffer, strlen(buffer), 0);
    }

    close(socketPlayer1);
    close(socketPlayer2);
    free(threadArgs);

    printf("Partida finalizada\n");
    return NULL;
}

int main(int argc, char *argv[]) {
    int socketfd;
    struct sockaddr_in serverAddress, player1Address, player2Address;
    unsigned int port, clientLength;
    int socketPlayer1 = -1, socketPlayer2;
    tThreadArgs *threadArgs;
    pthread_t threadID;

    if (argc != 2) {
        fprintf(stderr, "Uso: %s <puerto>\n", argv[0]);
        exit(1);
    }

    // Crear socket
    socketfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socketfd < 0) showError("Error creando socket");

    memset(&serverAddress, 0, sizeof(serverAddress));
    port = atoi(argv[1]);
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddress.sin_port = htons(port);

    if (bind(socketfd, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0)
        showError("Error en bind");

    listen(socketfd, 10);
    printf("Servidor escuchando en puerto %d...\n", port);

    clientLength = sizeof(player1Address);

    while (1) {
        int newSocket = accept(socketfd, (struct sockaddr *) &player1Address, &clientLength);
        if (newSocket < 0) showError("Error en accept");

        printf("Jugador conectado (socket %d)\n", newSocket);

        if (socketPlayer1 == -1) {
            socketPlayer1 = newSocket;
            send(socketPlayer1, "Esperando otro jugador...\n", 27, 0);
        } else {
            socketPlayer2 = newSocket;

            // Crear args para el hilo
            threadArgs = malloc(sizeof(tThreadArgs));
            threadArgs->socketPlayer1 = socketPlayer1;
            threadArgs->socketPlayer2 = socketPlayer2;

            // Lanzar hilo para la partida
            pthread_create(&threadID, NULL, handleGame, threadArgs);
            pthread_detach(threadID);

            // Reiniciar "jugador en espera"
            socketPlayer1 = -1;
        }
    }

    close(socketfd);
    return 0;
}
