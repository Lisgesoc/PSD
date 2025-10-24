#include "server.h"
// #include "blackJack.h"

/** Shared array that contains all the games. */
tGame games[MAX_GAMES];

void initGame(tGame *game)
{

	// Init players' name
	memset(game->player1Name, 0, STRING_LENGTH);
	memset(game->player2Name, 0, STRING_LENGTH);

	// Alloc memory for the decks
	clearDeck(&(game->player1Deck));
	clearDeck(&(game->player2Deck));
	initDeck(&(game->gameDeck));


	// GameCounter
	game->handCount = 0;
	// Bet and stack
	game->player1Bet = 0;
	game->player2Bet = 0;
	game->player1Stack = INITIAL_STACK;
	game->player2Stack = INITIAL_STACK;

	// Game status variables
	game->endOfGame = FALSE;
	game->status = gameEmpty;

	// Mutex and condition

	pthread_mutex_init(&game->mutex_status, NULL);
	pthread_cond_init(&game->turnCond, NULL);
	pthread_mutex_init(&game->mutex_register, NULL);
	pthread_cond_init(&game->startGameCond, NULL);
}

void initServerStructures(struct soap *soap)
{

	if (DEBUG_SERVER)
		printf("Initializing structures...\n");

	// Init seed
	srand(time(NULL));

	// Init each game (alloc memory and init)
	for (int i = 0; i < MAX_GAMES; i++)
	{
		games[i].player1Name = (xsd__string)soap_malloc(soap, STRING_LENGTH);
		games[i].player2Name = (xsd__string)soap_malloc(soap, STRING_LENGTH);
		allocDeck(soap, &(games[i].player1Deck));
		allocDeck(soap, &(games[i].player2Deck));
		allocDeck(soap, &(games[i].gameDeck));
		initGame(&(games[i]));
	}
}

void initDeck(blackJackns__tDeck *deck)
{

	deck->__size = DECK_SIZE;

	for (int i = 0; i < DECK_SIZE; i++)
		deck->cards[i] = i;
}

void clearDeck(blackJackns__tDeck *deck)
{

	// Set number of cards
	deck->__size = 0;

	for (int i = 0; i < DECK_SIZE; i++)
		deck->cards[i] = UNSET_CARD;
}

tPlayer calculateNextPlayer(tPlayer currentPlayer)
{
	return ((currentPlayer == player1) ? player2 : player1);
}

unsigned int getRandomCard(blackJackns__tDeck *deck)
{

	unsigned int card, cardIndex, i;

	// Get a random card
	cardIndex = rand() % deck->__size;
	card = deck->cards[cardIndex];

	// Remove the gap
	for (i = cardIndex; i < deck->__size - 1; i++)
		deck->cards[i] = deck->cards[i + 1];

	// Update the number of cards in the deck
	deck->__size--;
	deck->cards[deck->__size] = UNSET_CARD;

	return card;
}

unsigned int calculatePoints(blackJackns__tDeck *deck)
{

	unsigned int points = 0;

	for (int i = 0; i < deck->__size; i++)
	{

		if (deck->cards[i] % SUIT_SIZE < 9)
			points += (deck->cards[i] % SUIT_SIZE) + 1;
		else
			points += FIGURE_VALUE;
	}

	return points;
}

void copyGameStatusStructure(blackJackns__tBlock *status, char *message, blackJackns__tDeck *newDeck, int newCode)
{

	// Copy the message
	memset((status->msgStruct).msg, 0, STRING_LENGTH);
	strcpy((status->msgStruct).msg, message);
	(status->msgStruct).__size = strlen((status->msgStruct).msg);

	// Copy the deck, only if it is not NULL
	if (newDeck->__size > 0)
		memcpy((status->deck).cards, newDeck->cards, DECK_SIZE * sizeof(unsigned int));
	else
		(status->deck).cards = NULL;

	(status->deck).__size = newDeck->__size;

	// Set the new code
	status->code = newCode;
}
void dealInitialCards(tGame *game) {

    for(int i = 0; i < 2; i++) {
        game->player1Deck.cards[i] = getRandomCard(&game->gameDeck);
        game->player2Deck.cards[i] = getRandomCard(&game->gameDeck);
    }
    game->player1Deck.__size = 2;
    game->player2Deck.__size = 2;
}

int blackJackns__register(struct soap *soap, blackJackns__tMessage playerName, int *result)
{

	int gameIndex, code;

	// Set \0 at the end of the string
	playerName.msg[playerName.__size] = 0;

	if (DEBUG_SERVER)
		printf("[Register] Registering new player -> [%s]\n", playerName.msg);

	int i = 0;
	int found = FALSE;
	while (i < MAX_GAMES && !found)
	{
		pthread_mutex_lock(&games[i].mutex_register);
		if (games[i].status == gameEmpty)
		{
			initGame(&(games[i]));
			strcpy(games[i].player1Name, playerName.msg);
			games[i].status = gameWaitingPlayer;

			*result = i;
			found = TRUE;
			code = SOAP_OK;
			pthread_cond_wait(&games[i].startGameCond, &games[i].mutex_register);
		}
		else if (games[i].status == gameWaitingPlayer)
		{
			printf("Debug - Game[%d] status: %d\n", i, games[i].status);
			printf("Comparing names: '%s' with '%s'\n", games[i].player1Name, playerName.msg);
			if (strcmp(games[i].player1Name, playerName.msg) == 0)
			{
				printf("[Register] Player name already taken in game %d\n", i);
				code = ERROR_NAME_REPEATED;
			}
			else
			{
				strcpy(games[i].player2Name, playerName.msg);
				games[i].status = gameReady;
				games[i].currentPlayer = player1;
				games[i].player1Bet = DEFAULT_BET;
				games[i].player2Bet = DEFAULT_BET;
				dealInitialCards(&games[i]);
				*result = i;
				code = SOAP_OK;
				pthread_cond_signal(&games[i].startGameCond);
			}
			found = TRUE;
		}
		pthread_mutex_unlock(&games[i].mutex_register);

		++i;
		printf("Numero de sala: %d\n", i);
	}

	if (i == MAX_GAMES && !found)
	{

		code = ERROR_SERVER_FULL;
		if (DEBUG_SERVER)
			printf("[Register] No available games\n");
	}
	return code;
}
/*
	getStatus debe devolver al jugador el estado actual de su partida.
	Si no le toca → lo deja bloqueado esperando su turno.
	Si le toca → le informa que puede jugar.
	Si el juego terminó → devuelve GAME_WIN o GAME_LOSE.
*/
int blackJackns__getStatus(struct soap *soap, blackJackns__tMessage playerName, int gameId, blackJackns__tBlock *status)
{

	allocClearBlock(soap, status);

	tGame *game = &games[gameId];

	tPlayer player;

	if (strcmp(playerName.msg, game->player1Name) == 0)
		player = player1;
	else
		player = player2;

	blackJackns__tDeck *playerDeck = (player == player1) ? &game->player1Deck : &game->player2Deck;

	pthread_mutex_lock(&games[gameId].mutex_status);
	while (game->currentPlayer != player)
	{
		printf("[getStatus] %s espera su turno...\n", playerName.msg);
		pthread_cond_wait(&game->turnCond, &game->mutex_status);
		if (game->endOfGame)
		{
			if ((player == player1 && game->player1Stack == 0) || (player == player2 && game->player2Stack == 0))
				copyGameStatusStructure(status, "You have run out of stack. Game over.", playerDeck, GAME_LOSE);
			else
				copyGameStatusStructure(status, "Your rival has run out of stack. You win!", playerDeck, GAME_WIN);
			pthread_mutex_unlock(&games[gameId].mutex_status);
			return SOAP_OK;
		}
	}

	// Si le toca jugar
	copyGameStatusStructure(status, "Tu turno. Elige una acción: pedir carta o plantarte.", playerDeck, TURN_PLAY);

	printf("[DEBUG] Código asignado: %d\n", status->code);
	pthread_mutex_unlock(&games[gameId].mutex_status);

	return SOAP_OK;
};

int blackJackns__playerMove(struct soap *soap, blackJackns__tMessage playerName, int gameId, int action, blackJackns__tBlock *status)
{

	allocClearBlock(soap, status);
	tGame *game = &games[gameId];

	tPlayer player = game->currentPlayer;

	blackJackns__tDeck *playerDeck = (player == player1) ? &game->player1Deck : &game->player2Deck;

	printf("playerMove\n");
	if (action == PLAYER_HIT_CARD)
	{
		playerDeck->cards[playerDeck->__size] = getRandomCard(&(game->gameDeck));
		playerDeck->__size++;
		status->deck = *playerDeck;
		int points = calculatePoints(playerDeck);
		if (points <= 21)
		{
			printf("Player %d has %d points.\n", player + 1, points);
			copyGameStatusStructure(status, "Tu turno. Elige una acción: pedir carta o plantarte.", playerDeck, TURN_PLAY);
		}
		else
		{
			printf("Player %d has busted!\n", player + 1);
			game->currentPlayer = calculateNextPlayer(player);
			copyGameStatusStructure(status, "Has superado los 21 puntos. Has perdido la partida.", playerDeck, GAME_LOSE);
			game->handCount++;
			game->endOfGame = TRUE;
			pthread_cond_signal(&game->turnCond);
		}
	}
	else if (action == PLAYER_STAND)
	{
		game->currentPlayer = calculateNextPlayer(player);
		copyGameStatusStructure(status, "Has decidido plantarte. Espera tu próximo turno.", playerDeck, TURN_WAIT);
		game->handCount++;
		pthread_cond_signal(&game->turnCond); // Despertar al otro jugador
	}
	else
	{

		printf("Invalid option\n");
	}

	if (game->handCount == 2)
	{
 		handleEndOfHand(game, status, player, playerDeck);	
	}

	return SOAP_OK;
};

void handleEndOfHand(tGame *game, blackJackns__tBlock *status, tPlayer player, blackJackns__tDeck *playerDeck) {
    int pointsPlayer1 = calculatePoints(&game->player1Deck);
    int pointsPlayer2 = calculatePoints(&game->player2Deck);

    // Determinar ganador
    if (pointsPlayer1 > 21 && pointsPlayer2 > 21) {
        printf("Both players busted! It's a tie.\n");
    } else if (pointsPlayer1 <= 21 && pointsPlayer2 > 21) {
        updatePlayerStack(game, 1);
        printf("Player 1 wins the game!\n");
    } else if (pointsPlayer2 <= 21 && pointsPlayer1 > 21) {
        updatePlayerStack(game, 2);
        printf("Player 2 wins the game!\n");
    } else if (pointsPlayer1 <= 21 && pointsPlayer2 <= 21) {
        if (pointsPlayer1 > pointsPlayer2) {
            updatePlayerStack(game, 1);
            printf("Player 1 wins the game!\n");
        } else if (pointsPlayer2 > pointsPlayer1) {
            updatePlayerStack(game, 2);
            printf("Player 2 wins the game!\n");
        } else {
            printf("It's a tie between both players!\n");
        }
    }

    // Actualizar estado según stack
    if (game->player1Stack == 0 && player == player1)
        copyGameStatusStructure(status, "Player 1 has run out of stack. Game over.", playerDeck, GAME_LOSE);
    else if (game->player2Stack == 0 && player == player2)
        copyGameStatusStructure(status, "Player 2 has run out of stack. Game over.", playerDeck, GAME_LOSE);
    else if (game->player1Stack == 0 && player == player2)
        copyGameStatusStructure(status, "Player 1 has run out of stack. You win!", playerDeck, GAME_WIN);
    else if (game->player2Stack == 0 && player == player1)
        copyGameStatusStructure(status, "Player 2 has run out of stack. You win!", playerDeck, GAME_WIN);

    printf("Stacks after hand %d: Player 1: %u, Player 2: %u\n", 
           game->handCount, game->player1Stack, game->player2Stack);

    game->endOfGame = TRUE;
    game->handCount = 0;
}

void updatePlayerStack(tGame *game, int winner)
{
	if (winner == 1)
	{
		game->player1Stack += game->player2Bet;
		game->player2Stack -= game->player2Bet;
	}
	else if (winner == 2)
	{
		game->player2Stack += game->player1Bet;
		game->player1Stack -= game->player1Bet;
	}
}
void *processRequest(void *soap)
{

	pthread_detach(pthread_self());

	printf("Processing a new request... \n");

	soap_serve((struct soap *)soap);
	soap_destroy((struct soap *)soap);
	soap_end((struct soap *)soap);
	soap_done((struct soap *)soap);
	free(soap);

	return NULL;
}

int main(int argc, char **argv)
{

	struct soap soap;
	struct soap *tsoap;
	pthread_t tid;
	int port;
	SOAP_SOCKET m, s;

	// Check arguments
	if (argc != 2)
	{
		printf("Usage: %s port\n", argv[0]);
		exit(0);
	}
	soap_init(&soap);
	initServerStructures(&soap);
	// Configure timeouts
	soap.send_timeout = 60;		// 60 seconds
	soap.recv_timeout = 60;		// 60 seconds
	soap.accept_timeout = 3600; // server stops after 1 hour of inactivity
	soap.max_keep_alive = 100;	// max keep-alive sequence

	// Get listening port
	port = atoi(argv[1]);

	// Bind
	m = soap_bind(&soap, NULL, port, 100);

	if (!soap_valid_socket(m))
	{
		exit(1);
	}

	printf("Server is ON!\n");

	while (TRUE)
	{

		// Accept a new connection
		s = soap_accept(&soap);

		// Socket is not valid :(
		if (!soap_valid_socket(s))
		{

			if (soap.errnum)
			{
				soap_print_fault(&soap, stderr);
				exit(1);
			}

			fprintf(stderr, "Time out!\n");
			break;
		}

		// Copy the SOAP environment
		tsoap = soap_copy(&soap);

		if (!tsoap)
		{
			printf("SOAP copy error!\n");
			break;
		}

		// Create a new thread to process the request
		pthread_create(&tid, NULL, (void *(*)(void *))processRequest, (void *)tsoap);
	}

	// Detach SOAP environment
	soap_done(&soap);

	return 0;
}