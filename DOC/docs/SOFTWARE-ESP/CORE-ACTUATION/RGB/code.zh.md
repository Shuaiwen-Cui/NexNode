# RGB LED 代码

## 组件架构

```plaintext
- driver
    - node_rgb
        - include
            - node_rgb.h
        - node_rgb.c
        - CMakeLists.txt
```

## driver/node_rgb/CMakeLists.txt

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
)

idf_component_register(SRC_DIRS ${src_dirs} INCLUDE_DIRS ${include_dirs} REQUIRES ${requires})
```

!!! note
    注意，在驱动程序中，我们使用了 ESP-IDF 内置的 `driver` 库中的 gpio，因此，我们需要在 `CMakeLists.txt` 文件的 `REQUIRES` 字段中指定此依赖项。

## node_rgb.h
    
```c
/**
 * @file node_rgb.h
 * @brief SK6812MINI-C (SK6812-class) one-wire RGB: low-level fill, clear, optional chase step.
 *
 * No FreeRTOS tasks or delays here; use vTaskDelay (etc.) in the application for timing and animation.
 */

#pragma once

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Data line GPIO */
#define NODE_RGB_GPIO_DEFAULT (3)

typedef struct {
    int gpio_num;
    int num_leds;
    uint8_t brightness; /**< 0–255 scale */
} node_rgb_config_t;

#define NODE_RGB_CONFIG_DEFAULT()           \
    {                                       \
        .gpio_num = NODE_RGB_GPIO_DEFAULT,  \
        .num_leds = 1,                      \
        .brightness = 96,                   \
    }

esp_err_t node_rgb_init(const node_rgb_config_t *cfg);
void node_rgb_deinit(void);
bool node_rgb_is_initialized(void);

void node_rgb_set_brightness(uint8_t v);

/** Solid color across the strip and refresh */
void node_rgb_fill(uint8_t r, uint8_t g, uint8_t b);

void node_rgb_clear(void);

/**
 * Advance chase by one step (multi-LED: moving bright spot; single LED: cycles R→G→B internally).
 * Call rate is controlled by the application.
 */
void node_rgb_chase_step(uint8_t r, uint8_t g, uint8_t b);

/** Reset chase position (e.g. restart multi-LED chase from the beginning) */
void node_rgb_chase_reset(void);

#ifdef __cplusplus
}
#endif

```

## node_rgb.c

```c
/**
 * @file node_rgb.c
 * @brief SK6812MINI-C via espressif/led_strip (RMT); no RTOS usage inside this module.
 */

#include "node_rgb.h"
#include "led_strip.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "node_rgb";

static led_strip_handle_t s_strip = NULL;
static int s_num_leds = 1;
static uint8_t s_brightness = 96;
static uint32_t s_chase_pos = 0;
static uint8_t s_single_phase = 0;
static bool s_inited = false;

static inline uint8_t scale(uint8_t c)
{
    return (uint8_t)(((uint16_t)c * s_brightness) / 255);
}

static void apply_fill(uint8_t r, uint8_t g, uint8_t b)
{
    if (!s_strip) {
        return;
    }
    r = scale(r);
    g = scale(g);
    b = scale(b);
    for (int i = 0; i < s_num_leds; i++) {
        led_strip_set_pixel(s_strip, (uint32_t)i, r, g, b);
    }
    led_strip_refresh(s_strip);
}

esp_err_t node_rgb_init(const node_rgb_config_t *cfg)
{
    if (s_inited) {
        return ESP_OK;
    }
    if (!cfg || cfg->gpio_num < 0) {
        return ESP_ERR_INVALID_ARG;
    }

    s_num_leds = cfg->num_leds;
    if (s_num_leds < 1) {
        s_num_leds = 1;
    }
    if (s_num_leds > 256) {
        s_num_leds = 256;
    }
    s_brightness = cfg->brightness ? cfg->brightness : 96;

    led_strip_config_t strip_cfg = {
        .strip_gpio_num = cfg->gpio_num,
        .max_leds = (uint32_t)s_num_leds,
        .led_pixel_format = LED_PIXEL_FORMAT_GRB,
        .led_model = LED_MODEL_SK6812,
        .flags.invert_out = false,
    };

    led_strip_rmt_config_t rmt_cfg = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000,
        .flags.with_dma = false,
    };

    esp_err_t err = led_strip_new_rmt_device(&strip_cfg, &rmt_cfg, &s_strip);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "led_strip_new_rmt_device: %s", esp_err_to_name(err));
        return err;
    }

    led_strip_clear(s_strip);
    s_chase_pos = 0;
    s_single_phase = 0;
    s_inited = true;
    ESP_LOGI(TAG, "SK6812 GPIO%d x %d, brightness %u", cfg->gpio_num, s_num_leds, (unsigned)s_brightness);
    return ESP_OK;
}

void node_rgb_deinit(void)
{
    if (s_strip) {
        led_strip_clear(s_strip);
        led_strip_del(s_strip);
        s_strip = NULL;
    }
    s_inited = false;
}

bool node_rgb_is_initialized(void)
{
    return s_inited;
}

void node_rgb_set_brightness(uint8_t v)
{
    s_brightness = v;
}

void node_rgb_fill(uint8_t r, uint8_t g, uint8_t b)
{
    apply_fill(r, g, b);
}

void node_rgb_clear(void)
{
    if (s_strip) {
        led_strip_clear(s_strip);
    }
}

void node_rgb_chase_reset(void)
{
    s_chase_pos = 0;
    s_single_phase = 0;
}

void node_rgb_chase_step(uint8_t r, uint8_t g, uint8_t b)
{
    if (!s_strip) {
        return;
    }

    if (s_num_leds <= 1) {
        uint8_t rr = 0, gg = 0, bb = 0;
        switch (s_single_phase % 3) {
            case 0:
                rr = r;
                break;
            case 1:
                gg = g;
                break;
            default:
                bb = b;
                break;
        }
        s_single_phase++;
        apply_fill(rr, gg, bb);
        return;
    }

    uint8_t R = scale(r);
    uint8_t G = scale(g);
    uint8_t B = scale(b);

    for (int i = 0; i < s_num_leds; i++) {
        led_strip_set_pixel(s_strip, (uint32_t)i, 0, 0, 0);
    }

    int idx = (int)(s_chase_pos % (uint32_t)s_num_leds);
    led_strip_set_pixel(s_strip, (uint32_t)idx, R, G, B);

    int t1 = (idx - 1 + s_num_leds) % s_num_leds;
    int t2 = (idx - 2 + s_num_leds) % s_num_leds;
    led_strip_set_pixel(s_strip, (uint32_t)t1, R / 4, G / 4, B / 4);
    led_strip_set_pixel(s_strip, (uint32_t)t2, R / 16, G / 16, B / 16);

    s_chase_pos++;
    led_strip_refresh(s_strip);
}


```

## main.cpp

```cpp
/**
 * @file main.cpp
 * @author SHUAIWEN CUI (SHUAIWEN001@e.ntu.edu.sg)
 * @brief 
 * @version 1.0
 * @date 2025-10-22
 * 
 * @copyright Copyright (c) 2024
 * 
 */

/* DEPENDENCIES */
// ESP
#include "esp_system.h" // ESP32 System
#include "nvs_flash.h"  // ESP32 NVS
#include "esp_chip_info.h" // ESP32 Chip Info
#include "esp_psram.h" // ESP32 PSRAM
#include "esp_flash.h" // ESP32 Flash
#include "esp_log.h" // ESP32 Logging

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// BSP
#include "node_led.h"
#include "node_exit.h"
#include "node_spi.h"
#include "node_timer.h"
#include "node_rtc.h"
#include "node_sdcard.h"
#include "node_wifi.h"
#include "node_mqtt.h"
#include "node_rgb.h"

#ifdef __cplusplus
extern "C" {
#endif

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

    char mqtt_pub_buff[64];
    int count = 0;

    static TickType_t s_rgb_last_tick = 0;
    static uint8_t s_rgb_idx = 0;

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
    spi2_init();

    node_rgb_config_t rgb_cfg = NODE_RGB_CONFIG_DEFAULT();
    rgb_cfg.gpio_num = NODE_RGB_GPIO_DEFAULT;
    rgb_cfg.num_leds = 1;
    ret = node_rgb_init(&rgb_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "node_rgb_init failed: %s", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "node_rgb: SK6812MINI-C GPIO%u (colors switched in main loop)", (unsigned)NODE_RGB_GPIO_DEFAULT);
        node_rgb_fill(48, 0, 0);
        s_rgb_last_tick = xTaskGetTickCount();
        s_rgb_idx = 1;
    }

    // spiffs_test();                                                  /* Run SPIFFS test */
    while (sd_card_init())                               /* SD card not detected */
    {
        ESP_LOGW(TAG, "SD Card Error!");
        vTaskDelay(500);
        ESP_LOGW(TAG, "Please Check!");
        vTaskDelay(500);
    }

    ESP_LOGI(TAG, "SD Initialized!");

    sd_card_test_filesystem();                                        /* Run SD card test */

    ESP_LOGI(TAG, "SD Tested CSW!");

    // sd_card_unmount();

    vTaskDelay(3000);

    ESP_LOGI(TAG_WIFI, "WiFi STA Test");

    ret = wifi_sta_wpa2_init();
    if(ret == ESP_OK)
    {
        ESP_LOGI(TAG_WIFI, "WiFi STA Init OK");
        ESP_LOGI(TAG_WIFI, "WiFi STA Test OK");
    }
    else
    {
        ESP_LOGE(TAG_WIFI, "WiFi STA Init Failed");
    }

    // only when the ip is obtained, start mqtt
    EventBits_t ev = 0;
    ev = xEventGroupWaitBits(wifi_event_group,CONNECTED_BIT,pdTRUE,pdFALSE,portMAX_DELAY);
    if(ev & CONNECTED_BIT)
    {
        mqtt_app_start();
    }

    while (1)
    {
        if(s_is_mqtt_connected)
        {
            snprintf(mqtt_pub_buff,64,"{\"count\":\"%d\"}",count);
            esp_mqtt_client_publish(s_mqtt_client, MQTT_PUBLISH_TOPIC,
                            mqtt_pub_buff, strlen(mqtt_pub_buff),1, 0);
            count++;
        }
        led_toggle();

        /* RGB：每 1 s 轮换红→绿→蓝（与主循环周期无关，只看 tick） */
        if (node_rgb_is_initialized()) {
            TickType_t now = xTaskGetTickCount();
            if (s_rgb_last_tick != 0 &&
                (now - s_rgb_last_tick) >= pdMS_TO_TICKS(1000)) {
                s_rgb_last_tick = now;
                switch (s_rgb_idx++ % 3) {
                    case 0:
                        node_rgb_fill(48, 0, 0);
                        break;
                    case 1:
                        node_rgb_fill(0, 48, 0);
                        break;
                    default:
                        node_rgb_fill(0, 0, 48);
                        break;
                }
            }
        }

        ESP_LOGI(TAG, "Hello World!");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

#ifdef __cplusplus
}
#endif

```