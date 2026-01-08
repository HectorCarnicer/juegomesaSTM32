#ifndef GAME_TYPES_H
#define GAME_TYPES_H

#include <stdint.h>

// Maquina de estados...:
typedef enum {
    STATE_INIT,         // inicio
    STATE_IDLE,         // espera
    STATE_PLAYING,      // buscando
    STATE_CHECK_INPUT,  // checkeo
    STATE_WIN,          // abierta la caja fuerte
    STATE_LOSE          // alrama activada si fallas
} GameState_t;

// almacenamiento de estados/variables
typedef struct {
    GameState_t currentState;
    uint8_t secretCode[3];      // La clave de 3 nums
    uint8_t currentDigitIndex;  // el digito a adivinar, ya sea 1,2 o 3
    uint8_t playerInput;        // valor mapeado del potenciometro (en vez de 4096 q sea de 0 a 9)
    uint32_t lastActionTick;    // timeouts, no hay tiempo infinito
} Game_Handle_t;

#endif