# EXTENDED FUNCTION DEVELOPMENT

## Introduction

!!! info
    Extended functions mainly refer to the development of functions that depend on additional hardware modules beyond the core board, primarily involving the drivers and applications of various sensors and actuators.

## Branches

<!-- There are mainly two branches: C and C++. The primary difference lies in the main programming language used, while the driver parts are mostly written in C. Each version is derived from the corresponding core board branch.

- AIoTNode-C-CORE -> AIoTNode-C-MORE
- AIoTNode-CPP-CORE -> AIoTNode-CPP-MORE -->

## Function List

| Function Category | Function               | Module/Chip | Communication Method | Remarks                          |
| ----------------- | ---------------------- | ----------- | -------------------- | -------------------------------- |
| Sensing           | Accelerometer          | ADXL367     | I2C                 | 3-axis acceleration measurement |
| Sensing           | Accelerometer          | ADXL355     | SPI                 | 3-axis acceleration measurement |

<!-- | Control           | Hardware Real-Time Clock | DS3231      | I2C                 | High-precision RTC module        |
| Sensing           | Temperature and Humidity Sensor | DHT11       | Single Wire          | Low accuracy, basic temperature and humidity measurement | -->

