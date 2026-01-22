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
void ui_draw_password(void);
void ui_draw_menu(void);
void ui_draw_win_rate(void);
void ui_draw_stats(void);
void ui_draw_audio(void);
void ui_draw_lose_music(void);
void ui_draw_volume(void);
void ui_draw_startup_sequence(void);
void slot_srand(uint32_t seed);
uint32_t slot_rand(void);
uint32_t get_random_number(uint32_t min_value, uint32_t max_value);
void elevate_main_display_task(void);
void elevate_control_task(void);
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
	SlotUIDrawWinRate,
	SlotSetChance,
	SlotUIDrawStats,
	SlotUIDrawAudio,
	SlotSetAudio,
	SlotWaitForOk,

} SlotGamestateEnum;
SlotGamestateEnum GameState = SlotLocked;
uint8_t win_rate_percent = 20;
// because the winrate needs to be global
// and the menu dialog is controlled from a seperated task
// the new_win_rate percent will also be global
uint8_t new_win_rate_percent = 10;
static uint32_t prng_state;

struct SlotStats {
	uint32_t session_total_games;
	uint32_t session_won_games;
};
struct SlotStats slot_stats = { 0, 0 };

struct SlotAudioSettings {
	uint8_t lose_music_on;
	uint8_t volume;
};
struct SlotAudioSettings slot_audio_settings = { 1, 10 };

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
	char pressed_keys[20];
	char pressed_key = ' ';
	for (;;)
  {
		KeypadGetKey(&pressed_keys);
		pressed_key = pressed_keys[0];
		switch (GameState) {
		case SlotIdle:
			if (HAL_GPIO_ReadPin(stop_btn_0_GPIO_Port, stop_btn_0_Pin)) {
				GameState = SlotGameStart;
			}
			// the control display is always in the menu dialog during IDLE
			// we have to always elevate the prio of the control to make it snappy
			switch (pressed_key) {
			case '1':
				elevate_control_task();
				GameState = SlotUIDrawAudio;
				break;
			case '2':
				elevate_control_task();
				GameState = SlotUIDrawStats;
				break;
			case '3':
				elevate_control_task();
				GameState = SlotUIDrawWinRate;
				break;
			default:
				break;
			}
			break;
			// for all control display pages, that are only info
		case SlotWaitForOk:
			switch (pressed_key) {
			case 'A':
				GameState = SlotIdle;
				elevate_main_display_task();
				break;
			default:
				break;
			}
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
	uint8_t roll1_symbol = 0;
	uint8_t roll2_symbol = 0;
	uint8_t roll3_symbol = 0;

	uint8_t will_win = 0;
	uint8_t winning_symbol = 0;
	uint8_t ACTIVE_SYMBOLS[5] = { SNOWMAN, DIAMOND, PRESENT, TREE,
			PACMAN };

	// stop constantly redrawing winning symbols
	uint8_t will_win_draw_helper = 0;
	uint8_t random_number = 0;
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
				random_number = (uint8_t) get_random_number(0, 100);
				will_win = random_number <= win_rate_percent;
				random_number = (uint8_t) get_random_number(0,
						sizeof(ACTIVE_SYMBOLS) - 1);

				winning_symbol = ACTIVE_SYMBOLS[random_number];
				

			}
			osDelay(100);
			break;
		case SlotGameStart:
			// draw roll one until first button is pressed
			roll1_symbol = RA8876_SLOT_draw_roll(0, 1);
			break;
		case SlotGameRoll2:
			// first save the selected symbol of the roll before
			if (will_win && will_win_draw_helper == 0) {
			RA8876_SLOT_stop_roll(0, winning_symbol);
				will_win_draw_helper++;
			}
			roll2_symbol = RA8876_SLOT_draw_roll(1, 1);

			break;
		case SlotGameRoll3:
			// last roll
			if (will_win && will_win_draw_helper == 1) {
				RA8876_SLOT_stop_roll(1, winning_symbol);
				will_win_draw_helper++;
			}
			roll3_symbol = RA8876_SLOT_draw_roll(2, 1);
			break;
		case SlotGameEvaluation:

			if (will_win) {
				RA8876_SLOT_stop_roll(2, winning_symbol);

				// winner winner chicken dinner
				MP3_play_sound_effect(4);
				will_win_draw_helper = 0;
				osDelay(7000);
				RA8876_SLOT_clear();
				GameState = SlotIdle;
				slot_stats.session_won_games++;
			} else {
				// make sure the last roll is NOT correct
				if (roll3_symbol == roll2_symbol
						&& roll3_symbol == roll1_symbol) {
					random_number = (uint8_t) get_random_number(0,
							sizeof(ACTIVE_SYMBOLS) - 1);
					roll3_symbol = ACTIVE_SYMBOLS[random_number];
					RA8876_SLOT_stop_roll(2, roll3_symbol);

				} else {
					RA8876_SLOT_clear();
					GameState = SlotIdle;
				}
				if (slot_audio_settings.lose_music_on) {
				MP3_play_sound_effect(3);
				}
			}
			slot_stats.session_total_games++;
			osDelay(500);
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
	char pressed_key = ' ';
	char password_attempt[5] = { 0, 0, 0, 0, '\0' };
	char hidden_digits[5] = { 0, 0, 0, 0, '\0' };
	uint8_t password_index = 0;
	char enter_win_percent[4] = { 0, 0, 0, '\0' };
	uint8_t enter_win_percent_index = 0;
	uint8_t temp_win_rate = 0;
	LCD_Init();
	srand(HAL_GetTick());
	ui_draw_startup_sequence();
	MP3_init();
	osDelay(1000);
	MP3_set_volume(slot_audio_settings.volume);
	LCD_Clear();
	char *password = "1230";
  for(;;)
  {
		char pressed_keys[20];
		KeypadGetKey(&pressed_keys);
		pressed_key = pressed_keys[0];
		switch (GameState) {
		case SlotLocked:
			ui_draw_password();
			GameState = SlotEnterPassword;
			break;
		case SlotEnterPassword:
			if (pressed_key == 0) {
				break;
			}
			password_attempt[password_index] = pressed_key;
			hidden_digits[password_index] = '*';
			LCD_SetCursor(8, 1);
			LCD_PrintString(hidden_digits);
			LCD_SetCursor(8, 2);
			LCD_PrintString(hidden_digits);

			if (password_index >= 3) {
				if (strcmp(password, password_attempt) == 0) {
					GameState = SlotStartup;
				}
				password_index = 0;
				memset(password_attempt, 0, sizeof(password_attempt));
				memset(hidden_digits, 0, sizeof(hidden_digits) - 1);
				LCD_SetCursor(8, 1);
				LCD_PrintString("    ");
				LCD_SetCursor(8, 2);
				LCD_PrintString("    ");

			} else {
				password_index++;
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
			elevate_main_display_task();
			break;
		case SlotUIDrawAudio:
			ui_draw_audio();
			GameState = SlotSetAudio;
			break;
		case SlotSetAudio:
			switch (pressed_key) {
			case 'A':
				GameState = SlotIdle;
				elevate_main_display_task();
				break;
			case '1':
				slot_audio_settings.lose_music_on =
						!slot_audio_settings.lose_music_on;
				ui_draw_lose_music();
				break;
			case '2':
				if (slot_audio_settings.volume >= 30) {
					slot_audio_settings.volume = 30;
				} else {
					slot_audio_settings.volume++;
				}
				MP3_set_volume(slot_audio_settings.volume);
				ui_draw_volume();
				break;
			case '3':
				if (slot_audio_settings.volume <= 0) {
					slot_audio_settings.volume = 0;
				} else {
					slot_audio_settings.volume--;
				}
				MP3_set_volume(slot_audio_settings.volume);
				ui_draw_volume();
				break;
			default:
				break;
			}
			break;
		case SlotUIDrawStats:
			ui_draw_stats();
			GameState = SlotWaitForOk;
			break;
		case SlotUIDrawWinRate:
			ui_draw_win_rate();
			GameState = SlotSetChance;
			break;
		case SlotSetChance:
			// this is not optimal, but the menu wouldnt work otherwise
			switch (pressed_key) {
			case 'C':
			case 'D':
			case '*':
			case '#':
			case 0:
				break;
			case 'A':
				// always go into idle after new percentage and reset everything
				win_rate_percent = new_win_rate_percent;
				GameState = SlotIdle;
				memset(enter_win_percent, 0, sizeof(enter_win_percent));
				enter_win_percent_index = 0;
				elevate_main_display_task();
				break;
			case 'B':
				GameState = SlotIdle;
				memset(enter_win_percent, 0, sizeof(enter_win_percent));
				enter_win_percent_index = 0;
				elevate_main_display_task();
				break;
			default:
				// overwrite the old numbers on screen
				if (enter_win_percent_index + 1 >= 4) {
					enter_win_percent_index = 0;
					memset(enter_win_percent, 0, sizeof(enter_win_percent));
					LCD_SetCursor(9, 2);
					LCD_PrintString("   ");
				}
				enter_win_percent[enter_win_percent_index] = pressed_key;

				LCD_SetCursor(9, 2);
				LCD_PrintString(enter_win_percent);

				if (enter_win_percent_index == 2) {
					temp_win_rate = custom_parse_win_rate(enter_win_percent);
					if (temp_win_rate > 0) {
						new_win_rate_percent = temp_win_rate;
					}
				}
				enter_win_percent_index++;
				
				osDelay(250);
				break;
			}
			break;

		case SlotIdle:
			ui_draw_menu();
			osDelay(250);
			break;
		default:
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
void elevate_control_task() {
	osThreadSetPriority(main_displayHandle, osPriorityLow);
	osThreadSetPriority(controlDisplayHandle, osPriorityNormal);
}
void elevate_main_display_task() {
	osThreadSetPriority(main_displayHandle, osPriorityNormal);
	osThreadSetPriority(controlDisplayHandle, osPriorityLow);
}
void ui_draw_startup_sequence() {
	LCD_SetCursor(0, 0);
	LCD_PrintString(" OOO O    OOO  OOO  ");
	LCD_SetCursor(0, 1);
	LCD_PrintString(" OO  O   O   O  O   ");
	LCD_SetCursor(0, 2);
	LCD_PrintString("  OO O   O   O  O   ");
	LCD_SetCursor(0, 3);
	LCD_PrintString(" OOO OOO  OOO   O   ");
}
void ui_draw_password() {
	LCD_SetCursor(0, 0);
	LCD_PrintString("#-----Password-----#");
	LCD_SetCursor(0, 1);
	LCD_PrintString("#                  #");
	LCD_SetCursor(0, 2);
	LCD_PrintString("#                  #");
	LCD_SetCursor(0, 3);
	LCD_PrintString("#------------------#");
}
void ui_draw_menu() {
	LCD_SetCursor(0, 0);
	LCD_PrintString("     1) Audio       ");
	LCD_SetCursor(0, 1);
	LCD_PrintString("     2) Stats       ");
	LCD_SetCursor(0, 2);
	LCD_PrintString("     3) Win rate    ");
	LCD_SetCursor(0, 3);
	LCD_PrintString("  Made by Alexander ");
}
void ui_draw_audio() {
	// control lose music and volume

	LCD_SetCursor(0, 0);
	LCD_PrintString("#-------Audio------#");
	ui_draw_lose_music();
	ui_draw_volume();
	LCD_SetCursor(0, 3);
	LCD_PrintString("#--A-OK------------#");
}
void ui_draw_volume() {
	char volume_str[21];
	snprintf(volume_str, sizeof(volume_str), "#+2 -3 volume:  %02d #",
			slot_audio_settings.volume);
	LCD_SetCursor(0, 2);
	LCD_PrintString(volume_str);
}
void ui_draw_lose_music() {
	LCD_SetCursor(0, 1);
	if (slot_audio_settings.lose_music_on) {
		LCD_PrintString("#1) lose tune: on  #");
	} else {
		LCD_PrintString("#1) lose tune: off #");
	}
}
void ui_draw_stats() {
	// display slot_stats and the actual win rate
	char actual_win_rate_str[21];
	char total_games_str[21];
	uint8_t actual_win_rate_percent =
			(uint8_t) (
			((float) slot_stats.session_won_games
					/ slot_stats.session_total_games) * 100);
	snprintf(actual_win_rate_str, sizeof(actual_win_rate_str),
			"#  win rate:    %02d #",
			actual_win_rate_percent);
	snprintf(total_games_str, sizeof(total_games_str),
			"# total games: %03d #",
			slot_stats.session_total_games);
	LCD_SetCursor(0, 0);
	LCD_PrintString("#------Stats.------#");
	LCD_SetCursor(0, 1);
	LCD_PrintString(actual_win_rate_str);
	LCD_SetCursor(0, 2);
	LCD_PrintString(total_games_str);
	LCD_SetCursor(0, 3);
	LCD_PrintString("#--A-OK------------#");
}
void ui_draw_win_rate() {
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
uint32_t slot_rand(void) {
	prng_state = (1664525 * prng_state) + 1013904223;
	return prng_state;
}
void slot_srand(uint32_t seed) {
	if (seed == 0)
		prng_state = 1;
	else
		prng_state = seed;
}

uint32_t get_random_number(uint32_t min_value, uint32_t max_value) {
	// inclusive of max_value
	uint32_t range = max_value - min_value + 1;
	return (slot_rand() % range) + min_value;
}

/* USER CODE END Application */

