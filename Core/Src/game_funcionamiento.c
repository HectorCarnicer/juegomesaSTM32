#include "game_funcionamiento.h"
#include <stdlib.h> // Para el randomizer

#define TOLERANCE 1
#define MAX_VAL   9

static void GenerateCode(Game_Handle_t *game) {
	for(int i=0; i<3; i++) {
		game->secretCode[i] = rand() % 10; // 0-9
	}
  
}

void Game_Init(Game_Handle_t *game) {
	game->currentState = STATE_INIT;
	game->currentDigitIndex = 0;
	game->playerInput = 0;
}

void Game_Update(Game_Handle_t *game, uint32_t adcValue) { //maestro de juego

	game->playerInput = (adcValue * 10) / 4096; // Convertidor potenciometro
	if(game->playerInput > 9) game->playerInput = 9;

	// Maquina de estados
	switch (game->currentState) {
	case STATE_INIT:
		GenerateCode(game);
		game->currentState = STATE_IDLE;
		break;

	case STATE_IDLE:
		//game->currentState = STATE_PLAYING;
		// Presionar botón para iniciar
		break;

	case STATE_PLAYING:
		// aqui el jugador esta girando el potenciomtro
		// la logica seria en el main
		break;

	case STATE_CHECK_INPUT:
		// Pulso boton (sera el Game_HandleButton)
		if (abs(game->playerInput - game->secretCode[game->currentDigitIndex]) <= TOLERANCE) {
			// Correcto
			game->currentDigitIndex++;
			if (game->currentDigitIndex >= 3) {
				game->currentState = STATE_WIN;
			} else {
				game->currentState = STATE_PLAYING; // Siguiente numero
			}
		} else {
			// Fallo
			game->currentState = STATE_LOSE;
		}
		break;

	case STATE_WIN:
		// Espera reset
		break;

	case STATE_LOSE:
		// Espera reset
		break;
	}
}

void Game_HandleButton(Game_Handle_t *game) {
	if (game->currentState == STATE_WIN || game->currentState == STATE_LOSE) {
		// Reiniciar
		Game_Init(game);
	} else if (game->currentState == STATE_PLAYING) {
		// Jugar
		game->currentState = STATE_CHECK_INPUT;
	} else if (game->currentState == STATE_IDLE) {
		// Jugar
		game->currentState = STATE_PLAYING;
	}
}
