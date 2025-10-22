/**
 * @file node_i2c.c
 * @author SHUAIWEN CUI (SHUAIWEN001@e.ntu.edu.sg)
 * @brief This file contains the function prototypes for i2c master initialization. This is to serve the peripherals that require I2C communication.
 * @version 1.0
 * @date 2025-10-22
 *
 * @copyright Copyright (c) 2025
 *
 */
#include "node_i2c.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief i2c master initialization
     * @return esp_err_t ESP_OK on success, error code on failure
     */
    esp_err_t i2c_bus_init(void)
    {
        esp_err_t ret;
        i2c_config_t conf = {
            .mode = I2C_MODE_MASTER,
            .sda_io_num = I2C_MASTER_SDA_IO,
            .sda_pullup_en = GPIO_PULLUP_ENABLE,
            .scl_io_num = I2C_MASTER_SCL_IO,
            .scl_pullup_en = GPIO_PULLUP_ENABLE,
            .master.clk_speed = I2C_MASTER_FREQ_HZ,
            // ESP-IDF 6.0: clk_flags is deprecated, use default clock source
        };

        ret = i2c_param_config(I2C_MASTER_NUM, &conf);
        if (ret != ESP_OK)
        {
            ESP_LOGE("I2C", "I2C config failed: %s", esp_err_to_name(ret));
            return ret;
        }

        ret = i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
        if (ret != ESP_OK)
        {
            ESP_LOGE("I2C", "I2C install failed: %s", esp_err_to_name(ret));
            return ret;
        }

        ESP_LOGI("I2C", "I2C master initialized successfully");
        return ESP_OK;
    }

    /**
     * @brief i2c master deinitialization
     * @return esp_err_t ESP_OK on success, error code on failure
     */
    esp_err_t i2c_bus_deinit(void)
    {
        esp_err_t ret = i2c_driver_delete(I2C_MASTER_NUM);
        if (ret != ESP_OK)
        {
            ESP_LOGE("I2C", "I2C delete failed: %s", esp_err_to_name(ret));
            return ret;
        }

        ESP_LOGI("I2C", "I2C master deinitialized successfully");
        return ESP_OK;
    }

#ifdef __cplusplus
}
#endif