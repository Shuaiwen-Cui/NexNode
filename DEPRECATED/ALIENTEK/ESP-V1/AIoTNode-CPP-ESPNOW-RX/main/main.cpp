/**
 * @file main.cpp
 * @author SHUAIWEN CUI (SHUAIWEN001@e.ntu.edu.sg)
 * @brief ESP-NOW Receiver Example - Easy to understand tutorial version (ESP-IDF v6.0)
 * @version 1.0
 * @date 2025-10-22
 * 
 * @copyright Copyright (c) 2024
 * 
 */

/* DEPENDENCIES */
// ESP Core Libraries
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

// ESP-NOW related (ESP-NOW is based on WiFi, so these headers are needed)
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_now.h"

// BSP Drivers
#include "node_led.h"
#include "node_exit.h"
#include "node_spi.h"
#include "node_lcd.h"
#include "node_timer.h"
#include "node_rtc.h"
#include "node_sdcard.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========== Data Structure Definition ========== */
// Define the data structure we will receive
// This structure contains all the data to be transmitted
typedef struct {
    int counter;           // Sequence number from sender
    char text[30];        // Text message (max 30 characters)
    int value1;           // Value 1 (example)
    int value2;           // Value 2 (example)
} message_data_t;

/* ========== Global Variables ========== */
const char *TAG = "ESP_NOW_RX";

// Receive counter (records the number of received messages)
static int recv_count = 0;

// Last received message (for LCD display)
static message_data_t last_message;
static bool message_received = false;

/* ========== Callback Function: Send Completion Notification ========== */
// Receivers usually don't need send callback, but keep this function in case reply is needed
// Note: ESP-IDF v6.0 changed the callback function signature, now using wifi_tx_info_t structure
void send_callback(const wifi_tx_info_t *info, esp_now_send_status_t status)
{
    // In receiver mode, this callback is usually not used
    // If you need to reply to the sender, you can use this callback
    if (status == ESP_NOW_SEND_SUCCESS)
    {
        ESP_LOGI(TAG, "✓ Reply sent successfully");
    }
    else
    {
        ESP_LOGE(TAG, "✗ Reply send failed");
    }
}

/* ========== Callback Function: Data Received ========== */
// This function is automatically called by ESP-NOW when data is received
// Note: ESP-IDF v6.0 changed the callback function signature, now using esp_now_recv_info structure
void recv_callback(const esp_now_recv_info *recv_info, const uint8_t *data, int len)
{
    recv_count++;  // Increment receive counter
    
    // Get sender's MAC address from recv_info structure
    const uint8_t *mac_addr = recv_info->src_addr;
    
    // Print sender's MAC address
    ESP_LOGI(TAG, "✓ Data received! From: %02X:%02X:%02X:%02X:%02X:%02X, Length: %d",
             mac_addr[0], mac_addr[1], mac_addr[2], 
             mac_addr[3], mac_addr[4], mac_addr[5], len);
    
    // If data length matches our message structure, parse it
    if (len == sizeof(message_data_t))
    {
        message_data_t *msg = (message_data_t *)data;
        ESP_LOGI(TAG, "Message content: counter=%d, text=%s, value1=%d, value2=%d",
                 msg->counter, msg->text, msg->value1, msg->value2);
        
        // Save the last received message
        memcpy(&last_message, msg, sizeof(message_data_t));
        message_received = true;
        
        // Toggle LED to indicate data received
        led_toggle();
    }
    else
    {
        ESP_LOGW(TAG, "Data length mismatch! Expected: %d, Actual: %d", sizeof(message_data_t), len);
    }
}

/* ========== ESP-NOW Initialization Function ========== */
// This function initializes WiFi and ESP-NOW protocol
esp_err_t espnow_init(void)
{
    esp_err_t ret = ESP_OK;
    
    // Step 1: Initialize network interface (required for ESP-NOW)
    ESP_LOGI(TAG, "Step 1: Initializing network interface...");
    ret = esp_netif_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Network interface initialization failed");
        return ret;
    }
    
    // Step 2: Create event loop (required for ESP-NOW to handle events)
    ESP_LOGI(TAG, "Step 2: Creating event loop...");
    ret = esp_event_loop_create_default();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "Event loop creation failed");
        return ret;
    }
    
    // Step 3: Initialize WiFi (ESP-NOW is based on WiFi protocol)
    ESP_LOGI(TAG, "Step 3: Initializing WiFi...");
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ret = esp_wifi_init(&cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "WiFi initialization failed");
        return ret;
    }
    
    // Step 4: Set WiFi to Station mode (client mode)
    // Note: ESP-NOW doesn't need to connect to router, just start WiFi
    ret = esp_wifi_set_storage(WIFI_STORAGE_RAM);
    ret = esp_wifi_set_mode(WIFI_MODE_STA);  // Station mode
    ret = esp_wifi_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "WiFi start failed");
        return ret;
    }
    
    // Step 5: Set WiFi channel (sender and receiver must be on the same channel)
    ret = esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Channel setting failed");
        return ret;
    }
    
    // Step 6: Initialize ESP-NOW protocol
    ESP_LOGI(TAG, "Step 6: Initializing ESP-NOW...");
    ret = esp_now_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ESP-NOW initialization failed");
        return ret;
    }
    
    // Step 7: Register receive callback function (called when data is received)
    ESP_LOGI(TAG, "Step 7: Registering receive callback...");
    ret = esp_now_register_recv_cb(recv_callback);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Receive callback registration failed");
        return ret;
    }
    
    // Step 8: Optional - Register send callback (if you need to reply to sender)
    ESP_LOGI(TAG, "Step 8: Registering send callback...");
    ret = esp_now_register_send_cb(send_callback);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Send callback registration failed");
        return ret;
    }
    
    // Note: Receiver doesn't need to add peer, because receiver automatically receives all data sent to it
    // If you need to reply after receiving, you can dynamically add sender's peer when data is received
    
    // Get and print local MAC address
    uint8_t my_mac[6];
    esp_wifi_get_mac(WIFI_IF_STA, my_mac);
    ESP_LOGI(TAG, "✓ ESP-NOW receiver initialization successful!");
    ESP_LOGI(TAG, "Local MAC address: %02X:%02X:%02X:%02X:%02X:%02X",
             my_mac[0], my_mac[1], my_mac[2], my_mac[3], my_mac[4], my_mac[5]);
    ESP_LOGI(TAG, "Waiting for data...");
    
    return ESP_OK;
}

/* ========== Display Update Task Function ========== */
// This function runs in an independent FreeRTOS task, periodically updating LCD display
void display_task(void *pvParameters)
{
    char display_buf[80];     // LCD display buffer
    
    ESP_LOGI(TAG, "Display task started...");
    
    while (1)  // Infinite loop
    {
        // Update LCD display
        lcd_clear(WHITE);
        
        // Line 1: Title (Y=0, occupies 0-16)
        lcd_show_string(0, 0, 160, 16, 16, (char*)"ESP-NOW RX", GREEN);
        
        // Line 2: Receive count (Y=24, 8px spacing, occupies 24-40)
        snprintf(display_buf, sizeof(display_buf), "Recv #%d", recv_count);
        lcd_show_string(0, 24, 160, 16, 16, display_buf, BLACK);
        
        // If message received, display message content
        if (message_received)
        {
            // Line 3: Message text (Y=48, 8px spacing, occupies 48-64)
            snprintf(display_buf, sizeof(display_buf), "Text: %s", last_message.text);
            lcd_show_string(0, 48, 160, 16, 16, display_buf, BLUE);
            
            // Line 4: Counter value (Y=72, 8px spacing, occupies 72-88)
            snprintf(display_buf, sizeof(display_buf), "Cnt: %d", last_message.counter);
            lcd_show_string(0, 72, 160, 16, 16, display_buf, BLACK);
        }
        else
        {
            // Line 3: Waiting for message (Y=48, 8px spacing, occupies 48-64)
            lcd_show_string(0, 48, 160, 16, 16, (char*)"Waiting...", BLACK);
        }
        
        // Wait 500ms before updating display
        vTaskDelay(pdMS_TO_TICKS(500));
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
    lcd_init();    // Initialize LCD display
    
    // Clear screen and display initialization message
    lcd_clear(WHITE);
    lcd_show_string(0, 0, 160, 16, 16, (char*)"Initializing...", BLUE);
    
    // Note: SD card initialization is skipped here because ESP-NOW doesn't need SD card
    // If SD card is needed, uncomment the following
    /*
    while (sd_card_init())
    {
        lcd_show_string(0, 0, 200, 16, 16, (char*)"SD Card Error!", RED);
        vTaskDelay(500);
    }
    */

    // ========== Part 3: ESP-NOW Initialization ==========
    ESP_LOGI(TAG, "========== ESP-NOW Initialization ==========");
    
    lcd_fill(0, 0, 160, 16, WHITE);
    lcd_show_string(0, 0, 160, 16, 12, (char*)"Init ESP-NOW...", BLUE);
    
    ret = espnow_init();
    if (ret != ESP_OK)
    {
        // Initialization failed, display error on LCD and stop
        lcd_clear(WHITE);
        lcd_show_string(0, 0, 160, 16, 12, (char*)"ESP-NOW Init", RED);
        lcd_show_string(0, 16, 160, 16, 12, (char*)"Failed!", RED);
        ESP_LOGE(TAG, "ESP-NOW initialization failed, program stopped");
        while (1) {
            vTaskDelay(1000);  // Infinite wait
        }
    }
    
    // Initialization successful
    lcd_clear(WHITE);
    lcd_show_string(0, 0, 160, 16, 12, (char*)"ESP-NOW Ready!", GREEN);
    vTaskDelay(pdMS_TO_TICKS(1000));  // Wait 1 second for user to see the message
    
    // ========== Part 4: Create Display Update Task ==========
    ESP_LOGI(TAG, "========== Create Display Update Task ==========");
    
    // Create a FreeRTOS task to periodically update LCD display
    // Parameter description:
    // - display_task: Task function
    // - "display_task": Task name
    // - 4096: Task stack size (bytes)
    // - NULL: Parameters passed to task
    // - 5: Task priority (higher number = higher priority)
    // - NULL: Task handle (not needed here)
    xTaskCreate(display_task, "display_task", 4096, NULL, 5, NULL);
    
    ESP_LOGI(TAG, "✓ Program startup complete! Waiting for data...");
    
    // Main task can continue doing other things or go to sleep
    while (1)
    {
        // Main task does nothing now, just waits
        // Actual receive work is done by ESP-NOW receive callback
        // Display updates are done by display_task
        vTaskDelay(pdMS_TO_TICKS(5000));  // Print every 5 seconds
        ESP_LOGI(TAG, "Main task running... (Receive count: %d)", recv_count);
    }
}

#ifdef __cplusplus
}
#endif