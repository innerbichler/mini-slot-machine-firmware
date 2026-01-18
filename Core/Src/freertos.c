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
#include "Matrix_Keypad.h"
#include "LCD1602.h"
#include "MP3_player.h"
#include <stdio.h>
#include "string.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
static inline uint16_t to_rgb565(uint8_t r, uint8_t g, uint8_t b);
uint8_t simple_rand(void);
void stagger_stop_buttons(uint16_t);
void initialise_main_display(void);
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define RGB565(r, g, b) (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3))
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
typedef enum {
	SlotLocked, SlotEnterPassword, SlotStartup, SlotIdle, SlotGameStart,

} SlotGamestateEnum;
SlotGamestateEnum GameState = SlotLocked;


/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for main_display */
osThreadId_t main_displayHandle;
const osThreadAttr_t main_display_attributes = {
  .name = "main_display",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for buttons */
osThreadId_t buttonsHandle;
const osThreadAttr_t buttons_attributes = {
  .name = "buttons",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for controlDisplay */
osThreadId_t controlDisplayHandle;
const osThreadAttr_t controlDisplay_attributes = {
  .name = "controlDisplay",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityBelowNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void main_display_task(void *argument);
void buttons_task(void *argument);
void control_display(void *argument);

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

  /* creation of main_display */
  main_displayHandle = osThreadNew(main_display_task, NULL, &main_display_attributes);

  /* creation of buttons */
  buttonsHandle = osThreadNew(buttons_task, NULL, &buttons_attributes);

  /* creation of controlDisplay */
  controlDisplayHandle = osThreadNew(control_display, NULL, &controlDisplay_attributes);

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


	for (;;)
  {
		if (HAL_GPIO_ReadPin(stop_btn_1_GPIO_Port, stop_btn_1_Pin)) {
			MP3_play_track(3);
		}
		if (HAL_GPIO_ReadPin(stop_btn_2_GPIO_Port, stop_btn_2_Pin)) {
			MP3_play_track(4);
		}
		if (HAL_GPIO_ReadPin(stop_btn_3_GPIO_Port, stop_btn_3_Pin)) {
			MP3_play_track(5);
		}
		osDelay(250);
  }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_main_display_task */
/**
* @brief Function implementing the main_display thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_main_display_task */
void main_display_task(void *argument)
{
  /* USER CODE BEGIN main_display_task */
  /* Infinite loop */
	// first reset stuff
	HAL_GPIO_WritePin(display_reset_GPIO_Port, display_reset_Pin,
			GPIO_PIN_RESET);
	osDelay(50);
	HAL_GPIO_WritePin(display_reset_GPIO_Port, display_reset_Pin, GPIO_PIN_SET);
	osDelay(200);

	uint8_t initialised = 0;
	uint16_t color = 0xF0D0;
	uint8_t x = 0;
	uint8_t step = 5;
	int frame = 0;
	int ran_symbol = 1;
	osDelay(10);

  for(;;)
  {
		switch (GameState) {
		case SlotIdle:
			if (initialised == 0
								&& HAL_GPIO_ReadPin(display_wait_GPIO_Port,
			display_wait_Pin) == GPIO_PIN_SET) {
				initialise_main_display();
				initialised = 1;

			}

			// only send something over SPI if wait is HIGH, wich means ready
			if (initialised
					&& HAL_GPIO_ReadPin(display_wait_GPIO_Port,
							display_wait_Pin) == GPIO_PIN_SET) {

				ran_symbol = !ran_symbol;
				RA8876_SLOT_draw_roll(0, ran_symbol);
				RA8876_SLOT_draw_roll(1, !ran_symbol);
				RA8876_SLOT_draw_roll(2, ran_symbol);

				frame += 1;

				if (frame > 0xFE) {
					frame = 0;
				}

			}
			osDelay(500);
			break;
		default:
			osDelay(500);
			break;
		}


  }
  /* USER CODE END main_display_task */
}

/* USER CODE BEGIN Header_buttons_task */
/**
* @brief Function implementing the buttons thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_buttons_task */
void buttons_task(void *argument)
{
  /* USER CODE BEGIN buttons_task */
  /* Infinite loop */

  for(;;)
  {

		switch (GameState) {
		case SlotIdle:
			stagger_stop_buttons(75);

			if (HAL_GPIO_ReadPin(stop_btn_0_GPIO_Port, stop_btn_0_Pin)) {
				GameState = SlotGameStart;
				MP3_play_sound_effect(1);
			}

			break;
		default:
			osDelay(250);
			break;
		}
	}
  /* USER CODE END buttons_task */
}

/* USER CODE BEGIN Header_control_display */
/**
* @brief Function implementing the controlDisplay thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_control_display */
void control_display(void *argument)
{
  /* USER CODE BEGIN control_display */
  /* Infinite loop */
	char pressedKey = ' ';
	char passwordAttempt[5] = { 0, 0, 0, 0, '\0' };
	char hiddenDigits[5] = { 0, 0, 0, 0, '\0' };
	uint8_t passwordIndex = 0;
	LCD_Init();
	LCD_PrintString("Starting Up");
	MP3_init();
	LCD_Clear();
	char *password = "1230";
  for(;;)
  {
		char pressedKeys[20];
		KeypadGetKey(&pressedKeys);
		pressedKey = pressedKeys[0];
		switch (GameState) {
		case SlotLocked:
			LCD_SetCursor(0, 0);
			LCD_PrintString("#-----Password-----#");
			LCD_SetCursor(0, 1);
			LCD_PrintString("#                  #");
			LCD_SetCursor(0, 2);
			LCD_PrintString("#                  #");
			LCD_SetCursor(0, 3);
			LCD_PrintString("#------------------#");
			GameState = SlotEnterPassword;
			break;
		case SlotEnterPassword:
			if (pressedKey == 0) {
				break;
			}
			passwordAttempt[passwordIndex] = pressedKey;
			hiddenDigits[passwordIndex] = '*';
			LCD_SetCursor(8, 1);
			LCD_PrintString(hiddenDigits);
			LCD_SetCursor(8, 2);
			LCD_PrintString(hiddenDigits);

			if (passwordIndex >= 3) {
				if (strcmp(password, passwordAttempt) == 0) {
					GameState = SlotStartup;
				}
				passwordIndex = 0;
			}
			passwordIndex++;
			// delay because of pressedKeys being full

			osDelay(250);
			break;
		case SlotStartup:
			// state for the intermediate directly after unlocking
			// we lower the priority of this task
			osThreadSetPriority(main_displayHandle, osPriorityNormal);
			osThreadSetPriority(controlDisplayHandle, osPriorityLow);

			LCD_Clear();
			MP3_play_folder(1);
			GameState = SlotIdle;
			break;
		default:
			// the control display is always in the menu dialog
			LCD_SetCursor(5, 0);
			LCD_PrintString("1) Audio");
			LCD_SetCursor(5, 1);
			LCD_PrintString("2) Stats");
			LCD_SetCursor(5, 2);
			LCD_PrintString("3) Chance");
			LCD_SetCursor(2, 3);
			LCD_PrintString("Made by Alexander");
			break;
		}
		osDelay(250);

  }
  /* USER CODE END control_display */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
void stagger_stop_buttons(uint16_t time) {
	HAL_GPIO_TogglePin(stop_led_0_GPIO_Port, stop_led_0_Pin);
	osDelay(time);
	HAL_GPIO_TogglePin(stop_led_1_GPIO_Port, stop_led_1_Pin);
	osDelay(time);
	HAL_GPIO_TogglePin(stop_led_2_GPIO_Port, stop_led_2_Pin);
	osDelay(time);
	HAL_GPIO_TogglePin(stop_led_3_GPIO_Port, stop_led_3_Pin);
	osDelay(time);

}

void initialise_main_display() {
	RA8876_PLL_init();
	uint8_t config_register = RA8876_read_register(0x01);
	if ((config_register & 0x80) != 0x80) {
		//PLL init error

	}

	RA8876_SDRAM_init();
	osDelay(5);

	// Wait for SDRAM to finish check bit 6 of Register 0xE4
	while ((RA8876_read_register(0xE4) & 0x01) != 0x01) {
		osDelay(50);
	}

	RA8876_display_init();

	// turn display on test mode
	// RA8876_color_bar_test_on();
	HAL_GPIO_WritePin(display_backlight_GPIO_Port,
	display_backlight_Pin, GPIO_PIN_SET);

	RA8876_display_on();
	HAL_Delay(20);

	RA8876_clear_screen();

	RA8876_draw_mario(25, 25);
	osDelay(20);

	RA8876_fill_bottom_gradient();
	//RA8876_draw_tree(512, 300, 100);
}

static inline uint16_t to_rgb565(uint8_t r, uint8_t g, uint8_t b) {
	// Force everything to uint16_t before shifting to prevent bit-loss
	uint16_t rr = (uint16_t) (r & 0xF8) << 8;
	uint16_t gg = (uint16_t) (g & 0xFC) << 3;
	uint16_t bb = (uint16_t) (b >> 3);
	return (rr | gg | bb);
}

uint8_t simple_rand() {
	static uint32_t x = 123456789; // Seed

	// Tiny Xorshift algorithm (very fast, no HardFault)
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;

	return (uint8_t) (x & 0x01); // Returns 0 or 1
}
/* USER CODE END Application */

