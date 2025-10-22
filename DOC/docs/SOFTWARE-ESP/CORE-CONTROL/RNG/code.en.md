# RNG CODE

## Component Architecture

```plaintext
- driver
    - node_rng
        - include
            - node_rng.h
        - node_rng.c
        - CMakeLists.txt
```

## driver/node_rng/CMakeLists.txt

```cmake
set(src_dirs
    .
)

set(include_dirs
    include
)

set(requires
)

idf_component_register(SRC_DIRS ${src_dirs} INCLUDE_DIRS ${include_dirs} REQUIRES ${requires})
```

## node_rng.h
    
```c
/**
 * @file node_rng.h
 * @author
 * @brief This is the header file for the node_rng component.
 * @version 1.0
 * @date 2025-10-21
 * @ref Alientek RNG Driver
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <sys/param.h>
#include "esp_attr.h"
#include "esp_cpu.h"
#include "soc/wdev_reg.h"
#include "esp_random.h"
#include "esp_private/esp_clk.h"

/* Function Declarations */

/**
 * @brief       Get a random number
 * @param       None
 * @retval      Random number (32-bit)
 */
uint32_t rng_get_random_num(void); /* Get a random number */

/**
 * @brief       Get a random number within a specific range
 * @param       min,max: Minimum and maximum values
 * @retval      Random number (rval), satisfying: min <= rval <= max
 */
int rng_get_random_range(int min, int max); /* Get a random number within a specific range */

#ifdef __cplusplus
}
#endif

```

## node_rng.c

```c
/**
 * @file node_rng.c
 * @author
 * @brief This is the source file for the node_rng component.
 * @version 1.0
 * @date 2025-10-21
 * @ref Alientek RNG Driver
 * @copyright Copyright (c) 2024
 *
 */

#include "node_rng.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief       Get a random number
 * @param       None
 * @retval      Random number (32-bit)
 */
uint32_t rng_get_random_num(void)
{
    uint32_t randomnum;

    randomnum = esp_random();

    return randomnum;
}

/**
 * @brief       Get a random number within a specific range
 * @param       min,max: Minimum and maximum values
 * @retval      Random number (rval), satisfying: min <= rval <= max
 */
int rng_get_random_range(int min, int max)
{
    uint32_t randomnum;

    randomnum = esp_random();

    return randomnum % (max - min + 1) + min;
}

#ifdef __cplusplus
}
#endif
```

## main.c

```c
/**
 * @file main.c
 * @author
 * @brief Main application to demonstrate the use of ESP32 internal temperature sensor
 * @version 1.1
 * @date 2025-10-21
 *
 * @copyright Copyright (c) 2024
 *
 */

/* Dependencies */
// Basic
#include "esp_system.h"
#include "esp_chip_info.h"
#include "esp_psram.h"
#include "esp_flash.h"
#include "nvs_flash.h"
#include "esp_log.h"

// RTOS
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// BSP
#include "node_led.h"
#include "node_exit.h"
#include "node_rng.h"

void app_main(void)
{
    uint32_t random1, random2;
    esp_err_t ret;
    
    ret = nvs_flash_init();                                         /* Initialize NVS */

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    led_init();                                                     /* Initialize LED */

    while(1)
    {
        random1 = rng_get_random_num();
        printf("Random number 1: %ld\n", random1);
        random2 = rng_get_random_range(0, 9);
        printf("Random number 2: %ld\n", random2);
        led_toggle();
        vTaskDelay(1000);
    }
}
```

## main.cpp

```cpp
/**
 * @file main.c
 * @author
 * @brief Main application to demonstrate the use of ESP32 internal temperature sensor
 * @version 1.1
 * @date 2025-10-21
 *
 * @copyright Copyright (c) 2024
 *
 */

/* Dependencies */
// Basic
#include "esp_system.h"
#include "esp_chip_info.h"
#include "esp_psram.h"
#include "esp_flash.h"
#include "nvs_flash.h"
#include "esp_log.h"

// RTOS
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// BSP
#include "node_led.h"
#include "node_exit.h"
#include "node_rng.h"

extern "C" void app_main(void)
{
    uint32_t random1, random2;
    esp_err_t ret;

    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    led_init();

    while (1)
    {
        random1 = rng_get_random_num();
        printf("Random number 1: %ld\n", random1);
        random2 = rng_get_random_range(0, 9);
        printf("Random number 2: %ld\n", random2);
        led_toggle();
        vTaskDelay(1000);
    }
}
```