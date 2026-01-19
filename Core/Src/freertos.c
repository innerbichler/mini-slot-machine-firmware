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
uint8_t custom_parse_win_rate(const char *win_rate_str);
void ui_draw_chance();
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define RGB565(r, g, b) (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3))
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
typedef enum {
	SlotLocked,
	SlotEnterPassword,
	SlotStartup,
	SlotIdle,
	SlotGameStart,
	SlotGameRoll2,
	SlotGameRoll3,
	SlotGameEvaluation,
	SlotUIDrawChance,
	SlotSetChance,

} SlotGamestateEnum;
SlotGamestateEnum GameState = SlotLocked;
uint8_t win_rate_percent = 10;
// because the winrate needs to be global
// and the menu dialog is controlled from a seperated task
// the new_win_rate percent will also be global
uint8_t new_win_rate_percent = 10;


/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityBelowNormal,
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
	// goal
	char pressedKeys[20];
	char pressedKey = ' ';
	for (;;)
  {
		KeypadGetKey(&pressedKeys);
		pressedKey = pressedKeys[0];
		switch (GameState) {
		case SlotIdle:
			if (HAL_GPIO_ReadPin(stop_btn_0_GPIO_Port, stop_btn_0_Pin)) {
				GameState = SlotGameStart;
			}
			// the control display is always in the menu dialog
			
			switch (pressedKey) {
			case '3':
				//elivate the control display
				osThreadSetPriority(main_displayHandle, osPriorityLow);
				osThreadSetPriority(controlDisplayHandle, osPriorityNormal);
				GameState = SlotUIDrawChance;
				break;
			default:
				break;
			}
			break;
		case SlotSetChance:

			break;
		default:
			osDelay(100);
			break;
		}
		osDelay(50);
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
	int ran_symbol = 1;
	osDelay(10);
	uint8_t current_symbol = 0;
	uint8_t roll1_symbol = 0;
	uint8_t roll2_symbol = 0;
	uint8_t roll3_symbol = 0;


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
				//RA8876_SLOT_draw_roll(0, ran_symbol);
				//RA8876_SLOT_draw_roll(1, !ran_symbol);
				//RA8876_SLOT_draw_roll(2, ran_symbol);

			}
			osDelay(500);
			break;
		case SlotGameStart:
			// draw roll one until first button is pressed
			roll1_symbol = RA8876_SLOT_draw_roll(0, 1);
			break;
		case SlotGameRoll2:
			// first save the selected symbol of the roll before
			roll2_symbol = RA8876_SLOT_draw_roll(1, 1);
			break;
		case SlotGameRoll3:
			// last roll
			roll3_symbol = RA8876_SLOT_draw_roll(2, 1);
			break;
		case SlotGameEvaluation:
			if (roll3_symbol == roll2_symbol && roll3_symbol == roll1_symbol) {
				// winner winner chicken dinner
				MP3_play_sound_effect(4);
				osDelay(7000);
			} else {
				//MP3_play_sound_effect(3);
				osDelay(500);
			}
			RA8876_SLOT_clear();
			GameState = SlotIdle;
			break;
		default:
			osDelay(500);
			break;
		}
		osDelay(150);

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

			break;
		case SlotGameStart:
			// reset all other buttons
			HAL_GPIO_WritePin(stop_led_0_GPIO_Port, stop_led_0_Pin,
					GPIO_PIN_RESET);
			HAL_GPIO_WritePin(stop_led_1_GPIO_Port, stop_led_1_Pin,
					GPIO_PIN_RESET);
			HAL_GPIO_WritePin(stop_led_2_GPIO_Port, stop_led_2_Pin,
					GPIO_PIN_RESET);
			HAL_GPIO_TogglePin(stop_led_3_GPIO_Port, stop_led_3_Pin);
			if (HAL_GPIO_ReadPin(stop_btn_3_GPIO_Port, stop_btn_3_Pin)) {
				GameState = SlotGameRoll2;
			}
			break;
		case SlotGameRoll2:
			HAL_GPIO_WritePin(stop_led_3_GPIO_Port, stop_led_3_Pin,
					GPIO_PIN_SET);
			HAL_GPIO_TogglePin(stop_led_2_GPIO_Port, stop_led_2_Pin);
			if (HAL_GPIO_ReadPin(stop_btn_2_GPIO_Port, stop_btn_2_Pin)) {
				GameState = SlotGameRoll3;
			}
			break;
		case SlotGameRoll3:
			HAL_GPIO_WritePin(stop_led_2_GPIO_Port, stop_led_2_Pin,
					GPIO_PIN_SET);
			HAL_GPIO_TogglePin(stop_led_1_GPIO_Port, stop_led_1_Pin);
			if (HAL_GPIO_ReadPin(stop_btn_1_GPIO_Port, stop_btn_1_Pin)) {
				GameState = SlotGameEvaluation;
			}
			break;
		case SlotGameEvaluation:
			write_stop_buttons(GPIO_PIN_SET);
			osDelay(150);
			write_stop_buttons(GPIO_PIN_RESET);
			break;
		default:
			osDelay(250);
			break;
		}
		osDelay(150);
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
	char enter_win_percent[4] = { 0, 0, 0, '\0' };
	uint8_t enter_win_percent_index = 0;
	uint8_t temp_win_rate = 0;
	LCD_Init();
	LCD_PrintString("Starting Up");
	MP3_init();
	LCD_Clear();
	char *password = "1230";
  for(;;)
  {
		char pressed_keys[20];
		KeypadGetKey(&pressed_keys);
		pressedKey = pressed_keys[0];
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

			} else {
				passwordIndex++;
			}
			// delay for simple debouncing
			osDelay(250);
			break;
		case SlotStartup:
			// state for the intermediate directly after unlocking
			// we lower the priority of this task
			LCD_Clear();
			MP3_play_folder(1);
			GameState = SlotIdle;
			osThreadSetPriority(main_displayHandle, osPriorityNormal);
			osThreadSetPriority(controlDisplayHandle, osPriorityLow);
			break;
		case SlotUIDrawChance:
			ui_draw_chance();
			GameState = SlotSetChance;
			break;
		case SlotSetChance:
			// this is not optimal, but the menu wouldnt work otherwise
			switch (pressedKey) {
			case 'C':
			case 'D':
			case '*':
			case '#':
			case 0:
				break;
			case 'A':
				// always go into idle after new percentage
				win_rate_percent = new_win_rate_percent;
				GameState = SlotIdle;
				memset(enter_win_percent, 0, sizeof(enter_win_percent));
				enter_win_percent_index = 0;

				osThreadSetPriority(main_displayHandle, osPriorityNormal);
				osThreadSetPriority(controlDisplayHandle, osPriorityLow);

				break;
			case 'B':
				GameState = SlotIdle;
				memset(enter_win_percent, 0, sizeof(enter_win_percent));
				enter_win_percent_index = 0;

				osThreadSetPriority(main_displayHandle, osPriorityNormal);
				osThreadSetPriority(controlDisplayHandle, osPriorityLow);
				break;
			default:
				enter_win_percent[enter_win_percent_index] = pressedKey;

				LCD_SetCursor(9, 2);
				LCD_PrintString(enter_win_percent);
				if (enter_win_percent_index >= 2) {
					enter_win_percent_index = 0;
					temp_win_rate = custom_parse_win_rate(enter_win_percent);
					if (temp_win_rate > 0) {
						new_win_rate_percent = temp_win_rate;
					}
				} else {
					enter_win_percent_index++;
				}
				osDelay(250);
				break;
			}
			break;
		default:

			LCD_SetCursor(0, 0);
			LCD_PrintString("     1) Audio       ");
			LCD_SetCursor(0, 1);
			LCD_PrintString("     2) Stats       ");
			LCD_SetCursor(0, 2);
			LCD_PrintString("     3) Chance      ");
			LCD_SetCursor(0, 3);
			LCD_PrintString("  Made by Alexander ");
			break;
		}
		osDelay(250);

  }
  /* USER CODE END control_display */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
uint8_t custom_parse_win_rate(const char *win_rate_str) {
	int value = 0;
	for (int i = 0; i < 3; i++) {
		char c = win_rate_str[i];
		if (c < '0' || c > '9') {
			return 0;
		}
		int digit = c - '0';
		value = (value * 10) + digit;
	}
	if (value > 100) {
		value = 100;
	}
	else if (value == 0) {
		value = 1;
	}
	return (value);
}
void ui_draw_chance() {
	// set the win percentage 1-100
	char win_rate_str[21];
	snprintf(win_rate_str, sizeof(win_rate_str), "#    OLD %03d       #",
			win_rate_percent);
	LCD_SetCursor(0, 0);
	LCD_PrintString("#-----Win Rate-----#");
	LCD_SetCursor(0, 1);
	LCD_PrintString(win_rate_str);
	LCD_SetCursor(0, 2);
	LCD_PrintString("#    NEW           #");
	LCD_SetCursor(0, 3);
	LCD_PrintString("#--A-OK------B-NO--#");
}
void write_stop_buttons(GPIO_PinState state) {
	HAL_GPIO_WritePin(stop_led_0_GPIO_Port, stop_led_0_Pin, state);
	HAL_GPIO_WritePin(stop_led_1_GPIO_Port, stop_led_1_Pin, state);
	HAL_GPIO_WritePin(stop_led_2_GPIO_Port, stop_led_2_Pin, state);
	HAL_GPIO_WritePin(stop_led_3_GPIO_Port, stop_led_3_Pin, state);
}
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

