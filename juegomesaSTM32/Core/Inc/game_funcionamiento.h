#ifndef GAME_FSM_H
#define GAME_FSM_H

#include "game_types.h"
#include "main.h" // hay que acceder al HAL por si acaso

// Funciones publicas (game_handle es la estructura de game_tipos)
void Game_Init(Game_Handle_t* game);
void Game_Update(Game_Handle_t* game, uint32_t adcValue);
void Game_HandleButton(Game_Handle_t* game); // llamada desde la interrupcion

#endif