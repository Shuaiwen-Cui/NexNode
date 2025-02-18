/**
 * @file spi_sdcard.h
 * @author SHUAIWEN CUI (SHUAIWEN001@e.ntu.edu.sg)
 * @brief This file is for SD card initialization and related functions
 * @version 1.0
 * @date 2024-11-19
 * @ref Alientek SD card driver.
 * @copyright Copyright (c) 2024
 * 
 */

 #ifndef __SPI_SDCARD_H__
 #define __SPI_SDCARD_H__
 
 #include <unistd.h>
 #include "esp_vfs_fat.h"
 #include "driver/sdspi_host.h"
 #include "driver/spi_common.h"
 #include "sdmmc_cmd.h"
 #include "driver/sdmmc_host.h"
 #include "spi.h"
 
 
 /* Pin definitions */
 #define SD_NUM_CS       GPIO_NUM_2
 #define MOUNT_POINT     "/0:"
 
 /* Function declarations */
 esp_err_t sd_spi_init(void);                                                /* Initialize SD card */
 void sd_get_fatfs_usage(size_t *out_total_bytes, size_t *out_free_bytes);   /* Get SD card usage information */
 
 #endif