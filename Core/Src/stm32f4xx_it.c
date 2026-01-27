/* stm32f4xx_it.c - VERSIÓN LIMPIA PARA JUEGO POLLING */
#include "main.h"
#include "stm32f4xx_it.h"

/* Manejadores de Excepciones del Sistema (Standard) */
void NMI_Handler(void) { while (1) {} }
void HardFault_Handler(void) { while (1) {} }
void MemManage_Handler(void) { while (1) {} }
void BusFault_Handler(void) { while (1) {} }
void UsageFault_Handler(void) { while (1) {} }
void SVC_Handler(void) {}
void DebugMon_Handler(void) {}
void PendSV_Handler(void) {}

/* ESTE ES EL ÚNICO IMPORTANTE AHORA MISMO */
/* Se encarga de contar el tiempo para HAL_Delay() y HAL_GetTick() */
void SysTick_Handler(void)
{
  HAL_IncTick();
}

/* !!! ARREGLO 7: LIMPIEZA DE INTERRUPCIONES (Reporte) !!! */
/* Se han eliminado los manejadores EXTI0_IRQHandler y TIM1...
   para evitar conflictos, ya que el control ahora se realiza
   mediante sondeo (Polling) en main.c */
