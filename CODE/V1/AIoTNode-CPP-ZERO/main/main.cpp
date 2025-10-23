// Use C++ headers where appropriate and keep ESP-IDF C APIs
#include <cstdio>
#include <cinttypes>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_system.h"
#include "esp_chip_info.h"
#include "esp_psram.h"
#include "esp_flash.h"
#include "esp_log.h"

static const char *TAG = "main";

/**
 * Entry point for ESP-IDF applications when using C++.
 * app_main must use C linkage so the runtime can find it.
 */
extern "C" void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    uint32_t flash_size = 0;
    esp_chip_info_t chip_info{}; // value-initialize

    // Get FLASH size and chip info
    ESP_ERROR_CHECK(esp_flash_get_size(nullptr, &flash_size));
    esp_chip_info(&chip_info);

    // Log system info
    ESP_LOGI(TAG, "CPU Cores: %d", chip_info.cores);
    ESP_LOGI(TAG, "Flash size: %u MB", flash_size / (1024 * 1024));
    ESP_LOGI(TAG, "PSRAM size: %zu bytes", esp_psram_get_size());

    while (true)
    {
        ESP_LOGI(TAG, "Hello-ESP32");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}