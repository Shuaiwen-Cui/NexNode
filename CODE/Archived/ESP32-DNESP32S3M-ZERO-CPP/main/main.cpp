// main.cpp
#include <cinttypes>
#include <cstdio>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "nvs_flash.h"
#include "esp_system.h"
#include "esp_chip_info.h"
#include "esp_psram.h"
#include "esp_flash.h"
#include "esp_log.h"

static const char* TAG = "main_cpp";

extern "C" void app_main(void)
{
    esp_err_t ret;
    uint32_t flash_size = 0;
    esp_chip_info_t chip_info{};
    
    // Initialize NVS
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase()); // Erase NVS if required
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Get flash size and chip information
    ESP_ERROR_CHECK(esp_flash_get_size(nullptr, &flash_size));
    esp_chip_info(&chip_info);

    // Print CPU core count
    ESP_LOGI(TAG, "CPU Cores: %d", chip_info.cores);

    // Print flash size in MB
    ESP_LOGI(TAG, "Flash size: %lu MB",
             static_cast<unsigned long>(flash_size / (1024UL * 1024UL)));

    // Print PSRAM size in bytes (0 if no PSRAM available)
    size_t psram_bytes = esp_psram_get_size();
    ESP_LOGI(TAG, "PSRAM size: %lu bytes",
             static_cast<unsigned long>(psram_bytes));

    // Main loop: print hello message every second
    while (true) {
        ESP_LOGI(TAG, "Hello-ESP32");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
