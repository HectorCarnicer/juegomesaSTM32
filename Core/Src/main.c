#include "main.h"
#include "game_tipos.h"
#include "game_funcionamiento.h"
#include <stdlib.h> // Para abs

// Variables Globales
ADC_HandleTypeDef hadc1;
TIM_HandleTypeDef htim2;
Game_Handle_t hGame;
uint32_t rawValue = 0;
uint32_t tiempoInicioJuego = 0;

// Constantes
const uint32_t TIEMPO_LIMITE_JUEGO = 30000; // 30 Segundos

// Prototipos
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC1_Init(void);
static void MX_TIM2_Init(void);

int main(void)
{
  HAL_Init();
  SystemClock_Config();

  // Inicializacion
  MX_GPIO_Init();
  MX_ADC1_Init();
  MX_TIM2_Init();

  // Empezamos en IDLE
  hGame.currentState = STATE_IDLE;
  hGame.currentDigitIndex = 0;

  // Clave secreta (0-9-0)
  hGame.secretCode[0] = 0;
  hGame.secretCode[1] = 9;
  hGame.secretCode[2] = 0;

  // Reparacion boton PA0
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  __HAL_RCC_GPIOA_CLK_ENABLE();
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  uint32_t lastAdcTick = 0;

  while (1)
  {
      // ==========================================================
      // ESTADO 1: IDLE (ESPERA)
      // ==========================================================
      if (hGame.currentState == STATE_IDLE) {

          // 1. Animacion Circular
          static uint32_t tAnim = 0;
          static uint8_t ledPos = 0;

          if (HAL_GetTick() - tAnim > 150) {
              HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15, GPIO_PIN_RESET);
              if(ledPos == 0) HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_SET);
              if(ledPos == 1) HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, GPIO_PIN_SET);
              if(ledPos == 2) HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_SET);
              if(ledPos == 3) HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, GPIO_PIN_SET);
              ledPos++;
              if(ledPos > 3) ledPos = 0;
              tAnim = HAL_GetTick();
          }

          // 2. Detectar pulsacion larga (2s)
          if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET) {
              HAL_Delay(50);
              if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET) {

                  uint32_t tPresionado = HAL_GetTick();
                  uint8_t comenzar = 0;

                  while(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET) {
                      if(HAL_GetTick() - tPresionado > 2000) {
                          comenzar = 1;
                          break;
                      }
                  }

                  if (comenzar) {
                      // Feedback: Flash Blanco
                      for(int k=0; k<3; k++){
                          HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15);
                          HAL_Delay(50);
                      }
                      HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15, GPIO_PIN_RESET);

                      // --- AQUI ESTA EL ARREGLO (BARRERA DE ENTRADA) ---
                      // Esperamos a que SUELTES el boton antes de empezar el juego
                      // Asi evitamos que el click se cuele en el Nivel 1
                      while(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET);
                      HAL_Delay(500); // Medio segundo de cortesia extra
                      // --------------------------------------------------

                      // INICIAR JUEGO
                      hGame.currentDigitIndex = 0;
                      hGame.currentState = STATE_PLAYING;
                      tiempoInicioJuego = HAL_GetTick();
                  }
              }
          }
      }

      // ==========================================================
      // ESTADO 2: JUGANDO (PLAYING)
      // ==========================================================
      else if (hGame.currentState == STATE_PLAYING) {

          // 1. Tiempo Limite (30s)
          if (HAL_GetTick() - tiempoInicioJuego > TIEMPO_LIMITE_JUEGO) {
              hGame.currentState = STATE_LOSE;
          }

          // 2. Logica Boton
          if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET) {

              HAL_Delay(50);

              if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET) {

                  // Reset Hardware (Timer 2)
                  __HAL_TIM_SET_COUNTER(&htim2, 0);
                  HAL_TIM_Base_Start_IT(&htim2);

                  while(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET);
                  HAL_TIM_Base_Stop_IT(&htim2);

                  // JUGADA NORMAL
                  HAL_ADC_Start(&hadc1);
                  if (HAL_ADC_PollForConversion(&hadc1, 5) == HAL_OK) rawValue = HAL_ADC_GetValue(&hadc1);
                  HAL_ADC_Stop(&hadc1);

                  int inputVisual = (rawValue * 10) / 4096;
                  if(inputVisual > 9) inputVisual = 9;
                  hGame.playerInput = inputVisual;

                  int secreto = hGame.secretCode[hGame.currentDigitIndex];
                  int diff = abs(hGame.playerInput - secreto);

                  if (diff <= 1) { // Acierto
                      hGame.currentDigitIndex++;

                      // Feedback Acierto
                      HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_15, GPIO_PIN_RESET);
                      HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_SET);
                      HAL_Delay(300);
                      HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_RESET);

                      if (hGame.currentDigitIndex >= 3) hGame.currentState = STATE_WIN;
                  }
                  else { // Fallo
                      hGame.currentState = STATE_LOSE;
                  }
              }
          }

          // 3. Lectura ADC Visual
          if (HAL_GetTick() - lastAdcTick > 100) {
              HAL_ADC_Start(&hadc1);
              if (HAL_ADC_PollForConversion(&hadc1, 5) == HAL_OK) rawValue = HAL_ADC_GetValue(&hadc1);
              HAL_ADC_Stop(&hadc1);
              lastAdcTick = HAL_GetTick();
          }

          // 4. LEDs Frio/Caliente
          int inputVisual = (rawValue * 10) / 4096;
          if(inputVisual > 9) inputVisual = 9;

          int dist = abs(inputVisual - hGame.secretCode[hGame.currentDigitIndex]);
          uint32_t velocidad = 0;

          if (dist <= 1) velocidad = 100;
          else if (dist <= 3) velocidad = 300;
          else if (dist <= 5) velocidad = 600;
          else velocidad = 1200;

          static uint32_t t = 0;
          static int s = 0;
          if(HAL_GetTick() - t > velocidad) { s = !s; t = HAL_GetTick(); }

          if (hGame.currentDigitIndex > 0) HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_SET);
          else if (hGame.currentDigitIndex == 0) HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, s);

          if (hGame.currentDigitIndex > 1) HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, GPIO_PIN_SET);
          else if (hGame.currentDigitIndex == 1) HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, s);
          else HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, GPIO_PIN_RESET);

          if (hGame.currentDigitIndex > 2) HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, GPIO_PIN_SET);
          else if (hGame.currentDigitIndex == 2) HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, s);
          else HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, GPIO_PIN_RESET);
      }

      // ==========================================================
      // ESTADO 3: VICTORIA
      // ==========================================================
      else if (hGame.currentState == STATE_WIN) {
          HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15);
          HAL_Delay(100);

          if(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET) {
              while(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET);
              hGame.currentState = STATE_IDLE;
          }
      }

      // ==========================================================
      // ESTADO 4: DERROTA
      // ==========================================================
      else if (hGame.currentState == STATE_LOSE) {
          HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_15, GPIO_PIN_RESET);
          HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_14);
          HAL_Delay(200);

          if(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET) {
               while(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET);
               hGame.currentState = STATE_IDLE;
          }
      }
  }
}

// Interrupcion Timer 2 (Reset Emergencia)
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2) {
        HAL_TIM_Base_Stop_IT(&htim2);
        for(int k=0; k<5; k++) {
              HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_15);
              for(int d=0; d<200000; d++);
        }
        NVIC_SystemReset();
    }
}

// Configuraciones
static void MX_TIM2_Init(void)
{
  __HAL_RCC_TIM2_CLK_ENABLE(); // IMPORTANTE

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 47999;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 3999;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

  HAL_TIM_Base_Init(&htim2);

  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig);

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig);

  HAL_NVIC_SetPriority(TIM2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(TIM2_IRQn);
}

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

void TIM2_IRQHandler(void)
{
  HAL_TIM_IRQHandler(&htim2);
}

void Error_Handler(void) { __disable_irq(); while (1) {} }
