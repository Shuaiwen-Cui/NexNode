# LED 代码

## 组件架构

```plaintext
- driver
    - node_led
        - include
            - node_led.h
        - node_led.c
        - CMakeLists.txt
```

## driver/node_led/CMakeLists.txt

```cmake
set(src_dirs
    .
)

set(include_dirs
    include
)

set(requires
    driver
)

idf_component_register(SRC_DIRS ${src_dirs} INCLUDE_DIRS ${include_dirs} REQUIRES ${requires})
```

!!! note
    注意，在驱动程序中，我们使用了 ESP-IDF 内置的 `driver` 库中的 gpio，因此，我们需要在 `CMakeLists.txt` 文件的 `REQUIRES` 字段中指定此依赖项。

## NODE_LED.h
    
```c
/**
 * @file node_led.h
 * @author SHUAIWEN CUI (SHUAIWEN001@e.ntu.edu.sg)
 * @brief Led initialization and control.
 * @version 1.1
 * @date 2025-10-21
 *
 * @copyright Copyright (c) 2025
 */

#pragma once

/* Dependencies */
#include "driver/gpio.h"

/* GPIO Pin Definition */
#define LED_GPIO_PIN GPIO_NUM_1 /* GPIO port connected to LED */

/* GPIO States */
#define LED_PIN_RESET 0
#define LED_PIN_SET 1

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief       Initialize the LED
     * @param       None
     * @retval      None
     */
    void led_init(void);

    /**
     * @brief       Control the LED
     * @param       x: true for on, false for off
     * @retval      None
     */
    void led(bool x);

    /**
     * @brief       Toggle the LED
     * @param       None
     * @retval      None
     */
    void led_toggle(void);

#ifdef __cplusplus
}
#endif

```

## NODE_LED.c

```c
/**
 * @file node_led.c
 * @author SHUAIWEN CUI (SHUAIWEN001@e.ntu.edu.sg)
 * @brief Led initialization and control.
 * @version 1.1
 * @date 2025-10-21
 *
 * @copyright Copyright (c) 2025
 */

/* Dependencies */
#include "node_led.h"

#ifdef __cplusplus
extern "C"
{
#endif

  /**
   * @brief       Initialize the LED
   * @param       None
   * @retval      None
   */
  void led_init(void)
  {
    gpio_config_t gpio_init_struct = {0};

    gpio_init_struct.intr_type = GPIO_INTR_DISABLE;        /* Disable GPIO interrupt */
    gpio_init_struct.mode = GPIO_MODE_INPUT_OUTPUT;        /* Set GPIO mode to input-output */
    gpio_init_struct.pull_up_en = GPIO_PULLUP_ENABLE;      /* Enable pull-up resistor */
    gpio_init_struct.pull_down_en = GPIO_PULLDOWN_DISABLE; /* Disable pull-down resistor */
    gpio_init_struct.pin_bit_mask = 1ull
                                    << LED_GPIO_PIN; /* Set pin bit mask for the configured pin */
    gpio_config(&gpio_init_struct);                  /* Configure GPIO */

    led(true); /* Turn on the LED */
  }

  /**
   * @brief       Control the LED
   * @param       x: true for on, false for off
   * @retval      None
   */
  void led(bool x)
  {
    if (x)
    {
      gpio_set_level(LED_GPIO_PIN, LED_PIN_RESET);
    }
    else
    {
      gpio_set_level(LED_GPIO_PIN, LED_PIN_SET);
    }
  }

  /**
   * @brief       Toggle the LED
   * @param       None
   * @retval      None
   */
  void led_toggle(void)
  {
    gpio_set_level(LED_GPIO_PIN, !gpio_get_level(LED_GPIO_PIN));
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
 * @version 1.1
 * @date 2025-10-21
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

    while (1)
    {
        led_toggle();
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
 * @version 1.1
 * @date 2025-10-21
 * @copyright Copyright (c) 2024
 */

// ESP-IDF headers
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_chip_info.h"
#include "esp_psram.h"
#include "esp_flash.h"
#include "esp_log.h"

// FreeRTOS
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// BSP
#include "node_led.h"

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
        ESP_ERROR_CHECK(nvs_flash_erase());
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

    while (1)
    {
        led_toggle();
        ESP_LOGI(TAG, "Hello World!");
        vTaskDelay(1000);
    }
}
```