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
#include "esp_system.h"
#include "esp_chip_info.h"
#include "esp_psram.h"
#include "esp_flash.h"
#include "nvs_flash.h"
#include "esp_log.h"

// RTOS
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// BSP
#include "led.h"
#include "lcd.h"
#include "spi.h"
#include "spi_sdcard.h"

void app_main(void)
{
    esp_err_t ret;
    size_t bytes_total, bytes_free;                     /* Total and free space of the SD card */

    ret = nvs_flash_init();                             /* Initialize NVS */

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    led_init();                                         /* Initialize LED */
    spi2_init();                                        /* Initialize SPI */
    lcd_init();                                         /* Initialize LCD */

    while (sd_spi_init())                               /* SD card not detected */
    {
        lcd_show_string(0, 0, 200, 16, 16, "SD Card Error!", RED);
        vTaskDelay(500);
        lcd_show_string(0, 20, 200, 16, 16, "Please Check!", RED);
        vTaskDelay(500);
    }

    lcd_show_string(0, 0, 200, 16, 16, "SD Card OK!", RED);
    lcd_show_string(0, 20, 200, 16, 16, "Total:       MB", RED);
    lcd_show_string(0, 40, 200, 16, 16, "Free :       MB", RED);
    sd_get_fatfs_usage(&bytes_total, &bytes_free);

    lcd_show_num(60, 20, (int)bytes_total / 1024, 5, 16, BLUE);
    lcd_show_num(60, 40, (int)bytes_free / 1024, 5, 16, BLUE);

    while (1)
    {
        led_toggle();
        vTaskDelay(500);
    }
}