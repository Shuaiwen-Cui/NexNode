# ADXL355 CODE

## COMPONENT ARCHITECTURE

```plaintext
- driver
    - node_acc_adxl355
        - include
            - node_acc_adxl355.h
            - node_acc_adxl355_test.h
        - node_acc_adxl355.c
        - node_acc_adxl355_test.c
        - CMakeLists.txt
        - readme.txt
```

## driver/node_acc_adxl355/CMakeLists.txt

```cmake
idf_component_register(
    SRCS
        "node_acc_adxl355.c"
        "node_acc_adxl355_test.c"
    INCLUDE_DIRS
        "include"
    REQUIRES
        node_spi
        freertos
        log
        esp_driver_gpio
        esp_driver_spi
)

```

## node_acc_adxl355.h

```c
/**
 * @file node_acc_adxl355.h
 * @brief ADXL355 (SPI): init, range/ODR, XYZ/temp, DRDY, INT_MAP, FIFO, activity, offset, Phase 5 hardening.
 *
 * SPI framing matches Analog no-OS ref (driver/ref-adxl355). Bus: call @c spi3_init() from
 * node_spi before @ref node_acc_adxl355_init when using default eval wiring (CS=GPIO7, SCK/MOSI/MISO=15/16/17).
 */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Default SPI clock (Hz); ADXL355 supports up to 10 MHz on SPI. */
#define NODE_ACC_ADXL355_DEFAULT_SPI_HZ 10000000u

/* --- Register addresses (datasheet) --- */

#define NODE_ACC_ADXL355_REG_DEVID_AD 0x00u
#define NODE_ACC_ADXL355_REG_DEVID_MST 0x01u
#define NODE_ACC_ADXL355_REG_PARTID 0x02u
#define NODE_ACC_ADXL355_REG_REVID 0x03u
#define NODE_ACC_ADXL355_REG_STATUS 0x04u
#define NODE_ACC_ADXL355_REG_FIFO_ENTRIES 0x05u
#define NODE_ACC_ADXL355_REG_TEMP2 0x06u
#define NODE_ACC_ADXL355_REG_TEMP1 0x07u
#define NODE_ACC_ADXL355_REG_XDATA3 0x08u
#define NODE_ACC_ADXL355_REG_XDATA2 0x09u
#define NODE_ACC_ADXL355_REG_XDATA1 0x0Au
#define NODE_ACC_ADXL355_REG_YDATA3 0x0Bu
#define NODE_ACC_ADXL355_REG_YDATA2 0x0Cu
#define NODE_ACC_ADXL355_REG_YDATA1 0x0Du
#define NODE_ACC_ADXL355_REG_ZDATA3 0x0Eu
#define NODE_ACC_ADXL355_REG_ZDATA2 0x0Fu
#define NODE_ACC_ADXL355_REG_ZDATA1 0x10u
#define NODE_ACC_ADXL355_REG_FIFO_DATA 0x11u
#define NODE_ACC_ADXL355_REG_OFFSET_X_H 0x1Eu
#define NODE_ACC_ADXL355_REG_OFFSET_X_L 0x1Fu
#define NODE_ACC_ADXL355_REG_OFFSET_Y_H 0x20u
#define NODE_ACC_ADXL355_REG_OFFSET_Y_L 0x21u
#define NODE_ACC_ADXL355_REG_OFFSET_Z_H 0x22u
#define NODE_ACC_ADXL355_REG_OFFSET_Z_L 0x23u
#define NODE_ACC_ADXL355_REG_ACT_EN 0x24u
#define NODE_ACC_ADXL355_REG_ACT_THRESH_H 0x25u
#define NODE_ACC_ADXL355_REG_ACT_THRESH_L 0x26u
#define NODE_ACC_ADXL355_REG_ACT_COUNT 0x27u
#define NODE_ACC_ADXL355_REG_FILTER 0x28u
#define NODE_ACC_ADXL355_REG_FIFO_SAMPLES 0x29u
#define NODE_ACC_ADXL355_REG_INT_MAP 0x2Au
#define NODE_ACC_ADXL355_REG_SYNC 0x2Bu
#define NODE_ACC_ADXL355_REG_RANGE 0x2Cu
#define NODE_ACC_ADXL355_REG_POWER_CTL 0x2Du
#define NODE_ACC_ADXL355_REG_SELF_TEST 0x2Eu
#define NODE_ACC_ADXL355_REG_RESET 0x2Fu

#define NODE_ACC_ADXL355_REG_STATUS_DATA_RDY 0x01u
#define NODE_ACC_ADXL355_REG_STATUS_FIFO_FULL 0x02u
#define NODE_ACC_ADXL355_REG_STATUS_FIFO_OVR 0x04u
#define NODE_ACC_ADXL355_REG_STATUS_ACTIVITY 0x08u
#define NODE_ACC_ADXL355_REG_STATUS_NVM_BUSY 0x10u

#define NODE_ACC_ADXL355_REG_FILTER_HPF_SHIFT 4u
#define NODE_ACC_ADXL355_REG_FILTER_HPF_MASK 0xF0u
#define NODE_ACC_ADXL355_REG_FILTER_ODR_MASK 0x0Fu

#define NODE_ACC_ADXL355_REG_RANGE_RANGE_MASK 0x03u
#define NODE_ACC_ADXL355_REG_RANGE_INT_POL_SHIFT 6u
#define NODE_ACC_ADXL355_REG_RANGE_INT_POL_MASK 0x40u

#define NODE_ACC_ADXL355_REG_POWER_CTL_STANDBY 0x01u
#define NODE_ACC_ADXL355_REG_POWER_CTL_TEMP_OFF 0x02u
#define NODE_ACC_ADXL355_REG_POWER_CTL_DRDY_OFF 0x04u

#define NODE_ACC_ADXL355_REG_RESET_CODE 0x52u

/** Max value for FIFO_SAMPLES (0x29); matches Analog ref ADXL355_MAX_FIFO_SAMPLES_VAL. */
#define NODE_ACC_ADXL355_FIFO_SAMPLES_MAX 0x60u

/** Expected DEVID_AD (register 0x00). */
#define NODE_ACC_ADXL355_EXPECT_DEVID_AD 0xADu
#define NODE_ACC_ADXL355_EXPECT_DEVID_MST 0x1Du
#define NODE_ACC_ADXL355_EXPECT_PARTID 0xEDu

typedef enum {
    NODE_ACC_ADXL355_RANGE_2G = 0x01u,
    NODE_ACC_ADXL355_RANGE_4G = 0x02u,
    NODE_ACC_ADXL355_RANGE_8G = 0x03u,
} node_acc_adxl355_range_t;

/**
 * What to measure / expose (POWER_CTL TEMP_OFF + software policy).
 * Matches Analog @c adxl355_op_mode idea: accel-only clears the temperature path (TEMP_OFF).
 * TEMP_ONLY: chip still runs with temperature enabled (same HW as ACCEL_AND_TEMP); only
 * @ref node_acc_adxl355_read_raw_temp is valid; @ref node_acc_adxl355_read_raw_xyz returns
 * @c ESP_ERR_INVALID_STATE (ADXL355 has no separate “accel off, temp on” power state).
 */
typedef enum {
    NODE_ACC_ADXL355_MEAS_ACCEL_AND_TEMP = 0,
    NODE_ACC_ADXL355_MEAS_ACCEL_ONLY,
    NODE_ACC_ADXL355_MEAS_TEMP_ONLY,
} node_acc_adxl355_meas_mode_t;

typedef enum {
    NODE_ACC_ADXL355_ODR_4000_HZ = 0x00u,
    NODE_ACC_ADXL355_ODR_2000_HZ = 0x01u,
    NODE_ACC_ADXL355_ODR_1000_HZ = 0x02u,
    NODE_ACC_ADXL355_ODR_500_HZ = 0x03u,
    NODE_ACC_ADXL355_ODR_250_HZ = 0x04u,
    NODE_ACC_ADXL355_ODR_125_HZ = 0x05u,
    NODE_ACC_ADXL355_ODR_62_5_HZ = 0x06u,
    NODE_ACC_ADXL355_ODR_31_25_HZ = 0x07u,
    NODE_ACC_ADXL355_ODR_15_625_HZ = 0x08u,
    NODE_ACC_ADXL355_ODR_7_813_HZ = 0x09u,
    NODE_ACC_ADXL355_ODR_3_906_HZ = 0x0Au,
} node_acc_adxl355_odr_t;

typedef enum {
    NODE_ACC_ADXL355_STATUS_DATA_RDY = 0x01u,
    NODE_ACC_ADXL355_STATUS_FIFO_FULL = 0x02u,
    NODE_ACC_ADXL355_STATUS_FIFO_OVR = 0x04u,
    NODE_ACC_ADXL355_STATUS_ACTIVITY = 0x08u,
    NODE_ACC_ADXL355_STATUS_NVM_BUSY = 0x10u,
} node_acc_adxl355_status_t;

typedef struct {
    uint8_t devid_ad;
    uint8_t devid_mst;
    uint8_t partid;
    uint8_t revid;
} node_acc_adxl355_ids_t;

typedef struct {
    int32_t x;
    int32_t y;
    int32_t z;
} node_acc_adxl355_raw_xyz_t;

typedef struct {
    float x;
    float y;
    float z;
} node_acc_adxl355_g_t;

/** INT_MAP (0x2A): route sources to INT1 / INT2 (see datasheet; matches Analog ref bit layout). */
typedef struct {
    bool rdy_int1;
    bool fifo_full_int1;
    bool fifo_ovr_int1;
    bool activity_int1;
    bool rdy_int2;
    bool fifo_full_int2;
    bool fifo_ovr_int2;
    bool activity_int2;
} node_acc_adxl355_int_map_t;

/** RANGE[6] interrupt polarity (INT1/INT2/DRDY pin behavior). */
typedef enum {
    NODE_ACC_ADXL355_INT_POL_ACTIVE_LOW = 0,
    NODE_ACC_ADXL355_INT_POL_ACTIVE_HIGH = 1,
} node_acc_adxl355_int_pol_t;

/** ACT_EN (0x24): enable activity detection per axis (datasheet bit order). */
typedef struct {
    bool x;
    bool y;
    bool z;
} node_acc_adxl355_act_en_t;

/** Offset registers: 16-bit per axis, big-endian in SPI (Analog @c adxl355_set_offset). */
typedef struct {
    uint16_t x;
    uint16_t y;
    uint16_t z;
} node_acc_adxl355_offset_t;

/**
 * SPI device parameters. @a host bus must be initialized (e.g. @c spi3_init()) before
 * @ref node_acc_adxl355_init.
 */
typedef struct {
    spi_host_device_t host;
    gpio_num_t cs_gpio;
    uint32_t clock_hz;
    /** SPI mode 0..3; ADXL355 uses mode 0. */
    uint8_t spi_mode;
    int input_delay_ns;
    /**
     * DRDY MCU input (eval: GPIO6). Use @c GPIO_NUM_NC if @ref node_acc_adxl355_drdy_isr_install
     * is not used.
     */
    gpio_num_t gpio_drdy;
    /** Match RANGE INT_POL / board wiring (often @c GPIO_INTR_POSEDGE for active-high pulse). */
    gpio_int_type_t drdy_intr_type;
    /** INT1 / INT2 MCU inputs (eval: GPIO4 / GPIO5). @c GPIO_NUM_NC if unused. */
    gpio_num_t gpio_int1;
    gpio_num_t gpio_int2;
    /** Edges for INT1/INT2; align with @ref node_acc_adxl355_set_interrupt_polarity. */
    gpio_int_type_t int1_intr_type;
    gpio_int_type_t int2_intr_type;
    /** Initial @ref node_acc_adxl355_meas_mode_t (default eval: ACCEL_AND_TEMP). */
    node_acc_adxl355_meas_mode_t meas_mode;
    /**
     * If true (default), all SPI transfers are serialized with an internal recursive mutex
     * (safe with multi-task access). Set false only if a single task touches the device.
     */
    bool spi_mutex_enable;
    /** If true, log one INFO line after successful init. Default false (quieter production). */
    bool log_info_on_init;
} node_acc_adxl355_config_t;

typedef struct node_acc_adxl355_dev {
    spi_device_handle_t spi;
    node_acc_adxl355_range_t range;
    node_acc_adxl355_odr_t odr;
    float scale_factor;
    node_acc_adxl355_meas_mode_t meas_mode;
    gpio_num_t gpio_drdy;
    gpio_int_type_t drdy_intr_type;
    SemaphoreHandle_t drdy_sem;
    bool drdy_isr_installed;
    gpio_num_t gpio_int1;
    gpio_num_t gpio_int2;
    gpio_int_type_t int1_intr_type;
    gpio_int_type_t int2_intr_type;
    SemaphoreHandle_t int1_sem;
    SemaphoreHandle_t int2_sem;
    bool int1_isr_installed;
    bool int2_isr_installed;
    /** Recursive mutex when @a spi_mutex_enable was set at init; NULL otherwise. */
    SemaphoreHandle_t spi_mutex;
} node_acc_adxl355_dev_t;

/** Eval board defaults: CS GPIO7, SPI3, 10 MHz, mode 0, DRDY GPIO6 posedge. */
void node_acc_adxl355_config_default_eval(node_acc_adxl355_config_t *cfg);

esp_err_t node_acc_adxl355_init(node_acc_adxl355_dev_t *dev, const node_acc_adxl355_config_t *cfg,
                                node_acc_adxl355_range_t range, node_acc_adxl355_odr_t odr);

esp_err_t node_acc_adxl355_set_measurement_mode(node_acc_adxl355_dev_t *dev, node_acc_adxl355_meas_mode_t mode);

esp_err_t node_acc_adxl355_get_measurement_mode(node_acc_adxl355_dev_t *dev, node_acc_adxl355_meas_mode_t *mode);

/**
 * Remove GPIO ISRs, clear INT_MAP, disable DRDY pin and measurement, remove SPI device, delete SPI mutex.
 * Do not call concurrently with other tasks using this @a dev (take mutex first in your app or stop workers).
 */
esp_err_t node_acc_adxl355_deinit(node_acc_adxl355_dev_t *dev);

esp_err_t node_acc_adxl355_reset(node_acc_adxl355_dev_t *dev);

esp_err_t node_acc_adxl355_read_ids(node_acc_adxl355_dev_t *dev, node_acc_adxl355_ids_t *ids);

esp_err_t node_acc_adxl355_read_status(node_acc_adxl355_dev_t *dev, uint8_t *status);

esp_err_t node_acc_adxl355_read_raw_xyz(node_acc_adxl355_dev_t *dev, node_acc_adxl355_raw_xyz_t *out);

esp_err_t node_acc_adxl355_read_g(node_acc_adxl355_dev_t *dev, node_acc_adxl355_g_t *out);

esp_err_t node_acc_adxl355_read_raw_temp(node_acc_adxl355_dev_t *dev, uint16_t *raw_temp);

esp_err_t node_acc_adxl355_read_temp_c(node_acc_adxl355_dev_t *dev, float *temp_c);

esp_err_t node_acc_adxl355_set_range(node_acc_adxl355_dev_t *dev, node_acc_adxl355_range_t range);

esp_err_t node_acc_adxl355_get_range(node_acc_adxl355_dev_t *dev, node_acc_adxl355_range_t *range);

esp_err_t node_acc_adxl355_set_odr(node_acc_adxl355_dev_t *dev, node_acc_adxl355_odr_t odr);

esp_err_t node_acc_adxl355_get_odr(node_acc_adxl355_dev_t *dev, node_acc_adxl355_odr_t *odr);

esp_err_t node_acc_adxl355_enable_measurement(node_acc_adxl355_dev_t *dev);

esp_err_t node_acc_adxl355_disable_measurement(node_acc_adxl355_dev_t *dev);

esp_err_t node_acc_adxl355_enable_temperature(node_acc_adxl355_dev_t *dev);

esp_err_t node_acc_adxl355_disable_temperature(node_acc_adxl355_dev_t *dev);

esp_err_t node_acc_adxl355_enable_data_ready_pin(node_acc_adxl355_dev_t *dev);

esp_err_t node_acc_adxl355_disable_data_ready_pin(node_acc_adxl355_dev_t *dev);

esp_err_t node_acc_adxl355_acceleration_scale_g_per_lsb(node_acc_adxl355_dev_t *dev, float *scale);

/**
 * Install GPIO ISR on @a gpio_drdy from config: binary semaphore released on each DRDY edge.
 * Requires @ref node_acc_adxl355_enable_data_ready_pin (done in @ref node_acc_adxl355_init).
 * Call @ref node_acc_adxl355_drdy_isr_remove before @ref node_acc_adxl355_deinit or let deinit do it.
 */
esp_err_t node_acc_adxl355_drdy_isr_install(node_acc_adxl355_dev_t *dev);

esp_err_t node_acc_adxl355_drdy_isr_remove(node_acc_adxl355_dev_t *dev);

/** Block until next DRDY interrupt or timeout. Install ISR first. */
esp_err_t node_acc_adxl355_drdy_wait(node_acc_adxl355_dev_t *dev, uint32_t timeout_ms);

/** Write INT_MAP (0x2A). */
esp_err_t node_acc_adxl355_int_map_write(node_acc_adxl355_dev_t *dev, const node_acc_adxl355_int_map_t *map);

/** Read INT_MAP. */
esp_err_t node_acc_adxl355_int_map_read(node_acc_adxl355_dev_t *dev, node_acc_adxl355_int_map_t *map);

/** Set RANGE[6] interrupt polarity (INT1/INT2/DRDY). */
esp_err_t node_acc_adxl355_set_interrupt_polarity(node_acc_adxl355_dev_t *dev, node_acc_adxl355_int_pol_t pol);

/**
 * Install GPIO ISR on INT1/INT2 from config (binary semaphore per pin).
 * Map events with @ref node_acc_adxl355_int_map_write before relying on interrupts.
 */
esp_err_t node_acc_adxl355_int_isr_install(node_acc_adxl355_dev_t *dev);

esp_err_t node_acc_adxl355_int_isr_remove(node_acc_adxl355_dev_t *dev);

esp_err_t node_acc_adxl355_int1_wait(node_acc_adxl355_dev_t *dev, uint32_t timeout_ms);

esp_err_t node_acc_adxl355_int2_wait(node_acc_adxl355_dev_t *dev, uint32_t timeout_ms);

/* --- Phase 4: FIFO, activity, offset --- */

/** Write FIFO_SAMPLES (0x29); @a samples must be <= @ref NODE_ACC_ADXL355_FIFO_SAMPLES_MAX. */
esp_err_t node_acc_adxl355_set_fifo_samples(node_acc_adxl355_dev_t *dev, uint8_t samples);

esp_err_t node_acc_adxl355_get_fifo_samples(node_acc_adxl355_dev_t *dev, uint8_t *samples);

/** Read FIFO_ENTRIES (0x05): number of stored 3-byte axis samples (Analog ref semantics). */
esp_err_t node_acc_adxl355_read_fifo_entries(node_acc_adxl355_dev_t *dev, uint8_t *entries);

/**
 * Burst-read FIFO_DATA (0x11), parse XYZ frames (see Analog @c adxl355_get_raw_fifo_data).
 * @a out_frames is how many frames were written (<= @a max_frames). Drains FIFO when @a max_frames is large enough.
 */
esp_err_t node_acc_adxl355_read_fifo_xyz(node_acc_adxl355_dev_t *dev, node_acc_adxl355_raw_xyz_t *out,
                                         size_t max_frames, size_t *out_frames);

esp_err_t node_acc_adxl355_set_offset_xyz(node_acc_adxl355_dev_t *dev, const node_acc_adxl355_offset_t *off);

esp_err_t node_acc_adxl355_get_offset_xyz(node_acc_adxl355_dev_t *dev, node_acc_adxl355_offset_t *off);

esp_err_t node_acc_adxl355_set_activity_enable(node_acc_adxl355_dev_t *dev, const node_acc_adxl355_act_en_t *en);

esp_err_t node_acc_adxl355_get_activity_enable(node_acc_adxl355_dev_t *dev, node_acc_adxl355_act_en_t *en);

esp_err_t node_acc_adxl355_set_activity_threshold(node_acc_adxl355_dev_t *dev, uint16_t threshold);

esp_err_t node_acc_adxl355_get_activity_threshold(node_acc_adxl355_dev_t *dev, uint16_t *threshold);

esp_err_t node_acc_adxl355_set_activity_count(node_acc_adxl355_dev_t *dev, uint8_t count);

esp_err_t node_acc_adxl355_get_activity_count(node_acc_adxl355_dev_t *dev, uint8_t *count);

#ifdef __cplusplus
}
#endif

```

## node_acc_adxl355_test.h

```c
/**
 * @file node_acc_adxl355_test.h
 * @brief Phase 1–4 tests (polling, DRDY, INT_MAP, FIFO/activity/offset). Driver: node_acc_adxl355.h.
 */
#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Phase 1: @c spi3_init() from node_spi, then init ADXL355 on CS GPIO7.
 * Reads chip IDs, prints several XYZ + temperature samples, then deinitializes.
 */
esp_err_t node_acc_adxl355_run_phase1_test(void);

/**
 * Phase 2: DRDY on GPIO6 (default eval), @ref node_acc_adxl355_drdy_isr_install + wait + read XYZ.
 */
esp_err_t node_acc_adxl355_run_phase2_test(void);

/**
 * Phase 3: INT_MAP (DATA_RDY -> INT1), GPIO4, @ref node_acc_adxl355_int_isr_install + int1_wait + STATUS/XYZ.
 */
esp_err_t node_acc_adxl355_run_phase3_test(void);

/** ACCEL+TEMP / ACCEL-only / TEMP-only (software) paths; expects INVALID_STATE where documented. */
esp_err_t node_acc_adxl355_run_measurement_mode_test(void);

/** Phase 4: FIFO_SAMPLES / FIFO read, offset regs, activity regs (readback). */
esp_err_t node_acc_adxl355_run_phase4_test(void);

/** Phase 5: mutex + deinit path (init with optional log, deinit clears device). */
esp_err_t node_acc_adxl355_run_phase5_test(void);

#ifdef __cplusplus
}
#endif

```

## node_acc_adxl355.c

```c
/**
 * @file node_acc_adxl355.c
 * @brief ADXL355 SPI driver (phase 1–4: DRDY, INT_MAP, FIFO, activity, offset).
 */
#include "node_acc_adxl355.h"

#include <string.h>

#include "driver/gpio.h"
#include "esp_heap_caps.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

static const char *TAG = "node_acc_adxl355";

/* Temperature: datasheet intercept 1885 LSB @ 25 °C, slope -9.05 LSB/°C */
static const float k_temp_intercept_lsb = 1885.0f;
static const float k_temp_slope_lsb_per_c = -9.05f;
static const float k_temp_ref_c = 25.0f;

static const float k_scale_2g = 3.9e-6f;
static const float k_scale_4g = 7.8e-6f;
static const float k_scale_8g = 15.6e-6f;

static esp_err_t dev_spi_add(node_acc_adxl355_dev_t *dev, const node_acc_adxl355_config_t *cfg);
static esp_err_t dev_spi_remove(node_acc_adxl355_dev_t *dev);
static void dev_free_hw(node_acc_adxl355_dev_t *dev);
static void spi_lock(node_acc_adxl355_dev_t *dev);
static void spi_unlock(node_acc_adxl355_dev_t *dev);

static esp_err_t reg_read(node_acc_adxl355_dev_t *dev, uint8_t reg, uint8_t *val);
static esp_err_t reg_write(node_acc_adxl355_dev_t *dev, uint8_t reg, uint8_t val);
static esp_err_t regs_read(node_acc_adxl355_dev_t *dev, uint8_t reg, uint8_t *data, size_t len);
static esp_err_t regs_write(node_acc_adxl355_dev_t *dev, uint8_t reg, const uint8_t *data, size_t len);
static esp_err_t spi_read_after_cmd(node_acc_adxl355_dev_t *dev, uint8_t cmd, uint8_t *data, size_t len);

static void IRAM_ATTR node_acc_adxl355_sem_isr(void *arg)
{
    SemaphoreHandle_t sem = (SemaphoreHandle_t)arg;
    BaseType_t hp = pdFALSE;
    if (sem != NULL) {
        (void)xSemaphoreGiveFromISR(sem, &hp);
    }
    if (hp) {
        portYIELD_FROM_ISR();
    }
}

/** Call once per process; avoids ESP-IDF ERROR log on duplicate gpio_install_isr_service. */
static esp_err_t gpio_isr_service_ensure_installed(void)
{
    esp_err_t e = gpio_install_isr_service(0);
    if (e != ESP_OK && e != ESP_ERR_INVALID_STATE) {
        return e;
    }
    return ESP_OK;
}

void node_acc_adxl355_config_default_eval(node_acc_adxl355_config_t *cfg)
{
    if (cfg == NULL) {
        return;
    }
    memset(cfg, 0, sizeof(*cfg));
    cfg->host = SPI3_HOST;
    cfg->cs_gpio = GPIO_NUM_7;
    cfg->clock_hz = NODE_ACC_ADXL355_DEFAULT_SPI_HZ;
    cfg->spi_mode = 0;
    cfg->input_delay_ns = 30;
    cfg->gpio_drdy = GPIO_NUM_6;
    cfg->drdy_intr_type = GPIO_INTR_POSEDGE;
    cfg->gpio_int1 = GPIO_NUM_4;
    cfg->gpio_int2 = GPIO_NUM_5;
    cfg->int1_intr_type = GPIO_INTR_NEGEDGE;
    cfg->int2_intr_type = GPIO_INTR_NEGEDGE;
    cfg->meas_mode = NODE_ACC_ADXL355_MEAS_ACCEL_AND_TEMP;
    cfg->spi_mutex_enable = true;
    cfg->log_info_on_init = false;
}

static void spi_lock(node_acc_adxl355_dev_t *dev)
{
    if (dev != NULL && dev->spi_mutex != NULL) {
        xSemaphoreTakeRecursive(dev->spi_mutex, portMAX_DELAY);
    }
}

static void spi_unlock(node_acc_adxl355_dev_t *dev)
{
    if (dev != NULL && dev->spi_mutex != NULL) {
        xSemaphoreGiveRecursive(dev->spi_mutex);
    }
}

static void dev_free_hw(node_acc_adxl355_dev_t *dev)
{
    if (dev == NULL) {
        return;
    }
    if (dev->spi_mutex != NULL) {
        vSemaphoreDelete(dev->spi_mutex);
        dev->spi_mutex = NULL;
    }
    if (dev->spi != NULL) {
        (void)dev_spi_remove(dev);
    }
}

static esp_err_t dev_spi_add(node_acc_adxl355_dev_t *dev, const node_acc_adxl355_config_t *cfg)
{
    if (dev == NULL || cfg == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (cfg->cs_gpio < 0) {
        return ESP_ERR_INVALID_ARG;
    }

    /* No address_bits: ADXL355 expects one SPI stream (cmd + data) like Analog no-OS
     * write_and_read(1+size), discarding the first received byte. */
    spi_device_interface_config_t d = {
        .clock_speed_hz = (int)cfg->clock_hz,
        .mode = cfg->spi_mode,
        .spics_io_num = cfg->cs_gpio,
        .queue_size = 4,
        .command_bits = 0,
        .address_bits = 0,
        .dummy_bits = 0,
        .input_delay_ns = cfg->input_delay_ns,
    };

    return spi_bus_add_device(cfg->host, &d, &dev->spi);
}

static esp_err_t dev_spi_remove(node_acc_adxl355_dev_t *dev)
{
    if (dev == NULL || dev->spi == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t e = spi_bus_remove_device(dev->spi);
    dev->spi = NULL;
    return e;
}

/**
 * SPI read: same as Analog adxl355_read_device_data — one transaction of (1+len) bytes,
 * MOSI sends cmd then len zeros; first byte of MISO is discarded (command phase).
 */
static esp_err_t spi_read_after_cmd(node_acc_adxl355_dev_t *dev, uint8_t cmd, uint8_t *data, size_t len)
{
    if (dev == NULL || dev->spi == NULL || data == NULL || len == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    const size_t total = 1u + len;
    if (total > 320u) {
        return ESP_ERR_INVALID_ARG;
    }
    uint8_t tx_stack[33];
    uint8_t rx_stack[33];
    uint8_t *tx_buf = tx_stack;
    uint8_t *rx_buf = rx_stack;
    uint8_t *tx_heap = NULL;
    uint8_t *rx_heap = NULL;

    spi_lock(dev);

    if (total > sizeof(tx_stack)) {
        tx_heap = heap_caps_malloc(total, MALLOC_CAP_DMA);
        rx_heap = heap_caps_malloc(total, MALLOC_CAP_DMA);
        if (tx_heap == NULL || rx_heap == NULL) {
            heap_caps_free(tx_heap);
            heap_caps_free(rx_heap);
            spi_unlock(dev);
            return ESP_ERR_NO_MEM;
        }
        tx_buf = tx_heap;
        rx_buf = rx_heap;
    }

    memset(tx_buf, 0, total);
    tx_buf[0] = cmd;
    spi_transaction_t t = {0};
    t.length = (uint32_t)(total * 8u);
    t.tx_buffer = tx_buf;
    t.rx_buffer = rx_buf;
    esp_err_t err = spi_device_polling_transmit(dev->spi, &t);
    if (err == ESP_OK) {
        memcpy(data, &rx_buf[1], len);
    }
    heap_caps_free(tx_heap);
    heap_caps_free(rx_heap);
    spi_unlock(dev);
    return err;
}

static esp_err_t reg_read(node_acc_adxl355_dev_t *dev, uint8_t reg, uint8_t *val)
{
    uint8_t cmd = (uint8_t)((reg << 1) | 0x01u);
    return spi_read_after_cmd(dev, cmd, val, 1);
}

static esp_err_t reg_write(node_acc_adxl355_dev_t *dev, uint8_t reg, uint8_t val)
{
    if (dev == NULL || dev->spi == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    uint8_t cmd = (uint8_t)(reg << 1);
    uint8_t tx_buf[2] = {cmd, val};
    uint8_t rx_buf[2];

    spi_lock(dev);
    spi_transaction_t t = {0};
    t.length = 16;
    t.tx_buffer = tx_buf;
    t.rx_buffer = rx_buf;
    esp_err_t err = spi_device_polling_transmit(dev->spi, &t);
    spi_unlock(dev);
    return err;
}

static esp_err_t regs_read(node_acc_adxl355_dev_t *dev, uint8_t reg, uint8_t *data, size_t len)
{
    uint8_t cmd = (uint8_t)((reg << 1) | 0x01u);
    return spi_read_after_cmd(dev, cmd, data, len);
}

static esp_err_t regs_write(node_acc_adxl355_dev_t *dev, uint8_t reg, const uint8_t *data, size_t len)
{
    if (dev == NULL || dev->spi == NULL || data == NULL || len == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    const size_t total = 1u + len;
    if (total > 64u) {
        return ESP_ERR_INVALID_ARG;
    }
    uint8_t tx_buf[64];
    uint8_t rx_buf[64];
    tx_buf[0] = (uint8_t)(reg << 1);
    memcpy(&tx_buf[1], data, len);

    spi_lock(dev);
    spi_transaction_t t = {0};
    t.length = (uint32_t)(total * 8u);
    t.tx_buffer = tx_buf;
    t.rx_buffer = rx_buf;
    esp_err_t err = spi_device_polling_transmit(dev->spi, &t);
    spi_unlock(dev);
    return err;
}

static int32_t axis_raw_from_3bytes(const uint8_t data[3])
{
    int32_t v = 0;
    ((uint8_t *)&v)[1] = data[2];
    ((uint8_t *)&v)[2] = data[1];
    ((uint8_t *)&v)[3] = data[0];
    return v / 4096;
}

static bool fifo_frame_is_x_header(const uint8_t frame[9])
{
    return ((frame[2] & 1u) != 0) && ((frame[2] & 2u) == 0);
}

static void fifo_xyz_from_frame(const uint8_t *p, node_acc_adxl355_raw_xyz_t *out)
{
    out->x = axis_raw_from_3bytes(&p[0]);
    out->y = axis_raw_from_3bytes(&p[3]);
    out->z = axis_raw_from_3bytes(&p[6]);
}

static void raw_xyz_from_bytes(const uint8_t data[9], node_acc_adxl355_raw_xyz_t *out)
{
    int32_t x = 0;
    int32_t y = 0;
    int32_t z = 0;
    ((uint8_t *)&x)[1] = data[2];
    ((uint8_t *)&x)[2] = data[1];
    ((uint8_t *)&x)[3] = data[0];
    ((uint8_t *)&y)[1] = data[5];
    ((uint8_t *)&y)[2] = data[4];
    ((uint8_t *)&y)[3] = data[3];
    ((uint8_t *)&z)[1] = data[8];
    ((uint8_t *)&z)[2] = data[7];
    ((uint8_t *)&z)[3] = data[6];
    out->x = x / 4096;
    out->y = y / 4096;
    out->z = z / 4096;
}

static float scale_for_range(node_acc_adxl355_range_t range)
{
    switch (range) {
    case NODE_ACC_ADXL355_RANGE_2G:
        return k_scale_2g;
    case NODE_ACC_ADXL355_RANGE_4G:
        return k_scale_4g;
    case NODE_ACC_ADXL355_RANGE_8G:
        return k_scale_8g;
    default:
        return k_scale_2g;
    }
}

/** POWER_CTL TEMP_OFF: Analog MEAS_TEMP_OFF_* vs MEAS_TEMP_ON_* (driver/ref-adxl355 adxl355_op_mode). */
static esp_err_t apply_meas_mode_hw(node_acc_adxl355_dev_t *dev)
{
    ESP_RETURN_ON_FALSE(dev != NULL && dev->spi != NULL, ESP_ERR_INVALID_ARG, TAG, "dev");
    switch (dev->meas_mode) {
    case NODE_ACC_ADXL355_MEAS_ACCEL_AND_TEMP:
        return node_acc_adxl355_enable_temperature(dev);
    case NODE_ACC_ADXL355_MEAS_ACCEL_ONLY:
        return node_acc_adxl355_disable_temperature(dev);
    case NODE_ACC_ADXL355_MEAS_TEMP_ONLY:
        return node_acc_adxl355_enable_temperature(dev);
    default:
        return ESP_ERR_INVALID_ARG;
    }
}

esp_err_t node_acc_adxl355_set_measurement_mode(node_acc_adxl355_dev_t *dev, node_acc_adxl355_meas_mode_t mode)
{
    ESP_RETURN_ON_FALSE(dev != NULL && dev->spi != NULL, ESP_ERR_INVALID_ARG, TAG, "dev");
    dev->meas_mode = mode;
    return apply_meas_mode_hw(dev);
}

esp_err_t node_acc_adxl355_get_measurement_mode(node_acc_adxl355_dev_t *dev, node_acc_adxl355_meas_mode_t *mode)
{
    ESP_RETURN_ON_FALSE(dev != NULL && mode != NULL, ESP_ERR_INVALID_ARG, TAG, "arg");
    *mode = dev->meas_mode;
    return ESP_OK;
}

esp_err_t node_acc_adxl355_init(node_acc_adxl355_dev_t *dev, const node_acc_adxl355_config_t *cfg,
                                node_acc_adxl355_range_t range, node_acc_adxl355_odr_t odr)
{
    ESP_RETURN_ON_FALSE(dev != NULL && cfg != NULL, ESP_ERR_INVALID_ARG, TAG, "null arg");

    memset(dev, 0, sizeof(*dev));
    dev->gpio_drdy = cfg->gpio_drdy;
    dev->drdy_intr_type = cfg->drdy_intr_type;
    dev->gpio_int1 = cfg->gpio_int1;
    dev->gpio_int2 = cfg->gpio_int2;
    dev->int1_intr_type = cfg->int1_intr_type;
    dev->int2_intr_type = cfg->int2_intr_type;

    esp_err_t err = dev_spi_add(dev, cfg);
    if (err != ESP_OK) {
        return err;
    }

    if (cfg->spi_mutex_enable) {
        dev->spi_mutex = xSemaphoreCreateRecursiveMutex();
        if (dev->spi_mutex == NULL) {
            (void)dev_spi_remove(dev);
            return ESP_ERR_NO_MEM;
        }
    }

    dev->range = range;
    dev->odr = odr;
    dev->scale_factor = scale_for_range(range);

    err = node_acc_adxl355_reset(dev);
    if (err != ESP_OK) {
        goto fail;
    }
    vTaskDelay(pdMS_TO_TICKS(100));

    node_acc_adxl355_ids_t ids = {0};
    err = node_acc_adxl355_read_ids(dev, &ids);
    if (err != ESP_OK) {
        goto fail;
    }
    if (ids.devid_ad != NODE_ACC_ADXL355_EXPECT_DEVID_AD || ids.devid_mst != NODE_ACC_ADXL355_EXPECT_DEVID_MST ||
        ids.partid != NODE_ACC_ADXL355_EXPECT_PARTID) {
        ESP_LOGE(TAG, "ID mismatch: AD=0x%02X MST=0x%02X PART=0x%02X", ids.devid_ad, ids.devid_mst, ids.partid);
        err = ESP_ERR_NOT_FOUND;
        goto fail;
    }

    err = node_acc_adxl355_set_range(dev, range);
    if (err != ESP_OK) {
        goto fail;
    }
    err = node_acc_adxl355_set_odr(dev, odr);
    if (err != ESP_OK) {
        goto fail;
    }
    err = node_acc_adxl355_enable_measurement(dev);
    if (err != ESP_OK) {
        goto fail;
    }
    dev->meas_mode = cfg->meas_mode;
    err = apply_meas_mode_hw(dev);
    if (err != ESP_OK) {
        goto fail;
    }
    err = node_acc_adxl355_enable_data_ready_pin(dev);
    if (err != ESP_OK) {
        goto fail;
    }

    if (cfg->log_info_on_init) {
        ESP_LOGI(TAG, "init ok: range=%u odr=%u mode=%u", (unsigned)range, (unsigned)odr, (unsigned)dev->meas_mode);
    }
    return ESP_OK;

fail:
    dev_free_hw(dev);
    return err;
}

esp_err_t node_acc_adxl355_deinit(node_acc_adxl355_dev_t *dev)
{
    if (dev == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    (void)node_acc_adxl355_int_isr_remove(dev);
    (void)node_acc_adxl355_drdy_isr_remove(dev);

    if (dev->spi_mutex != NULL) {
        xSemaphoreTakeRecursive(dev->spi_mutex, portMAX_DELAY);
    }

    if (dev->spi != NULL) {
        node_acc_adxl355_int_map_t zmap = {0};
        (void)node_acc_adxl355_int_map_write(dev, &zmap);
        (void)node_acc_adxl355_disable_data_ready_pin(dev);
        (void)node_acc_adxl355_disable_measurement(dev);
        (void)dev_spi_remove(dev);
    }

    if (dev->spi_mutex != NULL) {
        vSemaphoreDelete(dev->spi_mutex);
        dev->spi_mutex = NULL;
    }

    memset(dev, 0, sizeof(*dev));
    return ESP_OK;
}

esp_err_t node_acc_adxl355_reset(node_acc_adxl355_dev_t *dev)
{
    ESP_RETURN_ON_FALSE(dev != NULL && dev->spi != NULL, ESP_ERR_INVALID_ARG, TAG, "dev");
    return reg_write(dev, NODE_ACC_ADXL355_REG_RESET, NODE_ACC_ADXL355_REG_RESET_CODE);
}

esp_err_t node_acc_adxl355_read_ids(node_acc_adxl355_dev_t *dev, node_acc_adxl355_ids_t *ids)
{
    ESP_RETURN_ON_FALSE(dev != NULL && ids != NULL && dev->spi != NULL, ESP_ERR_INVALID_ARG, TAG, "arg");

    uint8_t b[4] = {0};
    ESP_RETURN_ON_ERROR(regs_read(dev, NODE_ACC_ADXL355_REG_DEVID_AD, b, sizeof(b)), TAG, "regs");
    ids->devid_ad = b[0];
    ids->devid_mst = b[1];
    ids->partid = b[2];
    ids->revid = b[3];
    return ESP_OK;
}

esp_err_t node_acc_adxl355_read_status(node_acc_adxl355_dev_t *dev, uint8_t *status)
{
    ESP_RETURN_ON_FALSE(dev != NULL && status != NULL && dev->spi != NULL, ESP_ERR_INVALID_ARG, TAG, "arg");
    return reg_read(dev, NODE_ACC_ADXL355_REG_STATUS, status);
}

esp_err_t node_acc_adxl355_read_raw_xyz(node_acc_adxl355_dev_t *dev, node_acc_adxl355_raw_xyz_t *out)
{
    ESP_RETURN_ON_FALSE(dev != NULL && out != NULL && dev->spi != NULL, ESP_ERR_INVALID_ARG, TAG, "arg");
    if (dev->meas_mode == NODE_ACC_ADXL355_MEAS_TEMP_ONLY) {
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t data[9] = {0};
    ESP_RETURN_ON_ERROR(regs_read(dev, NODE_ACC_ADXL355_REG_XDATA3, data, sizeof(data)), TAG, "burst");
    raw_xyz_from_bytes(data, out);
    return ESP_OK;
}

esp_err_t node_acc_adxl355_read_g(node_acc_adxl355_dev_t *dev, node_acc_adxl355_g_t *out)
{
    ESP_RETURN_ON_FALSE(dev != NULL && out != NULL, ESP_ERR_INVALID_ARG, TAG, "arg");

    node_acc_adxl355_raw_xyz_t raw = {0};
    ESP_RETURN_ON_ERROR(node_acc_adxl355_read_raw_xyz(dev, &raw), TAG, "raw");

    float s = 0.0f;
    ESP_RETURN_ON_ERROR(node_acc_adxl355_acceleration_scale_g_per_lsb(dev, &s), TAG, "scale");
    out->x = (float)raw.x * s;
    out->y = (float)raw.y * s;
    out->z = (float)raw.z * s;
    return ESP_OK;
}

esp_err_t node_acc_adxl355_read_raw_temp(node_acc_adxl355_dev_t *dev, uint16_t *raw_temp)
{
    ESP_RETURN_ON_FALSE(dev != NULL && raw_temp != NULL && dev->spi != NULL, ESP_ERR_INVALID_ARG, TAG, "arg");
    if (dev->meas_mode == NODE_ACC_ADXL355_MEAS_ACCEL_ONLY) {
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t data[2] = {0};
    ESP_RETURN_ON_ERROR(regs_read(dev, NODE_ACC_ADXL355_REG_TEMP2, data, sizeof(data)), TAG, "temp regs");
    ((uint8_t *)raw_temp)[0] = data[1];
    ((uint8_t *)raw_temp)[1] = data[0];
    return ESP_OK;
}

esp_err_t node_acc_adxl355_read_temp_c(node_acc_adxl355_dev_t *dev, float *temp_c)
{
    ESP_RETURN_ON_FALSE(dev != NULL && temp_c != NULL, ESP_ERR_INVALID_ARG, TAG, "arg");
    if (dev->meas_mode == NODE_ACC_ADXL355_MEAS_ACCEL_ONLY) {
        return ESP_ERR_INVALID_STATE;
    }

    uint16_t raw = 0;
    ESP_RETURN_ON_ERROR(node_acc_adxl355_read_raw_temp(dev, &raw), TAG, "raw temp");

    float v = ((float)(int16_t)raw - k_temp_intercept_lsb) / k_temp_slope_lsb_per_c + k_temp_ref_c;
    *temp_c = v;
    return ESP_OK;
}

esp_err_t node_acc_adxl355_set_range(node_acc_adxl355_dev_t *dev, node_acc_adxl355_range_t range)
{
    ESP_RETURN_ON_FALSE(dev != NULL && dev->spi != NULL, ESP_ERR_INVALID_ARG, TAG, "dev");
    if (range != NODE_ACC_ADXL355_RANGE_2G && range != NODE_ACC_ADXL355_RANGE_4G &&
        range != NODE_ACC_ADXL355_RANGE_8G) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t r = 0;
    ESP_RETURN_ON_ERROR(reg_read(dev, NODE_ACC_ADXL355_REG_RANGE, &r), TAG, "read range");
    r = (uint8_t)((r & (uint8_t)~NODE_ACC_ADXL355_REG_RANGE_RANGE_MASK) | (range & NODE_ACC_ADXL355_REG_RANGE_RANGE_MASK));
    ESP_RETURN_ON_ERROR(reg_write(dev, NODE_ACC_ADXL355_REG_RANGE, r), TAG, "write range");

    uint8_t verify = 0;
    ESP_RETURN_ON_ERROR(reg_read(dev, NODE_ACC_ADXL355_REG_RANGE, &verify), TAG, "verify range");
    if (verify != r) {
        return ESP_ERR_INVALID_STATE;
    }

    dev->range = range;
    dev->scale_factor = scale_for_range(range);
    return ESP_OK;
}

esp_err_t node_acc_adxl355_get_range(node_acc_adxl355_dev_t *dev, node_acc_adxl355_range_t *range)
{
    ESP_RETURN_ON_FALSE(dev != NULL && range != NULL && dev->spi != NULL, ESP_ERR_INVALID_ARG, TAG, "arg");

    uint8_t r = 0;
    ESP_RETURN_ON_ERROR(reg_read(dev, NODE_ACC_ADXL355_REG_RANGE, &r), TAG, "read");
    *range = (node_acc_adxl355_range_t)(r & NODE_ACC_ADXL355_REG_RANGE_RANGE_MASK);
    return ESP_OK;
}

esp_err_t node_acc_adxl355_set_odr(node_acc_adxl355_dev_t *dev, node_acc_adxl355_odr_t odr)
{
    ESP_RETURN_ON_FALSE(dev != NULL && dev->spi != NULL, ESP_ERR_INVALID_ARG, TAG, "dev");

    uint8_t f = 0;
    ESP_RETURN_ON_ERROR(reg_read(dev, NODE_ACC_ADXL355_REG_FILTER, &f), TAG, "read filter");
    f = (uint8_t)((f & (uint8_t)~NODE_ACC_ADXL355_REG_FILTER_ODR_MASK) | ((uint8_t)odr & NODE_ACC_ADXL355_REG_FILTER_ODR_MASK));
    ESP_RETURN_ON_ERROR(reg_write(dev, NODE_ACC_ADXL355_REG_FILTER, f), TAG, "write filter");

    uint8_t verify = 0;
    ESP_RETURN_ON_ERROR(reg_read(dev, NODE_ACC_ADXL355_REG_FILTER, &verify), TAG, "verify");
    if (verify != f) {
        return ESP_ERR_INVALID_STATE;
    }
    dev->odr = odr;
    return ESP_OK;
}

esp_err_t node_acc_adxl355_get_odr(node_acc_adxl355_dev_t *dev, node_acc_adxl355_odr_t *odr)
{
    ESP_RETURN_ON_FALSE(dev != NULL && odr != NULL && dev->spi != NULL, ESP_ERR_INVALID_ARG, TAG, "arg");

    uint8_t f = 0;
    ESP_RETURN_ON_ERROR(reg_read(dev, NODE_ACC_ADXL355_REG_FILTER, &f), TAG, "read");
    *odr = (node_acc_adxl355_odr_t)(f & NODE_ACC_ADXL355_REG_FILTER_ODR_MASK);
    return ESP_OK;
}

esp_err_t node_acc_adxl355_enable_measurement(node_acc_adxl355_dev_t *dev)
{
    ESP_RETURN_ON_FALSE(dev != NULL && dev->spi != NULL, ESP_ERR_INVALID_ARG, TAG, "dev");

    uint8_t p = 0;
    ESP_RETURN_ON_ERROR(reg_read(dev, NODE_ACC_ADXL355_REG_POWER_CTL, &p), TAG, "read");
    p = (uint8_t)(p & (uint8_t)~NODE_ACC_ADXL355_REG_POWER_CTL_STANDBY);
    return reg_write(dev, NODE_ACC_ADXL355_REG_POWER_CTL, p);
}

esp_err_t node_acc_adxl355_disable_measurement(node_acc_adxl355_dev_t *dev)
{
    ESP_RETURN_ON_FALSE(dev != NULL && dev->spi != NULL, ESP_ERR_INVALID_ARG, TAG, "dev");

    uint8_t p = 0;
    ESP_RETURN_ON_ERROR(reg_read(dev, NODE_ACC_ADXL355_REG_POWER_CTL, &p), TAG, "read");
    p |= NODE_ACC_ADXL355_REG_POWER_CTL_STANDBY;
    return reg_write(dev, NODE_ACC_ADXL355_REG_POWER_CTL, p);
}

esp_err_t node_acc_adxl355_enable_temperature(node_acc_adxl355_dev_t *dev)
{
    ESP_RETURN_ON_FALSE(dev != NULL && dev->spi != NULL, ESP_ERR_INVALID_ARG, TAG, "dev");

    uint8_t p = 0;
    ESP_RETURN_ON_ERROR(reg_read(dev, NODE_ACC_ADXL355_REG_POWER_CTL, &p), TAG, "read");
    p = (uint8_t)(p & (uint8_t)~NODE_ACC_ADXL355_REG_POWER_CTL_TEMP_OFF);
    return reg_write(dev, NODE_ACC_ADXL355_REG_POWER_CTL, p);
}

esp_err_t node_acc_adxl355_disable_temperature(node_acc_adxl355_dev_t *dev)
{
    ESP_RETURN_ON_FALSE(dev != NULL && dev->spi != NULL, ESP_ERR_INVALID_ARG, TAG, "dev");

    uint8_t p = 0;
    ESP_RETURN_ON_ERROR(reg_read(dev, NODE_ACC_ADXL355_REG_POWER_CTL, &p), TAG, "read");
    p |= NODE_ACC_ADXL355_REG_POWER_CTL_TEMP_OFF;
    return reg_write(dev, NODE_ACC_ADXL355_REG_POWER_CTL, p);
}

esp_err_t node_acc_adxl355_enable_data_ready_pin(node_acc_adxl355_dev_t *dev)
{
    ESP_RETURN_ON_FALSE(dev != NULL && dev->spi != NULL, ESP_ERR_INVALID_ARG, TAG, "dev");

    uint8_t p = 0;
    ESP_RETURN_ON_ERROR(reg_read(dev, NODE_ACC_ADXL355_REG_POWER_CTL, &p), TAG, "read");
    p = (uint8_t)(p & (uint8_t)~NODE_ACC_ADXL355_REG_POWER_CTL_DRDY_OFF);
    return reg_write(dev, NODE_ACC_ADXL355_REG_POWER_CTL, p);
}

esp_err_t node_acc_adxl355_disable_data_ready_pin(node_acc_adxl355_dev_t *dev)
{
    ESP_RETURN_ON_FALSE(dev != NULL && dev->spi != NULL, ESP_ERR_INVALID_ARG, TAG, "dev");

    uint8_t p = 0;
    ESP_RETURN_ON_ERROR(reg_read(dev, NODE_ACC_ADXL355_REG_POWER_CTL, &p), TAG, "read");
    p |= NODE_ACC_ADXL355_REG_POWER_CTL_DRDY_OFF;
    return reg_write(dev, NODE_ACC_ADXL355_REG_POWER_CTL, p);
}

esp_err_t node_acc_adxl355_acceleration_scale_g_per_lsb(node_acc_adxl355_dev_t *dev, float *scale)
{
    ESP_RETURN_ON_FALSE(dev != NULL && scale != NULL, ESP_ERR_INVALID_ARG, TAG, "arg");

    node_acc_adxl355_range_t range = NODE_ACC_ADXL355_RANGE_2G;
    ESP_RETURN_ON_ERROR(node_acc_adxl355_get_range(dev, &range), TAG, "range");
    *scale = scale_for_range(range);
    return ESP_OK;
}

esp_err_t node_acc_adxl355_drdy_isr_install(node_acc_adxl355_dev_t *dev)
{
    ESP_RETURN_ON_FALSE(dev != NULL && dev->spi != NULL, ESP_ERR_INVALID_ARG, TAG, "dev");
    if (dev->gpio_drdy == GPIO_NUM_NC) {
        return ESP_ERR_NOT_SUPPORTED;
    }
    if (dev->drdy_intr_type == GPIO_INTR_DISABLE) {
        return ESP_ERR_INVALID_ARG;
    }

    if (dev->drdy_sem == NULL) {
        dev->drdy_sem = xSemaphoreCreateBinary();
        ESP_RETURN_ON_FALSE(dev->drdy_sem != NULL, ESP_ERR_NO_MEM, TAG, "sem");
    }
    if (dev->drdy_isr_installed) {
        return ESP_OK;
    }

    ESP_RETURN_ON_ERROR(gpio_isr_service_ensure_installed(), TAG, "isr_service");

    gpio_reset_pin(dev->gpio_drdy);
    gpio_config_t io = {
        .pin_bit_mask = 1ULL << (unsigned)dev->gpio_drdy,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = dev->drdy_intr_type,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&io), TAG, "gpio_config");
    ESP_RETURN_ON_ERROR(gpio_isr_handler_add(dev->gpio_drdy, node_acc_adxl355_sem_isr, dev->drdy_sem), TAG,
                        "isr_add");
    dev->drdy_isr_installed = true;
    gpio_intr_enable(dev->gpio_drdy);
    return ESP_OK;
}

esp_err_t node_acc_adxl355_drdy_isr_remove(node_acc_adxl355_dev_t *dev)
{
    if (dev == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!dev->drdy_isr_installed) {
        if (dev->drdy_sem != NULL) {
            vSemaphoreDelete(dev->drdy_sem);
            dev->drdy_sem = NULL;
        }
        return ESP_OK;
    }

    gpio_intr_disable(dev->gpio_drdy);
    (void)gpio_isr_handler_remove(dev->gpio_drdy);
    gpio_reset_pin(dev->gpio_drdy);
    dev->drdy_isr_installed = false;

    if (dev->drdy_sem != NULL) {
        vSemaphoreDelete(dev->drdy_sem);
        dev->drdy_sem = NULL;
    }
    return ESP_OK;
}

esp_err_t node_acc_adxl355_drdy_wait(node_acc_adxl355_dev_t *dev, uint32_t timeout_ms)
{
    ESP_RETURN_ON_FALSE(dev != NULL, ESP_ERR_INVALID_ARG, TAG, "dev");
    ESP_RETURN_ON_FALSE(dev->drdy_sem != NULL, ESP_ERR_INVALID_STATE, TAG, "no sem (install ISR first)");

    const TickType_t ticks = pdMS_TO_TICKS(timeout_ms);
    return xSemaphoreTake(dev->drdy_sem, ticks) == pdTRUE ? ESP_OK : ESP_ERR_TIMEOUT;
}

static uint8_t int_map_pack(const node_acc_adxl355_int_map_t *m)
{
    uint8_t v = 0;
    if (m->rdy_int1) {
        v |= 1u << 0;
    }
    if (m->fifo_full_int1) {
        v |= 1u << 1;
    }
    if (m->fifo_ovr_int1) {
        v |= 1u << 2;
    }
    if (m->activity_int1) {
        v |= 1u << 3;
    }
    if (m->rdy_int2) {
        v |= 1u << 4;
    }
    if (m->fifo_full_int2) {
        v |= 1u << 5;
    }
    if (m->fifo_ovr_int2) {
        v |= 1u << 6;
    }
    if (m->activity_int2) {
        v |= 1u << 7;
    }
    return v;
}

static void int_map_unpack(uint8_t v, node_acc_adxl355_int_map_t *m)
{
    m->rdy_int1 = (v & (1u << 0)) != 0;
    m->fifo_full_int1 = (v & (1u << 1)) != 0;
    m->fifo_ovr_int1 = (v & (1u << 2)) != 0;
    m->activity_int1 = (v & (1u << 3)) != 0;
    m->rdy_int2 = (v & (1u << 4)) != 0;
    m->fifo_full_int2 = (v & (1u << 5)) != 0;
    m->fifo_ovr_int2 = (v & (1u << 6)) != 0;
    m->activity_int2 = (v & (1u << 7)) != 0;
}

esp_err_t node_acc_adxl355_int_map_write(node_acc_adxl355_dev_t *dev, const node_acc_adxl355_int_map_t *map)
{
    ESP_RETURN_ON_FALSE(dev != NULL && map != NULL && dev->spi != NULL, ESP_ERR_INVALID_ARG, TAG, "arg");
    uint8_t v = int_map_pack(map);
    return reg_write(dev, NODE_ACC_ADXL355_REG_INT_MAP, v);
}

esp_err_t node_acc_adxl355_int_map_read(node_acc_adxl355_dev_t *dev, node_acc_adxl355_int_map_t *map)
{
    ESP_RETURN_ON_FALSE(dev != NULL && map != NULL && dev->spi != NULL, ESP_ERR_INVALID_ARG, TAG, "arg");
    uint8_t v = 0;
    ESP_RETURN_ON_ERROR(reg_read(dev, NODE_ACC_ADXL355_REG_INT_MAP, &v), TAG, "read");
    int_map_unpack(v, map);
    return ESP_OK;
}

esp_err_t node_acc_adxl355_set_interrupt_polarity(node_acc_adxl355_dev_t *dev, node_acc_adxl355_int_pol_t pol)
{
    ESP_RETURN_ON_FALSE(dev != NULL && dev->spi != NULL, ESP_ERR_INVALID_ARG, TAG, "dev");
    uint8_t r = 0;
    ESP_RETURN_ON_ERROR(reg_read(dev, NODE_ACC_ADXL355_REG_RANGE, &r), TAG, "read");
    r = (uint8_t)((r & (uint8_t)~NODE_ACC_ADXL355_REG_RANGE_INT_POL_MASK) |
                   (((uint8_t)pol << NODE_ACC_ADXL355_REG_RANGE_INT_POL_SHIFT) & NODE_ACC_ADXL355_REG_RANGE_INT_POL_MASK));
    return reg_write(dev, NODE_ACC_ADXL355_REG_RANGE, r);
}

static esp_err_t int_gpio_install_one(gpio_num_t gpio, gpio_int_type_t intr, SemaphoreHandle_t *sem, bool *installed)
{
    if (gpio == GPIO_NUM_NC) {
        return ESP_OK;
    }
    if (intr == GPIO_INTR_DISABLE) {
        return ESP_ERR_INVALID_ARG;
    }
    if (*sem == NULL) {
        *sem = xSemaphoreCreateBinary();
        ESP_RETURN_ON_FALSE(*sem != NULL, ESP_ERR_NO_MEM, TAG, "sem");
    }
    if (*installed) {
        return ESP_OK;
    }

    gpio_reset_pin(gpio);
    gpio_config_t io = {
        .pin_bit_mask = 1ULL << (unsigned)gpio,
        .mode = GPIO_MODE_INPUT,
        /* ADXL355 INT1/INT2 are open-drain active-low; idle high needs a pull (board or MCU). */
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = intr,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&io), TAG, "gpio_config");
    ESP_RETURN_ON_ERROR(gpio_isr_handler_add(gpio, node_acc_adxl355_sem_isr, *sem), TAG, "isr_add");
    *installed = true;
    gpio_intr_enable(gpio);
    return ESP_OK;
}

static void int_gpio_remove_one(gpio_num_t gpio, SemaphoreHandle_t *sem, bool *installed)
{
    if (gpio == GPIO_NUM_NC) {
        return;
    }
    if (!*installed) {
        if (*sem != NULL) {
            vSemaphoreDelete(*sem);
            *sem = NULL;
        }
        return;
    }
    gpio_intr_disable(gpio);
    (void)gpio_isr_handler_remove(gpio);
    gpio_reset_pin(gpio);
    *installed = false;
    if (*sem != NULL) {
        vSemaphoreDelete(*sem);
        *sem = NULL;
    }
}

esp_err_t node_acc_adxl355_int_isr_install(node_acc_adxl355_dev_t *dev)
{
    ESP_RETURN_ON_FALSE(dev != NULL && dev->spi != NULL, ESP_ERR_INVALID_ARG, TAG, "dev");
    if (dev->gpio_int1 == GPIO_NUM_NC && dev->gpio_int2 == GPIO_NUM_NC) {
        return ESP_ERR_NOT_SUPPORTED;
    }

    ESP_RETURN_ON_ERROR(gpio_isr_service_ensure_installed(), TAG, "isr_service");

    esp_err_t e = int_gpio_install_one(dev->gpio_int1, dev->int1_intr_type, &dev->int1_sem, &dev->int1_isr_installed);
    if (e != ESP_OK) {
        return e;
    }
    e = int_gpio_install_one(dev->gpio_int2, dev->int2_intr_type, &dev->int2_sem, &dev->int2_isr_installed);
    return e;
}

esp_err_t node_acc_adxl355_int_isr_remove(node_acc_adxl355_dev_t *dev)
{
    if (dev == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    int_gpio_remove_one(dev->gpio_int1, &dev->int1_sem, &dev->int1_isr_installed);
    int_gpio_remove_one(dev->gpio_int2, &dev->int2_sem, &dev->int2_isr_installed);
    return ESP_OK;
}

esp_err_t node_acc_adxl355_int1_wait(node_acc_adxl355_dev_t *dev, uint32_t timeout_ms)
{
    ESP_RETURN_ON_FALSE(dev != NULL, ESP_ERR_INVALID_ARG, TAG, "dev");
    ESP_RETURN_ON_FALSE(dev->int1_sem != NULL, ESP_ERR_INVALID_STATE, TAG, "INT1 ISR not installed");
    const TickType_t ticks = pdMS_TO_TICKS(timeout_ms);
    return xSemaphoreTake(dev->int1_sem, ticks) == pdTRUE ? ESP_OK : ESP_ERR_TIMEOUT;
}

esp_err_t node_acc_adxl355_int2_wait(node_acc_adxl355_dev_t *dev, uint32_t timeout_ms)
{
    ESP_RETURN_ON_FALSE(dev != NULL, ESP_ERR_INVALID_ARG, TAG, "dev");
    ESP_RETURN_ON_FALSE(dev->int2_sem != NULL, ESP_ERR_INVALID_STATE, TAG, "INT2 ISR not installed");
    const TickType_t ticks = pdMS_TO_TICKS(timeout_ms);
    return xSemaphoreTake(dev->int2_sem, ticks) == pdTRUE ? ESP_OK : ESP_ERR_TIMEOUT;
}

esp_err_t node_acc_adxl355_set_fifo_samples(node_acc_adxl355_dev_t *dev, uint8_t samples)
{
    ESP_RETURN_ON_FALSE(dev != NULL && dev->spi != NULL, ESP_ERR_INVALID_ARG, TAG, "dev");
    if (samples > NODE_ACC_ADXL355_FIFO_SAMPLES_MAX) {
        return ESP_ERR_INVALID_ARG;
    }
    return reg_write(dev, NODE_ACC_ADXL355_REG_FIFO_SAMPLES, samples);
}

esp_err_t node_acc_adxl355_get_fifo_samples(node_acc_adxl355_dev_t *dev, uint8_t *samples)
{
    ESP_RETURN_ON_FALSE(dev != NULL && samples != NULL && dev->spi != NULL, ESP_ERR_INVALID_ARG, TAG, "arg");
    return reg_read(dev, NODE_ACC_ADXL355_REG_FIFO_SAMPLES, samples);
}

esp_err_t node_acc_adxl355_read_fifo_entries(node_acc_adxl355_dev_t *dev, uint8_t *entries)
{
    ESP_RETURN_ON_FALSE(dev != NULL && entries != NULL && dev->spi != NULL, ESP_ERR_INVALID_ARG, TAG, "arg");
    return reg_read(dev, NODE_ACC_ADXL355_REG_FIFO_ENTRIES, entries);
}

esp_err_t node_acc_adxl355_read_fifo_xyz(node_acc_adxl355_dev_t *dev, node_acc_adxl355_raw_xyz_t *out,
                                         size_t max_frames, size_t *out_frames)
{
    if (out_frames != NULL) {
        *out_frames = 0;
    }
    ESP_RETURN_ON_FALSE(dev != NULL && dev->spi != NULL, ESP_ERR_INVALID_ARG, TAG, "dev");
    ESP_RETURN_ON_FALSE(out != NULL && max_frames > 0, ESP_ERR_INVALID_ARG, TAG, "out");
    if (dev->meas_mode == NODE_ACC_ADXL355_MEAS_TEMP_ONLY) {
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t ent = 0;
    ESP_RETURN_ON_ERROR(node_acc_adxl355_read_fifo_entries(dev, &ent), TAG, "fifo_entries");
    uint8_t n = (uint8_t)(((unsigned)ent / 3u) * 3u);
    if (n == 0) {
        return ESP_OK;
    }

    const size_t byte_len = (size_t)n * 3u;
    uint8_t *buf = heap_caps_malloc(byte_len, MALLOC_CAP_DMA);
    ESP_RETURN_ON_FALSE(buf != NULL, ESP_ERR_NO_MEM, TAG, "fifo buf");

    uint8_t cmd = (uint8_t)((NODE_ACC_ADXL355_REG_FIFO_DATA << 1) | 0x01u);
    esp_err_t err = spi_read_after_cmd(dev, cmd, buf, byte_len);
    if (err != ESP_OK) {
        heap_caps_free(buf);
        return err;
    }

    const size_t num_frames = byte_len / 9u;
    size_t k = 0;
    for (size_t i = 0; i < num_frames && k < max_frames; i++) {
        const uint8_t *p = &buf[i * 9u];
        if (fifo_frame_is_x_header(p)) {
            fifo_xyz_from_frame(p, &out[k]);
            k++;
        }
    }
    if (k == 0 && num_frames >= 1) {
        fifo_xyz_from_frame(buf, &out[0]);
        k = 1;
    }
    if (out_frames != NULL) {
        *out_frames = k;
    }
    heap_caps_free(buf);
    return ESP_OK;
}

esp_err_t node_acc_adxl355_set_offset_xyz(node_acc_adxl355_dev_t *dev, const node_acc_adxl355_offset_t *off)
{
    ESP_RETURN_ON_FALSE(dev != NULL && off != NULL && dev->spi != NULL, ESP_ERR_INVALID_ARG, TAG, "arg");
    uint8_t b[6];
    b[0] = (uint8_t)(off->x >> 8);
    b[1] = (uint8_t)(off->x);
    b[2] = (uint8_t)(off->y >> 8);
    b[3] = (uint8_t)(off->y);
    b[4] = (uint8_t)(off->z >> 8);
    b[5] = (uint8_t)(off->z);
    return regs_write(dev, NODE_ACC_ADXL355_REG_OFFSET_X_H, b, sizeof(b));
}

esp_err_t node_acc_adxl355_get_offset_xyz(node_acc_adxl355_dev_t *dev, node_acc_adxl355_offset_t *off)
{
    ESP_RETURN_ON_FALSE(dev != NULL && off != NULL && dev->spi != NULL, ESP_ERR_INVALID_ARG, TAG, "arg");
    uint8_t b[6] = {0};
    ESP_RETURN_ON_ERROR(regs_read(dev, NODE_ACC_ADXL355_REG_OFFSET_X_H, b, sizeof(b)), TAG, "offset read");
    off->x = (uint16_t)(((unsigned)b[0] << 8) | b[1]);
    off->y = (uint16_t)(((unsigned)b[2] << 8) | b[3]);
    off->z = (uint16_t)(((unsigned)b[4] << 8) | b[5]);
    return ESP_OK;
}

esp_err_t node_acc_adxl355_set_activity_enable(node_acc_adxl355_dev_t *dev, const node_acc_adxl355_act_en_t *en)
{
    ESP_RETURN_ON_FALSE(dev != NULL && en != NULL && dev->spi != NULL, ESP_ERR_INVALID_ARG, TAG, "arg");
    uint8_t v = 0;
    if (en->x) {
        v |= 1u;
    }
    if (en->y) {
        v |= 2u;
    }
    if (en->z) {
        v |= 4u;
    }
    return reg_write(dev, NODE_ACC_ADXL355_REG_ACT_EN, v);
}

esp_err_t node_acc_adxl355_get_activity_enable(node_acc_adxl355_dev_t *dev, node_acc_adxl355_act_en_t *en)
{
    ESP_RETURN_ON_FALSE(dev != NULL && en != NULL && dev->spi != NULL, ESP_ERR_INVALID_ARG, TAG, "arg");
    uint8_t v = 0;
    ESP_RETURN_ON_ERROR(reg_read(dev, NODE_ACC_ADXL355_REG_ACT_EN, &v), TAG, "read");
    en->x = (v & 1u) != 0;
    en->y = (v & 2u) != 0;
    en->z = (v & 4u) != 0;
    return ESP_OK;
}

esp_err_t node_acc_adxl355_set_activity_threshold(node_acc_adxl355_dev_t *dev, uint16_t threshold)
{
    ESP_RETURN_ON_FALSE(dev != NULL && dev->spi != NULL, ESP_ERR_INVALID_ARG, TAG, "dev");
    uint8_t d[2] = {(uint8_t)(threshold >> 8), (uint8_t)threshold};
    return regs_write(dev, NODE_ACC_ADXL355_REG_ACT_THRESH_H, d, sizeof(d));
}

esp_err_t node_acc_adxl355_get_activity_threshold(node_acc_adxl355_dev_t *dev, uint16_t *threshold)
{
    ESP_RETURN_ON_FALSE(dev != NULL && threshold != NULL && dev->spi != NULL, ESP_ERR_INVALID_ARG, TAG, "arg");
    uint8_t d[2] = {0};
    ESP_RETURN_ON_ERROR(regs_read(dev, NODE_ACC_ADXL355_REG_ACT_THRESH_H, d, sizeof(d)), TAG, "read");
    *threshold = (uint16_t)(((unsigned)d[0] << 8) | d[1]);
    return ESP_OK;
}

esp_err_t node_acc_adxl355_set_activity_count(node_acc_adxl355_dev_t *dev, uint8_t count)
{
    ESP_RETURN_ON_FALSE(dev != NULL && dev->spi != NULL, ESP_ERR_INVALID_ARG, TAG, "dev");
    return reg_write(dev, NODE_ACC_ADXL355_REG_ACT_COUNT, count);
}

esp_err_t node_acc_adxl355_get_activity_count(node_acc_adxl355_dev_t *dev, uint8_t *count)
{
    ESP_RETURN_ON_FALSE(dev != NULL && count != NULL && dev->spi != NULL, ESP_ERR_INVALID_ARG, TAG, "arg");
    return reg_read(dev, NODE_ACC_ADXL355_REG_ACT_COUNT, count);
}

```

## node_acc_adxl355_test.c

```c
/**
 * @file node_acc_adxl355_test.c
 * @brief Phase 1 polling; Phase 2 DRDY; Phase 3 INT_MAP; Phase 4 FIFO/activity/offset.
 */
#include "node_acc_adxl355_test.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "node_acc_adxl355.h"
#include "node_spi.h"

static const char *TAG = "adxl355_test";

#ifndef NODE_ACC_ADXL355_TEST_SAMPLES
#define NODE_ACC_ADXL355_TEST_SAMPLES 8
#endif

#ifndef NODE_ACC_ADXL355_TEST_SAMPLE_DELAY_MS
#define NODE_ACC_ADXL355_TEST_SAMPLE_DELAY_MS 100
#endif

#ifndef NODE_ACC_ADXL355_TEST_PHASE2_SAMPLES
#define NODE_ACC_ADXL355_TEST_PHASE2_SAMPLES 16
#endif

/** Per-sample wait; ODR 125 Hz => ~8 ms period, allow margin. */
#ifndef NODE_ACC_ADXL355_TEST_PHASE2_WAIT_MS
#define NODE_ACC_ADXL355_TEST_PHASE2_WAIT_MS 50
#endif

#ifndef NODE_ACC_ADXL355_TEST_PHASE3_SAMPLES
#define NODE_ACC_ADXL355_TEST_PHASE3_SAMPLES 12
#endif

#ifndef NODE_ACC_ADXL355_TEST_PHASE3_WAIT_MS
#define NODE_ACC_ADXL355_TEST_PHASE3_WAIT_MS 100
#endif

/** Override before init if board uses active-high INT (e.g. POSEDGE + ACTIVE_HIGH). */
#ifndef NODE_ACC_ADXL355_PHASE3_INT_POL
#define NODE_ACC_ADXL355_PHASE3_INT_POL NODE_ACC_ADXL355_INT_POL_ACTIVE_LOW
#endif
#ifndef NODE_ACC_ADXL355_PHASE3_INT_INTR
#define NODE_ACC_ADXL355_PHASE3_INT_INTR GPIO_INTR_NEGEDGE
#endif

#ifndef NODE_ACC_ADXL355_TEST_PHASE4_DRDY_WAITS
#define NODE_ACC_ADXL355_TEST_PHASE4_DRDY_WAITS 24
#endif

esp_err_t node_acc_adxl355_run_phase1_test(void)
{
    spi3_init();

    node_acc_adxl355_config_t cfg;
    node_acc_adxl355_config_default_eval(&cfg);

    node_acc_adxl355_dev_t dev = {0};
    esp_err_t ret =
        node_acc_adxl355_init(&dev, &cfg, NODE_ACC_ADXL355_RANGE_2G, NODE_ACC_ADXL355_ODR_125_HZ);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    node_acc_adxl355_ids_t ids = {0};
    ret = node_acc_adxl355_read_ids(&dev, &ids);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "IDs: AD=0x%02X MST=0x%02X PART=0x%02X REV=0x%02X", ids.devid_ad, ids.devid_mst, ids.partid,
                 ids.revid);
    }

    for (int i = 0; i < NODE_ACC_ADXL355_TEST_SAMPLES; i++) {
        node_acc_adxl355_raw_xyz_t raw = {0};
        node_acc_adxl355_g_t g = {0};
        float tc = 0.0f;
        uint8_t st = 0;

        ret = node_acc_adxl355_read_status(&dev, &st);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "status: %s", esp_err_to_name(ret));
            break;
        }
        ret = node_acc_adxl355_read_raw_xyz(&dev, &raw);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "raw xyz: %s", esp_err_to_name(ret));
            break;
        }
        ret = node_acc_adxl355_read_g(&dev, &g);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "g: %s", esp_err_to_name(ret));
            break;
        }
        ret = node_acc_adxl355_read_temp_c(&dev, &tc);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "temp: %s", esp_err_to_name(ret));
            break;
        }

        ESP_LOGI(TAG, "sample %d: ST=0x%02X raw xyz=%ld %ld %ld  g=(%.5f %.5f %.5f)  T=%.2f C", i,
                 (unsigned)st, (long)raw.x, (long)raw.y, (long)raw.z, (double)g.x, (double)g.y, (double)g.z,
                 (double)tc);

        vTaskDelay(pdMS_TO_TICKS(NODE_ACC_ADXL355_TEST_SAMPLE_DELAY_MS));
    }

    esp_err_t de = node_acc_adxl355_deinit(&dev);
    if (de != ESP_OK) {
        ESP_LOGW(TAG, "deinit: %s", esp_err_to_name(de));
    }
    if (ret != ESP_OK) {
        return ret;
    }
    return de;
}

esp_err_t node_acc_adxl355_run_phase2_test(void)
{
    spi3_init();

    node_acc_adxl355_config_t cfg;
    node_acc_adxl355_config_default_eval(&cfg);

    node_acc_adxl355_dev_t dev = {0};
    esp_err_t ret =
        node_acc_adxl355_init(&dev, &cfg, NODE_ACC_ADXL355_RANGE_2G, NODE_ACC_ADXL355_ODR_125_HZ);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "phase2 init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = node_acc_adxl355_drdy_isr_install(&dev);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "phase2 drdy_isr_install failed: %s", esp_err_to_name(ret));
        (void)node_acc_adxl355_deinit(&dev);
        return ret;
    }

    if (dev.drdy_sem != NULL) {
        while (xSemaphoreTake(dev.drdy_sem, 0) == pdTRUE) {
        }
    }

    ESP_LOGI(TAG, "phase2: DRDY GPIO%u %s, %d samples, wait %u ms", (unsigned)dev.gpio_drdy,
             (dev.drdy_intr_type == GPIO_INTR_POSEDGE) ? "posedge" : "edge", NODE_ACC_ADXL355_TEST_PHASE2_SAMPLES,
             (unsigned)NODE_ACC_ADXL355_TEST_PHASE2_WAIT_MS);

    for (int i = 0; i < NODE_ACC_ADXL355_TEST_PHASE2_SAMPLES; i++) {
        ret = node_acc_adxl355_drdy_wait(&dev, NODE_ACC_ADXL355_TEST_PHASE2_WAIT_MS);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "phase2 wait sample %d: %s", i, esp_err_to_name(ret));
            break;
        }

        node_acc_adxl355_raw_xyz_t raw = {0};
        node_acc_adxl355_g_t g = {0};
        ret = node_acc_adxl355_read_raw_xyz(&dev, &raw);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "phase2 raw xyz: %s", esp_err_to_name(ret));
            break;
        }
        ret = node_acc_adxl355_read_g(&dev, &g);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "phase2 g: %s", esp_err_to_name(ret));
            break;
        }
        ESP_LOGI(TAG, "phase2 sample %d: raw %ld %ld %ld  g=(%.5f %.5f %.5f)", i, (long)raw.x, (long)raw.y,
                 (long)raw.z, (double)g.x, (double)g.y, (double)g.z);
    }

    esp_err_t de = node_acc_adxl355_deinit(&dev);
    if (de != ESP_OK) {
        ESP_LOGW(TAG, "phase2 deinit: %s", esp_err_to_name(de));
    }
    if (ret != ESP_OK) {
        return ret;
    }
    return de;
}

esp_err_t node_acc_adxl355_run_phase3_test(void)
{
    spi3_init();

    node_acc_adxl355_config_t cfg;
    node_acc_adxl355_config_default_eval(&cfg);
    cfg.int1_intr_type = NODE_ACC_ADXL355_PHASE3_INT_INTR;

    node_acc_adxl355_dev_t dev = {0};
    esp_err_t ret =
        node_acc_adxl355_init(&dev, &cfg, NODE_ACC_ADXL355_RANGE_2G, NODE_ACC_ADXL355_ODR_125_HZ);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "phase3 init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = node_acc_adxl355_set_interrupt_polarity(&dev, NODE_ACC_ADXL355_PHASE3_INT_POL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "phase3 set_interrupt_polarity: %s", esp_err_to_name(ret));
        (void)node_acc_adxl355_deinit(&dev);
        return ret;
    }

    /* Arm GPIO ISR before INT_MAP so the first DATA_RDY pulse after mapping is not missed. */
    ret = node_acc_adxl355_int_isr_install(&dev);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "phase3 int_isr_install: %s", esp_err_to_name(ret));
        (void)node_acc_adxl355_deinit(&dev);
        return ret;
    }

    node_acc_adxl355_int_map_t imap = {0};
    imap.rdy_int1 = true;
    ret = node_acc_adxl355_int_map_write(&dev, &imap);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "phase3 int_map_write: %s", esp_err_to_name(ret));
        (void)node_acc_adxl355_deinit(&dev);
        return ret;
    }

    if (dev.int1_sem != NULL) {
        while (xSemaphoreTake(dev.int1_sem, 0) == pdTRUE) {
        }
    }

    /* Clear any pending DATA_RDY so the first int1_wait aligns to the next edge. */
    {
        const TickType_t t_end = xTaskGetTickCount() + pdMS_TO_TICKS(200);
        while (xTaskGetTickCount() < t_end) {
            uint8_t st = 0;
            ret = node_acc_adxl355_read_status(&dev, &st);
            if (ret != ESP_OK) {
                break;
            }
            if (st & NODE_ACC_ADXL355_REG_STATUS_DATA_RDY) {
                node_acc_adxl355_raw_xyz_t raw = {0};
                (void)node_acc_adxl355_read_raw_xyz(&dev, &raw);
                break;
            }
            vTaskDelay(pdMS_TO_TICKS(1));
        }
        while (dev.int1_sem != NULL && xSemaphoreTake(dev.int1_sem, 0) == pdTRUE) {
        }
    }

    ESP_LOGI(TAG, "phase3: INT1 GPIO%u (DATA_RDY), %d samples", (unsigned)dev.gpio_int1,
             NODE_ACC_ADXL355_TEST_PHASE3_SAMPLES);

    for (int i = 0; i < NODE_ACC_ADXL355_TEST_PHASE3_SAMPLES; i++) {
        ret = node_acc_adxl355_int1_wait(&dev, NODE_ACC_ADXL355_TEST_PHASE3_WAIT_MS);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "phase3 int1_wait %d: %s", i, esp_err_to_name(ret));
            break;
        }

        uint8_t st = 0;
        ret = node_acc_adxl355_read_status(&dev, &st);
        if (ret != ESP_OK) {
            break;
        }
        node_acc_adxl355_raw_xyz_t raw = {0};
        ret = node_acc_adxl355_read_raw_xyz(&dev, &raw);
        if (ret != ESP_OK) {
            break;
        }
        ESP_LOGI(TAG, "phase3 sample %d: STATUS=0x%02X (DATA_RDY=%u) raw %ld %ld %ld", i, (unsigned)st,
                 (unsigned)(st & NODE_ACC_ADXL355_REG_STATUS_DATA_RDY), (long)raw.x, (long)raw.y, (long)raw.z);
    }

    node_acc_adxl355_int_map_t zmap = {0};
    (void)node_acc_adxl355_int_map_write(&dev, &zmap);

    esp_err_t de = node_acc_adxl355_deinit(&dev);
    if (de != ESP_OK) {
        ESP_LOGW(TAG, "phase3 deinit: %s", esp_err_to_name(de));
    }
    if (ret != ESP_OK) {
        return ret;
    }
    return de;
}

esp_err_t node_acc_adxl355_run_phase4_test(void)
{
    spi3_init();

    node_acc_adxl355_config_t cfg;
    node_acc_adxl355_config_default_eval(&cfg);

    node_acc_adxl355_dev_t dev = {0};
    esp_err_t ret =
        node_acc_adxl355_init(&dev, &cfg, NODE_ACC_ADXL355_RANGE_2G, NODE_ACC_ADXL355_ODR_125_HZ);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "phase4 init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    node_acc_adxl355_offset_t off0 = {0};
    ret = node_acc_adxl355_get_offset_xyz(&dev, &off0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "phase4 get_offset: %s", esp_err_to_name(ret));
        (void)node_acc_adxl355_deinit(&dev);
        return ret;
    }
    ESP_LOGI(TAG, "phase4: offset readback X=0x%04X Y=0x%04X Z=0x%04X", (unsigned)off0.x, (unsigned)off0.y,
             (unsigned)off0.z);

    ret = node_acc_adxl355_set_fifo_samples(&dev, 32);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "phase4 set_fifo_samples: %s", esp_err_to_name(ret));
        (void)node_acc_adxl355_deinit(&dev);
        return ret;
    }
    uint8_t fs = 0;
    (void)node_acc_adxl355_get_fifo_samples(&dev, &fs);
    ESP_LOGI(TAG, "phase4: FIFO_SAMPLES reg=%u", (unsigned)fs);

    ret = node_acc_adxl355_drdy_isr_install(&dev);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "phase4 drdy_isr_install: %s", esp_err_to_name(ret));
        (void)node_acc_adxl355_deinit(&dev);
        return ret;
    }

    for (int w = 0; w < NODE_ACC_ADXL355_TEST_PHASE4_DRDY_WAITS; w++) {
        ret = node_acc_adxl355_drdy_wait(&dev, 100);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "phase4 drdy_wait %d: %s", w, esp_err_to_name(ret));
            break;
        }
    }

    uint8_t ent = 0;
    ret = node_acc_adxl355_read_fifo_entries(&dev, &ent);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "phase4 fifo_entries: %s", esp_err_to_name(ret));
        (void)node_acc_adxl355_deinit(&dev);
        return ret;
    }
    ESP_LOGI(TAG, "phase4: FIFO_ENTRIES=%u (after DRDY waits)", (unsigned)ent);

    node_acc_adxl355_raw_xyz_t fifo_xyz[8] = {0};
    size_t nfrm = 0;
    ret = node_acc_adxl355_read_fifo_xyz(&dev, fifo_xyz, 8, &nfrm);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "phase4 read_fifo_xyz: %s", esp_err_to_name(ret));
    } else if (nfrm > 0) {
        ESP_LOGI(TAG, "phase4: FIFO frames=%u  first raw %ld %ld %ld", (unsigned)nfrm, (long)fifo_xyz[0].x,
                 (long)fifo_xyz[0].y, (long)fifo_xyz[0].z);
    } else {
        ESP_LOGW(TAG, "phase4: FIFO parse got 0 frames (entries may still be valid for chip)");
    }

    node_acc_adxl355_act_en_t act = {.x = false, .y = false, .z = false};
    ret = node_acc_adxl355_set_activity_enable(&dev, &act);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "phase4 set_activity_enable: %s", esp_err_to_name(ret));
        (void)node_acc_adxl355_deinit(&dev);
        return ret;
    }
    ret = node_acc_adxl355_set_activity_threshold(&dev, 0x0400);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "phase4 set_activity_threshold: %s", esp_err_to_name(ret));
        (void)node_acc_adxl355_deinit(&dev);
        return ret;
    }
    ret = node_acc_adxl355_set_activity_count(&dev, 0x01);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "phase4 set_activity_count: %s", esp_err_to_name(ret));
        (void)node_acc_adxl355_deinit(&dev);
        return ret;
    }
    node_acc_adxl355_act_en_t ar = {0};
    uint16_t thr = 0;
    uint8_t ac = 0;
    (void)node_acc_adxl355_get_activity_enable(&dev, &ar);
    (void)node_acc_adxl355_get_activity_threshold(&dev, &thr);
    (void)node_acc_adxl355_get_activity_count(&dev, &ac);
    ESP_LOGI(TAG, "phase4: ACT readback en=%d%d%d thresh=0x%04X count=%u", ar.x ? 1 : 0, ar.y ? 1 : 0,
             ar.z ? 1 : 0, (unsigned)thr, (unsigned)ac);

    esp_err_t de = node_acc_adxl355_deinit(&dev);
    if (de != ESP_OK) {
        ESP_LOGW(TAG, "phase4 deinit: %s", esp_err_to_name(de));
    }
    if (ret != ESP_OK) {
        return ret;
    }
    return de;
}

esp_err_t node_acc_adxl355_run_phase5_test(void)
{
    spi3_init();

    node_acc_adxl355_config_t cfg;
    node_acc_adxl355_config_default_eval(&cfg);
    cfg.log_info_on_init = true;

    node_acc_adxl355_dev_t dev = {0};
    esp_err_t ret =
        node_acc_adxl355_init(&dev, &cfg, NODE_ACC_ADXL355_RANGE_2G, NODE_ACC_ADXL355_ODR_125_HZ);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "phase5 init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    node_acc_adxl355_raw_xyz_t raw = {0};
    ret = node_acc_adxl355_read_raw_xyz(&dev, &raw);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "phase5 read_raw_xyz: %s", esp_err_to_name(ret));
        (void)node_acc_adxl355_deinit(&dev);
        return ret;
    }
    ESP_LOGI(TAG, "phase5: sample raw %ld %ld %ld (mutex + SPI ok)", (long)raw.x, (long)raw.y, (long)raw.z);

    ret = node_acc_adxl355_deinit(&dev);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "phase5 deinit: %s", esp_err_to_name(ret));
        return ret;
    }
    ESP_LOGI(TAG, "phase5: deinit ok (device zeroed)");
    return ESP_OK;
}

esp_err_t node_acc_adxl355_run_measurement_mode_test(void)
{
    spi3_init();

    node_acc_adxl355_config_t cfg;
    node_acc_adxl355_config_default_eval(&cfg);

    node_acc_adxl355_dev_t dev = {0};
    esp_err_t ret =
        node_acc_adxl355_init(&dev, &cfg, NODE_ACC_ADXL355_RANGE_2G, NODE_ACC_ADXL355_ODR_125_HZ);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "meas_mode: init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    vTaskDelay(pdMS_TO_TICKS(50));

    ret = node_acc_adxl355_set_measurement_mode(&dev, NODE_ACC_ADXL355_MEAS_ACCEL_AND_TEMP);
    if (ret != ESP_OK) {
        goto out_deinit;
    }
    {
        node_acc_adxl355_raw_xyz_t raw = {0};
        float tc = 0.0f;
        ret = node_acc_adxl355_read_raw_xyz(&dev, &raw);
        if (ret != ESP_OK) {
            goto out_deinit;
        }
        ret = node_acc_adxl355_read_temp_c(&dev, &tc);
        if (ret != ESP_OK) {
            goto out_deinit;
        }
        ESP_LOGI(TAG, "ACCEL_AND_TEMP: raw z=%ld  T=%.2f C", (long)raw.z, (double)tc);
    }

    vTaskDelay(pdMS_TO_TICKS(50));
    ret = node_acc_adxl355_set_measurement_mode(&dev, NODE_ACC_ADXL355_MEAS_ACCEL_ONLY);
    if (ret != ESP_OK) {
        goto out_deinit;
    }
    vTaskDelay(pdMS_TO_TICKS(50));
    {
        node_acc_adxl355_raw_xyz_t raw = {0};
        float tc = 0.0f;
        ret = node_acc_adxl355_read_raw_xyz(&dev, &raw);
        if (ret != ESP_OK) {
            goto out_deinit;
        }
        ret = node_acc_adxl355_read_temp_c(&dev, &tc);
        if (ret != ESP_ERR_INVALID_STATE) {
            ESP_LOGE(TAG, "ACCEL_ONLY: expected temp INVALID_STATE, got %s", esp_err_to_name(ret));
            ret = ESP_FAIL;
            goto out_deinit;
        }
        ESP_LOGI(TAG, "ACCEL_ONLY: raw z=%ld  temp read rejected OK", (long)raw.z);
    }

    vTaskDelay(pdMS_TO_TICKS(50));
    ret = node_acc_adxl355_set_measurement_mode(&dev, NODE_ACC_ADXL355_MEAS_TEMP_ONLY);
    if (ret != ESP_OK) {
        goto out_deinit;
    }
    vTaskDelay(pdMS_TO_TICKS(50));
    {
        float tc = 0.0f;
        ret = node_acc_adxl355_read_temp_c(&dev, &tc);
        if (ret != ESP_OK) {
            goto out_deinit;
        }
        node_acc_adxl355_raw_xyz_t raw = {0};
        ret = node_acc_adxl355_read_raw_xyz(&dev, &raw);
        if (ret != ESP_ERR_INVALID_STATE) {
            ESP_LOGE(TAG, "TEMP_ONLY: expected xyz INVALID_STATE, got %s", esp_err_to_name(ret));
            ret = ESP_FAIL;
            goto out_deinit;
        }
        ESP_LOGI(TAG, "TEMP_ONLY: T=%.2f C  xyz read rejected OK", (double)tc);
    }

    ret = ESP_OK;

out_deinit : {
    esp_err_t de = node_acc_adxl355_deinit(&dev);
    if (de != ESP_OK) {
        ESP_LOGW(TAG, "meas_mode deinit: %s", esp_err_to_name(de));
    }
    if (ret != ESP_OK) {
        return ret;
    }
    return de;
}
}

```

## main.cpp

```cpp
/**
 * @file AIoTNode.cpp
 * @brief Application entry: ADXL355 typical usage demo (SPI, init, WHOAMI, DRDY-synced read, periodic polling).
 */
#include "nvs_flash.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "node_spi.h"
#include "node_acc_adxl355.h"

#ifdef __cplusplus
extern "C" {
#endif

static const char *TAG = "AIoTNode";

/** Number of DRDY interrupt-synchronized samples (for demo; switches to polling afterward). */
#ifndef AIOTNODE_ADXL355_DRDY_SAMPLES
#define AIOTNODE_ADXL355_DRDY_SAMPLES 5
#endif

/** Polling print interval (ms). */
#ifndef AIOTNODE_ADXL355_POLL_MS
#define AIOTNODE_ADXL355_POLL_MS 2000
#endif

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    spi3_init();

    node_acc_adxl355_config_t cfg;
    node_acc_adxl355_config_default_eval(&cfg);
    cfg.log_info_on_init = true;

    node_acc_adxl355_dev_t dev = {};
    ret = node_acc_adxl355_init(&dev, &cfg, NODE_ACC_ADXL355_RANGE_2G, NODE_ACC_ADXL355_ODR_125_HZ);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ADXL355 init failed: %s", esp_err_to_name(ret));
        for (;;) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    node_acc_adxl355_ids_t ids = {};
    ret = node_acc_adxl355_read_ids(&dev, &ids);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "WHOAMI: AD=0x%02X MST=0x%02X PART=0x%02X REV=0x%02X", ids.devid_ad, ids.devid_mst,
                 ids.partid, ids.revid);
    }

    /* Typical streaming acquisition: DRDY edge-synchronized read of XYZ + temperature (matching eval board GPIO6 wiring). */
    ret = node_acc_adxl355_drdy_isr_install(&dev);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "DRDY-synchronized samples (%d):", AIOTNODE_ADXL355_DRDY_SAMPLES);
        for (int i = 0; i < AIOTNODE_ADXL355_DRDY_SAMPLES; i++) {
            ret = node_acc_adxl355_drdy_wait(&dev, 150);
            if (ret != ESP_OK) {
                ESP_LOGW(TAG, "drdy_wait[%d]: %s", i, esp_err_to_name(ret));
                break;
            }
            node_acc_adxl355_raw_xyz_t raw = {};
            float tc = 0.0f;
            (void)node_acc_adxl355_read_raw_xyz(&dev, &raw);
            (void)node_acc_adxl355_read_temp_c(&dev, &tc);
            ESP_LOGI(TAG, "  [%d] raw xyz: %ld, %ld, %ld  T: %.2f C", i, (long)raw.x, (long)raw.y, (long)raw.z,
                     (double)tc);
        }
        (void)node_acc_adxl355_drdy_isr_remove(&dev);
    } else {
        ESP_LOGW(TAG, "DRDY ISR not installed (%s); skip DRDY demo", esp_err_to_name(ret));
    }

    ESP_LOGI(TAG, "Polling every %d ms (STATUS + raw xyz + T). Ctrl+C monitor to stop.", AIOTNODE_ADXL355_POLL_MS);

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(AIOTNODE_ADXL355_POLL_MS));

        uint8_t status = 0;
        node_acc_adxl355_raw_xyz_t raw = {};
        float tc = 0.0f;
        ret = node_acc_adxl355_read_status(&dev, &status);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "read_status: %s", esp_err_to_name(ret));
            continue;
        }
        ret = node_acc_adxl355_read_raw_xyz(&dev, &raw);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "read_raw_xyz: %s", esp_err_to_name(ret));
            continue;
        }
        ret = node_acc_adxl355_read_temp_c(&dev, &tc);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "read_temp_c: %s", esp_err_to_name(ret));
            continue;
        }

        ESP_LOGI(TAG, "STATUS=0x%02X (DRDY=%u) | xyz raw %ld, %ld, %ld | %.2f C", (unsigned)status,
                 (unsigned)(status & NODE_ACC_ADXL355_REG_STATUS_DATA_RDY), (long)raw.x, (long)raw.y, (long)raw.z,
                 (double)tc);
    }
}

#ifdef __cplusplus
}
#endif

```
