#ifndef __I2C_H__
#define __I2C_H__

#include <stdio.h>
#include "esp_log.h"
#include "unity.h"
#include "driver/i2c.h"
#include "esp_system.h"

#define I2C_MASTER_SCL_IO 4      /*!< gpio number for I2C master clock */
#define I2C_MASTER_SDA_IO 5      /*!< gpio number for I2C master data  */
#define I2C_MASTER_NUM I2C_NUM_0  /*!< I2C port number for master dev */
#define I2C_MASTER_FREQ_HZ 100000 /*!< I2C master clock frequency */

/**
 * @brief i2c master initialization
 */
void i2c_bus_init(void);

#endif /* __I2C_H__ */