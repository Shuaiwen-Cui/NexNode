# 代码

## 组件结构

```plaintext
- driver
    - node_mqtt
        - include
            - node_mqtt.h
        - node_mqtt.c
        - CMakeLists.txt
```

## driver/node_mqtt/CMakeLists.txt

```cmake
set(src_dirs
    .
)

set(include_dirs
    include
)

set(requires
    mqtt
)

idf_component_register(SRC_DIRS ${src_dirs} INCLUDE_DIRS ${include_dirs} REQUIRES ${requires})
```

!!! note
    可以看到这里的依赖关系十分简单，只有mqtt。

## node_mqtt.h

```c
/**
 * @file node_mqtt.h
 * @author SHUAIWEN CUI (SHUAIWEN001@e.ntu.edu.sg)
 * @brief This file contains the function prototypes for mqtt connection.
 * @version 1.0
 * @date 2025-10-22
 *
 * @copyright Copyright (c) 2025
 *
 */

#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

/* Dependencies */
#include <stdio.h>
#include "esp_log.h"
#include "mqtt_client.h"

/* Macros */
#define MQTT_ADDRESS "<YOUR MQTT SERVER>" // MQTT Broker URL
#define MQTT_PORT 1883                      // MQTT Broker Port
#define MQTT_CLIENT "<YOUR CLIENT ID>"     // Client ID (Unique for devices)
#define MQTT_USERNAME "<YOUR MQTT USERNAME>"            // MQTT Username
#define MQTT_PASSWORD "<YOUR MQTT PASSWORD>"           // MQTT Password

#define MQTT_PUBLISH_TOPIC "<YOUR PUBLISH TOPIC>"      // publish topic
#define MQTT_SUBSCRIBE_TOPIC "<YOUR SUBSCRIBE TOPIC>" // subscribe topic

    /* Variables */
    extern const char *TAG_MQTT; // tag for logging
    extern esp_mqtt_client_handle_t s_mqtt_client;
    extern bool s_is_mqtt_connected;

    /* Function Prototypes */
    /**
     * @brief MQTT client initialization and connection
     */
    void mqtt_app_start(void);

#ifdef __cplusplus
}
#endif

```

## node_mqtt.c

```c
/**
 * @file node_mqtt.c
 * @author SHUAIWEN CUI (SHUAIWEN001@e.ntu.edu.sg)
 * @brief This file contains the function prototypes for mqtt connection.
 * @version 1.0
 * @date 2025-10-22
 *
 * @copyright Copyright (c) 2025
 *
 */

#ifdef __cplusplus
extern "C"
{
#endif

#include "node_mqtt.h"

    /* Macros */

    /* Variables */
    const char *TAG_MQTT = "NODE-MQTT";            // tag for logging
    esp_mqtt_client_handle_t s_mqtt_client = NULL; // MQTT client handle
    bool s_is_mqtt_connected = false;              // MQTT connection status flag

    /* Function Prototypes */

    /**
     * @brief MQTT event handler
     *
     * @param event_handler_arg Argument passed to the event handler
     * @param event_base Event base identifier
     * @param event_id Event identifier
     * @param event_data Event-specific data
     */
    void aliot_mqtt_event_handler(void *event_handler_arg,
                                  esp_event_base_t event_base,
                                  int32_t event_id,
                                  void *event_data)
    {
        esp_mqtt_event_handle_t event = event_data;
        esp_mqtt_client_handle_t client = event->client;

        switch ((esp_mqtt_event_id_t)event_id)
        {
        case MQTT_EVENT_CONNECTED: // Connection established
            ESP_LOGI(TAG_MQTT, "MQTT connected");
            // Subscribe to the test topic upon successful connection
            esp_mqtt_client_subscribe_single(client, MQTT_SUBSCRIBE_TOPIC, 1);
            break;
        case MQTT_EVENT_DISCONNECTED: // Connection disconnected
            ESP_LOGI(TAG_MQTT, "MQTT disconnected");
            break;
        case MQTT_EVENT_SUBSCRIBED: // Subscription successful
            ESP_LOGI(TAG_MQTT, "MQTT subscribed, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED: // Unsubscription successful
            ESP_LOGI(TAG_MQTT, "MQTT unsubscribed, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED: // Publish acknowledgment received
            ESP_LOGI(TAG_MQTT, "MQTT published ack, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA: // Data received
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);
            break;
        case MQTT_EVENT_ERROR: // Error event
            ESP_LOGI(TAG_MQTT, "MQTT event error");
            break;
        default:
            ESP_LOGI(TAG_MQTT, "Unhandled MQTT event id: %ld", event_id);
            break;
        }
    }

    /**
     * @brief MQTT client initialization and connection
     */
    void mqtt_app_start(void)
    {
        esp_err_t ret;

        esp_mqtt_client_config_t mqtt_cfg = {0};
        mqtt_cfg.broker.address.uri = MQTT_ADDRESS;
        mqtt_cfg.broker.address.port = MQTT_PORT;

        // Client ID
        mqtt_cfg.credentials.client_id = MQTT_CLIENT;

        // Username
        mqtt_cfg.credentials.username = MQTT_USERNAME;

        // Password
        mqtt_cfg.credentials.authentication.password = MQTT_PASSWORD;

        ESP_LOGI(TAG_MQTT, "Connecting to MQTT broker...");

        // Initialize MQTT client with provided configuration
        esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);

        // Register MQTT event handler
        esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, aliot_mqtt_event_handler, client);

        // Start MQTT client
        ret = esp_mqtt_client_start(client);
        if (ret == ESP_OK)
        {
            ESP_LOGI(TAG_MQTT, "MQTT client started");
            s_mqtt_client = client;
            s_is_mqtt_connected = true;
        }
        else
        {
            ESP_LOGE(TAG_MQTT, "MQTT client start failed");
        }
    }

#ifdef __cplusplus
}
#endif

```

## main.c

```c
/**
 * @file main.c
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

// BSP
#include "node_led.h"
#include "node_exit.h"
#include "node_spi.h"
#include "node_timer.h"
#include "node_rtc.h"
#include "node_sdcard.h"
#include "node_wifi.h"
#include "node_mqtt.h"

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

        ESP_LOGI(TAG, "Hello World!");
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}
```

## main.cpp

```cpp
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

// BSP
#include "node_led.h"
#include "node_exit.h"
#include "node_spi.h"
#include "node_timer.h"
#include "node_rtc.h"
#include "node_sdcard.h"
#include "node_wifi.h"
#include "node_mqtt.h"

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

        ESP_LOGI(TAG, "Hello World!");
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

#ifdef __cplusplus
}
#endif

```