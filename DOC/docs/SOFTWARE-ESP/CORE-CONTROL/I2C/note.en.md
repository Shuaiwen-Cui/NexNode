# I2C

## I2C Introduction

!!! info "I2C"

    I2C, short for Inter-Integrated Circuit, is a serial bus developed by Philips for connecting microcontrollers and external devices. Devices on the I2C bus can communicate via two lines (SDA and SCL). Devices on the I2C bus are divided into master devices and slave devices. The master device is responsible for initiating communication, while the slave device passively receives instructions from the master device. Devices on the I2C bus can be addressed by address, enabling multiple devices to communicate on the same bus.

## Modular Design

To better manage and utilize I2C functionality in this project, we have adopted a modular design approach. The I2C functionality is encapsulated in an independent module that provides a set of simple interface functions for users to perform initialization, data reading and writing operations. The I2C bus and device configurations are placed in different components, so that for various devices, the same I2C functions are used, thereby improving code reusability and maintainability.