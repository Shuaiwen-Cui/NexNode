/**
 * @file main.cpp
 * @author SHUAIWEN CUI (SHUAIWEN001@e.ntu.edu.sg)
 * @brief 
 * @version 1.0
 * @date 2025-10-22
 * 
 * @copyright Copyright (c) 2024
 * 
 */

/* DEPENDENCIES */
// ESP
#include "esp_system.h" // ESP32 System
#include "nvs_flash.h"  // ESP32 NVS
#include "esp_chip_info.h" // ESP32 Chip Info
#include "esp_psram.h" // ESP32 PSRAM
#include "esp_flash.h" // ESP32 Flash
#include "esp_log.h" // ESP32 Logging

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// BSP
#include "node_led.h"
#include "node_exit.h"
#include "node_spi.h"
#include "node_timer.h"
#include "node_rtc.h"
#include "node_sdcard.h"
#include "node_wifi.h"
#include "node_mqtt.h"
#include "node_rgb.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Variables */
const char *TAG = "AIoTNode";

/**
 * @brief Entry point of the program
 * @param None
 * @retval None
 */
void app_main(void)
{
    esp_err_t ret;
    uint32_t flash_size;
    esp_chip_info_t chip_info;

    char mqtt_pub_buff[64];
    int count = 0;

    static TickType_t s_rgb_last_tick = 0;
    static uint8_t s_rgb_marquee_i = 0;

    /* Single-LED marquee: cycle named colors (~220 ms per step). */
    static const char *const k_rgb_marquee[] = {
        "red", "orange", "yellow", "green", "cyan", "blue", "purple", "pink",
    };
    const size_t k_rgb_marquee_n = sizeof(k_rgb_marquee) / sizeof(k_rgb_marquee[0]);

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

    // BSP Initialization
    led_init();
    exit_init();
    spi2_init();

    node_rgb_config_t rgb_cfg = NODE_RGB_CONFIG_DEFAULT();
    rgb_cfg.gpio_num = NODE_RGB_GPIO_DEFAULT;
    rgb_cfg.num_leds = 1;
    ret = node_rgb_init(&rgb_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "node_rgb_init failed: %s", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "node_rgb: GPIO%u marquee demo (named colors)", (unsigned)NODE_RGB_GPIO_DEFAULT);
        node_rgb_chase_reset();
        (void)node_rgb_str(k_rgb_marquee[0]);
        s_rgb_last_tick = xTaskGetTickCount();
        s_rgb_marquee_i = 1;
    }

    // spiffs_test();                                                  /* Run SPIFFS test */
    while (sd_card_init())                               /* SD card not detected */
    {
        ESP_LOGW(TAG, "SD Card Error!");
        vTaskDelay(500);
        ESP_LOGW(TAG, "Please Check!");
        vTaskDelay(500);
    }

    ESP_LOGI(TAG, "SD Initialized!");

    sd_card_test_filesystem();                                        /* Run SD card test */

    ESP_LOGI(TAG, "SD Tested CSW!");

    // sd_card_unmount();

    vTaskDelay(3000);

    ESP_LOGI(TAG_WIFI, "WiFi STA Test");

    ret = wifi_sta_wpa2_init();
    if(ret == ESP_OK)
    {
        ESP_LOGI(TAG_WIFI, "WiFi STA Init OK");
        ESP_LOGI(TAG_WIFI, "WiFi STA Test OK");
    }
    else
    {
        ESP_LOGE(TAG_WIFI, "WiFi STA Init Failed");
    }

    // only when the ip is obtained, start mqtt
    EventBits_t ev = 0;
    ev = xEventGroupWaitBits(wifi_event_group,CONNECTED_BIT,pdTRUE,pdFALSE,portMAX_DELAY);
    if(ev & CONNECTED_BIT)
    {
        mqtt_app_start();
    }

    while (1)
    {
        if(s_is_mqtt_connected)
        {
            snprintf(mqtt_pub_buff,64,"{\"count\":\"%d\"}",count);
            esp_mqtt_client_publish(s_mqtt_client, MQTT_PUBLISH_TOPIC,
                            mqtt_pub_buff, strlen(mqtt_pub_buff),1, 0);
            count++;
        }
        led_toggle();

        /* RGB marquee: advance by tick, independent of the 1 s loop delay below. */
        if (node_rgb_is_initialized()) {
            const TickType_t rgb_step = pdMS_TO_TICKS(220);
            TickType_t now = xTaskGetTickCount();
            if (s_rgb_last_tick != 0 && (now - s_rgb_last_tick) >= rgb_step) {
                s_rgb_last_tick = now;
                const char *c = k_rgb_marquee[s_rgb_marquee_i % k_rgb_marquee_n];
                if (node_rgb_str(c) != ESP_OK) {
                    node_rgb_rgb(24, 24, 24);
                }
                s_rgb_marquee_i++;
            }
        }

        ESP_LOGI(TAG, "Hello World!");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

#ifdef __cplusplus
}
#endif