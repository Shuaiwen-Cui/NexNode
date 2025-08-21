/**
 * @file led.h
 * @author SHUAIWEN CUI (SHUAIWEN001@e.ntu.edu.sg)
 * @brief The LED here indicates the onboard red LED on the Alientek DNESP32S3M
 * minimal development board.
 * @version 1.1
 * @date 2025-08-20
 *
 * @copyright Copyright (c) 2025
 */

#pragma once

/* Dependencies */
#include "driver/gpio.h"

/* GPIO Pin Definition */
#define LED_GPIO_PIN GPIO_NUM_1 /* GPIO port connected to LED */

/* GPIO States */
#define LED_PIN_RESET 0
#define LED_PIN_SET 1

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief       Initialize the LED
 * @param       None
 * @retval      None
 */
void led_init(void);

/**
 * @brief       Control the LED
 * @param       x: true for on, false for off
 * @retval      None
 */
void led(bool x);

/**
 * @brief       Toggle the LED
 * @param       None
 * @retval      None
 */
void led_toggle(void);

#ifdef __cplusplus
}
#endif
