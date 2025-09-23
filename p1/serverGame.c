#include "serverGame.h"
#include <pthread.h>
#include <stdbool.h>

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

int gestionarApuesta(int socketJugador, unsigned int stackJugador, unsigned int *betPtr) {
    sendCode(socketJugador, TURN_BET);
    sendCode(socketJugador, stackJugador);

    unsigned int bet;
    bool isValid = FALSE;
    while (!isValid) {
        int rec = recv(socketJugador, &bet, sizeof(bet), 0);
        if (rec <= 0) return -1; 

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


int gameStart() {
    while (1)
    {
        /* TODO */
    }
    
    return 0;
}
void *handleGame(void *args) {
    tThreadArgs *threadArgs = (tThreadArgs *) args;
    int socketPlayer1 = threadArgs->socketPlayer1;
    int socketPlayer2 = threadArgs->socketPlayer2;


    tSession session;

    printf("Nueva partida creada entre sockets %d y %d\n", socketPlayer1, socketPlayer2);

    char buffer[MAX_MSG_LENGTH];
     memset(buffer, 0, MAX_MSG_LENGTH);
     recvMessage(socketPlayer1, buffer, MAX_MSG_LENGTH-1);
     printf("Nombre de jugador de jugador 1: %s\n", buffer); 
     strncpy(session.player1Name, buffer, STRING_LENGTH-1);

     memset(buffer, 0, MAX_MSG_LENGTH);
     recvMessage(socketPlayer2, buffer, MAX_MSG_LENGTH-1);
     printf("Nombre de jugador de jugador 2: %s\n", buffer); 
     strncpy(session.player2Name, buffer, STRING_LENGTH-1);
     initSession(&session);
     printSession(&session);

    while (1) {
   
        gestionarApuesta(socketPlayer1, session.player1Stack, &session.player1Bet);
        gestionarApuesta(socketPlayer2, session.player1Stack, &session.player2Bet);
        gameStart();
 
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
