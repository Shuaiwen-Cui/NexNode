/**
 * @file main.cpp
 * @author SHUAIWEN CUI (SHUAIWEN001@e.ntu.edu.sg)
 * @brief This file serves as the entry point for the whole project.
 * @version 1.0
 * @date 2025-04-19
 *
 * @copyright Copyright (c) 2025
 *
 */

/* DEPENDENCIES */
// ESP
#include "esp_system.h"    // ESP32 System
#include "nvs_flash.h"     // ESP32 NVS
#include "esp_chip_info.h" // ESP32 Chip Info
#include "esp_psram.h"     // ESP32 PSRAM
#include "esp_flash.h"     // ESP32 Flash
#include "esp_log.h"       // ESP32 Logging

// Standard Libraries
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>

// FreeRTOS
#include "freertos/FreeRTOS.h"     // FreeRTOS
#include "freertos/task.h"         // FreeRTOS Task
#include "freertos/event_groups.h" // FreeRTOS Event Groups
#include "freertos/queue.h"        // FreeRTOS Queue
#include "freertos/timers.h"       // FreeRTOS Timers
#include "freertos/semphr.h"       // FreeRTOS Semaphore

// BSP

// BSP-Main Control
#include "exit.h"
#include "tim.h"
#include "esp_rtc.h"
#include "rng.h"
#include "spi.h"
#include "i2c.h"
#include "spi_sdcard.h"

// BSP-Communication
#include "wifi_wpa2_enterprise.h"
#include "mqtt.h"

// BSP-Perception
#include "mpu6050.h"

// BSP-Actuation
#include "led.h"
#include "lcd.h"

// Middleware Test
#include "mylib.hpp"
#include "cswlib.h"  // 只包含这个即可

// TinyAuton

// TinySH

/* NAMESPACES */
using namespace std;

/* TAGS */
const char *TAG_NODE = "NODE";

/* ENTRY POINT */
/**
 * @brief Entry point of the program
 * @param None
 * @retval None
 */
extern "C" void app_main(void)
{
    // Node Initialization Start
    esp_err_t ret;
    uint32_t flash_size;
    esp_chip_info_t chip_info;

    char mqtt_pub_buff[64];
    int count = 0;

    // Initialize NVS
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase()); // Erase if needed
        ret = nvs_flash_init();
    }

    // Get FLASH size
    esp_flash_get_size(NULL, &flash_size);
    esp_chip_info(&chip_info);

    // Display CPU core count
    printf("CPU Cores: %d\n", chip_info.cores);

    // Display FLASH size
    printf("Flash size: %ld MB flash\n", flash_size / (1024 * 1024));

    // Display PSRAM size
    printf("PSRAM size: %d bytes\n", esp_psram_get_size());

    /* BSP */
    led_init();                // Actuation - LED
    exit_init();               // Actuation - External Interrupt
    esptim_int_init(1000000);  // Main Control - Timer, callback is to toggle LED
    spi2_init();               // Main Control - SPI
    lcd_init();                // Actuation - LCD
    i2c_bus_init();            // Main Control - I2C
    i2c_sensor_mpu6050_init(); // Perception - MPU6050

    // spiffs_test();                                                  /* Run SPIFFS test */
    while (sd_card_init()) /* SD card not detected */
    {
        lcd_show_string(0, 0, 200, 16, 16, "SD Card Error!", RED);
        vTaskDelay(500);
        lcd_show_string(0, 20, 200, 16, 16, "Please Check!", RED);
        vTaskDelay(500);
    }

    // clean the screen
    lcd_clear(WHITE);

    lcd_show_string(0, 0, 200, 16, 16, "SD Initialized!", RED);

    sd_card_test_filesystem(); /* Run SD card test */

    lcd_show_string(0, 0, 200, 16, 16, "SD Tested CSW! ", RED);

    // sd_card_unmount();

    vTaskDelay(1000);

    lcd_show_string(0, 0, lcd_self.width, 16, 16, "WiFi STA Test  ", RED);

    ret = wifi_sta_wpa2_init();
    if (ret == ESP_OK)
    {
        ESP_LOGI(TAG_WIFI, "WiFi STA Init OK");
        lcd_show_string(0, 0, lcd_self.width, 16, 16, "WiFi STA Test OK", RED);
    }
    else
    {
        ESP_LOGE(TAG_WIFI, "WiFi STA Init Failed");
    }

    // only when the ip is obtained, start time sync and mqtt
    EventBits_t ev = 0;
    ev = xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, pdTRUE, pdFALSE, portMAX_DELAY);

    if (ev & CONNECTED_BIT)
    {
        mqtt_app_start();
        vTaskDelay(3000 / portTICK_PERIOD_MS); // wait for mqtt to connect
    }

    // ✅ 直接使用 C++ 类
    CswTest obj;
    obj.say_hello();

    while (1)
    {
        // cout << "Hello World!" << endl;

        mylib::say_hello();

        // time delay
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
