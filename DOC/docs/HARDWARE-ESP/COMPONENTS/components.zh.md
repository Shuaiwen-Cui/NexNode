# 元件选型

## 主控系统

| 模组    | 天线类型     | FLASH 大小     | PSRAM 大小   |
| ----------------| ---------------- | --------- | ------- |
| ESP32S3-WROOM-1/1U | PCB Antenna/IPEX Antenna | 16MB      | 8MB    |
| ESP32S3-WROOM-2 | PCB Antenna      | 32MB      | 16MB   |

![](MODULE.png)

!!! tip
    以上两个型号初步判断为引脚兼容，后续需要进一步确认。

<div class="grid cards" markdown>

-   :fontawesome-brands-bilibili:{ .lg .middle } __ESP32S3__

    ---

    [:octicons-arrow-right-24: <a href="https://documentation.espressif.com/esp32-s3_datasheet_cn.html" target="_blank"> 技术规格书 </a>](#)

    [:octicons-arrow-right-24: <a href="https://documentation.espressif.com/esp32-s3_technical_reference_manual_cn.pdf" target="_blank"> 技术参考手册 </a>](#)

-   :fontawesome-brands-bilibili:{ .lg .middle } __ESP32S3-WROOM-1/1U__

    ---

    [:octicons-arrow-right-24: <a href="https://documentation.espressif.com/esp32-s3-wroom-1_wroom-1u_datasheet_cn.html" target="_blank"> 技术说明书 </a>](#)

    [:octicons-arrow-right-24: <a href="https://documentation.espressif.com/ESP32-S3-WROOM-1U_V1.4_Reference_Design.zip" target="_blank"> 技术参考手册 </a>](#)

-   :fontawesome-brands-bilibili:{ .lg .middle } __ESP32S3-WROOM-2__

    ---

    [:octicons-arrow-right-24: <a href="https://documentation.espressif.com/esp32-s3-wroom-2_datasheet_cn.html" target="_blank"> 技术说明书 </a>](#)

    [:octicons-arrow-right-24: <a href="https://www.espressif.com/sites/default/files/documentation/ESP32-S3-WROOM-2_Reference_Design.zip" target="_blank"> 技术参考手册 </a>](#)
    
</div>

## 感知系统

### SHT45 — 数字温湿度传感器

![](SHT45.png)

| 项目 | 内容 |
|------|------|
| **厂家** | Sensirion（盛思锐） |
| **类型** | 数字温湿度传感器 |
| **接口** | I²C（最大 1 MHz） |
| **测量范围** | 温度：−40°C 至 +125°C；湿度：0%RH 至 100%RH |
| **精度** | 温度 ±0.1°C（典型），湿度 ±1.0%RH（典型） |
| **分辨率** | 温度 0.01°C，湿度 0.01%RH |
| **响应时间** | 湿度约 8 秒（τ63%） |
| **供电电压** | 1.08V ~ 3.6V |
| **功耗** | 典型 < 0.4 µA（低功耗模式） |
| **封装尺寸** | 1.5 × 1.5 × 0.5 mm |
| **特点** | 集成加热器、工厂校准、高精度、高稳定性、适合长期环境监测 |


### ADXL362 — 超低功耗三轴加速度计

![](ADXL362.png)

| 项目 | 内容 |
|------|------|
| **厂家** | Analog Devices（ADI） |
| **类型** | MEMS 三轴加速度计 |
| **接口** | SPI |
| **测量范围** | ±2g、±4g、±8g 可选 |
| **分辨率** | 12 位（典型） |
| **噪声密度** | 175 µg/√Hz（典型） |
| **输出数据率** | 12.5 Hz ~ 400 Hz |
| **供电电压** | 1.6V ~ 3.5V |
| **功耗** | 1.8 µA（唤醒模式），270 nA（待机模式） |
| **封装尺寸** | 3 × 3 × 1.06 mm（LGA） |
| **特点** | 超低功耗、内置 FIFO、内置唤醒/活动检测，适合电池供电节点用于触发或监测睡眠唤醒事件 |

### ADXL355 — 低噪声高精度三轴加速度计

![](ADXL355.png)

| 项目 | 内容 |
|------|------|
| **厂家** | Analog Devices（ADI） |
| **类型** | MEMS 三轴加速度计 |
| **接口** | SPI / I²C |
| **测量范围** | ±2g、±4g、±8g |
| **分辨率** | 20 位（有效分辨率约 19 位） |
| **噪声密度** | 25 µg/√Hz（典型） |
| **输出数据率** | 4 Hz ~ 4 kHz |
| **供电电压** | 2.25V ~ 3.6V |
| **功耗** | 200 µA（运行模式），21 µA（待机模式） |
| **封装尺寸** | 3 × 3 × 1.25 mm（LGA） |
| **特点** | 超低噪声、高分辨率、高稳定性、内置温度传感器与自校准机制，适合结构振动与地震监测等高精度场景 |

## 通讯系统

| 传输方式       | 型号         | 备注                     |
| ---------------- | ---------------- | ------------------------ |
| WIFI             | 集成在主控芯片中 | -                        |
| 蓝牙             | 集成在主控芯片中 | 低功耗                   |
| ESP-NOW          | 集成在主控芯片中  |                   |
| 蜂窝移动网络     | Quectel BG95   | LTE Cat.M1/NB-IoT, UART接口 |

![](NRF24L01.png)

![](BG95.png)

<!-- ## 执行系统

| 设备类型       | 型号         | 备注                     |
| ---------------- | ---------------- | ------------------------ |
| LED              | -                | -                        |
| RGBLED           | WS2812B         | -                        |
| 蜂鸣器           | -                | -                        |
| 显示屏           | -                | -                        |

## 电源系统

| 设备类型       | 型号         | 备注                     |
| ---------------- | ---------------- | ------------------------ |
| 充电管理芯片   | -         | -                        |
| 电源开关       | -                | -                        |
| 电池           | -                | -                        |

!!! warning "注意"
    以上内容均为初步选型，后续可能会根据实际需求进行调整。目前潜在问题是I2C和SPI设备过多，可能需要考虑使用多路复用器。 -->