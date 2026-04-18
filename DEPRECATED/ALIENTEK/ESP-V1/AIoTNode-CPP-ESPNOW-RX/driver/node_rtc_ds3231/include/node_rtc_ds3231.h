/**
 * @file node_rtc_ds3231.h
 * @author SHUAIWEN CUI (SHUAIWEN001@e.ntu.edu.sg)
 * @brief This file contains the function prototypes for DS3231 RTC module using our node_i2c module.
 * @version 1.0
 * @date 2025-01-27
 *
 * @copyright Copyright (c) 2025
 *
 */

#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include <sys/time.h>
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "node_i2c.h"

#define RTC_I2C_ADDRESS 0x68
#define RTC_DATA_LENGTH 0x13
#define RTC_I2C_TIMEOUT 1000

    /**
     * @brief Enumeration of all the register values of the DS3231 RTC chip
     */
    enum rtc_register_e
    {
        RTC_REGISTER_TIME_SECONDS = 0x00,    /*!< Time - Seconds register */
        RTC_REGISTER_TIME_MINUTES = 0x01,    /*!< Time - Minutes register */
        RTC_REGISTER_TIME_HOURS = 0x02,      /*!< Time - Hours register */
        RTC_REGISTER_TIME_DAY = 0x03,        /*!< Time - Days (Day of Week) register */
        RTC_REGISTER_TIME_DATE = 0x04,       /*!< Time - Date (Day of Month register */
        RTC_REGISTER_TIME_MONTH = 0x05,      /*!< Time - Month register */
        RTC_REGISTER_TIME_YEAR = 0x06,       /*!< Time - Year register */
        RTC_REGISTER_ALARM1_SECONDS = 0x07,  /*!< Alarm 1 - Seconds register */
        RTC_REGISTER_ALARM1_MINUTES = 0x08,  /*!< Alarm 1 - Minutes register */
        RTC_REGISTER_ALARM1_HOURS = 0x09,    /*!< Alarm 1 - Hours register */
        RTC_REGISTER_ALARM1_DAY_DATE = 0x0A, /*!< Alarm 1 - Day or Date register */
        RTC_REGISTER_ALARM2_MINUTES = 0x0B,  /*!< Alarm 2 - Minutes register */
        RTC_REGISTER_ALARM2_HOURS = 0x0C,    /*!< Alarm 2 - Hours register */
        RTC_REGISTER_ALARM2_DAY_DATE = 0x0D, /*!< Alarm 2 - Day or Date register */
        RTC_REGISTER_CONTROL = 0x0E,         /*!< Control & Status registers */
        RTC_REGISTER_CONTROL_UPPER = 0x0E,   /*!< Upper Control & Status registers */
        RTC_REGISTER_CONTROL_LOWER = 0x0F,   /*!< Lower Control & Status registers */
        RTC_REGISTER_AGING_OFFSET = 0x10,    /*!< Aging Offset register */
        RTC_REGISTER_TEMPERATURE = 0x11      /*!< Temperature registers */
    };

    /**
     * @brief Enumeration of all the Days of the Week values as used by the DS3231 chip.
     */
    enum rtc_day_of_week_e
    {
        RTC_MONDAY = 1,
        RTC_TUESDAY = 2,
        RTC_WEDNESDAY = 3,
        RTC_THURSDAY = 4,
        RTC_FRIDAY = 5,
        RTC_SATURDAY = 6,
        RTC_SUNDAY = 7
    };

    /**
     * @brief Enumeration of all the Days of the Week values as used by the POSIX time struct.
     */
    enum tm_day_of_week_e
    {
        TM_SUNDAY = 0,
        TM_MONDAY = 1,
        TM_TUESDAY = 2,
        TM_WEDNESDAY = 3,
        TM_THURSDAY = 4,
        TM_FRIDAY = 5,
        TM_SATURDAY = 6
    };

    /**
     * @brief Enumeration of the four frequency values available for output as a square wave on the DS3231
     */
    enum rtc_square_wave_freq_e
    {
        RTC_SQUARE_WAVE_FREQ_1000HZ = 0,
        RTC_SQUARE_WAVE_FREQ_1024HZ = 1,
        RTC_SQUARE_WAVE_FREQ_4096HZ = 2,
        RTC_SQUARE_WAVE_FREQ_8192HZ = 3
    };

    /**
     * @brief The alarm functionality of the DS3231 is versatile in its functionality.
     */
    enum rtc_alarm_rate_e
    {
        RTC_ALARM_MATCH_EVERY_SECOND = 15,                   /*!< Alarm 1 Only - The alarm will fire every second. */
        RTC_ALARM_MATCH_SECONDS_A1_OR_EVERY_MINUTES_A2 = 14, /*!< Alarm 1 Only - The alarm will fire when the seconds value in
                                                                the alarm matches the seconds value of the DS3231.
                                                                Alarm 2 Only - The alarm will fire every minute when the
                                                                seconds value on the DS3231 hits 00.*/
        RTC_ALARM_MATCH_MINUTES = 12,                        /*!< The alarm will fire every hour when the minutes (and seconds for Alarm 1) value matches the
                                                                the DS3231 minutes (and seconds for Alarm 1) value */
        RTC_ALARM_MATCH_HOURS = 8,                           /*!< The alarm will fire every day when the hour, minutes (and seconds for Alarm 1) value matches the
                                                                the DS3231 hour, minutes (and seconds for Alarm 1) value  */
        RTC_ALARM_MATCH_DAY_DATE = 0,                        /*!< The alarm will fire every day when the hour, minutes (and seconds for Alarm 1) value matches
                                                                the the DS3231 hour, minutes (and seconds for Alarm 1) value */
        RTC_ALARM_MATCH_INVALID = -999                       /*!< Invalid alarm rate */
    };

    enum rtc_intr_sqr_e
    {
        square = false,
        interrupts = true
    };

#pragma pack(push, 1)

    /**
     * @brief Union of the 2 control and status registers (0Eh and 0Fh) of the DS3231. All flags are active if 1.
     */
    typedef union rtc_control_status_t
    {
        uint16_t data; /*!< Native type representation of the data. */
        struct
        {
            unsigned alarm1_enable : 1;     /*!< Alarm 1 Enable. */
            unsigned alarm2_enable : 1;     /*!< Alarm 2 Enable. */
            unsigned interrup_control : 1;  /*!< Interrupt/Square Wave Control. Square Wave is active when set to 1. */
            unsigned square_freq : 2;       /*!< Square wave frequency control register (2 bits) */
            unsigned convert_temp : 1;      /*!< Force a temperature conversion. */
            unsigned battery_square : 1;    /*!< Run the square wave when on battery. */
            unsigned enable_oscillator : 1; /*!< Active 0 - When inactive, all registers are read only */
            unsigned alarm1_flag : 1;       /*!< Flag indicating Alarm 1 triggered. */
            unsigned alarm2_flag : 1;       /*!< Flag indicating Alarm 2 triggered. */
            unsigned busy_flag : 1;         /*!< Temperature conversion is processing */
            unsigned enable_32kHz_out : 1;  /*!< Enables output of 32.765kHz square wave. */
            unsigned unused : 3;
            unsigned oscillaror_stop_flag : 1; /*!< Indicates the oscillator is stopped. */
        };
    } rtc_control_status_t;

    /**
     * @brief Simple alias of i2c_master_dev_handle.
     */
    typedef i2c_master_dev_handle_t rtc_handle_t;

#pragma pack(pop)

    /**
     * @brief Initialize the DS3231 RTC Module using our node_i2c module.
     * @param scl_speed_hz SCL clock speed in Hz (default: 200000)
     * @retval rtc_handle_t: Handle required for subsequent operations.
     * @retval NULL: The handle creation failed. Check serial monitor for ERROR condition.
     */
    rtc_handle_t node_rtc_ds3231_init(uint32_t scl_speed_hz);

    /**
     * @brief Deinitialize the DS3231 RTC Module.
     * @param rtc_handle RTC handle to deinitialize
     * @return esp_err_t ESP_OK on success, error code on failure
     */
    esp_err_t node_rtc_ds3231_deinit(rtc_handle_t rtc_handle);

    /**
     * @brief Get current time from DS3231 as struct tm.
     * @note The returned value must be freed by the caller to prevent memory leakage
     * @param rtc_handle RTC handle
     * @return struct tm* Pointer to struct tm populated with the current time.
     */
    struct tm *node_rtc_ds3231_time_get(rtc_handle_t rtc_handle);

    /**
     * @brief Set time on DS3231 using struct tm.
     * @param rtc_handle RTC handle
     * @param time struct tm with time values
     * @return esp_err_t ESP_OK on success, error code on failure
     */
    esp_err_t node_rtc_ds3231_time_set(rtc_handle_t rtc_handle, struct tm time);

    /**
     * @brief Get current time from DS3231 as time_t (Unix timestamp).
     * @param rtc_handle RTC handle
     * @return time_t Unix timestamp
     */
    time_t node_rtc_ds3231_time_unix_get(rtc_handle_t rtc_handle);

    /**
     * @brief Set time on DS3231 using time_t (Unix timestamp).
     * @param rtc_handle RTC handle
     * @param unix_time Unix timestamp
     * @return esp_err_t ESP_OK on success, error code on failure
     */
    esp_err_t node_rtc_ds3231_time_unix_set(rtc_handle_t rtc_handle, time_t unix_time);

    /**
     * @brief Get temperature from DS3231.
     * @param rtc_handle RTC handle
     * @return float Temperature in Celsius
     */
    float node_rtc_ds3231_temperature_get(rtc_handle_t rtc_handle);

    /**
     * @brief Get control and status flags from DS3231.
     * @note The returned value must be freed by the caller to prevent memory leakage
     * @param rtc_handle RTC handle
     * @return rtc_control_status_t* Pointer to control status structure
     */
    rtc_control_status_t *node_rtc_ds3231_control_status_get(rtc_handle_t rtc_handle);

    /**
     * @brief Set alarm 1 for day of week.
     * @param rtc_handle RTC handle
     * @param dow Day of week (1-7, Monday=1)
     * @param hour Hour (0-23)
     * @param minutes Minutes (0-59)
     * @param seconds Seconds (0-59)
     * @return esp_err_t ESP_OK on success, error code on failure
     */
    esp_err_t node_rtc_ds3231_alarm1_day_of_week_set(rtc_handle_t rtc_handle, enum rtc_day_of_week_e dow,
                                                     int8_t hour, int8_t minutes, int8_t seconds);

    /**
     * @brief Set alarm 1 for day of month.
     * @param rtc_handle RTC handle
     * @param day Day of month (1-31)
     * @param hour Hour (0-23)
     * @param minutes Minutes (0-59)
     * @param seconds Seconds (0-59)
     * @return esp_err_t ESP_OK on success, error code on failure
     */
    esp_err_t node_rtc_ds3231_alarm1_day_of_month_set(rtc_handle_t rtc_handle, int8_t day,
                                                      int8_t hour, int8_t minutes, int8_t seconds);

    /**
     * @brief Set alarm 2 for day of week.
     * @param rtc_handle RTC handle
     * @param dow Day of week (1-7, Monday=1)
     * @param hour Hour (0-23)
     * @param minutes Minutes (0-59)
     * @return esp_err_t ESP_OK on success, error code on failure
     */
    esp_err_t node_rtc_ds3231_alarm2_day_of_week_set(rtc_handle_t rtc_handle, enum rtc_day_of_week_e dow,
                                                     int8_t hour, int8_t minutes);

    /**
     * @brief Set alarm 2 for day of month.
     * @param rtc_handle RTC handle
     * @param day Day of month (1-31)
     * @param hour Hour (0-23)
     * @param minutes Minutes (0-59)
     * @return esp_err_t ESP_OK on success, error code on failure
     */
    esp_err_t node_rtc_ds3231_alarm2_day_of_month_set(rtc_handle_t rtc_handle, int8_t day,
                                                      int8_t hour, int8_t minutes);

    /**
     * @brief Enable or disable alarm 1.
     * @param rtc_handle RTC handle
     * @param enabled true to enable, false to disable
     * @return esp_err_t ESP_OK on success, error code on failure
     */
    esp_err_t node_rtc_ds3231_alarm1_enable(rtc_handle_t rtc_handle, bool enabled);

    /**
     * @brief Enable or disable alarm 2.
     * @param rtc_handle RTC handle
     * @param enabled true to enable, false to disable
     * @return esp_err_t ESP_OK on success, error code on failure
     */
    esp_err_t node_rtc_ds3231_alarm2_enable(rtc_handle_t rtc_handle, bool enabled);

    /**
     * @brief Check if alarm 1 has fired.
     * @param rtc_handle RTC handle
     * @return bool true if alarm 1 has fired, false otherwise
     */
    bool node_rtc_ds3231_alarm1_fired(rtc_handle_t rtc_handle);

    /**
     * @brief Check if alarm 2 has fired.
     * @param rtc_handle RTC handle
     * @return bool true if alarm 2 has fired, false otherwise
     */
    bool node_rtc_ds3231_alarm2_fired(rtc_handle_t rtc_handle);

    /**
     * @brief Reset alarm 1 fired flag.
     * @param rtc_handle RTC handle
     * @return esp_err_t ESP_OK on success, error code on failure
     */
    esp_err_t node_rtc_ds3231_alarm1_fired_reset(rtc_handle_t rtc_handle);

    /**
     * @brief Reset alarm 2 fired flag.
     * @param rtc_handle RTC handle
     * @return esp_err_t ESP_OK on success, error code on failure
     */
    esp_err_t node_rtc_ds3231_alarm2_fired_reset(rtc_handle_t rtc_handle);

    /**
     * @brief Set square wave frequency.
     * @param rtc_handle RTC handle
     * @param frequency Square wave frequency
     * @return esp_err_t ESP_OK on success, error code on failure
     */
    esp_err_t node_rtc_ds3231_square_wave_freq_set(rtc_handle_t rtc_handle, enum rtc_square_wave_freq_e frequency);

    /**
     * @brief Get square wave frequency.
     * @param rtc_handle RTC handle
     * @return enum rtc_square_wave_freq_e Square wave frequency
     */
    enum rtc_square_wave_freq_e node_rtc_ds3231_square_wave_freq_get(rtc_handle_t rtc_handle);

    /**
     * @brief Set interrupt/square wave control.
     * @param rtc_handle RTC handle
     * @param mode Interrupt or square wave mode
     * @return esp_err_t ESP_OK on success, error code on failure
     */
    esp_err_t node_rtc_ds3231_interrupt_square_wave_control_set(rtc_handle_t rtc_handle, enum rtc_intr_sqr_e mode);

    /**
     * @brief Get interrupt/square wave control.
     * @param rtc_handle RTC handle
     * @return enum rtc_intr_sqr_e Current mode
     */
    enum rtc_intr_sqr_e node_rtc_ds3231_interrupt_square_wave_control_get(rtc_handle_t rtc_handle);

    /**
     * @brief Debug function to print all DS3231 register data.
     * @param rtc_handle RTC handle
     */
    void node_rtc_ds3231_debug_print_data(rtc_handle_t rtc_handle);

#ifdef __cplusplus
}
#endif