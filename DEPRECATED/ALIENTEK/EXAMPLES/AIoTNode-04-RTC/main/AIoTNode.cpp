/**
 * @file main.cpp
 * @author SHUAIWEN CUI (SHUAIWEN001@e.ntu.edu.sg)
 * @brief Main entry for AIoTNode
 * @version 1.0
 * @date 2025-08-20
 * @copyright Copyright (c) 2024
 */

// ESP-IDF headers
#include "esp_system.h"    // ESP32 System
#include "nvs_flash.h"     // ESP32 NVS
#include "esp_chip_info.h" // ESP32 Chip Info
#include "esp_psram.h"     // ESP32 PSRAM
#include "esp_flash.h"     // ESP32 Flash
#include "esp_log.h"       // ESP32 Logging

// FreeRTOS
#include "freertos/FreeRTOS.h" // ESP32 FreeRTOS
#include "freertos/task.h"     // ESP32 FreeRTOS Task

// BSP
#include "node_led.h"
#include "node_exit.h"
#include "node_rtc.h"

const char *weekdays[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saterday"};

extern "C" void app_main(void)
{
    esp_err_t ret;
    uint8_t tbuf[40];
    uint8_t t = 0;

    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    led_init();
    rtc_set_time(2025, 10, 21, 22, 24, 00);

    while (1)
    {
        t++;

        if ((t % 10) == 0)
        {
            rtc_get_time();
            sprintf((char *)tbuf, "Time:%02d:%02d:%02d", calendar.hour, calendar.min, calendar.sec);
            printf("Time:%02d:%02d:%02d\r\n", calendar.hour, calendar.min, calendar.sec);
            sprintf((char *)tbuf, "Date:%04d-%02d-%02d", calendar.year, calendar.month, calendar.date);
            printf("Date:%02d-%02d-%02d\r\n", calendar.year, calendar.month, calendar.date);
            sprintf((char *)tbuf, "Week:%s", weekdays[calendar.week - 1]);
        }

        if ((t % 20) == 0)
        {
            led_toggle();
        }

        vTaskDelay(100);
    }
}