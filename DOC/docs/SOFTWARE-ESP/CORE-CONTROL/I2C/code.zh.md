# I2C 代码

## 组件架构

```plaintext
- driver
    - node_i2c
        - include
            - node_i2c.h
        - node_i2c.c
        - CMakeLists.txt
```

## driver/node_i2c/CMakeLists.txt

```cmake
set(src_dirs
    .
)

set(include_dirs
    include
)

set(requires
    driver
    log
)

idf_component_register(SRC_DIRS ${src_dirs} INCLUDE_DIRS ${include_dirs} REQUIRES ${requires})
```

!!! note
    注意，在驱动程序中，我们使用了 ESP-IDF 内置的 `driver` 库中的i2c和gpio相关函数，因此，我们需要在 `CMakeLists.txt` 文件的 `REQUIRES` 字段中指明这个依赖关系。

## node_i2c.h

```c
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

```

## node_i2c.c

```c
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
```

## main.c

```c
Refer to the codes for i2c peripherals.
```

!!! note
    请注意，本章需要与其他使用i2c的部件联合使用，才能看到效果。在使用基于i2c的外设时，请确保您已经正确配置和初始化了i2c总线。(例如在主程序中调用`i2c_bus_init()`函数)