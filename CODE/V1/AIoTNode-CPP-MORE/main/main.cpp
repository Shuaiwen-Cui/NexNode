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