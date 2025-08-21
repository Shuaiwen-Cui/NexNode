# 发布

## 代码仓库

<div class="grid cards" markdown>

-   :simple-github:{ .lg .middle } __NexNode__

    ---

    [:octicons-arrow-right-24: <a href="https://github.com/Shuaiwen-Cui/NexNode.git" target="_blank"> Portal </a>](#)

</div>

## 发布说明

| 模块   | 子模块         | 功能             | ZERO | CORE | IOT |
|:------:|:--------------:|:----------------:|:----:|:----:|:----:|
| 主控   | 外部中断       | 外部中断         | 🚫   | ✅   | ✅   |
|        | 定时器         | 定时器           | 🚫   | ✅   | ✅   |
|        | 实时时钟       | 实时时钟         | 🚫   | ✅   | ✅   |
|        | 随机数生成器   | 随机数生成器     | 🚫   | ✅   | ✅   |
|        | SPI            | 串行周边总线     | 🚫   | ✅   | ✅   |
|        | IIC            | 内部集成电路总线 | 🚫   | ✅   | ✅   |
|        | SD卡           | SD卡和文件系统   | 🚫   | ✅   | ✅   |
| 感知   | -              | -                | 🚫   | 🚫   | 🚫   |
| 通信   | WIFI           | WIFI             | 🚫   | ✅   | ✅   |
|        | MQTT           | MQTT             | 🚫   | 🚫   | ✅   |
| 执行   | LED            | LED灯            | 🚫   | ✅   | ✅   |
|        | LCD            | LCD显示器        | 🚫   | ✅   | ✅   |

## 文件夹 - CODE

### 分支 - ESP32-DEFAULTS3

这是使用ESP-IDF创建的默认项目，目标设备为ESP32S3

### 分支 - ESP32-DNESP32S3M-ZERO

这是使用ESP-IDF创建的初始项目，只有hello world功能，目标设备为ESP32S3M，没有集成其他驱动和应用。

### 分支 - ESP32-DNESP32S3M-ZERO-CPP

ESP32-DNESP32S3M-ZERO的C++版本。

### 分支 - ESP32-DNESP32S3M-CORE

这是使用ESP-IDF为ALK DNESP32S3M创建的项目，基于板载设备的功能，不涉及外部组件。

### 分支 - ESP32-DNESP32S3M-CORE-CPP

这是ESP32-DNESP32S3M-CORE的C++版本。

### 分支 - ESP32-DNESP32S3M-IOT

基于ESP32-DNESP32S3M-CORE，增加了MQTT功能。

### 分支 - ESP32-DNESP32S3M-IOT-CPP

ESP32-DNESP32S3M-IOT的C++版本。









<!-- ## 文件夹 - CODE-ESP32-DNESP32S3M

| 分支 | 状态 | 初始化模板 ✅ | 主控制功能 ✅ | 外设功能 📆 | 数学 + DSP + AI 🏗️|
| :---: | :---: | :---: | :---: | :---: | :---: |
| DNESP32S3M-INIT | ✅ | ✔️ | | | |
| DNESP32S3M-CORE | ✅ | ✔️ | ✔️ | | |
| DNESP32S3M-IOT | ✅ | ✔️ | ✔️ | ✔️ | |
| DNESP32S3M-AI | 🏗️ | ✔️ | ✔️ | ✔️ | ✔️ |

### 版本跟踪

> DNESP32S3M-INIT

初始化模板，无实际功能，可用于快速创建新的分支。

> DNESP32S3M-CORE

主控制功能，包括基本的控制功能，如串口、GPIO、定时器、中断等。不包含IOT、AI等高级功能。

> DNESP32S3M-IOT

DNESP32S3M-CORE + WIFI + IOT + Sensing

> DNESP32S3M-AI

DNESP32S3M-CORE + AI

> DNESP32S3M-IOT-AI

DNESP32S3M-IOT + AI

## 文件夹 - CODE-ESP32-DNESP32S3

📆

## 文件夹 - CODE-STM32-FKH743M2-IIT6

📆
 -->
