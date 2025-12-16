# 定时器 代码

## 组件架构

```plaintext
- driver
    - node_timer
        - include
            - node_timer.h
        - node_timer.c
        - CMakeLists.txt
```

## driver/node_timer/CMakeLists.txt

```cmake
set(src_dirs
    .
)

set(include_dirs
    include
)

set(requires
    driver
    esp_driver_gpio
    esp_timer
    node_led
)

idf_component_register(SRC_DIRS ${src_dirs} INCLUDE_DIRS ${include_dirs} REQUIRES ${requires})
```

!!! note
    注意，在驱动程序中，我们使用了 ESP-IDF 内置的 `driver` 库中的 gpio，因此，我们需要在 `CMakeLists.txt` 文件的 `REQUIRES` 字段中指定此依赖项。
    此外，我们使用LED来体现外部中断的效果，因此我们需要在`REQUIRES`字段中添加`node_led`。 
    再有，对于定时器，我们需要在`REQUIRES`字段中添加`esp_timer`。

## node_timer.h
    
```c
/**
 * @file node_timer.h
 * @author SHUAIWEN CUI (SHUAIWEN001@e.ntu.edu.sg)
 * @brief This file contains the function prototypes for the node_timer component.
 * @version 1.0
 * @date 2025-10-21
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

/* Dependencies */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "node_led.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /* Function Prototypes */

    /**
     * @brief       Initialize a high-precision timer (ESP_TIMER)
     * @param       tps: Timer period in microseconds (μs). For example, to execute the timer interrupt once every second,
     *                   set tps = 1s = 1000000μs.
     * @retval      None
     */
    void esptim_int_init(uint64_t tps);

    /**
     * @brief       Timer callback function
     * @param       arg: No arguments passed
     * @retval      None
     */
    void esptim_callback(void *arg);

#ifdef __cplusplus
}
#endif
```

## node_timer.c

```c
/**
 * @file node_timer.c
 * @author SHUAIWEN CUI (SHUAIWEN001@e.ntu.edu.sg)
 * @brief This file contains the implementation of the node_timer component.
 * @version 1.0
 * @date 2025-10-21
 *
 * @copyright Copyright (c) 2024
 *
 */

#include "node_timer.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief       Initialize a high-precision timer (ESP_TIMER)
     * @param       tps: Timer period in microseconds (μs). For example, to execute the timer interrupt once every second,
     *                   set tps = 1s = 1000000μs.
     * @retval      None
     */
    void esptim_int_init(uint64_t tps)
    {
        esp_timer_handle_t esp_tim_handle; /* Timer callback function handle */

        /* Define a timer configuration structure */
        esp_timer_create_args_t tim_periodic_arg = {
            .callback = &esptim_callback, /* Set the callback function */
            .arg = NULL,                  /* No arguments passed */
        };

        esp_timer_create(&tim_periodic_arg, &esp_tim_handle); /* Create a timer event */
        esp_timer_start_periodic(esp_tim_handle, tps);        /* Trigger periodically based on the timer period */
    }

    /**
     * @brief       Timer callback function
     * @param       arg: No arguments passed
     * @retval      None
     */
    void esptim_callback(void *arg)
    {
        led_toggle();
    }

#ifdef __cplusplus
}
#endif
```

## main.c

```c
/**
 * @file main.c
 * @author SHUAIWEN CUI (SHUAIWEN001@e.ntu.edu.sg)
 * @brief
 * @version 1.0
 * @date 2025-08-20
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

// FreeRTOS
#include "freertos/FreeRTOS.h" // ESP32 FreeRTOS
#include "freertos/task.h"     // ESP32 FreeRTOS Task

// BSP
#include "node_led.h"
#include "node_exit.h"
#include "node_timer.h"

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
    esptim_int_init(1000000);

    while (1)
    {
        ESP_LOGI(TAG, "Hello World!");
        vTaskDelay(1000);
    }
}


```

## main.cpp

```cpp

/**
 * @file main.cpp
 * @author SHUAIWEN CUI (SHUAIWEN001@e.ntu.edu.sg)
 * @brief Main entry for AIoTNode
 * @version 1.0
 * @date 2025-08-20
 * @copyright Copyright (c) 2024
 */

// ESP-IDF headers
#include "esp_system.h"    // ESP32 System
#include "nvs_flash.h"     // ESP32 NVS
#include "esp_chip_info.h" // ESP32 Chip Info
#include "esp_psram.h"     // ESP32 PSRAM
#include "esp_flash.h"     // ESP32 Flash
#include "esp_log.h"       // ESP32 Logging

// FreeRTOS
#include "freertos/FreeRTOS.h" // ESP32 FreeRTOS
#include "freertos/task.h"     // ESP32 FreeRTOS Task

// BSP
#include "node_led.h"
#include "node_exit.h"
#include "node_timer.h"

const char *TAG = "AIoTNode";

extern "C" void app_main(void)
{
    esp_err_t ret;
    uint32_t flash_size;
    esp_chip_info_t chip_info;

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
    esptim_int_init(1000000);

    while (1)
    {
        ESP_LOGI(TAG, "Hello World!");
        vTaskDelay(1000);
    }
}

```