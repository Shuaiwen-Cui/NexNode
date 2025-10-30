# Node RTC DS3231 Module

This module provides a high-level interface for the DS3231 Real-Time Clock (RTC) module using our custom `node_i2c` module. It supports time management, alarm functionality, and temperature monitoring.

## Features

- **Time Management**: Read and set current time using standard C time structures
- **Alarm Support**: Set up to 2 alarms with flexible scheduling (day of week or day of month)
- **Temperature Monitoring**: Read internal temperature sensor
- **Square Wave Output**: Configurable frequency output
- **Interrupt Support**: Handle alarm events via GPIO interrupts
- **Control Flags**: Access to all DS3231 control and status registers

## Hardware Configuration

The module is configured to use the following GPIO pins:
- **SCL (Clock)**: GPIO 47
- **SDA (Data)**: GPIO 48
- **I2C Address**: 0x68 (default DS3231 address)

## Dependencies

- `node_i2c` module for I2C communication
- ESP-IDF v5.0 or later
- FreeRTOS

## API Reference

### Initialization

```c
// Initialize DS3231 with default I2C speed (200kHz)
rtc_handle_t* rtc_handle = node_rtc_ds3231_init(200000);

// Deinitialize when done
esp_err_t ret = node_rtc_ds3231_deinit(rtc_handle);
```

### Time Management

```c
// Get current time
struct tm* current_time = node_rtc_ds3231_time_get(rtc_handle);
if (current_time != NULL) {
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", current_time);
    printf("Current time: %s\n", time_str);
    free(current_time);
}

// Set time
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

// Get/set Unix timestamp
time_t unix_time = node_rtc_ds3231_time_unix_get(rtc_handle);
esp_err_t ret = node_rtc_ds3231_time_unix_set(rtc_handle, unix_time);
```

### Alarm Functions

```c
// Set alarm 1 for every Monday at 8:00 AM
esp_err_t ret = node_rtc_ds3231_alarm1_day_of_week_set(rtc_handle, RTC_MONDAY, 8, 0, 0);

// Set alarm 1 for 15th of every month at 2:30 PM
esp_err_t ret = node_rtc_ds3231_alarm1_day_of_month_set(rtc_handle, 15, 14, 30, 0);

// Set alarm 2 for every Friday at 6:00 PM (no seconds for alarm 2)
esp_err_t ret = node_rtc_ds3231_alarm2_day_of_week_set(rtc_handle, RTC_FRIDAY, 18, 0);

// Enable/disable alarms
esp_err_t ret = node_rtc_ds3231_alarm1_enable(rtc_handle, true);
esp_err_t ret = node_rtc_ds3231_alarm2_enable(rtc_handle, true);

// Check if alarms have fired
bool alarm1_fired = node_rtc_ds3231_alarm1_fired(rtc_handle);
bool alarm2_fired = node_rtc_ds3231_alarm2_fired(rtc_handle);

// Reset alarm flags
esp_err_t ret = node_rtc_ds3231_alarm1_fired_reset(rtc_handle);
esp_err_t ret = node_rtc_ds3231_alarm2_fired_reset(rtc_handle);
```

### Temperature Monitoring

```c
// Get temperature
float temperature = node_rtc_ds3231_temperature_get(rtc_handle);
printf("Temperature: %.2f°C\n", temperature);
```

### Control Functions

```c
// Set square wave frequency
esp_err_t ret = node_rtc_ds3231_square_wave_freq_set(rtc_handle, RTC_SQUARE_WAVE_FREQ_1000HZ);

// Get square wave frequency
enum rtc_square_wave_freq_e freq = node_rtc_ds3231_square_wave_freq_get(rtc_handle);

// Set interrupt/square wave control
esp_err_t ret = node_rtc_ds3231_interrupt_square_wave_control_set(rtc_handle, interrupts);

// Get control and status flags
rtc_control_status_t* status = node_rtc_ds3231_control_status_get(rtc_handle);
if (status != NULL) {
    printf("Oscillator enabled: %s\n", status->enable_oscillator ? "Yes" : "No");
    printf("Alarm 1 enabled: %s\n", status->alarm1_enable ? "Yes" : "No");
    printf("Alarm 2 enabled: %s\n", status->alarm2_enable ? "Yes" : "No");
    free(status);
}
```

### Debug Functions

```c
// Print all register data and current status
node_rtc_ds3231_debug_print_data(rtc_handle);
```

## Enumerations

### Days of Week
- `RTC_MONDAY` = 1
- `RTC_TUESDAY` = 2
- `RTC_WEDNESDAY` = 3
- `RTC_THURSDAY` = 4
- `RTC_FRIDAY` = 5
- `RTC_SATURDAY` = 6
- `RTC_SUNDAY` = 7

### Square Wave Frequencies
- `RTC_SQUARE_WAVE_FREQ_1000HZ` = 0 (1.000 kHz)
- `RTC_SQUARE_WAVE_FREQ_1024HZ` = 1 (1.024 kHz)
- `RTC_SQUARE_WAVE_FREQ_4096HZ` = 2 (4.096 kHz)
- `RTC_SQUARE_WAVE_FREQ_8192HZ` = 3 (8.192 kHz)

### Interrupt/Square Wave Control
- `square` = false (Square wave output)
- `interrupts` = true (Interrupt output)

## Error Handling

All functions return `esp_err_t` status codes:
- `ESP_OK`: Success
- `ESP_ERR_INVALID_ARG`: Invalid parameter
- `ESP_ERR_INVALID_STATE`: I2C bus not initialized
- `ESP_ERR_FAIL`: General failure

## Memory Management

Functions that return pointers (like `node_rtc_ds3231_time_get()`) allocate memory that must be freed by the caller:

```c
struct tm* time = node_rtc_ds3231_time_get(rtc_handle);
if (time != NULL) {
    // Use time structure
    free(time); // Important: free the memory
}
```

## Example Usage

See `example_usage.c` for a complete example showing:
- Module initialization
- Time reading and setting
- Alarm configuration
- Temperature monitoring
- Control flag management

## Notes

- The DS3231 uses BCD (Binary Coded Decimal) format internally
- Day of week values differ between DS3231 (1-7, Monday=1) and POSIX (0-6, Sunday=0)
- The module automatically handles these conversions
- All time operations are in UTC unless otherwise specified
- The DS3231 has a built-in temperature sensor with 0.25°C resolution
