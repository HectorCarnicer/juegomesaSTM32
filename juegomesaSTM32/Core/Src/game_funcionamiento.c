
/* Includes ------*/
#include "main.h"
// INCLUDES
#include "game_tipos.h"
#include "game_funcionamiento.h"

/* variables privadas ------------------------*/
ADC_HandleTypeDef hadc1; 

Game_Handle_t hGame;             // la instancia 

/* Private user code ---------------------------------------------------------*/

int main(void)
{
  HAL_Init();
  SystemClock_Config();
  
  MX_GPIO_Init(); //falta por hacer las configuraciones de pines <<<<----------
  MX_ADC1_Init(); 

  Game_Init(&hGame);

  while (1)
  {
    // LEER SENSORES 
    HAL_ADC_Start(&hadc1);
    if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK) {
        uint32_t rawValue = HAL_ADC_GetValue(&hadc1);
        
        //  ACTUALIZAR LÓGICA
        Game_Update(&hGame, rawValue);
    }

    // ACTUALIZAR ACTUADORES (leds)
 
    // Mostrar progreso en 3 LEDs nuestros
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, (hGame.currentDigitIndex > 0) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, (hGame.currentDigitIndex > 1) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, (hGame.currentDigitIndex > 2) ? GPIO_PIN_SET : GPIO_PIN_RESET);

    // Feedback de estado ya sea Ganar/Perder
    if(hGame.currentState == STATE_WIN) {
        // Parpadeo verde
        HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_0); 
        HAL_Delay(200); // Bloqueo temporal
    } 
    else if (hGame.currentState == STATE_LOSE) {
        // Parpadeo Rojo rápido + Buzzeador
        HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_1);
        HAL_Delay(50);
    }

    HAL_Delay(10); //por si aca
  }
}
