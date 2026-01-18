/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define display_CS_Pin GPIO_PIN_6
#define display_CS_GPIO_Port GPIOE
#define PC14_OSC32_IN_Pin GPIO_PIN_14
#define PC14_OSC32_IN_GPIO_Port GPIOC
#define PC15_OSC32_OUT_Pin GPIO_PIN_15
#define PC15_OSC32_OUT_GPIO_Port GPIOC
#define PH0_OSC_IN_Pin GPIO_PIN_0
#define PH0_OSC_IN_GPIO_Port GPIOH
#define PH1_OSC_OUT_Pin GPIO_PIN_1
#define PH1_OSC_OUT_GPIO_Port GPIOH
#define OTG_FS_PowerSwitchOn_Pin GPIO_PIN_0
#define OTG_FS_PowerSwitchOn_GPIO_Port GPIOC
#define PDM_OUT_Pin GPIO_PIN_3
#define PDM_OUT_GPIO_Port GPIOC
#define stop_led_0_Pin GPIO_PIN_0
#define stop_led_0_GPIO_Port GPIOA
#define stop_btn_0_Pin GPIO_PIN_1
#define stop_btn_0_GPIO_Port GPIOA
#define stop_led_1_Pin GPIO_PIN_2
#define stop_led_1_GPIO_Port GPIOA
#define stop_btn_1_Pin GPIO_PIN_3
#define stop_btn_1_GPIO_Port GPIOA
#define stop_led_2_Pin GPIO_PIN_4
#define stop_led_2_GPIO_Port GPIOA
#define stop_btn_2_Pin GPIO_PIN_5
#define stop_btn_2_GPIO_Port GPIOA
#define stop_led_3_Pin GPIO_PIN_6
#define stop_led_3_GPIO_Port GPIOA
#define stop_btn_3_Pin GPIO_PIN_7
#define stop_btn_3_GPIO_Port GPIOA
#define CLK_IN_Pin GPIO_PIN_10
#define CLK_IN_GPIO_Port GPIOB
#define keyboard_col_0_Pin GPIO_PIN_8
#define keyboard_col_0_GPIO_Port GPIOD
#define keyboard_row_3_Pin GPIO_PIN_9
#define keyboard_row_3_GPIO_Port GPIOD
#define keyboard_row_2_Pin GPIO_PIN_10
#define keyboard_row_2_GPIO_Port GPIOD
#define keyboard_row_1_Pin GPIO_PIN_11
#define keyboard_row_1_GPIO_Port GPIOD
#define keyboard_row_0_Pin GPIO_PIN_12
#define keyboard_row_0_GPIO_Port GPIOD
#define keyboard_col_3_Pin GPIO_PIN_13
#define keyboard_col_3_GPIO_Port GPIOD
#define keyboard_col_2_Pin GPIO_PIN_14
#define keyboard_col_2_GPIO_Port GPIOD
#define keyboard_col_1_Pin GPIO_PIN_15
#define keyboard_col_1_GPIO_Port GPIOD
#define VBUS_FS_Pin GPIO_PIN_9
#define VBUS_FS_GPIO_Port GPIOA
#define OTG_FS_ID_Pin GPIO_PIN_10
#define OTG_FS_ID_GPIO_Port GPIOA
#define OTG_FS_DM_Pin GPIO_PIN_11
#define OTG_FS_DM_GPIO_Port GPIOA
#define SWDIO_Pin GPIO_PIN_13
#define SWDIO_GPIO_Port GPIOA
#define SWCLK_Pin GPIO_PIN_14
#define SWCLK_GPIO_Port GPIOA
#define display_reset_Pin GPIO_PIN_15
#define display_reset_GPIO_Port GPIOA
#define I2S3_SCK_Pin GPIO_PIN_10
#define I2S3_SCK_GPIO_Port GPIOC
#define display_wait_Pin GPIO_PIN_3
#define display_wait_GPIO_Port GPIOD
#define Audio_RST_Pin GPIO_PIN_4
#define Audio_RST_GPIO_Port GPIOD
#define OTG_FS_OverCurrent_Pin GPIO_PIN_5
#define OTG_FS_OverCurrent_GPIO_Port GPIOD
#define SWO_Pin GPIO_PIN_3
#define SWO_GPIO_Port GPIOB
#define display_backlight_Pin GPIO_PIN_8
#define display_backlight_GPIO_Port GPIOB
#define display_interrupt_Pin GPIO_PIN_0
#define display_interrupt_GPIO_Port GPIOE

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
