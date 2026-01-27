#include "main.h"
#include "game_tipos.h"
#include "game_funcionamiento.h"
#include <stdlib.h>

/* VARIABLES */
ADC_HandleTypeDef hadc1;
Game_Handle_t hGame;
uint32_t rawValue = 0;

/* PROTOTIPOS */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC1_Init(void);

int main(void)
{
  HAL_Init();
  SystemClock_Config();

  /* 1. INICIALIZAMOS (El ADC rompera el boton aqui por la libreria) */
  MX_GPIO_Init();
  MX_ADC1_Init();

  /* 2. CONFIGURACION JUEGO */
  hGame.currentDigitIndex = 0;
  hGame.currentState = STATE_PLAYING;

  /* CLAVE SECRETA FORZADA PARA TESTING */
  hGame.secretCode[0] = 0; // Nivel 1: GND
  hGame.secretCode[1] = 9; // Nivel 2: 3V
  hGame.secretCode[2] = 0; // Nivel 3: GND

  /* --- BYPASS PARA EL BOTON PA0 --- */
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  __HAL_RCC_GPIOA_CLK_ENABLE();
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  /* ----------------------------------------- */

  /* Animacion de arranque */
  for(int i=0; i<3; i++) {
      HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15);
      HAL_Delay(100);
  }
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15, GPIO_PIN_RESET);

  uint32_t lastAdcTick = 0;

  while (1)
  {
      /* 1. LECTURA DEL BOTON (PRIORIDAD) */
      if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET) {

          HAL_Delay(50); // Antirrebote

          if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET) {

              /* LEER CABLE AL INSTANTE PARA PRECISION */
              HAL_ADC_Start(&hadc1);
              if (HAL_ADC_PollForConversion(&hadc1, 5) == HAL_OK) {
                  rawValue = HAL_ADC_GetValue(&hadc1);
              }
              HAL_ADC_Stop(&hadc1);

              /* LOGICA DE JUEGO */
              int inputVisual = (rawValue * 10) / 4096;
              if(inputVisual > 9) inputVisual = 9;
              hGame.playerInput = inputVisual;

              int secreto = hGame.secretCode[hGame.currentDigitIndex];
              int diff = abs(hGame.playerInput - secreto);

              /* Tolerancia (<= que uno) */
              if (diff <= 1) {
                  /* ACIERTO */
                  hGame.currentDigitIndex++;

                  /* Confirmacion: Flashazo Verde al pasar (opcional) */
                  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_15, GPIO_PIN_RESET); // Apagar otros
                  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_SET);
                  HAL_Delay(300);
                  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_RESET);

                  if (hGame.currentDigitIndex >= 3) hGame.currentState = STATE_WIN;
              }
              else {
                  /* FALLO: Transicion inmediata a estado LOSE */
                  hGame.currentState = STATE_LOSE;
              }

              /* Esperar a soltar */
              while(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET) HAL_Delay(10);
          }
      }

      /* 2. LECTURA ADC DE FONDO  */
      if (HAL_GetTick() - lastAdcTick > 100) {
          HAL_ADC_Start(&hadc1);
          if (HAL_ADC_PollForConversion(&hadc1, 5) == HAL_OK) {
              rawValue = HAL_ADC_GetValue(&hadc1);
          }
          HAL_ADC_Stop(&hadc1);
          lastAdcTick = HAL_GetTick();
      }

      /* 3. VISUALIZACION  */
      if(hGame.currentState == STATE_PLAYING)
      {
          int inputVisual = (rawValue * 10) / 4096;
          if(inputVisual > 9) inputVisual = 9;

          int dist = abs(inputVisual - hGame.secretCode[hGame.currentDigitIndex]);
          uint32_t velocidad = 0;

          /* TABLA DE VELOCIDADES MEJORADA */
          if (dist <= 1) velocidad = 100;       // ARDIENDO
          else if (dist <= 3) velocidad = 300;  // CALIENTE
          else if (dist <= 5) velocidad = 600;  // TEMPLADO
          else velocidad = 1200;                // FRIO


          static uint32_t t = 0;
          static int s = 0;

          if(HAL_GetTick() - t > velocidad) {
              s = !s; t = HAL_GetTick();
          }

          /* LED Verde (Nivel 1) */
          /* Si ya pasamos nivel 1 -> FIJO. Si estamos jugando -> PARPADEA */
          if (hGame.currentDigitIndex > 0) HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_SET);
          else if (hGame.currentDigitIndex == 0) HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, s);

          /* LED Naranja (Nivel 2) */
          if (hGame.currentDigitIndex > 1) HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, GPIO_PIN_SET);
          else if (hGame.currentDigitIndex == 1) HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, s);
          else HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, GPIO_PIN_RESET); // Si no llegamos, apagado

          /* LED Azul (Nivel 3) */
          if (hGame.currentDigitIndex > 2) HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, GPIO_PIN_SET);
          else if (hGame.currentDigitIndex == 2) HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, s);
          else HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, GPIO_PIN_RESET); // Si no llegamos, apagado
      }

      /* ESTADO DE VICTORIA */
      else if (hGame.currentState == STATE_WIN) {
          HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15);
          HAL_Delay(100);
          if(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET) NVIC_SystemReset();
      }

      /* ESTADO DE DERROTA (AHORA PARPADEA ROJO INMEDIATAMENTE) */
      else if (hGame.currentState == STATE_LOSE) {
          /* Apagamos el resto para que se vea bien el error */
          HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_15, GPIO_PIN_RESET);

          /* Parpadeo Rojo de Alerta */
          HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_14);
          HAL_Delay(200);

          /* Pulsar para reiniciar */
          if(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET) NVIC_SystemReset();
      }
  }
}

/* --- CONFIGURACIONES --- */
static void MX_GPIO_Init(void) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15, GPIO_PIN_RESET);

  GPIO_InitStruct.Pin = GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /* Config inicial (Pisada por ADC luego, pero necesaria) */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

static void MX_ADC1_Init(void) {
  ADC_ChannelConfTypeDef sConfig = {0};
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  HAL_ADC_Init(&hadc1);
  sConfig.Channel = ADC_CHANNEL_1;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_84CYCLES;
  HAL_ADC_ConfigChannel(&hadc1, &sConfig);
}

void SystemClock_Config(void) {
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 192;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 8;
  HAL_RCC_OscConfig(&RCC_OscInitStruct);
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3);
}
void Error_Handler(void) { __disable_irq(); while (1) {} }
