#include "client.h"

unsigned int readBet()
{

	int isValid, bet = 0;
	xsd__string enteredMove;

	// While player does not enter a correct bet...
	do
	{
		// Init...
		enteredMove = (xsd__string)malloc(STRING_LENGTH);
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
			printf("Entered value is not correct. It must be a number greater than 0\n");
		else
			bet = atoi(enteredMove);

	} while (!isValid);

	printf("\n");
	free(enteredMove);

	return ((unsigned int)bet);
}

unsigned int readOption()
{

	unsigned int bet;

	do
	{

		printf("What is your move? Press %d to hit a card and %d to stand\n", PLAYER_HIT_CARD, PLAYER_STAND);
		bet = readBet();
		if ((bet != PLAYER_HIT_CARD) && (bet != PLAYER_STAND))
			printf("Wrong option!\n");
	} while ((bet != PLAYER_HIT_CARD) && (bet != PLAYER_STAND));

	return bet;
}

int main(int argc, char **argv)
{

	struct soap soap;				  /** Soap struct */
	char *serverURL;				  /** Server URL */
	blackJackns__tMessage playerName; /** Player name */

	blackJackns__tBlock gameStatus;

	unsigned int playerMove; /** Player's move */
	int resCode, gameId;	 /** Result and gameId */
	int endGame = 0;
	// Init gSOAP environment
	soap_init(&soap);

	// Obtain server address
	serverURL = argv[1];

	// Allocate memory
	allocClearMessage(&soap, &(playerName));
	allocClearBlock(&soap, &gameStatus);

	gameStatus.code = 0;

	// Check arguments
	if (argc != 2)
	{
		printf("Usage: %s http://server:port\n", argv[0]);
		exit(0);
	}
	
	gameId = ERROR_NAME_REPEATED;

	while(gameId == ERROR_NAME_REPEATED){
		printf("Enter your name:\n ");
		fgets(playerName.msg, STRING_LENGTH - 1, stdin);
		playerName.msg[strlen(playerName.msg) - 1] = 0;

		soap_call_blackJackns__register(&soap, serverURL, "", playerName, &gameId);
		printf("[DEBUG] soap_call result: %d, gameId(returned): %d\n", resCode, gameId);
		if(gameId == ERROR_NAME_REPEATED){
			printf("[Register] Player name already taken. Please choose another name\n");
		}
		else if(gameId == ERROR_SERVER_FULL){
			printf("[Register] No available games\n");
			endGame = 1;
		}

	}

	

	while (!endGame)
	{
		soap_call_blackJackns__getStatus(&soap, serverURL, "", playerName, gameId, &gameStatus);
		printStatus(&gameStatus, DEBUG_CLIENT);
		while (gameStatus.code == TURN_PLAY)
		{

			playerMove = readOption();
			soap_call_blackJackns__playerMove(&soap, serverURL, "", playerName, gameId, playerMove, &gameStatus);

			printStatus(&gameStatus, DEBUG_CLIENT);
		}

		switch (gameStatus.code)
		{
		case TURN_WAIT:
			printf("\n");
			printf("Waiting for your turn...\n");
			//printStatus(&gameStatus, DEBUG_CLIENT);
			break;
		case GAME_LOSE:
			printf("\n");
			printf("You lose the hand!\n");
			endGame = 1;
			break;
		case GAME_WIN:
			printf("\n");
			printf("You win the hand\n");
			endGame = 1;
			break;
		default:			
			break;
		}
	}
	/*
		Mientras (no acabe el juego)
			getStatus (nombreJugador, idPartida, …);
			int blackJackns__getStatus(blackJackns__tMessage playerName, int gameId, blackJackns__tBlock **status);
			Imprimir estado del juego
			Mientras (jugador tiene el turno)
				jugada = Leer opción del jugador
				playerMove (nombreJugador,idPartida, jugada, …)
				Imprimir estado del juego
		*/
	if (soap.error)
	{
		soap_print_fault(&soap, stderr);
		printf("Error code: %d\n", soap.error);
	}

	// Clean the environment
	soap_destroy(&soap);
	soap_end(&soap);
	soap_done(&soap);

	return 0;
}
