# SYSTEM

!!! warning "Note"
	The current version is 1.0. Its primary goal is to support scientific research and algorithm validation. There is still a significant gap before real-world field deployment and commercial use. The system will be adjusted and optimized later based on practical requirements and constraints.

## Physical Structure

We use a three-layer board design:

- Top Layer - Expansion Board: Provides various interfaces and expansion features, supporting connections to multiple sensors and actuators.
- Middle Layer - Main Control Board: Equipped with an ESP32S3 chip for core processing and control, and also provides common baseline functions such as storage and basic status feedback.
- Bottom Layer - Power Board: Provides stable power for the entire system, supports both battery power and wired power, and includes power management to optimize energy consumption and extend battery life. (This board is currently sourced directly from Waveshare's power management products, and may be customized later based on actual needs.)

## System Architecture

The system architecture is divided into the following parts:

### Main Control System

- Microcontroller chip or module - based on ESP32S3

### Sensing System

- Temperature and humidity
- Acceleration
- Sound (optional)
- Image (optional)

### Communication System

- Wi-Fi
- Bluetooth - low power
- 2.4G wireless communication
- Cellular network - low power (Cat-M1/NB-IoT) (optional)

### Actuation System

- LED/RGB LED
- Buzzer (optional)
- Display (optional)

### Power System

- Battery/wired power supply
- Power management

### Enclosure System

- Enclosure
- Mounting system design
