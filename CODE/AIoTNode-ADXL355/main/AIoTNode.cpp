/**
 * @file AIoTNode.cpp
 * @brief Application entry: ADXL355 typical usage demo (SPI, init, WHOAMI, DRDY-synced read, periodic polling).
 */
#include "nvs_flash.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "node_spi.h"
#include "node_acc_adxl355.h"

#ifdef __cplusplus
extern "C" {
#endif

static const char *TAG = "AIoTNode";

/** Number of DRDY interrupt-synchronized samples (for demo; switches to polling afterward). */
#ifndef AIOTNODE_ADXL355_DRDY_SAMPLES
#define AIOTNODE_ADXL355_DRDY_SAMPLES 5
#endif

/** Polling print interval (ms). */
#ifndef AIOTNODE_ADXL355_POLL_MS
#define AIOTNODE_ADXL355_POLL_MS 2000
#endif

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    spi3_init();

    node_acc_adxl355_config_t cfg;
    node_acc_adxl355_config_default_eval(&cfg);
    cfg.log_info_on_init = true;

    node_acc_adxl355_dev_t dev = {};
    ret = node_acc_adxl355_init(&dev, &cfg, NODE_ACC_ADXL355_RANGE_2G, NODE_ACC_ADXL355_ODR_125_HZ);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ADXL355 init failed: %s", esp_err_to_name(ret));
        for (;;) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    node_acc_adxl355_ids_t ids = {};
    ret = node_acc_adxl355_read_ids(&dev, &ids);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "WHOAMI: AD=0x%02X MST=0x%02X PART=0x%02X REV=0x%02X", ids.devid_ad, ids.devid_mst,
                 ids.partid, ids.revid);
    }

    /* Typical streaming acquisition: DRDY edge-synchronized read of XYZ + temperature (matching eval board GPIO6 wiring). */
    ret = node_acc_adxl355_drdy_isr_install(&dev);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "DRDY-synchronized samples (%d):", AIOTNODE_ADXL355_DRDY_SAMPLES);
        for (int i = 0; i < AIOTNODE_ADXL355_DRDY_SAMPLES; i++) {
            ret = node_acc_adxl355_drdy_wait(&dev, 150);
            if (ret != ESP_OK) {
                ESP_LOGW(TAG, "drdy_wait[%d]: %s", i, esp_err_to_name(ret));
                break;
            }
            node_acc_adxl355_raw_xyz_t raw = {};
            float tc = 0.0f;
            (void)node_acc_adxl355_read_raw_xyz(&dev, &raw);
            (void)node_acc_adxl355_read_temp_c(&dev, &tc);
            ESP_LOGI(TAG, "  [%d] raw xyz: %ld, %ld, %ld  T: %.2f C", i, (long)raw.x, (long)raw.y, (long)raw.z,
                     (double)tc);
        }
        (void)node_acc_adxl355_drdy_isr_remove(&dev);
    } else {
        ESP_LOGW(TAG, "DRDY ISR not installed (%s); skip DRDY demo", esp_err_to_name(ret));
    }

    ESP_LOGI(TAG, "Polling every %d ms (STATUS + raw xyz + T). Ctrl+C monitor to stop.", AIOTNODE_ADXL355_POLL_MS);

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(AIOTNODE_ADXL355_POLL_MS));

        uint8_t status = 0;
        node_acc_adxl355_raw_xyz_t raw = {};
        float tc = 0.0f;
        ret = node_acc_adxl355_read_status(&dev, &status);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "read_status: %s", esp_err_to_name(ret));
            continue;
        }
        ret = node_acc_adxl355_read_raw_xyz(&dev, &raw);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "read_raw_xyz: %s", esp_err_to_name(ret));
            continue;
        }
        ret = node_acc_adxl355_read_temp_c(&dev, &tc);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "read_temp_c: %s", esp_err_to_name(ret));
            continue;
        }

        ESP_LOGI(TAG, "STATUS=0x%02X (DRDY=%u) | xyz raw %ld, %ld, %ld | %.2f C", (unsigned)status,
                 (unsigned)(status & NODE_ACC_ADXL355_REG_STATUS_DATA_RDY), (long)raw.x, (long)raw.y, (long)raw.z,
                 (double)tc);
    }
}

#ifdef __cplusplus
}
#endif
