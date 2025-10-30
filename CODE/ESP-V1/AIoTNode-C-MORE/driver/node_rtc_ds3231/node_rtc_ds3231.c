/**
 * @file node_rtc_ds3231.c
 * @author SHUAIWEN CUI (SHUAIWEN001@e.ntu.edu.sg)
 * @brief This file contains the functions for DS3231 RTC module using our node_i2c module.
 * @version 1.0
 * @date 2025-01-27
 *
 * @copyright Copyright (c) 2025
 *
 */

#include "node_rtc_ds3231.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/queue.h"
#include "esp_debug_helpers.h"

static const char *TAG = "node_rtc_ds3231";

// Bitfield mapping is LSB to MSB
#pragma pack(push, 1)
/**
 * @brief Union representing seconds, minutes or year of the DS3231. All three types use a compatible format.
 */
typedef union
{
    uint8_t data; /*!< Native type representation of the data.  */
    struct
    {
        unsigned ones : 4; /*!< 1's digit, represented as BCD */
        unsigned tens : 3; /*!< 10's digit, represented as BCD */
        unsigned alarm_rate : 1;
    };
} rtc_seconds_or_minutes_t;

/**
 * @brief Union representing seconds, minutes or year of the DS3231. All three types use a compatible format.
 */
typedef union
{
    uint8_t data; /*!< Native type representation of the data.  */
    struct
    {
        unsigned ones : 4; /*!< 1's digit, represented as BCD */
        unsigned tens : 4; /*!< 10's digit, represented as BCD */
    };
} rtc_years_t;

/**
 * @brief Union representing hours of the DS3231.
 */
typedef union
{
    uint8_t data; /*!< Native type representation of the data.  */
    struct
    {
        unsigned ones : 4;               /*!< 1's digit, represented as BCD */
        unsigned tens : 1;               /*!< 10's digit, represented as single bit. BCD conversion compliant. */
        unsigned ampm_hour_twenties : 1; /*!< AM/PM **OR** 20's digit. When 0, AM. When 1 is PM if 'twelve_hour' is 0, 20's
                                            digit if 'twelve_hour' is 1 */
        unsigned twelve_hour : 1;        /*!< Represents 12 hour time if 0, 24 hour time if 1 */
        unsigned alarm_rate : 1;
    };
} rtc_hours_t;

/**
 * @brief Union representing days of the DS3231.
 */
typedef union
{
    uint8_t data; /*!< Native type representation of the data.  */
    struct
    {
        unsigned day_date : 4; /*!< Represents the day of month or (only for alarms AND dy_dt is 0). If an alarm and dy_dt is 1,
                              represents the day of the week, 1-7 starting on Monday. Both values are expressed in BCD format. */
        unsigned tens : 2;     /*!< 10's value of date. BCD conversion compliant */
        unsigned dy_dt : 1;    /*!< Determine if day_date represents a day of week (DOW) or day of month (DOM). If 0, it is DOM. If
                                  1, it is DOW. */
        unsigned alarm_rate : 1;
    };
} rtc_day_date_t;

/**
 * @brief Union representing the month **AND** century flag of the DS3231.
 */
typedef union
{
    uint8_t data; /*!< Native type representation of the data.  */
    struct
    {
        unsigned ones : 4; /*!< 1's digit, represented as BCD */
        unsigned tens : 1; /*!< 10's digit, represented as single bit. BCD conversion compliant. */
        unsigned unused : 2;
        unsigned century : 1; /*!< Century is 1900 if 0, 2000 if 1 */
    };
} rtc_month_century_t;

/**
 * @brief Union representing the aging offset register (10h) of the DS3231.
 */
typedef union
{
    int8_t data; /*!< Native type representation of the data.  */
    struct
    {
        signed offset : 8; /*!< See the DS3231 data sheet for more details. */
    };
} rtc_aging_offset_t;

/**
 * @brief Union representing the temperature in degrees Celsius registers (11h & 12h) of the DS3231.
 */
typedef union
{
    int16_t data; /*!< Native type representation of the data.  */
    struct
    {
        signed temp : 8;             /*!< Signed int8 representing the whole number temperature in degree Celsius */
        unsigned unused : 6;         /*!< Not used in register  */
        unsigned quarter_degree : 2; /*!< The DS3231 has 0.25 degree Celsius resolution. */
    };
} rtc_temperature_t;
#pragma pack(pop)

// Helper function declarations
static uint8_t convert_to_bcd(uint8_t value);
static uint8_t convert_from_bcd(uint8_t bcd_value);
static esp_err_t read_registers(rtc_handle_t rtc_handle, uint8_t *data, size_t length);
static esp_err_t write_register(rtc_handle_t rtc_handle, uint8_t reg_addr, uint8_t data);
static esp_err_t write_registers(rtc_handle_t rtc_handle, uint8_t *data, size_t length);
static int8_t get_seconds_or_minutes(rtc_handle_t rtc_handle, enum rtc_register_e reg);
static int8_t get_hours(rtc_handle_t rtc_handle, enum rtc_register_e reg);
static int8_t get_day_of_week(rtc_handle_t rtc_handle, enum rtc_register_e reg);
static int8_t get_day_of_month(rtc_handle_t rtc_handle, enum rtc_register_e reg);
static int8_t get_month(rtc_handle_t rtc_handle);
static int16_t get_year(rtc_handle_t rtc_handle);

/**
 * @brief Initialize the DS3231 RTC Module using our node_i2c module.
 */
rtc_handle_t node_rtc_ds3231_init(uint32_t scl_speed_hz)
{
    esp_err_t ret;
    static i2c_master_dev_handle_t dev_handle = NULL;

    // Initialize I2C bus if not already initialized
    if (i2c_bus_handle == NULL)
    {
        ret = i2c_bus_init();
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to initialize I2C bus: %s", esp_err_to_name(ret));
            return NULL;
        }
    }

    // Add DS3231 device to I2C bus
    ret = i2c_add_device(RTC_I2C_ADDRESS, scl_speed_hz, &dev_handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to add DS3231 device to I2C bus: %s", esp_err_to_name(ret));
        return NULL;
    }

    ESP_LOGI(TAG, "DS3231 RTC module initialized successfully");
    return dev_handle;
}

/**
 * @brief Deinitialize the DS3231 RTC Module.
 */
esp_err_t node_rtc_ds3231_deinit(rtc_handle_t rtc_handle)
{
    if (rtc_handle == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret = i2c_remove_device(rtc_handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to remove DS3231 device from I2C bus: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "DS3231 RTC module deinitialized successfully");
    return ESP_OK;
}

/**
 * @brief Get current time from DS3231 as struct tm.
 */
struct tm *node_rtc_ds3231_time_get(rtc_handle_t rtc_handle)
{
    if (rtc_handle == NULL)
    {
        return NULL;
    }

    struct tm *rtc_time = (struct tm *)calloc(1, sizeof(struct tm));
    if (rtc_time == NULL)
    {
        ESP_LOGE(TAG, "Failed to allocate memory for time structure");
        return NULL;
    }

    // Get year
    int16_t year = get_year(rtc_handle);
    if (year == -1)
    {
        free(rtc_time);
        return NULL;
    }
    rtc_time->tm_year = year - 1900;

    // Get month
    int8_t month = get_month(rtc_handle);
    if (month == -1)
    {
        free(rtc_time);
        return NULL;
    }
    rtc_time->tm_mon = month - 1;

    // Get day of month
    int8_t mday = get_day_of_month(rtc_handle, RTC_REGISTER_TIME_DATE);
    if (mday == -1)
    {
        free(rtc_time);
        return NULL;
    }
    rtc_time->tm_mday = mday;

    // Get day of week
    int8_t wday = get_day_of_week(rtc_handle, RTC_REGISTER_TIME_DAY);
    if (wday == -1)
    {
        free(rtc_time);
        return NULL;
    }
    rtc_time->tm_wday = (wday == 7) ? 0 : wday; // Convert DS3231 format (1-7) to POSIX format (0-6)

    // Get hour
    int8_t hour = get_hours(rtc_handle, RTC_REGISTER_TIME_HOURS);
    if (hour == -1)
    {
        free(rtc_time);
        return NULL;
    }
    rtc_time->tm_hour = hour;

    // Get minute
    int8_t minute = get_seconds_or_minutes(rtc_handle, RTC_REGISTER_TIME_MINUTES);
    if (minute == -1)
    {
        free(rtc_time);
        return NULL;
    }
    rtc_time->tm_min = minute;

    // Get second
    int8_t second = get_seconds_or_minutes(rtc_handle, RTC_REGISTER_TIME_SECONDS);
    if (second == -1)
    {
        free(rtc_time);
        return NULL;
    }
    rtc_time->tm_sec = second;

    return rtc_time;
}

/**
 * @brief Set time on DS3231 using struct tm.
 */
esp_err_t node_rtc_ds3231_time_set(rtc_handle_t rtc_handle, struct tm time)
{
    if (rtc_handle == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t time_data[8];
    time_data[0] = RTC_REGISTER_TIME_SECONDS;
    time_data[1] = convert_to_bcd(time.tm_sec);
    time_data[2] = convert_to_bcd(time.tm_min);
    time_data[3] = convert_to_bcd(time.tm_hour);
    time_data[4] = convert_to_bcd(time.tm_wday + 1); // Convert POSIX format (0-6) to DS3231 format (1-7)
    time_data[5] = convert_to_bcd(time.tm_mday);
    time_data[6] = convert_to_bcd(time.tm_mon + 1) + ((time.tm_year >= 100) ? 128 : 0); // Add century bit
    time_data[7] = convert_to_bcd(time.tm_year >= 100 ? time.tm_year - 100 : time.tm_year);

    esp_err_t ret = write_registers(rtc_handle, time_data, 8);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to set time: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Time set successfully");
    return ESP_OK;
}

/**
 * @brief Get current time from DS3231 as time_t (Unix timestamp).
 */
time_t node_rtc_ds3231_time_unix_get(rtc_handle_t rtc_handle)
{
    struct tm *rtc_time = node_rtc_ds3231_time_get(rtc_handle);
    if (rtc_time == NULL)
    {
        return -1;
    }

    time_t unix_time = mktime(rtc_time);
    free(rtc_time);
    return unix_time;
}

/**
 * @brief Set time on DS3231 using time_t (Unix timestamp).
 */
esp_err_t node_rtc_ds3231_time_unix_set(rtc_handle_t rtc_handle, time_t unix_time)
{
    struct tm *time_info = gmtime(&unix_time);
    if (time_info == NULL)
    {
        ESP_LOGE(TAG, "Failed to convert Unix time to struct tm");
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret = node_rtc_ds3231_time_set(rtc_handle, *time_info);
    return ret;
}

/**
 * @brief Get temperature from DS3231.
 */
float node_rtc_ds3231_temperature_get(rtc_handle_t rtc_handle)
{
    if (rtc_handle == NULL)
    {
        return -9999.0f;
    }

    uint8_t data[RTC_DATA_LENGTH];
    esp_err_t ret = read_registers(rtc_handle, data, RTC_DATA_LENGTH);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to read temperature registers: %s", esp_err_to_name(ret));
        return -9999.0f;
    }

    rtc_temperature_t temp_data;
    int16_t raw_temp = ((int16_t)data[RTC_REGISTER_TEMPERATURE + 1] << 8) | data[RTC_REGISTER_TEMPERATURE];
    temp_data.data = raw_temp;

    return temp_data.temp + (temp_data.quarter_degree * 0.25f);
}

/**
 * @brief Get control and status flags from DS3231.
 */
rtc_control_status_t *node_rtc_ds3231_control_status_get(rtc_handle_t rtc_handle)
{
    if (rtc_handle == NULL)
    {
        return NULL;
    }

    uint8_t data[RTC_DATA_LENGTH];
    esp_err_t ret = read_registers(rtc_handle, data, RTC_DATA_LENGTH);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to read control/status registers: %s", esp_err_to_name(ret));
        return NULL;
    }

    rtc_control_status_t *status = (rtc_control_status_t *)calloc(1, sizeof(rtc_control_status_t));
    if (status == NULL)
    {
        ESP_LOGE(TAG, "Failed to allocate memory for control status");
        return NULL;
    }

    uint16_t raw_status = ((uint16_t)data[RTC_REGISTER_CONTROL + 1] << 8) | data[RTC_REGISTER_CONTROL];
    status->data = raw_status;

    return status;
}

/**
 * @brief Set alarm 1 for day of week.
 */
esp_err_t node_rtc_ds3231_alarm1_day_of_week_set(rtc_handle_t rtc_handle, enum rtc_day_of_week_e dow,
                                                 int8_t hour, int8_t minutes, int8_t seconds)
{
    if (rtc_handle == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t alarm_data[5];
    alarm_data[0] = RTC_REGISTER_ALARM1_SECONDS;
    alarm_data[1] = convert_to_bcd(seconds);
    alarm_data[2] = convert_to_bcd(minutes);
    alarm_data[3] = convert_to_bcd(hour);
    alarm_data[4] = convert_to_bcd(dow) | 0b01000000; // Set dy/dt bit for day of week

    esp_err_t ret = write_registers(rtc_handle, alarm_data, 5);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to set alarm 1 day of week: %s", esp_err_to_name(ret));
    }

    return ret;
}

/**
 * @brief Set alarm 1 for day of month.
 */
esp_err_t node_rtc_ds3231_alarm1_day_of_month_set(rtc_handle_t rtc_handle, int8_t day,
                                                  int8_t hour, int8_t minutes, int8_t seconds)
{
    if (rtc_handle == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t alarm_data[5];
    alarm_data[0] = RTC_REGISTER_ALARM1_SECONDS;
    alarm_data[1] = convert_to_bcd(seconds);
    alarm_data[2] = convert_to_bcd(minutes);
    alarm_data[3] = convert_to_bcd(hour);
    alarm_data[4] = convert_to_bcd(day) & 0b10111111; // Clear dy/dt bit for day of month

    esp_err_t ret = write_registers(rtc_handle, alarm_data, 5);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to set alarm 1 day of month: %s", esp_err_to_name(ret));
    }

    return ret;
}

/**
 * @brief Set alarm 2 for day of week.
 */
esp_err_t node_rtc_ds3231_alarm2_day_of_week_set(rtc_handle_t rtc_handle, enum rtc_day_of_week_e dow,
                                                 int8_t hour, int8_t minutes)
{
    if (rtc_handle == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t alarm_data[4];
    alarm_data[0] = RTC_REGISTER_ALARM2_MINUTES;
    alarm_data[1] = convert_to_bcd(minutes);
    alarm_data[2] = convert_to_bcd(hour);
    alarm_data[3] = convert_to_bcd(dow) | 0b01000000; // Set dy/dt bit for day of week

    esp_err_t ret = write_registers(rtc_handle, alarm_data, 4);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to set alarm 2 day of week: %s", esp_err_to_name(ret));
    }

    return ret;
}

/**
 * @brief Set alarm 2 for day of month.
 */
esp_err_t node_rtc_ds3231_alarm2_day_of_month_set(rtc_handle_t rtc_handle, int8_t day,
                                                  int8_t hour, int8_t minutes)
{
    if (rtc_handle == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t alarm_data[4];
    alarm_data[0] = RTC_REGISTER_ALARM2_MINUTES;
    alarm_data[1] = convert_to_bcd(minutes);
    alarm_data[2] = convert_to_bcd(hour);
    alarm_data[3] = convert_to_bcd(day) & 0b10111111; // Clear dy/dt bit for day of month

    esp_err_t ret = write_registers(rtc_handle, alarm_data, 4);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to set alarm 2 day of month: %s", esp_err_to_name(ret));
    }

    return ret;
}

/**
 * @brief Enable or disable alarm 1.
 */
esp_err_t node_rtc_ds3231_alarm1_enable(rtc_handle_t rtc_handle, bool enabled)
{
    if (rtc_handle == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    rtc_control_status_t *status = node_rtc_ds3231_control_status_get(rtc_handle);
    if (status == NULL)
    {
        return ESP_FAIL;
    }

    status->alarm1_enable = enabled;

    uint8_t data[2];
    data[0] = RTC_REGISTER_CONTROL_UPPER;
    data[1] = (uint8_t)(status->data >> 8);

    esp_err_t ret = write_registers(rtc_handle, data, 2);
    free(status);

    return ret;
}

/**
 * @brief Enable or disable alarm 2.
 */
esp_err_t node_rtc_ds3231_alarm2_enable(rtc_handle_t rtc_handle, bool enabled)
{
    if (rtc_handle == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    rtc_control_status_t *status = node_rtc_ds3231_control_status_get(rtc_handle);
    if (status == NULL)
    {
        return ESP_FAIL;
    }

    status->alarm2_enable = enabled;

    uint8_t data[2];
    data[0] = RTC_REGISTER_CONTROL_UPPER;
    data[1] = (uint8_t)(status->data >> 8);

    esp_err_t ret = write_registers(rtc_handle, data, 2);
    free(status);

    return ret;
}

/**
 * @brief Check if alarm 1 has fired.
 */
bool node_rtc_ds3231_alarm1_fired(rtc_handle_t rtc_handle)
{
    if (rtc_handle == NULL)
    {
        return false;
    }

    rtc_control_status_t *status = node_rtc_ds3231_control_status_get(rtc_handle);
    if (status == NULL)
    {
        return false;
    }

    bool fired = status->alarm1_flag;
    free(status);
    return fired;
}

/**
 * @brief Check if alarm 2 has fired.
 */
bool node_rtc_ds3231_alarm2_fired(rtc_handle_t rtc_handle)
{
    if (rtc_handle == NULL)
    {
        return false;
    }

    rtc_control_status_t *status = node_rtc_ds3231_control_status_get(rtc_handle);
    if (status == NULL)
    {
        return false;
    }

    bool fired = status->alarm2_flag;
    free(status);
    return fired;
}

/**
 * @brief Reset alarm 1 fired flag.
 */
esp_err_t node_rtc_ds3231_alarm1_fired_reset(rtc_handle_t rtc_handle)
{
    if (rtc_handle == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    rtc_control_status_t *status = node_rtc_ds3231_control_status_get(rtc_handle);
    if (status == NULL)
    {
        return ESP_FAIL;
    }

    status->alarm1_flag = 0;

    uint8_t data[2];
    data[0] = RTC_REGISTER_CONTROL_LOWER;
    data[1] = (uint8_t)(status->data & 0xFF);

    esp_err_t ret = write_registers(rtc_handle, data, 2);
    free(status);

    return ret;
}

/**
 * @brief Reset alarm 2 fired flag.
 */
esp_err_t node_rtc_ds3231_alarm2_fired_reset(rtc_handle_t rtc_handle)
{
    if (rtc_handle == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    rtc_control_status_t *status = node_rtc_ds3231_control_status_get(rtc_handle);
    if (status == NULL)
    {
        return ESP_FAIL;
    }

    status->alarm2_flag = 0;

    uint8_t data[2];
    data[0] = RTC_REGISTER_CONTROL_LOWER;
    data[1] = (uint8_t)(status->data & 0xFF);

    esp_err_t ret = write_registers(rtc_handle, data, 2);
    free(status);

    return ret;
}

/**
 * @brief Set square wave frequency.
 */
esp_err_t node_rtc_ds3231_square_wave_freq_set(rtc_handle_t rtc_handle, enum rtc_square_wave_freq_e frequency)
{
    if (rtc_handle == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    rtc_control_status_t *status = node_rtc_ds3231_control_status_get(rtc_handle);
    if (status == NULL)
    {
        return ESP_FAIL;
    }

    status->square_freq = frequency;

    uint8_t data[2];
    data[0] = RTC_REGISTER_CONTROL_UPPER;
    data[1] = (uint8_t)(status->data >> 8);

    esp_err_t ret = write_registers(rtc_handle, data, 2);
    free(status);

    return ret;
}

/**
 * @brief Get square wave frequency.
 */
enum rtc_square_wave_freq_e node_rtc_ds3231_square_wave_freq_get(rtc_handle_t rtc_handle)
{
    if (rtc_handle == NULL)
    {
        return RTC_SQUARE_WAVE_FREQ_1000HZ;
    }

    rtc_control_status_t *status = node_rtc_ds3231_control_status_get(rtc_handle);
    if (status == NULL)
    {
        return RTC_SQUARE_WAVE_FREQ_1000HZ;
    }

    enum rtc_square_wave_freq_e freq = status->square_freq;
    free(status);
    return freq;
}

/**
 * @brief Set interrupt/square wave control.
 */
esp_err_t node_rtc_ds3231_interrupt_square_wave_control_set(rtc_handle_t rtc_handle, enum rtc_intr_sqr_e mode)
{
    if (rtc_handle == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    rtc_control_status_t *status = node_rtc_ds3231_control_status_get(rtc_handle);
    if (status == NULL)
    {
        return ESP_FAIL;
    }

    status->interrup_control = mode;

    uint8_t data[2];
    data[0] = RTC_REGISTER_CONTROL_UPPER;
    data[1] = (uint8_t)(status->data >> 8);

    esp_err_t ret = write_registers(rtc_handle, data, 2);
    free(status);

    return ret;
}

/**
 * @brief Get interrupt/square wave control.
 */
enum rtc_intr_sqr_e node_rtc_ds3231_interrupt_square_wave_control_get(rtc_handle_t rtc_handle)
{
    if (rtc_handle == NULL)
    {
        return square;
    }

    rtc_control_status_t *status = node_rtc_ds3231_control_status_get(rtc_handle);
    if (status == NULL)
    {
        return square;
    }

    enum rtc_intr_sqr_e mode = (enum rtc_intr_sqr_e)status->interrup_control;
    free(status);
    return mode;
}

/**
 * @brief Debug function to print all DS3231 register data.
 */
void node_rtc_ds3231_debug_print_data(rtc_handle_t rtc_handle)
{
    if (rtc_handle == NULL)
    {
        ESP_LOGE(TAG, "Invalid RTC handle");
        return;
    }

    uint8_t data[RTC_DATA_LENGTH];
    esp_err_t ret = read_registers(rtc_handle, data, RTC_DATA_LENGTH);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to read registers: %s", esp_err_to_name(ret));
        return;
    }

    ESP_LOGI(TAG, "DS3231 Register Data:");
    for (int i = 0; i < RTC_DATA_LENGTH; i++)
    {
        ESP_LOGI(TAG, "Register 0x%02X: 0x%02X (%d)", i, data[i], data[i]);
    }

    // Print current time
    struct tm *current_time = node_rtc_ds3231_time_get(rtc_handle);
    if (current_time != NULL)
    {
        char time_str[64];
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", current_time);
        ESP_LOGI(TAG, "Current time: %s", time_str);
        free(current_time);
    }

    // Print temperature
    float temperature = node_rtc_ds3231_temperature_get(rtc_handle);
    ESP_LOGI(TAG, "Temperature: %.2f°C", temperature);
}

// Helper function implementations

/**
 * @brief Convert decimal to BCD
 */
static uint8_t convert_to_bcd(uint8_t value)
{
    uint8_t tens = (value / 10) << 4;
    uint8_t ones = value % 10;
    return tens | ones;
}

/**
 * @brief Convert BCD to decimal
 */
static uint8_t convert_from_bcd(uint8_t bcd_value)
{
    return ((bcd_value >> 4) * 10) + (bcd_value & 0x0F);
}

/**
 * @brief Read registers from DS3231
 */
static esp_err_t read_registers(rtc_handle_t rtc_handle, uint8_t *data, size_t length)
{
    uint8_t reg_addr = 0x00; // Start from register 0
    return i2c_write_read_data(rtc_handle, &reg_addr, 1, data, length, RTC_I2C_TIMEOUT);
}

/**
 * @brief Write single register to DS3231
 */
static esp_err_t write_register(rtc_handle_t rtc_handle, uint8_t reg_addr, uint8_t data)
{
    uint8_t write_data[2] = {reg_addr, data};
    return i2c_write_data(rtc_handle, write_data, 2, RTC_I2C_TIMEOUT);
}

/**
 * @brief Write multiple registers to DS3231
 */
static esp_err_t write_registers(rtc_handle_t rtc_handle, uint8_t *data, size_t length)
{
    return i2c_write_data(rtc_handle, data, length, RTC_I2C_TIMEOUT);
}

/**
 * @brief Get seconds or minutes from DS3231
 */
static int8_t get_seconds_or_minutes(rtc_handle_t rtc_handle, enum rtc_register_e reg)
{
    uint8_t data[RTC_DATA_LENGTH];
    esp_err_t ret = read_registers(rtc_handle, data, RTC_DATA_LENGTH);
    if (ret != ESP_OK)
    {
        return -1;
    }

    rtc_seconds_or_minutes_t value = {.data = data[reg]};
    return (value.tens * 10) + value.ones;
}

/**
 * @brief Get hours from DS3231
 */
static int8_t get_hours(rtc_handle_t rtc_handle, enum rtc_register_e reg)
{
    uint8_t data[RTC_DATA_LENGTH];
    esp_err_t ret = read_registers(rtc_handle, data, RTC_DATA_LENGTH);
    if (ret != ESP_OK)
    {
        return -1;
    }

    rtc_hours_t value = {.data = data[reg]};
    return (value.ampm_hour_twenties * 20) + (value.tens * 10) + value.ones;
}

/**
 * @brief Get day of week from DS3231
 */
static int8_t get_day_of_week(rtc_handle_t rtc_handle, enum rtc_register_e reg)
{
    uint8_t data[RTC_DATA_LENGTH];
    esp_err_t ret = read_registers(rtc_handle, data, RTC_DATA_LENGTH);
    if (ret != ESP_OK)
    {
        return -1;
    }

    rtc_day_date_t value = {.data = data[reg]};
    if (reg == RTC_REGISTER_TIME_DAY)
    {
        return value.day_date;
    }
    else
    {
        if (value.dy_dt)
        {
            return value.day_date;
        }
    }
    return -1;
}

/**
 * @brief Get day of month from DS3231
 */
static int8_t get_day_of_month(rtc_handle_t rtc_handle, enum rtc_register_e reg)
{
    uint8_t data[RTC_DATA_LENGTH];
    esp_err_t ret = read_registers(rtc_handle, data, RTC_DATA_LENGTH);
    if (ret != ESP_OK)
    {
        return -1;
    }

    rtc_day_date_t value = {.data = data[reg]};
    if (reg == RTC_REGISTER_TIME_DATE)
    {
        return (value.tens * 10) + value.day_date;
    }
    else if (!value.dy_dt)
    {
        return (value.tens * 10) + value.day_date;
    }
    return -1;
}

/**
 * @brief Get month from DS3231
 */
static int8_t get_month(rtc_handle_t rtc_handle)
{
    uint8_t data[RTC_DATA_LENGTH];
    esp_err_t ret = read_registers(rtc_handle, data, RTC_DATA_LENGTH);
    if (ret != ESP_OK)
    {
        return -1;
    }

    rtc_month_century_t value = {.data = data[RTC_REGISTER_TIME_MONTH]};
    if (value.ones == 0)
    {
        value.ones = 1; // Handle power-on reset condition
    }
    return (value.tens * 10) + value.ones;
}

/**
 * @brief Get year from DS3231
 */
static int16_t get_year(rtc_handle_t rtc_handle)
{
    uint8_t data[RTC_DATA_LENGTH];
    esp_err_t ret = read_registers(rtc_handle, data, RTC_DATA_LENGTH);
    if (ret != ESP_OK)
    {
        return -1;
    }

    rtc_years_t year_value = {.data = data[RTC_REGISTER_TIME_YEAR]};
    rtc_month_century_t month_value = {.data = data[RTC_REGISTER_TIME_MONTH]};

    return 1900 + (month_value.century * 100) + (year_value.tens * 10) + year_value.ones;
}