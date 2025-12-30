/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "gpio.h"
#include "RA8876_driver.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */

	// first reset stuff
	HAL_GPIO_WritePin(display_reset_GPIO_Port, display_reset_Pin,
			GPIO_PIN_RESET);
	osDelay(50);

	HAL_GPIO_WritePin(display_reset_GPIO_Port, display_reset_Pin, GPIO_PIN_SET);

	osDelay(200);

	uint8_t initialised = 0;



	for (;;)
  {

		if (initialised == 0
				&& HAL_GPIO_ReadPin(display_wait_GPIO_Port, display_wait_Pin)
						== GPIO_PIN_SET) {
			RA8876_PLL_init();
			uint8_t config_register = RA8876_read_register(0x01);
			if ((config_register & 0x80) != 0x80) {
				//PLL init error
				HAL_GPIO_WritePin(LD5_GPIO_Port, LD3_Pin, GPIO_PIN_SET);

			}

			RA8876_SDRAM_init();
			osDelay(5);

			// Wait for SDRAM to finish check bit 6 of Register 0xE4
			while ((RA8876_read_register(0xE4) & 0x01) != 0x01) {
				HAL_GPIO_TogglePin(LD6_GPIO_Port, LD6_Pin);
				osDelay(50);
			}

			RA8876_display_init();

			// turn display on test mode
			//RA8876_color_bar_test_on();
			//
			RA8876_display_on();
			RA8876_clear_screen();
			//RA8876_set_foreground_color(0x0000);
			HAL_Delay(20);

			//RA8876_draw_rect(10, 10, 1000, 500, 0xFF00FF00);
			HAL_GPIO_WritePin(LD4_GPIO_Port, LD4_Pin, GPIO_PIN_SET);
			initialised = 1;
		}
		// only send something over SPI if wait is HIGH, wich means ready
		if (HAL_GPIO_ReadPin(display_wait_GPIO_Port, display_wait_Pin)
				== GPIO_PIN_SET) {
			//uint8_t config_register = RA8876_read_register(0x01);
			HAL_Delay(2);

			uint8_t config_register = RA8876_read_register(0x12);
			//uint8_t status = RA8876_read_status_register();
			HAL_Delay(2);


		}

		HAL_GPIO_TogglePin(LD6_GPIO_Port, LD6_Pin);
		osDelay(250);

  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

