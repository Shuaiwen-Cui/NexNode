/**
 * @file adxl355.h
 * @author SHUAIWEN CUI (SHUAIWEN001@e.ntu.edu.sg)
 * @brief ADXL355 3-axis accelerometer driver
 * @version 1.0
 * @date 2025-08-23
 * @copyright Copyright (c) 2025
 *
 */

#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "driver/spi_master.h"

//==============================================================================
// GPIO Pin Definitions
//==============================================================================

/// @brief ADXL355 CS (Chip Select) GPIO pin
#define ADXL355_CS_GPIO_PIN GPIO_NUM_4

//==============================================================================
// Constants (exactly like plasmapper)
//==============================================================================

/// @brief Default SPI frequency
#define ADXL355_DEFAULT_SPI_FREQUENCY 10000000

/// @brief Temperature intercept, LSB
#define ADXL355_TEMPERATURE_INTERCEPT_LSB 1885

/// @brief Temperature intercept, °C
#define ADXL355_TEMPERATURE_INTERCEPT_DEG_C 25.0f

/// @brief Temperature slope, LSB/°C
#define ADXL355_TEMPERATURE_SLOPE -9.05f

/// @brief Acceleration scale factor for ±2 g range, g/LSB
#define ADXL355_ACCELERATION_SCALE_FACTOR_RANGE_2G 3.9e-6f

/// @brief Acceleration scale factor for ±4 g range, g/LSB
#define ADXL355_ACCELERATION_SCALE_FACTOR_RANGE_4G 7.8e-6f

/// @brief Acceleration scale factor for ±8 g range, g/LSB
#define ADXL355_ACCELERATION_SCALE_FACTOR_RANGE_8G 15.6e-6f

/// @brief Maximum number of the FIFO samples
#define ADXL355_MAX_NUMBER_OF_FIFO_SAMPLES 96

//==============================================================================
// SPI Configuration (exactly like plasmapper)
//==============================================================================

/// @brief SPI mode (8 = Mode 0 in ESP-IDF)
#define ADXL355_SPI_MODE 8

/// @brief Number of address bits
#define ADXL355_SPI_NUMBER_OF_ADDRESS_BITS 8

/// @brief Max SCLK-MISO delay in ns
#define ADXL355_SPI_MAX_SCLK_MISO_DELAY 30

    //==============================================================================
    // Register Addresses (exactly like plasmapper)
    //==============================================================================

#define ADXL355_REG_DEVID_AD 0x00
#define ADXL355_REG_DEVID_MST 0x01
#define ADXL355_REG_PARTID 0x02
#define ADXL355_REG_REVID 0x03
#define ADXL355_REG_STATUS 0x04
#define ADXL355_REG_FIFO_ENTRIES 0x05
#define ADXL355_REG_TEMP2 0x06
#define ADXL355_REG_TEMP1 0x07
#define ADXL355_REG_XDATA3 0x08
#define ADXL355_REG_XDATA2 0x09
#define ADXL355_REG_XDATA1 0x0A
#define ADXL355_REG_YDATA3 0x0B
#define ADXL355_REG_YDATA2 0x0C
#define ADXL355_REG_YDATA1 0x0D
#define ADXL355_REG_ZDATA3 0x0E
#define ADXL355_REG_ZDATA2 0x0F
#define ADXL355_REG_ZDATA1 0x10
#define ADXL355_REG_FIFO_DATA 0x11
#define ADXL355_REG_OFFSET_X_H 0x1E
#define ADXL355_REG_OFFSET_X_L 0x1F
#define ADXL355_REG_OFFSET_Y_H 0x20
#define ADXL355_REG_OFFSET_Y_L 0x21
#define ADXL355_REG_OFFSET_Z_H 0x22
#define ADXL355_REG_OFFSET_Z_L 0x23
#define ADXL355_REG_ACT_EN 0x24
#define ADXL355_REG_ACT_THRESH_H 0x25
#define ADXL355_REG_ACT_THRESH_L 0x26
#define ADXL355_REG_ACT_COUNT 0x27
#define ADXL355_REG_FILTER 0x28
#define ADXL355_REG_FIFO_SAMPLES 0x29
#define ADXL355_REG_INT_MAP 0x2A
#define ADXL355_REG_SYNC 0x2B
#define ADXL355_REG_RANGE 0x2C
#define ADXL355_REG_POWER_CTL 0x2D
#define ADXL355_REG_SELF_TEST 0x2E
#define ADXL355_REG_RESET 0x2F

    //==============================================================================
    // Register Bit Masks (exactly like plasmapper)
    //==============================================================================

#define ADXL355_REG_STATUS_DATA_RDY 0x01
#define ADXL355_REG_STATUS_FIFO_FULL 0x02
#define ADXL355_REG_STATUS_FIFO_OVR 0x04
#define ADXL355_REG_STATUS_ACTIVITY 0x08
#define ADXL355_REG_STATUS_NVM_BUSY 0x10

#define ADXL355_REG_FILTER_HPF_SHIFT 4
#define ADXL355_REG_FILTER_HPF_MASK 0xF0
#define ADXL355_REG_FILTER_ODR_MASK 0x0F

#define ADXL355_REG_SYNC_SYNC_MAP 0x03
#define ADXL355_REG_SYNC_EXT_CLK 0x04

#define ADXL355_REG_RANGE_RANGE_MASK 0x03
#define ADXL355_REG_RANGE_RANGE_PM2G 0x01
#define ADXL355_REG_RANGE_RANGE_PM4G 0x02
#define ADXL355_REG_RANGE_RANGE_PM8G 0x03
#define ADXL355_REG_RANGE_INT_POL_SHIFT 6
#define ADXL355_REG_RANGE_INT_POL_MASK 0x40
#define ADXL355_REG_RANGE_I2C_HS_SHIFT 7
#define ADXL355_REG_RANGE_I2C_HS_MASK 0x80

#define ADXL355_REG_POWER_CTL_STANDBY 0x01
#define ADXL355_REG_POWER_CTL_TEMP_OFF 0x02
#define ADXL355_REG_POWER_CTL_DRDY_OFF 0x04

#define ADXL355_REG_SELF_TEST_ST1 0x01
#define ADXL355_REG_SELF_TEST_ST2 0x02

#define ADXL355_REG_RESET_RESET_CODE 0x52

    //==============================================================================
    // Enums (exactly like plasmapper)
    //==============================================================================

    /// @brief ADXL355 status
    typedef enum
    {
        ADXL355_STATUS_DATA_RDY = 0x01,  ///< Data ready
        ADXL355_STATUS_FIFO_FULL = 0x02, ///< FIFO full
        ADXL355_STATUS_FIFO_OVR = 0x04,  ///< FIFO overrun
        ADXL355_STATUS_ACTIVITY = 0x08,  ///< Activity detected
        ADXL355_STATUS_NVM_BUSY = 0x10   ///< NVM busy
    } adxl355_status_t;

    /// @brief ADXL355 high-pass filter frequency
    typedef enum
    {
        ADXL355_HPF_NONE = 0x00,   ///< High-pass filter disabled
        ADXL355_HPF_24_7 = 0x01,   ///< ODR*24.700e-4
        ADXL355_HPF_6_2084 = 0x02, ///< ODR*6.2084e-4
        ADXL355_HPF_1_5545 = 0x03, ///< ODR*1.5545e-4
        ADXL355_HPF_0_3862 = 0x04, ///< ODR*0.3862e-4
        ADXL355_HPF_0_0954 = 0x05, ///< ODR*0.0954e-4
        ADXL355_HPF_0_0238 = 0x06  ///< ODR*0.0238e-4
    } adxl355_hpf_frequency_t;

    /// @brief ADXL355 output data rate
    typedef enum
    {
        ADXL355_ODR_4000 = 0x00,   ///< 4000 Hz
        ADXL355_ODR_2000 = 0x01,   ///< 2000 Hz
        ADXL355_ODR_1000 = 0x02,   ///< 1000 Hz
        ADXL355_ODR_500 = 0x03,    ///< 500 Hz
        ADXL355_ODR_250 = 0x04,    ///< 250 Hz
        ADXL355_ODR_125 = 0x05,    ///< 125 Hz
        ADXL355_ODR_62_5 = 0x06,   ///< 62.5 Hz
        ADXL355_ODR_31_25 = 0x07,  ///< 31.25 Hz
        ADXL355_ODR_15_625 = 0x08, ///< 15.625 Hz
        ADXL355_ODR_7_813 = 0x09,  ///< 7.813 Hz
        ADXL355_ODR_3_906 = 0x0A   ///< 3.906 Hz
    } adxl355_output_data_rate_t;

    /// @brief ADXL355 acceleration range
    typedef enum
    {
        ADXL355_RANGE_2G = 0x01, ///< ±2 g
        ADXL355_RANGE_4G = 0x02, ///< ±4 g
        ADXL355_RANGE_8G = 0x03  ///< ±8 g
    } adxl355_range_t;

    /// @brief ADXL355 interrupt polarity
    typedef enum
    {
        ADXL355_INT_POL_ACTIVE_LOW = 0x00, ///< Active low
        ADXL355_INT_POL_ACTIVE_HIGH = 0x01 ///< Active high
    } adxl355_interrupt_polarity_t;

    /// @brief ADXL355 I2C speed
    typedef enum
    {
        ADXL355_I2C_SPEED_FAST = 0x00,      ///< Fast
        ADXL355_I2C_SPEED_HIGH_SPEED = 0x01 ///< High speed
    } adxl355_i2c_speed_t;

    /// @brief ADXL355 synchronization
    typedef enum
    {
        ADXL355_SYNC_INTERNAL = 0x00,                   ///< Internal
        ADXL355_SYNC_EXTERNAL = 0x01,                   ///< External
        ADXL355_SYNC_EXTERNAL_WITH_INTERPOLATION = 0x02 ///< External with interpolation
    } adxl355_synchronization_t;

    //==============================================================================
    // Data Structures (exactly like plasmapper)
    //==============================================================================

    /// @brief ADXL355 device information
    typedef struct
    {
        uint8_t vendor_id;   ///< Vendor ID
        uint8_t family_id;   ///< Device family ID
        uint8_t device_id;   ///< Device ID
        uint8_t revision_id; ///< Revision ID
    } adxl355_device_info_t;

    /// @brief ADXL355 raw accelerations
    typedef struct
    {
        int32_t x; ///< Raw X-axis acceleration
        int32_t y; ///< Raw Y-axis acceleration
        int32_t z; ///< Raw Z-axis acceleration
    } adxl355_raw_accelerations_t;

    /// @brief ADXL355 accelerations in g
    typedef struct
    {
        float x; ///< X-axis acceleration, g
        float y; ///< Y-axis acceleration, g
        float z; ///< Z-axis acceleration, g
    } adxl355_accelerations_t;

    /// @brief ADXL355 handle
    typedef struct
    {
        spi_device_handle_t spi_handle; ///< SPI device handle
        adxl355_range_t range;          ///< Current range setting
        adxl355_output_data_rate_t odr; ///< Current ODR setting
        float scale_factor;             ///< Current scale factor
    } adxl355_handle_t;

    //==============================================================================
    // Function Declarations
    //==============================================================================

    /**
     * @brief Initialize ADXL355 (including SPI device configuration)
     * @param handle ADXL355 handle
     * @param range Initial range setting
     * @param odr Initial output data rate
     * @return ESP_OK on success, error code on failure
     */
    esp_err_t adxl355_init(adxl355_handle_t *handle, adxl355_range_t range, adxl355_output_data_rate_t odr);

    /**
     * @brief Deinitialize ADXL355
     * @param handle ADXL355 handle
     * @return ESP_OK on success, error code on failure
     */
    esp_err_t adxl355_deinit(adxl355_handle_t *handle);

    /**
     * @brief Reset ADXL355 device
     * @param handle ADXL355 handle
     * @return ESP_OK on success, error code on failure
     */
    esp_err_t adxl355_reset(adxl355_handle_t *handle);

    /**
     * @brief Read device information
     * @param handle ADXL355 handle
     * @param device_info Device information output
     * @return ESP_OK on success, error code on failure
     */
    esp_err_t adxl355_read_device_info(adxl355_handle_t *handle, adxl355_device_info_t *device_info);

    /**
     * @brief Read device status
     * @param handle ADXL355 handle
     * @param status Status output
     * @return ESP_OK on success, error code on failure
     */
    esp_err_t adxl355_read_status(adxl355_handle_t *handle, adxl355_status_t *status);

    /**
     * @brief Read raw accelerations
     * @param handle ADXL355 handle
     * @param raw_accel Raw accelerations output
     * @return ESP_OK on success, error code on failure
     */
    esp_err_t adxl355_read_raw_accelerations(adxl355_handle_t *handle, adxl355_raw_accelerations_t *raw_accel);

    /**
     * @brief Read accelerations in g
     * @param handle ADXL355 handle
     * @param accel Accelerations output in g
     * @return ESP_OK on success, error code on failure
     */
    esp_err_t adxl355_read_accelerations(adxl355_handle_t *handle, adxl355_accelerations_t *accel);

    /**
     * @brief Read raw temperature
     * @param handle ADXL355 handle
     * @param raw_temp Raw temperature output
     * @return ESP_OK on success, error code on failure
     */
    esp_err_t adxl355_read_raw_temperature(adxl355_handle_t *handle, uint16_t *raw_temp);

    /**
     * @brief Read temperature in °C
     * @param handle ADXL355 handle
     * @param temp Temperature output in °C
     * @return ESP_OK on success, error code on failure
     */
    esp_err_t adxl355_read_temperature(adxl355_handle_t *handle, float *temp);

    /**
     * @brief Set acceleration range
     * @param handle ADXL355 handle
     * @param range Range setting
     * @return ESP_OK on success, error code on failure
     */
    esp_err_t adxl355_set_range(adxl355_handle_t *handle, adxl355_range_t range);

    /**
     * @brief Read acceleration range
     * @param handle ADXL355 handle
     * @param range Range output
     * @return ESP_OK on success, error code on failure
     */
    esp_err_t adxl355_read_range(adxl355_handle_t *handle, adxl355_range_t *range);

    /**
     * @brief Set output data rate
     * @param handle ADXL355 handle
     * @param odr Output data rate
     * @return ESP_OK on success, error code on failure
     */
    esp_err_t adxl355_set_odr(adxl355_handle_t *handle, adxl355_output_data_rate_t odr);

    /**
     * @brief Read output data rate
     * @param handle ADXL355 handle
     * @param odr Output data rate output
     * @return ESP_OK on success, error code on failure
     */
    esp_err_t adxl355_read_odr(adxl355_handle_t *handle, adxl355_output_data_rate_t *odr);

    /**
     * @brief Enable measurement mode
     * @param handle ADXL355 handle
     * @return ESP_OK on success, error code on failure
     */
    esp_err_t adxl355_enable_measurement(adxl355_handle_t *handle);

    /**
     * @brief Disable measurement mode (standby)
     * @param handle ADXL355 handle
     * @return ESP_OK on success, error code on failure
     */
    esp_err_t adxl355_disable_measurement(adxl355_handle_t *handle);

    /**
     * @brief Check if measurement is enabled
     * @param handle ADXL355 handle
     * @param is_enabled Output: true if measurement enabled
     * @return ESP_OK on success, error code on failure
     */
    esp_err_t adxl355_is_measurement_enabled(adxl355_handle_t *handle, bool *is_enabled);

    /**
     * @brief Enable temperature processing
     * @param handle ADXL355 handle
     * @return ESP_OK on success, error code on failure
     */
    esp_err_t adxl355_enable_temperature(adxl355_handle_t *handle);

    /**
     * @brief Disable temperature processing
     * @param handle ADXL355 handle
     * @return ESP_OK on success, error code on failure
     */
    esp_err_t adxl355_disable_temperature(adxl355_handle_t *handle);

    /**
     * @brief Check if temperature is enabled
     * @param handle ADXL355 handle
     * @param is_enabled Output: true if temperature enabled
     * @return ESP_OK on success, error code on failure
     */
    esp_err_t adxl355_is_temperature_enabled(adxl355_handle_t *handle, bool *is_enabled);

    /**
     * @brief Enable data ready output
     * @param handle ADXL355 handle
     * @return ESP_OK on success, error code on failure
     */
    esp_err_t adxl355_enable_data_ready(adxl355_handle_t *handle);

    /**
     * @brief Disable data ready output
     * @param handle ADXL355 handle
     * @return ESP_OK on success, error code on failure
     */
    esp_err_t adxl355_disable_data_ready(adxl355_handle_t *handle);

    /**
     * @brief Check if data ready is enabled
     * @param handle ADXL355 handle
     * @param is_enabled Output: true if data ready enabled
     * @return ESP_OK on success, error code on failure
     */
    esp_err_t adxl355_is_data_ready_enabled(adxl355_handle_t *handle, bool *is_enabled);

    /**
     * @brief Read acceleration scale factor
     * @param handle ADXL355 handle
     * @param scale_factor Scale factor output
     * @return ESP_OK on success, error code on failure
     */
    esp_err_t adxl355_read_acceleration_scale_factor(adxl355_handle_t *handle, float *scale_factor);

    /**
     * @brief ADXL355 specific: Write command then read data
     * @param handle ADXL355 handle
     * @param cmd Command byte to send
     * @param data Buffer to store received data
     * @param len Length of data to read
     * @return ESP_OK on success, error code on failure
     */
    esp_err_t adxl355_spi_read(adxl355_handle_t *handle, uint8_t cmd, uint8_t *data, int len);

#ifdef __cplusplus
}
#endif
