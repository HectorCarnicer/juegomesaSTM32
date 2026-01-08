#include "game_funcionamiento.h"
#include <stdlib.h> // para el randomizer de despues

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
    
    game->playerInput = (adcValue * 10) / 4096; //convertidor potenciometro
    if(game->playerInput > 9) game->playerInput = 9;

    // maquina de estados
    switch (game->currentState) {
        case STATE_INIT:
            GenerateCode(game);
            game->currentState = STATE_IDLE;
            break;

        case STATE_IDLE:
            game->currentState = STATE_PLAYING;
            break;

        case STATE_PLAYING:
            // aqui el jugador esta girando el potenciomtro
            // la logica seria en el main
            break;

        case STATE_CHECK_INPUT:
            // pulso boton (sera el Game_HandleButton)
            if (abs(game->playerInput - game->secretCode[game->currentDigitIndex]) <= TOLERANCE) {
                // correcto
                game->currentDigitIndex++;
                if (game->currentDigitIndex >= 3) {
                    game->currentState = STATE_WIN;
                } else {
                    game->currentState = STATE_PLAYING; // vas al siguiente numero
                }
            } else {
                // fallo
                game->currentState = STATE_LOSE;
            }
            break;

        case STATE_WIN:
            // espera reset
            break;

        case STATE_LOSE:
            // espera reset
            break;
    }
}

void Game_HandleButton(Game_Handle_t *game) {
    if (game->currentState == STATE_WIN || game->currentState == STATE_LOSE) {
        // Reiniciar 
        Game_Init(game);
    } else if (game->currentState == STATE_PLAYING) {
      // jugar
        game->currentState = STATE_CHECK_INPUT;
    }
}
