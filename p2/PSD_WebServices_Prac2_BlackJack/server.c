#include "server.h"
//#include "blackJack.h"

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

	// Bet and stack
	game->player1Bet = 0;
	game->player2Bet = 0;
	game->player1Stack = INITIAL_STACK;
	game->player2Stack = INITIAL_STACK;

	// Game status variables
	game->endOfGame = FALSE;
	game->status = gameEmpty;


	// Mutex and condition 

	pthread_mutex_init(&game->mutex, NULL);
    pthread_cond_init(&game->turnCond, NULL);
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

int blackJackns__register(struct soap *soap, blackJackns__tMessage playerName, int *result)
{

	int gameIndex,code;

	// Set \0 at the end of the string
	playerName.msg[playerName.__size] = 0;

	if (DEBUG_SERVER)
		printf("[Register] Registering new player -> [%s]\n", playerName.msg);

	int i = 0;
	int found = FALSE;
	while (i < MAX_GAMES && !found)
	{
		if (games[i].status == gameEmpty)
		{
			initGame(&(games[i]));
			strcpy(games[i].player1Name, playerName.msg);
			games[i].status = gameWaitingPlayer;
		
			
			*result = i;
			found = TRUE;
			code = SOAP_OK;
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
				*result = i;
				code = SOAP_OK;
			
			}
			found = TRUE;
		}
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
int blackJackns__getStatus(struct soap *soap, blackJackns__tMessage playerName, int gameId, blackJackns__tBlock **status)
{ 	

	*status = (blackJackns__tBlock *)soap_malloc(soap, sizeof(blackJackns__tBlock));
    memset(*status, 0, sizeof(blackJackns__tBlock));

    
    (*status)->msgStruct.msg = (xsd__string)soap_malloc(soap, STRING_LENGTH);
    (*status)->deck.cards = (unsigned int *)soap_malloc(soap, DECK_SIZE * sizeof(unsigned int));

	tGame *game = &games[gameId];
	tPlayer thisPlayer;
    if (strcmp(playerName.msg, game->player1Name) == 0)
        thisPlayer = player1;
    else
        thisPlayer = player2;

	pthread_mutex_lock(&games[gameId].mutex);
    while (game->currentPlayer != thisPlayer)
    {
        printf("[getStatus] %s espera su turno...\n", playerName.msg);
        pthread_cond_wait(&game->turnCond, &game->mutex);
    }


	//Si le toca jugar
	printf("aaaaaaaaaaa \n");
	copyGameStatusStructure(*status, "Tu turno. Elige una acción: pedir carta o plantarte.", 
        (thisPlayer == player1 ? &game->player1Deck : &game->player2Deck), TURN_PLAY);
	printf("[DEBUG] Código asignado: %d\n", (*status)->code);  
	pthread_mutex_unlock(&games[gameId].mutex);



	return SOAP_OK;
};

int blackJackns__playerMove(struct soap *soap, blackJackns__tMessage playerName, int gameId, int action, blackJackns__tBlock **status)
{
	//TODO
	printf("playerMove\n");
	return SOAP_OK;
};
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