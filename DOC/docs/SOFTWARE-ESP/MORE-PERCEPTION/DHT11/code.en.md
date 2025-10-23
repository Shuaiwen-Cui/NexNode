# DHT11 CODE

## COMPONENT STRUCTURE

```plaintxt
- driver
    - node_th_dht11
        - include
            - node_th_dht11.h
        - node_th_dht11.c
        - CMakeLists.txt

```

## drvier/node_th_dht1/CMakeLists.txt

```cmake
set(src_dirs
    .
)

set(include_dirs
    include
)

set(requires
    driver
)

idf_component_register(SRC_DIRS ${src_dirs} INCLUDE_DIRS ${include_dirs} REQUIRES ${requires})

```

## node_th_dht11.h

```c
/**
   Copyright 2024 Achim Pieters | StudioPieters®

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all
   copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NON INFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

   for more information visit https://www.studiopieters.nl
 **/

#pragma once

#include <driver/gpio.h>
#include <esp_err.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef enum
    {
        DHT_TYPE_DHT11 = 0,
        DHT_TYPE_AM2301,
        DHT_TYPE_SI7021
    } dht_sensor_type_t;

    esp_err_t dht_read_data(dht_sensor_type_t sensor_type, gpio_num_t pin, int16_t *humidity, int16_t *temperature);
    esp_err_t dht_read_float_data(dht_sensor_type_t sensor_type, gpio_num_t pin, float *humidity, float *temperature);

#ifdef __cplusplus
}
#endif

```

## node_th_dht11.c

```c
/**
   Copyright 2024 Achim Pieters | StudioPieters®

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all
   copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NON INFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

   for more information visit https://www.studiopieters.nl
 **/

#include "node_th_dht11.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string.h>
#include <esp_log.h>
#include <esp_rom_sys.h>
#include <driver/gpio.h>

// DHT timer precision in microseconds
#define DHT_TIMER_INTERVAL 1
#define DHT_DATA_BITS 40
#define DHT_DATA_BYTES (DHT_DATA_BITS / 8)

static const char *TAG = "dht";

static portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
#define PORT_ENTER_CRITICAL() portENTER_CRITICAL(&mux)
#define PORT_EXIT_CRITICAL() portEXIT_CRITICAL(&mux)

#define CHECK_ARG(VAL)                  \
    do                                  \
    {                                   \
        if (!(VAL))                     \
            return ESP_ERR_INVALID_ARG; \
    } while (0)

#define CHECK_LOGE(x, msg, ...)                \
    do                                         \
    {                                          \
        esp_err_t __err = (x);                 \
        if (__err != ESP_OK)                   \
        {                                      \
            ESP_LOGE(TAG, msg, ##__VA_ARGS__); \
            return __err;                      \
        }                                      \
    } while (0)

static esp_err_t dht_await_pin_state(gpio_num_t pin, uint32_t timeout, int expected_pin_state, uint32_t *duration)
{
    gpio_set_direction(pin, GPIO_MODE_INPUT);
    for (uint32_t i = 0; i < timeout; i += DHT_TIMER_INTERVAL)
    {
        esp_rom_delay_us(DHT_TIMER_INTERVAL);
        if (gpio_get_level(pin) == expected_pin_state)
        {
            if (duration)
                *duration = i;
            return ESP_OK;
        }
    }
    return ESP_ERR_TIMEOUT;
}

static esp_err_t dht_fetch_data(dht_sensor_type_t sensor_type, gpio_num_t pin, uint8_t data[DHT_DATA_BYTES])
{
    uint32_t low_duration, high_duration;
    const int max_retries = 3;
    
    for (int retry = 0; retry < max_retries; retry++)
    {
        // Reset GPIO state
        gpio_set_direction(pin, GPIO_MODE_OUTPUT_OD);
        gpio_set_level(pin, 1);
        esp_rom_delay_us(1000); // Wait 1ms for stabilization
        
        // Start signal
        gpio_set_level(pin, 0);
        esp_rom_delay_us(sensor_type == DHT_TYPE_SI7021 ? 500 : 20000);
        gpio_set_level(pin, 1);

        // Wait for sensor response
        esp_err_t result = dht_await_pin_state(pin, 40, 0, NULL);
        if (result != ESP_OK) {
            ESP_LOGW(TAG, "Phase 'B' timeout, retry %d/%d", retry + 1, max_retries);
            continue;
        }
        
        result = dht_await_pin_state(pin, 88, 1, NULL);
        if (result != ESP_OK) {
            ESP_LOGW(TAG, "Phase 'C' timeout, retry %d/%d", retry + 1, max_retries);
            continue;
        }
        
        result = dht_await_pin_state(pin, 88, 0, NULL);
        if (result != ESP_OK) {
            ESP_LOGW(TAG, "Phase 'D' timeout, retry %d/%d", retry + 1, max_retries);
            continue;
        }

        // Read data bits
        bool data_valid = true;
        for (int i = 0; i < DHT_DATA_BITS; i++)
        {
            result = dht_await_pin_state(pin, 65, 1, &low_duration);
            if (result != ESP_OK) {
                ESP_LOGW(TAG, "Low duration timeout at bit %d, retry %d/%d", i, retry + 1, max_retries);
                data_valid = false;
                break;
            }
            
            result = dht_await_pin_state(pin, 75, 0, &high_duration);
            if (result != ESP_OK) {
                ESP_LOGW(TAG, "High duration timeout at bit %d, retry %d/%d", i, retry + 1, max_retries);
                data_valid = false;
                break;
            }

            uint8_t byte_index = i / 8;
            uint8_t bit_index = i % 8;
            if (!bit_index)
                data[byte_index] = 0;

            data[byte_index] |= (high_duration > low_duration) << (7 - bit_index);
        }
        
        if (data_valid) {
            return ESP_OK;
        }
        
        // Wait before retry
        esp_rom_delay_us(2000);
    }
    
    return ESP_ERR_TIMEOUT;
}

static inline int16_t dht_convert_data(dht_sensor_type_t sensor_type, uint8_t msb, uint8_t lsb)
{
    int16_t result;
    if (sensor_type == DHT_TYPE_DHT11)
    {
        result = msb * 10;
    }
    else
    {
        result = (msb & 0x7F) << 8 | lsb;
        if (msb & 0x80)
            result = -result;
    }
    return result;
}

esp_err_t dht_read_data(dht_sensor_type_t sensor_type, gpio_num_t pin, int16_t *humidity, int16_t *temperature)
{
    CHECK_ARG(humidity || temperature);

    uint8_t data[DHT_DATA_BYTES] = {0};

    gpio_set_direction(pin, GPIO_MODE_OUTPUT_OD);
    gpio_set_level(pin, 1);

    esp_err_t result = dht_fetch_data(sensor_type, pin, data);

    gpio_set_direction(pin, GPIO_MODE_OUTPUT_OD);
    gpio_set_level(pin, 1);

    if (result != ESP_OK)
        return result;

    if (data[4] != ((data[0] + data[1] + data[2] + data[3]) & 0xFF))
    {
        ESP_LOGE(TAG, "Checksum failed");
        return ESP_ERR_INVALID_CRC;
    }

    if (humidity)
        *humidity = dht_convert_data(sensor_type, data[0], data[1]);
    if (temperature)
        *temperature = dht_convert_data(sensor_type, data[2], data[3]);

    ESP_LOGD(TAG, "Humidity: %d, Temperature: %d", *humidity, *temperature);

    return ESP_OK;
}

esp_err_t dht_read_float_data(dht_sensor_type_t sensor_type, gpio_num_t pin, float *humidity, float *temperature)
{
    CHECK_ARG(humidity || temperature);

    int16_t int_humidity, int_temperature;
    esp_err_t result = dht_read_data(sensor_type, pin, humidity ? &int_humidity : NULL, temperature ? &int_temperature : NULL);
    if (result != ESP_OK)
        return result;

    if (humidity)
        *humidity = int_humidity / 10.0f;
    if (temperature)
        *temperature = int_temperature / 10.0f;

    return ESP_OK;
}

```

## main.c

```c
/**
 * @file main.c
 * @author SHUAIWEN CUI (SHUAIWEN001@e.ntu.edu.sg)
 * @brief 
 * @version 1.0
 * @date 2024-11-17
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
#include "node_lcd.h"
#include "node_timer.h"
#include "node_rtc.h"
#include "node_sdcard.h"
#include "node_wifi.h"
#include "node_mqtt.h"
#include "node_th_dht11.h"

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

    // DHT SENSOR
    dht_sensor_type_t sensor_type;
    sensor_type = DHT_TYPE_DHT11;
    gpio_num_t gpio_num = (gpio_num_t)14;

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
    lcd_init();

    // spiffs_test();                                                  /* Run SPIFFS test */
    while (sd_card_init())                               /* SD card not detected */
    {
        lcd_show_string(0, 0, 200, 16, 16, "SD Card Error!", RED);
        vTaskDelay(500);
        lcd_show_string(0, 20, 200, 16, 16, "Please Check!", RED);
        vTaskDelay(500);
    }

    // clean the screen
    lcd_clear(WHITE);

    lcd_show_string(0, 0, 200, 16, 16, "SD Initialized!", RED);

    sd_card_test_filesystem();                                        /* Run SD card test */

    lcd_show_string(0, 0, 200, 16, 16, "SD Tested CSW! ", RED);

    // sd_card_unmount();

    vTaskDelay(3000);

    lcd_show_string(0, 0, lcd_self.width, 16, 16, "WiFi STA Test  ", RED);
    
    ret = wifi_sta_wpa2_init();
    if(ret == ESP_OK)
    {
        ESP_LOGI(TAG_WIFI, "WiFi STA Init OK");
        lcd_show_string(0, 0, lcd_self.width, 16, 16, "WiFi STA Test OK", RED);
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

    // DHT - Configure GPIO with pull-up
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT_OUTPUT_OD;
    io_conf.pin_bit_mask = (1ULL << gpio_num);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);

    while (1)
    {
        float humidity = 0, temperature = 0;
        esp_err_t result = dht_read_float_data(sensor_type, gpio_num, &humidity, &temperature);
        if (result == ESP_OK)
        {
            ESP_LOGI(TAG, "Humidity: %.1f%% Temperature: %.1f°C", humidity, temperature);
            char mqtt_msg[128];
            snprintf(mqtt_msg, sizeof(mqtt_msg), "{\"temperature\":%.1f,\"humidity\":%.1f}", temperature, humidity);
            if (s_is_mqtt_connected)
            {
                esp_mqtt_client_publish(s_mqtt_client, MQTT_PUBLISH_TOPIC, mqtt_msg, strlen(mqtt_msg), 1, 0);
            }
        }
        else
        {
            ESP_LOGE(TAG, "Failed to read sensor data: %s", esp_err_to_name(result));
        }
        vTaskDelay(pdMS_TO_TICKS(2000)); // Delay for 2 seconds
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
 * @date 2024-11-17
 *
 * @copyright Copyright (c) 2024
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

// BSP
#include "node_led.h"
#include "node_exit.h"
#include "node_spi.h"
#include "node_lcd.h"
#include "node_timer.h"
#include "node_rtc.h"
#include "node_sdcard.h"
#include "node_wifi.h"
#include "node_mqtt.h"
#include "node_th_dht11.h"

/* Variables */
const char *TAG = "AIoTNode";

extern "C"
{

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

        // DHT SENSOR
        dht_sensor_type_t sensor_type;
        sensor_type = DHT_TYPE_DHT11;
        gpio_num_t gpio_num = (gpio_num_t)14;

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
        lcd_init();

        // spiffs_test();                                                  /* Run SPIFFS test */
        while (sd_card_init()) /* SD card not detected */
        {
            lcd_show_string(0, 0, 200, 16, 16, (char *)"SD Card Error!", RED);
            vTaskDelay(500);
            lcd_show_string(0, 20, 200, 16, 16, (char *)"Please Check!", RED);
            vTaskDelay(500);
        }

        // clean the screen
        lcd_clear(WHITE);

        lcd_show_string(0, 0, 200, 16, 16, (char *)"SD Initialized!", RED);

        sd_card_test_filesystem(); /* Run SD card test */

        lcd_show_string(0, 0, 200, 16, 16, (char *)"SD Tested CSW! ", RED);

        // sd_card_unmount();

        vTaskDelay(3000);

        lcd_show_string(0, 0, lcd_self.width, 16, 16, (char *)"WiFi STA Test  ", RED);

        ret = wifi_sta_wpa2_init();
        if (ret == ESP_OK)
        {
            ESP_LOGI(TAG_WIFI, "WiFi STA Init OK");
            lcd_show_string(0, 0, lcd_self.width, 16, 16, (char *)"WiFi STA Test OK", RED);
        }
        else
        {
            ESP_LOGE(TAG_WIFI, "WiFi STA Init Failed");
        }

        // only when the ip is obtained, start mqtt
        EventBits_t ev = 0;
        ev = xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, pdTRUE, pdFALSE, portMAX_DELAY);
        if (ev & CONNECTED_BIT)
        {
            mqtt_app_start();
        }

        // DHT - Configure GPIO with pull-up
        gpio_config_t io_conf = {};
        io_conf.intr_type = GPIO_INTR_DISABLE;
        io_conf.mode = GPIO_MODE_INPUT_OUTPUT_OD;
        io_conf.pin_bit_mask = (1ULL << gpio_num);
        io_conf.pull_down_en = (gpio_pulldown_t)0;
        io_conf.pull_up_en = (gpio_pullup_t)1;
        gpio_config(&io_conf);

        while (1)
        {
            float humidity = 0, temperature = 0;
            esp_err_t result = dht_read_float_data(sensor_type, gpio_num, &humidity, &temperature);
            if (result == ESP_OK)
            {
                ESP_LOGI(TAG, "Humidity: %.1f%% Temperature: %.1f°C", humidity, temperature);
                char mqtt_msg[128];
                snprintf(mqtt_msg, sizeof(mqtt_msg), "{\"temperature\":%.1f,\"humidity\":%.1f}", temperature, humidity);
                if (s_is_mqtt_connected)
                {
                    esp_mqtt_client_publish(s_mqtt_client, MQTT_PUBLISH_TOPIC, mqtt_msg, strlen(mqtt_msg), 1, 0);
                }
            }
            else
            {
                ESP_LOGE(TAG, "Failed to read sensor data: %s", esp_err_to_name(result));
            }
            vTaskDelay(pdMS_TO_TICKS(2000)); // Delay for 2 seconds
        }

    }

} // extern "C"
```