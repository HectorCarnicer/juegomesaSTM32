#ifndef GAME_TIPOS_H
#define GAME_TIPOS_H

#include <stdint.h>

// Maquina de estados...:
typedef enum {
	STATE_INIT,         // Inicio
	STATE_IDLE,         // Espera
	STATE_PLAYING,      // Buscando
	STATE_CHECK_INPUT,  // Checkeo
	STATE_WIN,          // Abierta la caja fuerte
	STATE_LOSE          // Alarma activada si fallas
} GameState_t;

// Almacenamiento de datos del juegos
typedef struct {
	GameState_t currentState;
	uint8_t secretCode[3];      // La clave de 3 nums
	uint8_t currentDigitIndex;  // El digito a adivinar, ya sea 1,2 o 3
	uint8_t playerInput;        // Valor mapeado del potenciometro (en vez de 4096 q sea de 0 a 9)
	uint32_t lastActionTick;    // Timeouts, no hay tiempo infinito
} Game_Handle_t;

#endif
