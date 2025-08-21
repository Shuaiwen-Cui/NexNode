cmake_minimum_required(VERSION 3.5)

idf_component_register(SRCS "pl_spi_base.cpp" "pl_spi_controller.cpp" INCLUDE_DIRS "include" REQUIRES "esp_driver_spi" "pl_common")