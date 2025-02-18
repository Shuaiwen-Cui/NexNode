/**
 * @file spi_sdcard.c
 * @author SHUAIWEN CUI
 * @brief This file is for SD card initialization and related functions
 * @version 1.0
 * @date 2024-11-19
 * @ref Alientek SD card driver.
 */

 #include "spi_sdcard.h"

 spi_device_handle_t MY_SD_Handle = NULL;                            /* SD card handle */
 sdmmc_card_t *card;                                                 /* SD/MMC card structure */
 const char mount_point[] = MOUNT_POINT;                             /* Mount point/root directory */
 esp_err_t ret = ESP_OK;
 esp_err_t mount_ret = ESP_OK;
 
 /**
  * @brief       Initialize SD card
  * @param       None
  * @retval      esp_err_t
  */
 esp_err_t sd_spi_init(void)
 {
     if (MY_SD_Handle != NULL)                                       /* Re-mount or reinitialize SD card */
     {
         spi_bus_remove_device(MY_SD_Handle);                        /* Remove SD card device from SPI bus */
 
         if (mount_ret == ESP_OK)
         {
             esp_vfs_fat_sdcard_unmount(mount_point, card);          /* Unmount file system */
         }
     }
 
     /* SPI driver interface configuration. SPI SD card clock is 20-25MHz */
     spi_device_interface_config_t devcfg = {
         .clock_speed_hz = 20 * 1000 * 1000,                         /* SPI clock speed */
         .mode = 0,                                                  /* SPI mode 0 */
         .spics_io_num = SD_NUM_CS,                                  /* Chip select pin */
         .queue_size = 7,                                            /* Queue size: 7 transactions */
     };
 
     /* Add SPI bus device */
     ret = spi_bus_add_device(SPI2_HOST, &devcfg, &MY_SD_Handle);
 
     /* File system mount configuration */
     esp_vfs_fat_sdmmc_mount_config_t mount_config = {
         .format_if_mount_failed = false,                            /* If mounting fails: true to reformat, false otherwise */
         .max_files = 5,                                             /* Maximum number of open files */
         .allocation_unit_size = 4 * 1024 * sizeof(uint8_t)          /* Allocation unit size of disk clusters */
     };
 
     /* SD card host configuration */
     sdmmc_host_t host = SDSPI_HOST_DEFAULT();
 
     /* SD card pin configuration */
     sdspi_device_config_t slot_config = {0};
     slot_config.host_id   = host.slot;
     slot_config.gpio_cs   = SD_NUM_CS;
     slot_config.gpio_cd   = GPIO_NUM_NC;
     slot_config.gpio_wp   = GPIO_NUM_NC;
     slot_config.gpio_int  = GPIO_NUM_NC;
 
     mount_ret = esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config, &mount_config, &card);      /* Mount file system */
     ret |= mount_ret;
 
     return ret;
 }
 
 /**
  * @brief       Get SD card usage information
  * @param       out_total_bytes: Total size
  * @param       out_free_bytes: Free size
  * @retval      None
  */
 void sd_get_fatfs_usage(size_t *out_total_bytes, size_t *out_free_bytes)
 {
     FATFS *fs;
     size_t free_clusters;
     int res = f_getfree("0:", (DWORD *)&free_clusters, &fs);
     assert(res == FR_OK);
     size_t total_sectors = (fs->n_fatent - 2) * fs->csize;
     size_t free_sectors = free_clusters * fs->csize;
 
     size_t sd_total = total_sectors / 1024;
     size_t sd_total_KB = sd_total * fs->ssize;
     size_t sd_free = free_sectors / 1024;
     size_t sd_free_KB = sd_free * fs->ssize;
 
     /* Assuming total size is less than 4GiB, which should be true for SPI flash */
     if (out_total_bytes != NULL)
     {
         *out_total_bytes = sd_total_KB;
     }
 
     if (out_free_bytes != NULL)
     {
         *out_free_bytes = sd_free_KB;
     }
 }