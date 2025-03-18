/**
 * @file mqtt.h
 * @author SHUAIWEN CUI (SHUAIWEN001@e.ntu.edu.sg)
 * @brief This file contains the function prototypes for mqtt connection.
 * @version 1.0
 * @date 2025-03-17
 *
 * @copyright Copyright (c) 2025
 *
 */

#ifndef __MQTT_H__
#define __MQTT_H__

/* Dependencies */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_eap_client.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_netif.h"
#include "mqtt_client.h"

/* Macros */
#define MQTT_ADDRESS "mqtt://8.222.194.160" // MQTT Broker URL
#define MQTT_PORT 1883                      // MQTT Broker Port
#define MQTT_CLIENT "CSW-ESP32-S3-Node-001" // Client ID (Unique for devices)
#define MQTT_USERNAME "cshwstem"            // MQTT Username
#define MQTT_PASSWORD "Cshw0918#"           // MQTT Password

#define MQTT_PUBLIC_TOPIC      "/test/topic1"       // for test, publish topic
#define MQTT_SUBSCRIBE_TOPIC   "/test/topic2"      // for test, subscribe topic

/* Variables */
extern const char *TAG_MQTT; // tag for logging
extern bool s_is_mqtt_connected;

#endif /* __MQTT_H__ */