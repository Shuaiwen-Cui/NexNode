# RELEASE

## CODE REPOSITORY

<div class="grid cards" markdown>

-   :simple-github:{ .lg .middle } __NexNode__

    ---

    [:octicons-arrow-right-24: <a href="https://github.com/Shuaiwen-Cui/NexNode.git" target="_blank"> Portal </a>](#)

</div>

## RELEASE NOTE

| Module   | Submodule         | Function             | ZERO | CORE | IOT |
|:--------:|:----------------:|:-------------------:|:----:|:----:|:----:|
| Main     | External Interrupt| External Interrupt  | 🚫   | ✅   | ✅   |
|          | Timer            | Timer               | 🚫   | ✅   | ✅   |
|          | RTC              | Real-Time Clock     | 🚫   | ✅   | ✅   |
|          | RNG              | Random Number Generator | 🚫   | ✅   | ✅   |
|          | SPI              | Serial Peripheral Interface | 🚫   | ✅   | ✅   |
|          | IIC              | Inter-Integrated Circuit | 🚫   | ✅   | ✅   |
|          | SD Card          | SD Card & File System | 🚫   | ✅   | ✅   |
| Sensing  | -                | -                   | 🚫   | 🚫   | 🚫   |
| Comm     | WIFI             | WIFI                | 🚫   | ✅   | ✅   |
|          | MQTT             | MQTT                | 🚫   | 🚫   | ✅   |
| Execute  | LED              | LED Light           | 🚫   | ✅   | ✅   |
|          | LCD              | LCD Display         | 🚫   | ✅   | ✅   |

## Folder - CODE

### Branch - ESP32-DEFAULTS3

This is the default project created using ESP-IDF, targeting the ESP32S3 device.

### Branch - ESP32-DNESP32S3M-ZERO

This is the initial project created using ESP-IDF, with only the hello world functionality. The target device is ESP32S3M, and no other drivers or applications are integrated.

### Branch - ESP32-DNESP32S3M-ZERO-CPP

The C++ version of ESP32-DNESP32S3M-ZERO.

### Branch - ESP32-DNESP32S3-CORE

This is a project created using ESP-IDF for ALK DNESP32S3M, based on onboard device functions and does not involve external components.

### Branch - ESP32-DNESP32S3-CORE-CPP

The C++ version of ESP32-DNESP32S3-CORE.

### Branch - ESP32-DNESP32S3M-IOT

Based on ESP32-DNESP32S3M-CORE, it adds MQTT functionality.

### Branch - ESP32-DNESP32S3M-IOT-CPP

The C++ version of ESP32-DNESP32S3M-IOT.




























<!-- 
## FOLDER - CODE-ESP32-DNESP32S3M

| BRANCH | STATUS | INIT TEMPLATE ✅ | MAIN CONTROL FUNCTIONS ✅ | PERIPHERAL FUNCTIONS 📆 | MATH + DSP + AI 🏗️ |
| :---: | :---: | :---: | :---: | :---: | :---: |
| DNESP32S3M-INIT | ✅ | ✔️ | | | |
| DNESP32S3M-CORE | ✅ | ✔️ | ✔️ | | |
| DNESP32S3M-IOT | ✅ | ✔️ | ✔️ | ✔️ | |
| DNESP32S3M-AI | 🏗️ | ✔️ | ✔️ | ✔️ | ✔️ |

### VERSION TRACKING

> DNESP32S3M-INIT

Initialization template for rapid creation of new branches. Contains minimal configuration.

> DNESP32S3M-CORE

Core functionality module, providing essential controls such as UART, GPIO, timers, and interrupts. Does not include advanced features like IoT or AI.

> DNESP32S3M-IOT

IoT functionality extension module built upon DNESP32S3M-CORE, adding IoT-related capabilities. Does not include AI features.

> DNESP32S3M-AI

DNESP32S3M-CORE + AI

> DNESP32S3M-IOT-AI

DNESP32S3M-IOT + AI

## FOLDER - CODE-ESP32-DNESP32S3

📆

## FOLDER - CODE-STM32-FKH743M2-IIT6

📆 -->

