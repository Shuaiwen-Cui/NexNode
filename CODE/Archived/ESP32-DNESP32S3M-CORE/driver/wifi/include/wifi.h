// // wifi.h
// // WiFi helper header
// // Provides simple WiFi station initialization and default settings

// #pragma once

// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <netdb.h>

// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "freertos/event_groups.h"

// #include "esp_system.h"
// #include "esp_wifi.h"
// #include "esp_event.h"

// #include "led.h"
// #include "lcd.h"

// #ifdef __cplusplus
// extern "C" {
// #endif

// /* Default WiFi credentials */
// #define DEFAULT_SSID        "CSW@CEE"
// #define DEFAULT_PWD         "88888888"

// /* Event group bits for WiFi status */
// #define WIFI_CONNECTED_BIT  BIT0
// #define WIFI_FAIL_BIT       BIT1

// /*
//  * Default WiFi configuration initializer.
//  * Expands to a partial initializer suitable where a wifi_init_config_t or
//  * wifi_config_t initializer is expected. Adjust fields as needed.
//  */
// #define WIFICONFIG()   {                            \
//     .sta = {                                        \
//         .ssid = DEFAULT_SSID,                       \
//         .password = DEFAULT_PWD,                    \
//         .threshold.authmode = WIFI_AUTH_WPA2_PSK,   \
//     },                                              \
// }

// /**
//  * Initialize WiFi in station mode using the default configuration.
//  * This function will configure and start the WiFi driver and attempt
//  * to connect to the configured SSID.
//  */
// void wifi_sta_init(void);

// #ifdef __cplusplus
// }
// #endif