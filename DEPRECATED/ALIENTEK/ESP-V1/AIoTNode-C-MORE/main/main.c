/**
 * @file example_usage.c
 * @author SHUAIWEN CUI (SHUAIWEN001@e.ntu.edu.sg)
 * @brief Example usage of node_rtc_ds3231 module
 * @version 1.0
 * @date 2025-01-27
 *
 * @copyright Copyright (c) 2025
 *
 */

 #include "node_rtc_ds3231.h"
 #include "esp_log.h"
 #include "freertos/FreeRTOS.h"
 #include "freertos/task.h"
 
 static const char* TAG = "rtc_example";
 
 void rtc_example_task(void* pvParameters)
 {
    // Initialize DS3231 RTC module
    rtc_handle_t rtc_handle = node_rtc_ds3231_init(200000); // 200kHz I2C speed
     if (rtc_handle == NULL) {
         ESP_LOGE(TAG, "Failed to initialize DS3231 RTC module");
         vTaskDelete(NULL);
         return;
     }
     
     ESP_LOGI(TAG, "DS3231 RTC module initialized successfully");
     
     // Print debug information
     node_rtc_ds3231_debug_print_data(rtc_handle);
     
     // Get current time
     struct tm* current_time = node_rtc_ds3231_time_get(rtc_handle);
     if (current_time != NULL) {
         char time_str[64];
         strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", current_time);
         ESP_LOGI(TAG, "Current RTC time: %s", time_str);
         free(current_time);
     }
     
     // Get temperature
     float temperature = node_rtc_ds3231_temperature_get(rtc_handle);
     ESP_LOGI(TAG, "RTC temperature: %.2f°C", temperature);
     
     // Set a new time (example: 2025-01-27 12:00:00)
     struct tm new_time = {
         .tm_year = 125,  // 2025 - 1900
         .tm_mon = 0,     // January (0-based)
         .tm_mday = 27,   // 27th
         .tm_hour = 12,   // 12:00
         .tm_min = 0,     // 00 minutes
         .tm_sec = 0,     // 00 seconds
         .tm_wday = 1     // Monday
     };
     
     esp_err_t ret = node_rtc_ds3231_time_set(rtc_handle, new_time);
     if (ret == ESP_OK) {
         ESP_LOGI(TAG, "Time set successfully");
     } else {
         ESP_LOGE(TAG, "Failed to set time: %s", esp_err_to_name(ret));
     }
     
     // Set alarm 1 for every day at 8:00 AM
     ret = node_rtc_ds3231_alarm1_day_of_week_set(rtc_handle, RTC_MONDAY, 8, 0, 0);
     if (ret == ESP_OK) {
         ESP_LOGI(TAG, "Alarm 1 set for Monday 8:00 AM");
     }
     
     // Enable alarm 1
     ret = node_rtc_ds3231_alarm1_enable(rtc_handle, true);
     if (ret == ESP_OK) {
         ESP_LOGI(TAG, "Alarm 1 enabled");
     } else {
         ESP_LOGE(TAG, "Failed to enable alarm 1: %s", esp_err_to_name(ret));
     }
     
     // Set square wave output to 1Hz
     ret = node_rtc_ds3231_square_wave_freq_set(rtc_handle, RTC_SQUARE_WAVE_FREQ_1000HZ);
     if (ret == ESP_OK) {
         ESP_LOGI(TAG, "Square wave frequency set to 1Hz");
     }
     
     // Set interrupt mode (alarms will trigger interrupts)
     ret = node_rtc_ds3231_interrupt_square_wave_control_set(rtc_handle, interrupts);
     if (ret == ESP_OK) {
         ESP_LOGI(TAG, "Interrupt mode enabled");
     }
     
     // Main loop - check for alarms
     while (1) {
         // Check if alarm 1 has fired
         if (node_rtc_ds3231_alarm1_fired(rtc_handle)) {
             ESP_LOGI(TAG, "Alarm 1 fired!");
             
             // Get current time when alarm fired
             struct tm* alarm_time = node_rtc_ds3231_time_get(rtc_handle);
             if (alarm_time != NULL) {
                 char time_str[64];
                 strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", alarm_time);
                 ESP_LOGI(TAG, "Alarm fired at: %s", time_str);
                 free(alarm_time);
             }
             
             // Reset alarm flag
             node_rtc_ds3231_alarm1_fired_reset(rtc_handle);
         }
         
         // Check if alarm 2 has fired
         if (node_rtc_ds3231_alarm2_fired(rtc_handle)) {
             ESP_LOGI(TAG, "Alarm 2 fired!");
             node_rtc_ds3231_alarm2_fired_reset(rtc_handle);
         }
         
         // Print current time every 10 seconds
         static int counter = 0;
         if (++counter >= 10) {
             struct tm* current_time = node_rtc_ds3231_time_get(rtc_handle);
             if (current_time != NULL) {
                 char time_str[64];
                 strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", current_time);
                 ESP_LOGI(TAG, "Current time: %s", time_str);
                 free(current_time);
             }
             counter = 0;
         }
         
         vTaskDelay(pdMS_TO_TICKS(1000)); // Wait 1 second
     }
     
     // Cleanup (this will never be reached in this example)
     node_rtc_ds3231_deinit(rtc_handle);
     vTaskDelete(NULL);
 }
 
 void app_main(void)
 {
     ESP_LOGI(TAG, "Starting DS3231 RTC example");
     
     // Create task for RTC operations
     xTaskCreate(rtc_example_task, "rtc_example", 4096, NULL, 5, NULL);
 }
 