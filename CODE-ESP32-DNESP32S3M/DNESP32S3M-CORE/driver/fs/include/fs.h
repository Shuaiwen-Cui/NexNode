/**
 * @file spiffs.h
 * @author SHUAIWEN CUI (SHUAIWEN001@e.ntu.edu.sg)
 * @brief This file wraps up some frequently used functions for file system operations.
 * @version 1.0
 * @date 2024-11-19
 * 
 * @copyright Copyright (c) 2024
 * 
 */

/* Dependencies */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "led.h"
#include "spi.h"
#include "lcd.h"
// #include "esp_spiffs.h"
#include "esp_vfs.h"
#include "esp_spiffs.h"
#include <sys/stat.h>
#include "esp_log.h"

/* Dependencies */
// Basic
#include "esp_system.h"
#include "esp_chip_info.h"
// #include "esp_psram.h"
// #include "esp_flash.h"
// #include "nvs_flash.h"
#include "spi_sdcard.h"


/* Definitions */
#define DEFAULT_FD_NUM          5
#define DEFAULT_MOUNT_POINT     "/spiffs"
#define WRITE_DATA              "ESP32-S3 CSW\r\n"

/**
 * @brief       SPIFFS initialization
 * @param       partition_label: The name of the partition in the partition table
 * @param       mount_point: The path prefix associated with the file system
 * @param       max_files: Maximum number of files that can be opened simultaneously
 * @retval      None
 */
esp_err_t spiffs_init(char *partition_label, char *mount_point, size_t max_files);

/**
 * @brief       Deinitialize SPIFFS
 * @param       partition_label: Partition label
 * @retval      None
 */
esp_err_t spiffs_deinit(char *partition_label);

/**
 * @brief       Test SPIFFS functionality
 * @param       None
 * @retval      None
 */
void spiffs_test(void);