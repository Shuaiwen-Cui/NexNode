# COMPONENTS SELECTION

## MAIN CONTROL

### CORE MODULE ESP32S3

For simplicity, the main control of this project uses the ESP32S3 module instead of a bare chip. The candidate modules are:

| Module Model    | Antenna Type     | FLASH     | PSRAM   |
| ----------------| ---------------- | --------- | ------- |
| ESP32S3-WROOM-1 | PCB Antenna/IPEX Antenna | 16MB      | 8MB    |
| ESP32S3-WROOM-2 | PCB Antenna      | 32MB      | 16MB   |

<!-- A simple ESP32S3 is not enough to meet the main control requirements of the project, and some peripheral components must be used in conjunction:
| Component       | Function         | Note                     |
| ---------------- | ---------------- | ------------------------ |
| 32.768KHz Crystal Oscillator | Real-time Clock | Optional                   |
| 3.3V LDO         | Voltage Regulation | Output current at least 500
mA      |
| 5V Boost         | Voltage Regulation | Output current at least 1A        |
| Power Management Chip | Battery Charging and Discharging | Optional                   |
| Button          | Reset, Programming, User | Optional                   |
| Indicator Light | Status Indication | Optional                   |
| TF Card Slot    | Storage Expansion | Optional                   | -->

### RTC

### LDO/DC-DC

### TF Card Slot

## SENSING

This project currently mainly considers temperature and humidity sensors and accelerometers in terms of sensing.

### Temperature and Humidity Sensor
- Temperature and Humidity Sensor: SHT45

### Accelerometer

- Triggered Accelerometer: ADXL362
- High-Precision Accelerometer: ADXL355

## COMMUNICATION

### WIFI

Integrated in the main control

### Bluetooth

Integrated in the main control

### Local Wireless Communication

- 2.4G Wireless Transceiver: NRF24L01+

### Cellular Network

- LTE Cat.M1/NB-IoT: Quectel BG95

## ACTUATOR

- WS2812 LED

## POWER