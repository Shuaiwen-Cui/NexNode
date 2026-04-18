/**
 * @file main.cpp
 * @author SHUAIWEN CUI (SHUAIWEN001@e.ntu.edu.sg)
 * @brief I2C Test Application
 * @version 1.0
 * @date 2025-10-22
 * 
 * @copyright Copyright (c) 2024
 * 
 */

 #include "node_i2c.h"

 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /**
  * @brief Entry point of the program
  * @param None
  * @retval None
  */
 void app_main(void) {
     // 1. initialize the i2c bus
     esp_err_t ret = i2c_bus_init();
     if (ret != ESP_OK) return;
 
     // 2. add i2c device to bus
     i2c_master_dev_handle_t dev_handle;
     ret = i2c_add_device(0x68, 400000, &dev_handle);
     if (ret != ESP_OK) return;
 
     // 3. write data to i2c device
     uint8_t data[] = {0x75};
     i2c_write_data(dev_handle, data, 1, 1000);
 
     uint8_t response[1];
     i2c_read_data(dev_handle, response, 1, 1000);
 
     // 4. clean up resources
     i2c_remove_device(dev_handle);
     i2c_bus_deinit();
 }
 
 #ifdef __cplusplus
 }
 #endif