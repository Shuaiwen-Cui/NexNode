# SYSTEM

!!! TIP "Note"
    Note that currently the hardware and software design are being developed separately and simultaneously, and the software part is currently being developed based on existing ESP32 development boards on the market, which will be adapted to our designed hardware platform in the future.

!!! warning "Note"
	The current version is 1.0. Its primary goal is to support scientific research and algorithm validation. There is still a significant gap before real-world field deployment and commercial use. The system will be adjusted and optimized later based on practical requirements and constraints.

## Physical Structure

We use a three-board design, which are:

- Extension Board: Provides various interfaces and expansion functions, supporting the connection of multiple sensors and actuators.
- Main Control Board: Equipped with the ESP32S3 chip, responsible for the core processing and control of the system, while also providing common basic functions such as storage and basic status feedback.
- Power Board: Responsible for providing stable power supply to the entire system, supporting both battery and wired power supply, and equipped with power management functions to optimize energy consumption and extend battery life. (This board directly purchases third-party power management board from Waveshare Electronics, and may be custom-designed later based on actual needs.)

## System Architecture

The system architecture is divided into the following parts:

### Main Control System

- Microcontroller chip or module - based on ESP32S3

### Sensing System

- Temperature
- Acceleration
<!-- - Sound (optional)
- Image (optional) -->

### Communication System

- Wi-Fi
- Bluetooth - low power
- 2.4G wireless communication
<!-- - Cellular network - low power (Cat-M1/NB-IoT) (optional) -->

### Actuation System

- LED/RGB LED
<!-- - Buzzer (optional)
- Display (optional) -->

### Power System

- Battery/wired power supply
- Power management

### Enclosure System

- Enclosure
- Mounting system design
