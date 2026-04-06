/**
 * @file AIoTNode.cpp
 */
#include "nvs_flash.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "node_i2c.h"
#include "node_acc_adxl367_test.h"

#ifdef __cplusplus
extern "C" {
#endif

static const char *TAG = "AIoTNode";

#ifndef AIOTNODE_BOOT_RUN_ADXL367_STATS
#define AIOTNODE_BOOT_RUN_ADXL367_STATS 0
#endif

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(i2c_bus_init());
#if AIOTNODE_BOOT_RUN_ADXL367_STATS
    ret = node_acc_adxl367_run_xyz_stats_capture();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "XYZ stats failed: %s", esp_err_to_name(ret));
    }
#endif
    node_acc_adxl367_run_phase6_default_app();
}

#ifdef __cplusplus
}
#endif
