# ADXL355 代码

## 组件架构

```plaintext
- driver
    - node_acc_adxl355
        - include
            - node_acc_adxl355.h
        - node_acc_adxl355.c
        - CMakeLists.txt
```

## driver/node_acc_adxl355/CMakeLists.txt

```cmake
set(src_dirs
    .
)

set(include_dirs
    include
)

set(requires
    node_spi    
)

idf_component_register(SRC_DIRS ${src_dirs} INCLUDE_DIRS ${include_dirs} REQUIRES ${requires})
```

## node_acc_adxl355.h

```c
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


```

## node_acc_adxl355.c

```c
/**
 * @file adxl355.c
 * @author SHUAIWEN CUI (SHUAIWEN001@e.ntu.edu.sg)
 * @brief ADXL355 3-axis accelerometer driver implementation
 * @version 1.0
 * @date 2025-08-23
 * @copyright Copyright (c) 2025
 *
 */

#include "node_acc_adxl355.h"
#include "node_spi.h"
#include "esp_log.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

//==============================================================================

static const char *TAG = "ADXL355";

//==============================================================================

/* Private function declarations */
static esp_err_t adxl355_configure_spi_device(spi_device_handle_t *device_handle);
static esp_err_t adxl355_read_register(adxl355_handle_t *handle, uint8_t reg, uint8_t *value);
static esp_err_t adxl355_read_registers(adxl355_handle_t *handle, uint8_t reg, uint8_t *data, uint8_t len);
static esp_err_t adxl355_write_register(adxl355_handle_t *handle, uint8_t reg, uint8_t value);
static esp_err_t adxl355_write_registers(adxl355_handle_t *handle, uint8_t reg, const uint8_t *data, uint8_t len);
static const char *adxl355_get_range_string(adxl355_range_t range);
static const char *adxl355_get_odr_string(adxl355_output_data_rate_t odr);

//==============================================================================

esp_err_t adxl355_init(adxl355_handle_t *handle, adxl355_range_t range, adxl355_output_data_rate_t odr)
{
    esp_err_t ret;

    ESP_LOGI(TAG, "=== ADXL355 INITIALIZATION START ===");
    ESP_LOGI(TAG, "Initialization parameters: range=%s, ODR=%s",
             adxl355_get_range_string(range), adxl355_get_odr_string(odr));

    if (handle == NULL)
    {
        ESP_LOGE(TAG, "Invalid argument: handle=%p", (void *)handle);
        return ESP_ERR_INVALID_ARG;
    }

    /* Step 1: Configure SPI device for ADXL355 */
    ESP_LOGI(TAG, "Configuring SPI device for ADXL355...");
    ESP_LOGI(TAG, "Note: SPI3 bus is initialized with DMA enabled (SPI_DMA_CH_AUTO)");
    ESP_LOGI(TAG, "DMA will be used automatically for SPI transactions to reduce CPU usage");
    ret = adxl355_configure_spi_device(&handle->spi_handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to configure SPI device: %s", esp_err_to_name(ret));
        return ret;
    }
    ESP_LOGI(TAG, "SPI device configured successfully (DMA enabled)");

    /* Initialize handle */
    handle->range = range;
    handle->odr = odr;

    /* Set initial scale factor based on range */
    ESP_LOGI(TAG, "Setting scale factor for range %s...", adxl355_get_range_string(range));
    switch (range)
    {
    case ADXL355_RANGE_2G:
        handle->scale_factor = ADXL355_ACCELERATION_SCALE_FACTOR_RANGE_2G;
        ESP_LOGI(TAG, "Range %s selected, scale factor: %.2e g/LSB",
                 adxl355_get_range_string(range), handle->scale_factor);
        break;
    case ADXL355_RANGE_4G:
        handle->scale_factor = ADXL355_ACCELERATION_SCALE_FACTOR_RANGE_4G;
        ESP_LOGI(TAG, "Range %s selected, scale factor: %.2e g/LSB",
                 adxl355_get_range_string(range), handle->scale_factor);
        break;
    case ADXL355_RANGE_8G:
        handle->scale_factor = ADXL355_ACCELERATION_SCALE_FACTOR_RANGE_8G;
        ESP_LOGI(TAG, "Range %s selected, scale factor: %.2e g/LSB",
                 adxl355_get_range_string(range), handle->scale_factor);
        break;
    default:
        ESP_LOGE(TAG, "Invalid range setting: %d", range);
        return ESP_ERR_INVALID_ARG;
    }

    /* Reset device */
    ESP_LOGI(TAG, "Resetting ADXL355 device...");
    ret = adxl355_reset(handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to reset device: %s", esp_err_to_name(ret));
        return ret;
    }
    ESP_LOGI(TAG, "Device reset completed");

    /* Wait for reset to complete */
    vTaskDelay(pdMS_TO_TICKS(100));

    /* Read device info to verify communication */
    ESP_LOGI(TAG, "Reading device information...");
    adxl355_device_info_t device_info;
    ret = adxl355_read_device_info(handle, &device_info);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to read device info: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Device ID: 0x%02X, Vendor ID: 0x%02X", device_info.device_id, device_info.vendor_id);

    /* Set range and ODR */
    ESP_LOGI(TAG, "Setting range to %s...", adxl355_get_range_string(range));
    ret = adxl355_set_range(handle, range);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to set range: %s", esp_err_to_name(ret));
        return ret;
    }
    ESP_LOGI(TAG, "Range set successfully");

    ESP_LOGI(TAG, "Setting ODR to %s...", adxl355_get_odr_string(odr));
    ret = adxl355_set_odr(handle, odr);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to set ODR: %s", esp_err_to_name(ret));
        return ret;
    }
    ESP_LOGI(TAG, "ODR set successfully");

    /* Enable measurement mode */
    ESP_LOGI(TAG, "Enabling measurement mode...");
    ret = adxl355_enable_measurement(handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to enable measurement: %s", esp_err_to_name(ret));
        return ret;
    }
    ESP_LOGI(TAG, "Measurement mode enabled");

    /* Enable temperature processing */
    ESP_LOGI(TAG, "Enabling temperature processing...");
    ret = adxl355_enable_temperature(handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to enable temperature: %s", esp_err_to_name(ret));
        return ret;
    }
    ESP_LOGI(TAG, "Temperature processing enabled");

    ESP_LOGI(TAG, "=== ADXL355 INITIALIZATION COMPLETED SUCCESSFULLY ===");
    ESP_LOGI(TAG, "Final configuration: range=%s, ODR=%s, scale_factor=%.2e g/LSB",
             adxl355_get_range_string(handle->range), adxl355_get_odr_string(handle->odr), handle->scale_factor);
    return ESP_OK;
}

//==============================================================================

esp_err_t adxl355_deinit(adxl355_handle_t *handle)
{
    if (handle == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    /* Disable measurement mode */
    esp_err_t ret = adxl355_disable_measurement(handle);
    if (ret != ESP_OK)
    {
        ESP_LOGW(TAG, "Failed to disable measurement during deinit");
    }

    /* Clear handle */
    handle->spi_handle = NULL;
    handle->range = 0;
    handle->odr = 0;
    handle->scale_factor = 0.0f;

    return ESP_OK;
}

//==============================================================================

esp_err_t adxl355_reset(adxl355_handle_t *handle)
{
    if (handle == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    /* Send reset command */
    esp_err_t ret = adxl355_write_register(handle, ADXL355_REG_RESET, ADXL355_REG_RESET_RESET_CODE);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to send reset command");
        return ret;
    }

    return ESP_OK;
}

//==============================================================================

esp_err_t adxl355_read_device_info(adxl355_handle_t *handle, adxl355_device_info_t *device_info)
{
    esp_err_t ret;
    uint8_t data[4];

    if (handle == NULL || device_info == NULL)
    {
        ESP_LOGE(TAG, "Invalid arguments: handle=%p, device_info=%p", (void *)handle, (void *)device_info);
        return ESP_ERR_INVALID_ARG;
    }

    /* Read 4 bytes starting from DEVID_AD register */
    ret = adxl355_read_registers(handle, ADXL355_REG_DEVID_AD, data, 4);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to read device info registers: %s", esp_err_to_name(ret));
        return ret;
    }

    /* Extract device information (exactly like plasmapper) */
    device_info->vendor_id = data[0];
    device_info->family_id = data[1];
    device_info->device_id = data[2];
    device_info->revision_id = data[3];

    return ESP_OK;
}

//==============================================================================

esp_err_t adxl355_read_status(adxl355_handle_t *handle, adxl355_status_t *status)
{
    if (handle == NULL || status == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t status_reg;
    esp_err_t ret = adxl355_read_register(handle, ADXL355_REG_STATUS, &status_reg);
    if (ret != ESP_OK)
    {
        return ret;
    }

    *status = (adxl355_status_t)status_reg;
    return ESP_OK;
}

//==============================================================================

esp_err_t adxl355_read_raw_accelerations(adxl355_handle_t *handle, adxl355_raw_accelerations_t *raw_accel)
{
    esp_err_t ret;
    uint8_t data[9];

    if (handle == NULL || raw_accel == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    /* Read 9 bytes starting from XDATA3 register */
    ret = adxl355_read_registers(handle, ADXL355_REG_XDATA3, data, 9);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to read acceleration data");
        return ret;
    }

    /* Initialize to 0 (exactly like plasmapper) */
    raw_accel->x = 0;
    raw_accel->y = 0;
    raw_accel->z = 0;

    /* Reconstruct 20-bit values (exactly like plasmapper) */
    /* X-axis: data[2], data[1], data[0] */
    ((uint8_t *)&raw_accel->x)[1] = data[2];
    ((uint8_t *)&raw_accel->x)[2] = data[1];
    ((uint8_t *)&raw_accel->x)[3] = data[0];

    /* Y-axis: data[5], data[4], data[3] */
    ((uint8_t *)&raw_accel->y)[1] = data[5];
    ((uint8_t *)&raw_accel->y)[2] = data[4];
    ((uint8_t *)&raw_accel->y)[3] = data[3];

    /* Z-axis: data[8], data[7], data[6] */
    ((uint8_t *)&raw_accel->z)[1] = data[8];
    ((uint8_t *)&raw_accel->z)[2] = data[7];
    ((uint8_t *)&raw_accel->z)[3] = data[6];

    /* Convert to 20-bit signed values (divide by 4096) */
    raw_accel->x /= 4096;
    raw_accel->y /= 4096;
    raw_accel->z /= 4096;

    return ESP_OK;
}

//==============================================================================

esp_err_t adxl355_read_accelerations(adxl355_handle_t *handle, adxl355_accelerations_t *accel)
{
    esp_err_t ret;
    adxl355_raw_accelerations_t raw_accel;
    float scale_factor;

    if (handle == NULL || accel == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    /* Read raw accelerations */
    ret = adxl355_read_raw_accelerations(handle, &raw_accel);
    if (ret != ESP_OK)
    {
        return ret;
    }

    /* Get current scale factor */
    ret = adxl355_read_acceleration_scale_factor(handle, &scale_factor);
    if (ret != ESP_OK)
    {
        return ret;
    }

    /* Convert to g units */
    accel->x = raw_accel.x * scale_factor;
    accel->y = raw_accel.y * scale_factor;
    accel->z = raw_accel.z * scale_factor;

    return ESP_OK;
}

//==============================================================================

esp_err_t adxl355_read_raw_temperature(adxl355_handle_t *handle, uint16_t *raw_temp)
{
    esp_err_t ret;
    uint8_t data[2];

    if (handle == NULL || raw_temp == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    /* Read 2 bytes starting from TEMP2 register */
    ret = adxl355_read_registers(handle, ADXL355_REG_TEMP2, data, 2);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to read temperature data");
        return ret;
    }

    /* Reconstruct 16-bit value (exactly like plasmapper) */
    ((uint8_t *)raw_temp)[0] = data[1];
    ((uint8_t *)raw_temp)[1] = data[0];

    return ESP_OK;
}

//==============================================================================

esp_err_t adxl355_read_temperature(adxl355_handle_t *handle, float *temp)
{
    esp_err_t ret;
    uint16_t raw_temp;

    if (handle == NULL || temp == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    /* Read raw temperature */
    ret = adxl355_read_raw_temperature(handle, &raw_temp);
    if (ret != ESP_OK)
    {
        return ret;
    }

    /* Convert to °C using plasmapper's formula */
    *temp = (((int16_t)raw_temp) - ((int16_t)ADXL355_TEMPERATURE_INTERCEPT_LSB)) /
                ADXL355_TEMPERATURE_SLOPE +
            ADXL355_TEMPERATURE_INTERCEPT_DEG_C;

    return ESP_OK;
}

//==============================================================================

esp_err_t adxl355_set_range(adxl355_handle_t *handle, adxl355_range_t range)
{
    esp_err_t ret;
    uint8_t range_register;

    if (handle == NULL)
    {
        ESP_LOGE(TAG, "Invalid handle: NULL");
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Setting ADXL355 range to %s...", adxl355_get_range_string(range));

    /* Read current range register value like plasmapper */
    ret = adxl355_read_register(handle, ADXL355_REG_RANGE, &range_register);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to read range register: %s", esp_err_to_name(ret));
        return ret;
    }

    /* Clear range bits (bits 0-1) like plasmapper */
    range_register &= ~ADXL355_REG_RANGE_RANGE_MASK;

    /* Set new range value */
    uint8_t new_range_value = range & ADXL355_REG_RANGE_RANGE_MASK;
    range_register |= new_range_value;

    /* Write updated register value */
    ret = adxl355_write_register(handle, ADXL355_REG_RANGE, range_register);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to write range register: %s", esp_err_to_name(ret));
        return ret;
    }

    /* Verify the write by reading back */
    uint8_t verify_register;
    ret = adxl355_read_register(handle, ADXL355_REG_RANGE, &verify_register);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to verify range register: %s", esp_err_to_name(ret));
        return ret;
    }

    if (verify_register != range_register)
    {
        ESP_LOGE(TAG, "Range register verification failed! Expected: 0x%02X, Got: 0x%02X",
                 range_register, verify_register);
        return ESP_ERR_INVALID_STATE;
    }

    /* Update scale factor */
    float old_scale_factor = handle->scale_factor;
    switch (range)
    {
    case ADXL355_RANGE_2G:
        handle->scale_factor = ADXL355_ACCELERATION_SCALE_FACTOR_RANGE_2G;
        break;
    case ADXL355_RANGE_4G:
        handle->scale_factor = ADXL355_ACCELERATION_SCALE_FACTOR_RANGE_4G;
        break;
    case ADXL355_RANGE_8G:
        handle->scale_factor = ADXL355_ACCELERATION_SCALE_FACTOR_RANGE_8G;
        break;
    default:
        ESP_LOGE(TAG, "Invalid range value: %d", range);
        return ESP_ERR_INVALID_ARG;
    }

    handle->range = range;
    ESP_LOGI(TAG, "Range changed from %.2e to %s, scale factor: %.2e g/LSB",
             old_scale_factor, adxl355_get_range_string(range), handle->scale_factor);
    return ESP_OK;
}

//==============================================================================

esp_err_t adxl355_read_range(adxl355_handle_t *handle, adxl355_range_t *range)
{
    esp_err_t ret;
    uint8_t range_register;

    if (handle == NULL || range == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    ret = adxl355_read_register(handle, ADXL355_REG_RANGE, &range_register);
    if (ret != ESP_OK)
    {
        return ret;
    }

    *range = (adxl355_range_t)(range_register & ADXL355_REG_RANGE_RANGE_MASK);
    return ESP_OK;
}

//==============================================================================

esp_err_t adxl355_set_odr(adxl355_handle_t *handle, adxl355_output_data_rate_t odr)
{
    esp_err_t ret;
    uint8_t filter_reg;

    if (handle == NULL)
    {
        ESP_LOGE(TAG, "Invalid handle: NULL");
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Setting ADXL355 ODR to %s...", adxl355_get_odr_string(odr));

    /* Read current filter register value */
    ret = adxl355_read_register(handle, ADXL355_REG_FILTER, &filter_reg);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to read filter register: %s", esp_err_to_name(ret));
        return ret;
    }

    /* Clear ODR bits and set new value (like plasmapper) */
    uint8_t old_odr = filter_reg & ADXL355_REG_FILTER_ODR_MASK;
    filter_reg &= ~ADXL355_REG_FILTER_ODR_MASK;

    uint8_t new_odr_value = odr & ADXL355_REG_FILTER_ODR_MASK;
    filter_reg |= new_odr_value;

    /* Write updated register value */
    ret = adxl355_write_register(handle, ADXL355_REG_FILTER, filter_reg);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to write filter register: %s", esp_err_to_name(ret));
        return ret;
    }

    /* Verify the write by reading back */
    uint8_t verify_register;
    ret = adxl355_read_register(handle, ADXL355_REG_FILTER, &verify_register);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to verify filter register: %s", esp_err_to_name(ret));
        return ret;
    }

    if (verify_register != filter_reg)
    {
        ESP_LOGE(TAG, "Filter register verification failed! Expected: 0x%02X, Got: 0x%02X",
                 filter_reg, verify_register);
        return ESP_ERR_INVALID_STATE;
    }

    handle->odr = odr;
    ESP_LOGI(TAG, "ODR changed from %s to %s",
             adxl355_get_odr_string(old_odr), adxl355_get_odr_string(odr));
    return ESP_OK;
}

//==============================================================================

esp_err_t adxl355_read_odr(adxl355_handle_t *handle, adxl355_output_data_rate_t *odr)
{
    esp_err_t ret;
    uint8_t filter_reg;

    if (handle == NULL || odr == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    ret = adxl355_read_register(handle, ADXL355_REG_FILTER, &filter_reg);
    if (ret != ESP_OK)
    {
        return ret;
    }

    *odr = (adxl355_output_data_rate_t)(filter_reg & ADXL355_REG_FILTER_ODR_MASK);
    return ESP_OK;
}

//==============================================================================

esp_err_t adxl355_enable_measurement(adxl355_handle_t *handle)
{
    esp_err_t ret;
    uint8_t power_ctl_register;

    if (handle == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    /* Read current power control register value */
    ret = adxl355_read_register(handle, ADXL355_REG_POWER_CTL, &power_ctl_register);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to read power control register");
        return ret;
    }

    /* Clear standby bit to enable measurement */
    power_ctl_register &= ~ADXL355_REG_POWER_CTL_STANDBY;

    /* Write updated register value */
    ret = adxl355_write_register(handle, ADXL355_REG_POWER_CTL, power_ctl_register);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to enable measurement");
        return ret;
    }

    return ESP_OK;
}

//==============================================================================

esp_err_t adxl355_disable_measurement(adxl355_handle_t *handle)
{
    esp_err_t ret;
    uint8_t power_ctl_register;

    if (handle == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    /* Read current power control register value */
    ret = adxl355_read_register(handle, ADXL355_REG_POWER_CTL, &power_ctl_register);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to read power control register");
        return ret;
    }

    /* Set standby bit to disable measurement */
    power_ctl_register |= ADXL355_REG_POWER_CTL_STANDBY;

    /* Write updated register value */
    ret = adxl355_write_register(handle, ADXL355_REG_POWER_CTL, power_ctl_register);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to disable measurement");
        return ret;
    }

    return ESP_OK;
}

//==============================================================================

esp_err_t adxl355_is_measurement_enabled(adxl355_handle_t *handle, bool *is_enabled)
{
    esp_err_t ret;
    uint8_t power_ctl_register;

    if (handle == NULL || is_enabled == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    ret = adxl355_read_register(handle, ADXL355_REG_POWER_CTL, &power_ctl_register);
    if (ret != ESP_OK)
    {
        return ret;
    }

    *is_enabled = !(power_ctl_register & ADXL355_REG_POWER_CTL_STANDBY);
    return ESP_OK;
}

//==============================================================================

esp_err_t adxl355_enable_temperature(adxl355_handle_t *handle)
{
    esp_err_t ret;
    uint8_t power_ctl_register;

    if (handle == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    /* Read current power control register value */
    ret = adxl355_read_register(handle, ADXL355_REG_POWER_CTL, &power_ctl_register);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to read power control register");
        return ret;
    }

    /* Clear temperature off bit to enable temperature processing */
    power_ctl_register &= ~ADXL355_REG_POWER_CTL_TEMP_OFF;

    /* Write updated register value */
    ret = adxl355_write_register(handle, ADXL355_REG_POWER_CTL, power_ctl_register);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to enable temperature");
        return ret;
    }

    return ESP_OK;
}

//==============================================================================

esp_err_t adxl355_disable_temperature(adxl355_handle_t *handle)
{
    esp_err_t ret;
    uint8_t power_ctl_register;

    if (handle == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    /* Read current power control register value */
    ret = adxl355_read_register(handle, ADXL355_REG_POWER_CTL, &power_ctl_register);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to read power control register");
        return ret;
    }

    /* Set temperature off bit to disable temperature processing */
    power_ctl_register |= ADXL355_REG_POWER_CTL_TEMP_OFF;

    /* Write updated register value */
    ret = adxl355_write_register(handle, ADXL355_REG_POWER_CTL, power_ctl_register);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to disable temperature");
        return ret;
    }

    return ESP_OK;
}

//==============================================================================

esp_err_t adxl355_is_temperature_enabled(adxl355_handle_t *handle, bool *is_enabled)
{
    esp_err_t ret;
    uint8_t power_ctl_register;

    if (handle == NULL || is_enabled == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    ret = adxl355_read_register(handle, ADXL355_REG_POWER_CTL, &power_ctl_register);
    if (ret != ESP_OK)
    {
        return ret;
    }

    *is_enabled = !(power_ctl_register & ADXL355_REG_POWER_CTL_TEMP_OFF);
    return ESP_OK;
}

//==============================================================================

esp_err_t adxl355_enable_data_ready(adxl355_handle_t *handle)
{
    esp_err_t ret;
    uint8_t power_ctl_register;

    if (handle == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    /* Read current power control register value */
    ret = adxl355_read_register(handle, ADXL355_REG_POWER_CTL, &power_ctl_register);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to read power control register");
        return ret;
    }

    /* Clear data ready off bit to enable data ready output */
    power_ctl_register &= ~ADXL355_REG_POWER_CTL_DRDY_OFF;

    /* Write updated register value */
    ret = adxl355_write_register(handle, ADXL355_REG_POWER_CTL, power_ctl_register);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to enable data ready");
        return ret;
    }

    return ESP_OK;
}

//==============================================================================

esp_err_t adxl355_disable_data_ready(adxl355_handle_t *handle)
{
    esp_err_t ret;
    uint8_t power_ctl_register;

    if (handle == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    /* Read current power control register value */
    ret = adxl355_read_register(handle, ADXL355_REG_POWER_CTL, &power_ctl_register);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to read power control register");
        return ret;
    }

    /* Set data ready off bit to disable data ready output */
    power_ctl_register |= ADXL355_REG_POWER_CTL_DRDY_OFF;

    /* Write updated register value */
    ret = adxl355_write_register(handle, ADXL355_REG_POWER_CTL, power_ctl_register);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to disable data ready");
        return ret;
    }

    return ESP_OK;
}

//==============================================================================

esp_err_t adxl355_is_data_ready_enabled(adxl355_handle_t *handle, bool *is_enabled)
{
    esp_err_t ret;
    uint8_t power_ctl_register;

    if (handle == NULL || is_enabled == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    ret = adxl355_read_register(handle, ADXL355_REG_POWER_CTL, &power_ctl_register);
    if (ret != ESP_OK)
    {
        return ret;
    }

    *is_enabled = !(power_ctl_register & ADXL355_REG_POWER_CTL_DRDY_OFF);
    return ESP_OK;
}

//==============================================================================

esp_err_t adxl355_read_acceleration_scale_factor(adxl355_handle_t *handle, float *scale_factor)
{
    esp_err_t ret;
    adxl355_range_t range;

    if (handle == NULL || scale_factor == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    ret = adxl355_read_range(handle, &range);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to read range");
        return ret;
    }

    switch (range)
    {
    case ADXL355_RANGE_2G:
        *scale_factor = ADXL355_ACCELERATION_SCALE_FACTOR_RANGE_2G;
        break;
    case ADXL355_RANGE_4G:
        *scale_factor = ADXL355_ACCELERATION_SCALE_FACTOR_RANGE_4G;
        break;
    case ADXL355_RANGE_8G:
        *scale_factor = ADXL355_ACCELERATION_SCALE_FACTOR_RANGE_8G;
        break;
    default:
        *scale_factor = 1.0f;
        break;
    }

    return ESP_OK;
}

//==============================================================================
// Private Functions
//==============================================================================

/**
 * @brief Configure and add ADXL355 SPI device to SPI3 bus
 * @param device_handle Output parameter, returns SPI device handle
 * @return ESP_OK on success, error code on failure
 */
static esp_err_t adxl355_configure_spi_device(spi_device_handle_t *device_handle)
{
    esp_err_t ret;
    spi_device_interface_config_t dev_config = {0};

    /* Configure ADXL355-specific SPI device parameters */
    dev_config.clock_speed_hz = 10000000;          // 10 MHz (ADXL355 maximum supported)
    dev_config.mode = 0;                           // SPI Mode 0 (CPOL=0, CPHA=0)
    dev_config.spics_io_num = ADXL355_CS_GPIO_PIN; // CS pin
    dev_config.queue_size = 1;                     // Single transaction queue
    dev_config.command_bits = 0;                   // No command bits
    dev_config.address_bits = 8;                   // 8-bit address
    dev_config.input_delay_ns = 30;                // 30ns delay
    dev_config.flags = 0;                          // Default flags (full-duplex)

    /* Add SPI device to bus */
    ret = spi_bus_add_device(SPI3_HOST, &dev_config, device_handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to add ADXL355 device to SPI3 bus: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "ADXL355 SPI device configured successfully");
    return ESP_OK;
}

//==============================================================================

/**
 * @brief Read single register
 * @param handle ADXL355 handle
 * @param reg Register address
 * @param value Output value
 * @return ESP_OK on success, error code on failure
 */
static esp_err_t adxl355_read_register(adxl355_handle_t *handle, uint8_t reg, uint8_t *value)
{
    ESP_LOGD(TAG, "READ_REG: reg=0x%02X, handle=%p, value_ptr=%p", reg, (void *)handle, (void *)value);

    if (handle == NULL || value == NULL)
    {
        ESP_LOGE(TAG, "READ_REG: Invalid arguments - handle=%p, value=%p", (void *)handle, (void *)value);
        return ESP_ERR_INVALID_ARG;
    }

    /* Left shift address by 1 and set LSB to 1 for read (exactly like plasmapper) */
    uint8_t read_cmd = (reg << 1) | 0x01;
    ESP_LOGD(TAG, "READ_REG: reg=0x%02X, shifted=0x%02X, read_cmd=0x%02X", reg, reg << 1, read_cmd);

    /* Use SPI transaction: write command then read data */
    spi_transaction_t t = {0};
    t.cmd = 0;           /* No command bits */
    t.addr = read_cmd;   /* Address with read bit set */
    t.length = 8;        /* 1 byte = 8 bits */
    t.tx_buffer = NULL;  /* No write data */
    t.rx_buffer = value; /* Read data buffer */
    t.flags = 0;         /* Use default flags */

    ESP_LOGD(TAG, "READ_REG: SPI transaction - addr=0x%02X, length=%d bits", t.addr, t.length);

    esp_err_t ret = spi_device_polling_transmit(handle->spi_handle, &t);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "READ_REG: SPI read transaction failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGD(TAG, "READ_REG: Success - reg=0x%02X, value=0x%02X", reg, *value);
    return ESP_OK;
}

//==============================================================================

/**
 * @brief Read multiple registers
 * @param handle ADXL355 handle
 * @param reg Starting register address
 * @param data Output data buffer
 * @param len Number of bytes to read
 * @return ESP_OK on success, error code on failure
 */
static esp_err_t adxl355_read_registers(adxl355_handle_t *handle, uint8_t reg, uint8_t *data, uint8_t len)
{
    if (handle == NULL || data == NULL || len == 0)
    {
        return ESP_ERR_INVALID_ARG;
    }

    /* Left shift address by 1 and set LSB to 1 for read (exactly like plasmapper) */
    uint8_t read_cmd = (reg << 1) | 0x01;

    /* Use the new ADXL355-specific SPI read function */
    return adxl355_spi_read(handle, read_cmd, data, len);
}

//==============================================================================

/**
 * @brief Write single register
 * @param handle ADXL355 handle
 * @param reg Register address
 * @param value Value to write
 * @return ESP_OK on success, error code on failure
 */
static esp_err_t adxl355_write_register(adxl355_handle_t *handle, uint8_t reg, uint8_t value)
{
    ESP_LOGD(TAG, "WRITE_REG: reg=0x%02X, value=0x%02X, handle=%p", reg, value, (void *)handle);

    if (handle == NULL)
    {
        ESP_LOGE(TAG, "WRITE_REG: Invalid handle - NULL");
        return ESP_ERR_INVALID_ARG;
    }

    /* Left shift address by 1 (exactly like plasmapper) */
    uint8_t write_cmd = reg << 1;
    ESP_LOGD(TAG, "WRITE_REG: reg=0x%02X, shifted=0x%02X, write_cmd=0x%02X", reg, reg << 1, write_cmd);

    /* Use SPI transaction: write command and data */
    spi_transaction_t t = {0};
    t.cmd = 0;            /* No command bits */
    t.addr = write_cmd;   /* Address with write bit clear */
    t.length = 8;         /* 1 byte = 8 bits */
    t.tx_buffer = &value; /* Write data */
    t.rx_buffer = NULL;   /* No read data */
    t.flags = 0;          /* Use default flags */

    ESP_LOGD(TAG, "WRITE_REG: SPI transaction - addr=0x%02X, length=%d bits, data=0x%02X",
             t.addr, t.length, value);

    esp_err_t ret = spi_device_polling_transmit(handle->spi_handle, &t);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "WRITE_REG: SPI write transaction failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGD(TAG, "WRITE_REG: Success - reg=0x%02X, value=0x%02X", reg, value);
    return ESP_OK;
}

//==============================================================================

/**
 * @brief Write multiple registers
 * @param handle ADXL355 handle
 * @param reg Starting register address
 * @param data Data to write
 * @param len Number of bytes to write
 * @return ESP_OK on success, error code on failure
 */
static esp_err_t adxl355_write_registers(adxl355_handle_t *handle, uint8_t reg, const uint8_t *data, uint8_t len)
{
    if (handle == NULL || data == NULL || len == 0)
    {
        return ESP_ERR_INVALID_ARG;
    }

    /* Left shift address by 1 (exactly like plasmapper) */
    uint8_t write_cmd = reg << 1;

    /* Use SPI transaction: write command and data */
    spi_transaction_t t = {0};
    t.cmd = 0;          /* No command bits */
    t.addr = write_cmd; /* Address with write bit clear */
    t.length = len * 8; /* Data size in bits */
    t.tx_buffer = data; /* Write data */
    t.rx_buffer = NULL; /* No read data */
    t.flags = 0;        /* Use default flags */

    esp_err_t ret = spi_device_polling_transmit(handle->spi_handle, &t);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "SPI write registers transaction failed: %s", esp_err_to_name(ret));
        return ret;
    }

    return ESP_OK;
}

//==============================================================================

/**
 * @brief ADXL355 specific: Write command then read data
 * @param handle SPI handle
 * @param cmd Command byte to send
 * @param data Buffer to store received data
 * @param len Length of data to read
 * @return ESP_OK on success, error code on failure
 */
esp_err_t adxl355_spi_read(adxl355_handle_t *handle, uint8_t cmd, uint8_t *data, int len)
{
    if (handle == NULL || data == NULL || len <= 0)
    {
        ESP_LOGE(TAG, "Invalid arguments for ADXL355 SPI read");
        return ESP_ERR_INVALID_ARG;
    }

    /* Configure transaction exactly like plasmapper */
    /* command = 0, address = cmd, writeData = NULL, readData = data, dataSize = len * 8 */
    spi_transaction_t t = {0};
    t.cmd = 0;          /* No command bits */
    t.addr = cmd;       /* Address (already formatted) */
    t.length = len * 8; /* Data size in bits */
    t.tx_buffer = NULL; /* No write data */
    t.rx_buffer = data; /* Read data buffer */
    t.flags = 0;        /* Use default flags */

    /* Use polling transmit - DMA will be used automatically if enabled on SPI bus */
    /* Note: spi_device_polling_transmit() works with DMA enabled, CPU waits for DMA completion */
    esp_err_t ret = spi_device_polling_transmit(handle->spi_handle, &t);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "ADXL355 SPI read failed: %s", esp_err_to_name(ret));
        return ret;
    }

    return ESP_OK;
}

/**
 * @brief Get string representation of range setting
 * @param range Range setting
 * @return String representation
 */
static const char *adxl355_get_range_string(adxl355_range_t range)
{
    switch (range)
    {
    case ADXL355_RANGE_2G:
        return "±2G";
    case ADXL355_RANGE_4G:
        return "±4G";
    case ADXL355_RANGE_8G:
        return "±8G";
    default:
        return "Unknown";
    }
}

/**
 * @brief Get string representation of ODR setting
 * @param odr ODR setting
 * @return String representation
 */
static const char *adxl355_get_odr_string(adxl355_output_data_rate_t odr)
{
    switch (odr)
    {
    case ADXL355_ODR_4000:
        return "4000 Hz";
    case ADXL355_ODR_2000:
        return "2000 Hz";
    case ADXL355_ODR_1000:
        return "1000 Hz";
    case ADXL355_ODR_500:
        return "500 Hz";
    case ADXL355_ODR_250:
        return "250 Hz";
    case ADXL355_ODR_125:
        return "125 Hz";
    case ADXL355_ODR_62_5:
        return "62.5 Hz";
    case ADXL355_ODR_31_25:
        return "31.25 Hz";
    case ADXL355_ODR_15_625:
        return "15.625 Hz";
    case ADXL355_ODR_7_813:
        return "7.813 Hz";
    case ADXL355_ODR_3_906:
        return "3.906 Hz";
    default:
        return "Unknown";
    }
}

```

## main.c

```c
/**
 * @file main.c
 * @author SHUAIWEN CUI (SHUAIWEN001@e.ntu.edu.sg)
 * @brief 
 * @version 1.0
 * @date 2024-11-17
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

// BSP
#include "node_led.h"
#include "node_exit.h"
#include "node_spi.h"
#include "node_lcd.h"
#include "node_timer.h"
#include "node_rtc.h"
#include "node_sdcard.h"
#include "node_wifi.h"
#include "node_mqtt.h"
#include "node_acc_adxl355.h"

/* Variables */
const char *TAG = "AIoTNode";

/* ADXL355 handle */
adxl355_handle_t adxl355_handle;

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
    // int count = 0;
    
    /* ADXL355 variables */
    adxl355_accelerations_t accel;
    // adxl355_raw_accelerations_t raw_accel;  // Unused variable, commented out
    float temperature;
    
    // SPI3 device handle is now managed internally by ADXL355

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
    // spi3_init();  // Refactored, now called during ADXL355 initialization
    lcd_init();

    // spiffs_test();                                                  /* Run SPIFFS test */
    while (sd_card_init())                               /* SD card not detected */
    {
        lcd_show_string(0, 0, 200, 16, 16, "SD Card Error!", RED);
        vTaskDelay(500);
        lcd_show_string(0, 20, 200, 16, 16, "Please Check!", RED);
        vTaskDelay(500);
    }

    // clean the screen
    lcd_clear(WHITE);

    lcd_show_string(0, 0, 200, 16, 16, "SD Initialized!", RED);

    sd_card_test_filesystem();                                        /* Run SD card test */

    lcd_show_string(0, 0, 200, 16, 16, "SD Tested CSW! ", RED);

    // sd_card_unmount();

    vTaskDelay(3000);

    // Initialize WiFi and MQTT first
    lcd_show_string(0, 0, lcd_self.width, 16, 16, "WiFi STA Test  ", RED);
    
    ret = wifi_sta_wpa2_init();
    if(ret == ESP_OK)
    {
        ESP_LOGI(TAG_WIFI, "WiFi STA Init OK");
        lcd_show_string(0, 0, lcd_self.width, 16, 16, "WiFi STA Test OK", RED);
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

    // Initialize and test ADXL355 after WiFi and MQTT
    lcd_clear(WHITE);
    lcd_show_string(0, 0, 200, 16, 16, "Testing ADXL355...", BLUE);
    
    ESP_LOGI(TAG, "=== MAIN: Starting ADXL355 test ===");
    
    // Step 1: Initialize SPI3 bus (general purpose)
    spi3_init();
    
    // Step 2: Initialize ADXL355 sensor (includes SPI device configuration)
    ret = adxl355_init(&adxl355_handle, ADXL355_RANGE_2G, ADXL355_ODR_1000);
    
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "✓ ADXL355 initialization successful!");
        lcd_show_string(0, 20, 200, 16, 16, "ADXL355 OK!", GREEN);
        
        // Read device info
        adxl355_device_info_t device_info;
        esp_err_t info_ret = adxl355_read_device_info(&adxl355_handle, &device_info);
        
        if (info_ret == ESP_OK) {
            ESP_LOGI(TAG, "Device ID: 0x%02X, Vendor ID: 0x%02X", device_info.device_id, device_info.vendor_id);
            
            char info_str[64];
            snprintf(info_str, sizeof(info_str), "ID: 0x%02X, 0x%02X, 0x%02X", 
                     device_info.vendor_id, device_info.family_id, device_info.device_id);
            lcd_show_string(0, 40, 200, 16, 16, info_str, BLACK);
            
            // Test temperature reading
            float test_temp;
            esp_err_t temp_test_ret = adxl355_read_temperature(&adxl355_handle, &test_temp);
            if (temp_test_ret == ESP_OK) {
                ESP_LOGI(TAG, "Temperature test: %.2f°C", test_temp);
                char temp_str[64];
                snprintf(temp_str, sizeof(temp_str), "Temp: %.1f°C", test_temp);
                lcd_show_string(0, 60, 200, 16, 16, temp_str, BLUE);
            } else {
                ESP_LOGE(TAG, "Temperature test failed: %s", esp_err_to_name(temp_test_ret));
                lcd_show_string(0, 60, 200, 16, 16, "Temp: Failed!", RED);
            }
        } else {
            ESP_LOGE(TAG, "✗ Failed to read device info: %s", esp_err_to_name(info_ret));
        }
        
    } else {
        ESP_LOGE(TAG, "✗ ADXL355 initialization failed: %s", esp_err_to_name(ret));
        lcd_show_string(0, 20, 200, 16, 16, "ADXL355 Failed!", RED);
    }
    
    vTaskDelay(3000);
    
    ESP_LOGI(TAG, "ADXL355 initialized and ready for MQTT publishing");

    ESP_LOGI(TAG, "=== MAIN: Entering main loop ===");
    
    while (1)
    {
        if(s_is_mqtt_connected)
        {
            // Read both acceleration and temperature data
            esp_err_t accel_ret = adxl355_read_accelerations(&adxl355_handle, &accel);
            esp_err_t temp_ret = adxl355_read_temperature(&adxl355_handle, &temperature);
            
            if (accel_ret == ESP_OK && temp_ret == ESP_OK) {
                // Format MQTT payload with both acceleration and temperature
                snprintf(mqtt_pub_buff, 64,
                    "{\"x\":%.4f,\"y\":%.4f,\"z\":%.4f,\"temp\":%.2f}",
                    accel.x, accel.y, accel.z, temperature);
                
                int mqtt_ret = esp_mqtt_client_publish(s_mqtt_client, MQTT_PUBLISH_TOPIC,
                                mqtt_pub_buff, strlen(mqtt_pub_buff), 1, 0);
                
                if (mqtt_ret < 0) {
                    ESP_LOGE(TAG, "✗ MQTT publish failed: %d", mqtt_ret);
                }
                
                // Output both acceleration and temperature to console
                printf("Accelerations: X: %f g, Y: %f g, Z: %f g | Temperature: %.2f°C\n", 
                       accel.x, accel.y, accel.z, temperature);
                
                // Also log to ESP log system
                ESP_LOGI(TAG, "Data: X=%.4f, Y=%.4f, Z=%.4f g, Temp=%.2f°C", 
                         accel.x, accel.y, accel.z, temperature);
                
            } else {
                if (accel_ret != ESP_OK) {
                    ESP_LOGE(TAG, "✗ Failed to read ADXL355 acceleration data: %s", esp_err_to_name(accel_ret));
                }
                if (temp_ret != ESP_OK) {
                    ESP_LOGE(TAG, "✗ Failed to read ADXL355 temperature data: %s", esp_err_to_name(temp_ret));
                }
            }
        } else {
            ESP_LOGW(TAG, "MQTT not connected, skipping data read");
        }
        
        led_toggle();
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}
```

## main.cpp

```cpp
/**
 * @file main.c
 * @author SHUAIWEN CUI (SHUAIWEN001@e.ntu.edu.sg)
 * @brief 
 * @version 1.0
 * @date 2024-11-17
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

// BSP
#include "node_led.h"
#include "node_exit.h"
#include "node_spi.h"
#include "node_lcd.h"
#include "node_timer.h"
#include "node_rtc.h"
#include "node_sdcard.h"
#include "node_wifi.h"
#include "node_mqtt.h"
#include "node_acc_adxl355.h"

/* Variables */
const char *TAG = "AIoTNode";

/* ADXL355 handle */
adxl355_handle_t adxl355_handle;

extern "C" void app_main();
/**
 * @brief Entry point of the program
 * @param None
 * @retval None
 */
void app_main()
{
    esp_err_t ret;
    uint32_t flash_size;
    esp_chip_info_t chip_info;

    char mqtt_pub_buff[64];
    // int count = 0;
    
    /* ADXL355 variables */
    adxl355_accelerations_t accel;
    // adxl355_raw_accelerations_t raw_accel;  // Unused variable, commented out
    float temperature;
    
    // SPI3 device handle is now managed internally by ADXL355

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
    // spi3_init();  // Refactored, now called during ADXL355 initialization
    lcd_init();

    // spiffs_test();                                                  /* Run SPIFFS test */
    while (sd_card_init())                               /* SD card not detected */
    {
        lcd_show_string(0, 0, 200, 16, 16, (char*)"SD Card Error!", RED);
        vTaskDelay(500);
        lcd_show_string(0, 20, 200, 16, 16, (char*)"Please Check!", RED);
        vTaskDelay(500);
    }

    // clean the screen
    lcd_clear(WHITE);

    lcd_show_string(0, 0, 200, 16, 16, (char*)"SD Initialized!", RED);

    sd_card_test_filesystem();                                        /* Run SD card test */

    lcd_show_string(0, 0, 200, 16, 16, (char*)"SD Tested CSW! ", RED);

    // sd_card_unmount();

    vTaskDelay(3000);

    // Initialize WiFi and MQTT first
    lcd_show_string(0, 0, lcd_self.width, 16, 16, (char*)"WiFi STA Test  ", RED);
    
    ret = wifi_sta_wpa2_init();
    if(ret == ESP_OK)
    {
        ESP_LOGI(TAG_WIFI, "WiFi STA Init OK");
        lcd_show_string(0, 0, lcd_self.width, 16, 16, (char*)"WiFi STA Test OK", RED);
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

    // Initialize and test ADXL355 after WiFi and MQTT
    lcd_clear(WHITE);
    lcd_show_string(0, 0, 200, 16, 16, (char*)"Testing ADXL355...", BLUE);
    
    ESP_LOGI(TAG, "=== MAIN: Starting ADXL355 test ===");
    
    // Step 1: Initialize SPI3 bus (general purpose)
    spi3_init();
    
    // Step 2: Initialize ADXL355 sensor (includes SPI device configuration)
    ret = adxl355_init(&adxl355_handle, ADXL355_RANGE_2G, ADXL355_ODR_1000);
    
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "✓ ADXL355 initialization successful!");
        lcd_show_string(0, 20, 200, 16, 16, (char*)"ADXL355 OK!", GREEN);
        
        // Read device info
        adxl355_device_info_t device_info;
        esp_err_t info_ret = adxl355_read_device_info(&adxl355_handle, &device_info);
        
        if (info_ret == ESP_OK) {
            ESP_LOGI(TAG, "Device ID: 0x%02X, Vendor ID: 0x%02X", device_info.device_id, device_info.vendor_id);
            
            char info_str[64];
            snprintf(info_str, sizeof(info_str), "ID: 0x%02X, 0x%02X, 0x%02X", 
                     device_info.vendor_id, device_info.family_id, device_info.device_id);
            lcd_show_string(0, 40, 200, 16, 16, (char*)info_str, BLACK);
            
            // Test temperature reading
            float test_temp;
            esp_err_t temp_test_ret = adxl355_read_temperature(&adxl355_handle, &test_temp);
            if (temp_test_ret == ESP_OK) {
                ESP_LOGI(TAG, "Temperature test: %.2f°C", test_temp);
                char temp_str[64];
                snprintf(temp_str, sizeof(temp_str), "Temp: %.1f°C", test_temp);
                lcd_show_string(0, 60, 200, 16, 16, (char*)temp_str, BLUE);
            } else {
                ESP_LOGE(TAG, "Temperature test failed: %s", esp_err_to_name(temp_test_ret));
                lcd_show_string(0, 60, 200, 16, 16, (char*)"Temp: Failed!", RED);
            }
        } else {
            ESP_LOGE(TAG, "✗ Failed to read device info: %s", esp_err_to_name(info_ret));
        }
        
    } else {
        ESP_LOGE(TAG, "✗ ADXL355 initialization failed: %s", esp_err_to_name(ret));
        lcd_show_string(0, 20, 200, 16, 16, (char*)"ADXL355 Failed!", RED);
    }
    
    vTaskDelay(3000);
    
    ESP_LOGI(TAG, "ADXL355 initialized and ready for MQTT publishing");

    ESP_LOGI(TAG, "=== MAIN: Entering main loop ===");
    
    while (1)
    {
        if(s_is_mqtt_connected)
        {
            // Read both acceleration and temperature data
            esp_err_t accel_ret = adxl355_read_accelerations(&adxl355_handle, &accel);
            esp_err_t temp_ret = adxl355_read_temperature(&adxl355_handle, &temperature);
            
            if (accel_ret == ESP_OK && temp_ret == ESP_OK) {
                // Format MQTT payload with both acceleration and temperature
                snprintf(mqtt_pub_buff, 64,
                    "{\"x\":%.4f,\"y\":%.4f,\"z\":%.4f,\"temp\":%.2f}",
                    accel.x, accel.y, accel.z, temperature);
                
                int mqtt_ret = esp_mqtt_client_publish(s_mqtt_client, MQTT_PUBLISH_TOPIC,
                                mqtt_pub_buff, strlen(mqtt_pub_buff), 1, 0);
                
                if (mqtt_ret < 0) {
                    ESP_LOGE(TAG, "✗ MQTT publish failed: %d", mqtt_ret);
                }
                
                // Output both acceleration and temperature to console
                printf("Accelerations: X: %f g, Y: %f g, Z: %f g | Temperature: %.2f°C\n", 
                       accel.x, accel.y, accel.z, temperature);
                
                // Also log to ESP log system
                ESP_LOGI(TAG, "Data: X=%.4f, Y=%.4f, Z=%.4f g, Temp=%.2f°C", 
                         accel.x, accel.y, accel.z, temperature);
                
            } else {
                if (accel_ret != ESP_OK) {
                    ESP_LOGE(TAG, "✗ Failed to read ADXL355 acceleration data: %s", esp_err_to_name(accel_ret));
                }
                if (temp_ret != ESP_OK) {
                    ESP_LOGE(TAG, "✗ Failed to read ADXL355 temperature data: %s", esp_err_to_name(temp_ret));
                }
            }
        } else {
            ESP_LOGW(TAG, "MQTT not connected, skipping data read");
        }
        
        led_toggle();
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}
```
