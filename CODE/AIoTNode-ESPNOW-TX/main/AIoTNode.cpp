/**
 * @file main.cpp
 * @author SHUAIWEN CUI (SHUAIWEN001@e.ntu.edu.sg)
 * @brief ESP-NOW Transmitter Example - Simple and Easy to Understand Tutorial Version
 * @version 1.0
 * @date 2025-10-22
 * 
 * @copyright Copyright (c) 2024
 * 
 */

/* DEPENDENCIES */
// ESP Basic Libraries
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_chip_info.h"
#include "esp_psram.h"
#include "esp_flash.h"
#include "esp_log.h"
#include <string.h>

// FreeRTOS
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ESP-NOW Related (ESP-NOW is based on WiFi, so these headers are needed)
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_now.h"

// BSP Drivers
#include "node_led.h"
#include "node_exit.h"
#include "node_spi.h"
#include "node_timer.h"
#include "node_rtc.h"
#include "node_sdcard.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========== Global Variables ========== */
const char *TAG = "ESP_NOW_TX";

// Receiver MAC address (6 bytes)
// 0xFF means broadcast address, all ESP-NOW devices can receive
// To target a specific device, replace with the target device's MAC address
static uint8_t receiver_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Send counter (used to identify each sent data)
static int send_count = 0;

// Receive counter (records the number of received messages)
static int recv_count = 0;

/* ========== Data Structure Definition ========== */
// Define the data structure we want to send
// This structure contains all the data to be transmitted
typedef struct {
    int counter;           // Send sequence number
    char text[30];        // Text message (max 30 characters)
    int value1;           // Value 1 (example)
    int value2;           // Value 2 (example)
} message_data_t;

/* ========== Callback Function: Notification after Send Completion ========== */
// When data transmission completes (success or failure), ESP-NOW will automatically call this function
// Note: ESP-IDF v6.0 changed the callback function signature, now uses wifi_tx_info_t structure
void send_callback(const wifi_tx_info_t *info, esp_now_send_status_t status)
{
    // status == ESP_NOW_SEND_SUCCESS means send successful
    if (status == ESP_NOW_SEND_SUCCESS)
    {
        ESP_LOGI(TAG, "✓ Send Success");
    }
    else
    {
        ESP_LOGE(TAG, "✗ Send Failed");
    }
}

/* ========== Callback Function: Data Received ========== */
// When ESP-NOW data is received, ESP-NOW will automatically call this function
// Note: ESP-IDF v6.0 changed the callback function signature, now uses esp_now_recv_info structure
void recv_callback(const esp_now_recv_info *recv_info, const uint8_t *data, int len)
{
    recv_count++;  // Increment receive counter
    
    // Get sender's MAC address from recv_info structure
    const uint8_t *mac_addr = recv_info->src_addr;
    
    // Print sender's MAC address
    ESP_LOGI(TAG, "Data received! From: %02X:%02X:%02X:%02X:%02X:%02X, Length: %d",
             mac_addr[0], mac_addr[1], mac_addr[2], 
             mac_addr[3], mac_addr[4], mac_addr[5], len);
    
    // If data length matches our message structure, parse it
    if (len == sizeof(message_data_t))
    {
        message_data_t *msg = (message_data_t *)data;
        ESP_LOGI(TAG, "Message content: counter=%d, text=%s, value1=%d, value2=%d",
                 msg->counter, msg->text, msg->value1, msg->value2);
        
        // Receive info is already displayed in the send task above, no need to display separately here
        // If need to update receive count, can update when send task executes next time
    }
}

/* ========== ESP-NOW Initialization Function ========== */
// This function is responsible for initializing WiFi and ESP-NOW protocol
esp_err_t espnow_init(void)
{
    esp_err_t ret = ESP_OK;
    
    // Step 1: Initialize network interface (ESP-NOW needs this)
    ESP_LOGI(TAG, "Step 1: Initialize network interface...");
    ret = esp_netif_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Network interface initialization failed");
        return ret;
    }
    
    // Step 2: Create event loop (ESP-NOW needs this to handle events)
    ESP_LOGI(TAG, "Step 2: Create event loop...");
    ret = esp_event_loop_create_default();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "Event loop creation failed");
        return ret;
    }
    
    // Step 3: Initialize WiFi (ESP-NOW is based on WiFi protocol)
    ESP_LOGI(TAG, "Step 3: Initialize WiFi...");
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ret = esp_wifi_init(&cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "WiFi initialization failed");
        return ret;
    }
    
    // Step 4: Set WiFi to Station mode (client mode)
    // Note: ESP-NOW does not need to connect to router, only WiFi needs to be started
    ret = esp_wifi_set_storage(WIFI_STORAGE_RAM);
    ret = esp_wifi_set_mode(WIFI_MODE_STA);  // Station mode
    ret = esp_wifi_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "WiFi start failed");
        return ret;
    }
    
    // Step 5: Set WiFi channel (transmitter and receiver must be on the same channel)
    ret = esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Channel setting failed");
        return ret;
    }
    
    // Step 6: Initialize ESP-NOW protocol
    ESP_LOGI(TAG, "Step 4: Initialize ESP-NOW...");
    ret = esp_now_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ESP-NOW initialization failed");
        return ret;
    }
    
    // Step 7: Register send callback function (will call send_callback after send completes)
    ESP_LOGI(TAG, "Step 5: Register send callback...");
    ret = esp_now_register_send_cb(send_callback);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Register send callback failed");
        return ret;
    }
    
    // Step 8: Register receive callback function (will call recv_callback when data is received)
    ESP_LOGI(TAG, "Step 6: Register receive callback...");
    ret = esp_now_register_recv_cb(recv_callback);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Register receive callback failed");
        return ret;
    }
    
    // Step 9: Add receiver peer (tell ESP-NOW where to send data)
    ESP_LOGI(TAG, "Step 7: Add receiver peer...");
    esp_now_peer_info_t peer_info;
    memset(&peer_info, 0, sizeof(peer_info));  // Clear structure
    memcpy(peer_info.peer_addr, receiver_mac, 6);  // Copy MAC address
    peer_info.channel = 1;        // Channel (must match WiFi channel)
    peer_info.ifidx = WIFI_IF_STA; // Use Station interface
    peer_info.encrypt = false;    // No encryption (simplified version)
    
    ret = esp_now_add_peer(&peer_info);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Add receiver peer failed");
        return ret;
    }
    
    // Get and print local MAC address
    uint8_t my_mac[6];
    esp_wifi_get_mac(WIFI_IF_STA, my_mac);
    ESP_LOGI(TAG, "✓ ESP-NOW initialization successful!");
    ESP_LOGI(TAG, "Local MAC address: %02X:%02X:%02X:%02X:%02X:%02X",
             my_mac[0], my_mac[1], my_mac[2], my_mac[3], my_mac[4], my_mac[5]);
    ESP_LOGI(TAG, "Target MAC address: %02X:%02X:%02X:%02X:%02X:%02X",
             receiver_mac[0], receiver_mac[1], receiver_mac[2], 
             receiver_mac[3], receiver_mac[4], receiver_mac[5]);
    
    return ESP_OK;
}

/* ========== Send Task Function ========== */
// This function runs in an independent FreeRTOS task, periodically sending data
void send_task(void *pvParameters)
{
    message_data_t message;  // Data to send
    
    ESP_LOGI(TAG, "Send task started...");
    
    while (1)  // Infinite loop
    {
        // 1. Prepare data to send
        send_count++;  // Increment counter
        message.counter = send_count;
        snprintf(message.text, sizeof(message.text), "Hello #%d", send_count);
        message.value1 = send_count * 10;
        message.value2 = send_count * 20;
        
        // 2. Send data (asynchronous send, function returns immediately)
        // esp_now_send() does not wait after sending, result is notified via callback function
        esp_err_t ret = esp_now_send(receiver_mac, (uint8_t *)&message, sizeof(message));
        
        if (ret == ESP_OK)
        {
            ESP_LOGI(TAG, "Send data #%d: %s", send_count, message.text);
        }
        else
        {
            // Send failed
            ESP_LOGE(TAG, "Send failed: %s", esp_err_to_name(ret));
        }
        
        // 4. Toggle LED state (visual feedback)
        led_toggle();
        
        // 5. Wait 2 seconds before sending again
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

/* ========== Main Function ========== */
void app_main(void)
{
    esp_err_t ret;
    uint32_t flash_size;
    esp_chip_info_t chip_info;

    // ========== Part 1: System Initialization ==========
    ESP_LOGI(TAG, "========== System Initialization ==========");
    
    // Initialize NVS (Non-Volatile Storage)
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    // Get chip information (for debugging)
    esp_flash_get_size(NULL, &flash_size);
    esp_chip_info(&chip_info);
    ESP_LOGI(TAG, "CPU cores: %d", chip_info.cores);
    ESP_LOGI(TAG, "Flash size: %ld MB", flash_size / (1024 * 1024));
    ESP_LOGI(TAG, "PSRAM size: %d bytes", esp_psram_get_size());

    // ========== Part 2: Hardware Initialization ==========
    ESP_LOGI(TAG, "========== Hardware Initialization ==========");
    
    led_init();    // Initialize LED
    exit_init();   // Initialize external interrupt
    spi2_init();   // Initialize SPI interface
    
    // Note: SD card initialization is skipped here because ESP-NOW doesn't need SD card
    // If SD card is needed, uncomment below
    /*
    while (sd_card_init())
    {
        lcd_show_string(0, 0, 200, 16, 16, (char*)"SD Card Error!", RED);
        vTaskDelay(500);
    }
    */

    // ========== Part 3: ESP-NOW Initialization ==========
    ESP_LOGI(TAG, "========== ESP-NOW Initialization ==========");
    
    ret = espnow_init();
    if (ret != ESP_OK)
    {
        // Initialization failed
        ESP_LOGE(TAG, "ESP-NOW initialization failed, program stopped");
        while (1) {
            vTaskDelay(1000);  // Infinite wait
        }
    }
    
    // Initialization successful
    vTaskDelay(pdMS_TO_TICKS(1000));  // Wait 1 second
    
    // ========== Part 4: Create Send Task ==========
    ESP_LOGI(TAG, "========== Create Send Task ==========");
    
    // Create a FreeRTOS task to periodically send data
    // Parameter description:
    // - send_task: task function
    // - "send_task": task name
    // - 4096: task stack size (bytes)
    // - NULL: parameters passed to task
    // - 5: task priority (higher number = higher priority)
    // - NULL: task handle (no need to return)
    xTaskCreate(send_task, "send_task", 4096, NULL, 5, NULL);
    
    ESP_LOGI(TAG, "✓ Program started! Begin sending data...");
    
    // Main task can continue doing other things, or go to sleep
    while (1)
    {
        // Main task does nothing now, just waits
        // Actual send work is done by send_task
        vTaskDelay(pdMS_TO_TICKS(5000));  // Print every 5 seconds
        ESP_LOGI(TAG, "Main task running... (Send:%d, Recv:%d)", send_count, recv_count);
    }
}

#ifdef __cplusplus
}
#endif
