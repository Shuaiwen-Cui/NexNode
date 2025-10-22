/**
 * @file node_i2c.h
 * @author SHUAIWEN CUI (SHUAIWEN001@e.ntu.edu.sg)
 * @brief This file contains the function prototypes for i2c master initialization. This is to serve the peripherals that require I2C communication.
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

#include <stdio.h>
#include "esp_log.h"
#include "driver/i2c.h"
#include "esp_system.h"

#define I2C_MASTER_SCL_IO 10       /*!< gpio number for I2C master clock */
#define I2C_MASTER_SDA_IO 11       /*!< gpio number for I2C master data  */
#define I2C_MASTER_NUM I2C_NUM_0  /*!< I2C port number for master dev */
#define I2C_MASTER_FREQ_HZ 100000 /*!< I2C master clock frequency */

   /**
    * @brief i2c master initialization
    * @return esp_err_t ESP_OK on success, error code on failure
    */
   esp_err_t i2c_bus_init(void);

   /**
    * @brief i2c master deinitialization
    * @return esp_err_t ESP_OK on success, error code on failure
    */
   esp_err_t i2c_bus_deinit(void);

#ifdef __cplusplus
}
#endif
