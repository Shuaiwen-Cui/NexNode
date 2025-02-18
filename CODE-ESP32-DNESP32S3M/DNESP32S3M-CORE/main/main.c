/**
 * @file main.c
 * @author
 * @brief Main application to demonstrate the use of ESP32 internal temperature sensor
 * @version 1.0
 * @date 2024-11-17
 *
 * @copyright Copyright (c) 2024
 *
 */

/* Dependencies */
// Basic
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_system.h"
#include "esp_chip_info.h"
#include "esp_psram.h"
#include "esp_flash.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include <sys/stat.h>

// RTOS
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// BSP
#include "led.h"
#include "exit.h"
#include "lcd.h"
#include "spi.h"
#include "esp_rtc.h"
#include "rng.h"
#include "spi_sdcard.h"
#include "fs.h"

/**
 * @brief       Program entry point
 * @param       None
 * @retval      None
 */
void app_main(void)
{
    esp_err_t ret;

    ret = nvs_flash_init();                                         /* Initialize NVS */

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret);

    led_init();                                                     /* Initialize LED */
    spi2_init();                                                    /* Initialize SPI */
    lcd_init();                                                     /* Initialize LCD */
    spiffs_init("storage", DEFAULT_MOUNT_POINT, DEFAULT_FD_NUM);    /* Initialize SPIFFS */

    /* Display test information */
    lcd_show_string(0, 0, 200, 16, 16, "SPIFFS TEST", RED);
    lcd_show_string(0, 20, 200, 16, 16, "Read file:", BLUE);

    spiffs_test();                                                  /* Run SPIFFS test */

    while (1)
    {
        led_toggle();
        vTaskDelay(500);
    }
}