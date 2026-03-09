# MICROCONTROLLER

Due to the requirements of IoT functionality and edge AI processing, we chose the ESP32S3 as the main control chip. The ESP32S3 is a high-performance, low-power Wi-Fi and Bluetooth dual-mode SoC launched by Espressif, with powerful processing capabilities and rich peripheral interfaces, making it very suitable for use in AIoT nodes. To simplify hardware design, we chose the official ESP32-S3-WROOM module from Esspressif as the development basis, which has a built-in ESP32S3 chip, and different specifications of Flash and PSRAM to meet different application needs.

Currently, there are two generations of this module, with the first generation divided into versions with and without built-in antennas, while the second generation only has a version with a built-in antenna:

| Module | Antenna Type | FLASH Size | PSRAM Size |
| ---------------- | ---------------- | --------- | ------- |
| ESP32S3-WROOM-1/1U | PCB Antenna / IPEX Antenna | 16MB | 8MB |
| ESP32S3-WROOM-2 | PCB Antenna | 32MB | 16MB |

![](MODULE.png)

For SHM applications, we mainly consider using the WROOM-1U version, which connects to an external antenna to achieve better wireless performance and longer communication distance; for indoor edge AI computing, we consider using the WROOM-2 version, as it provides larger Flash and PSRAM capacity, which can better support complex AI models and algorithms.

<div class="grid cards" markdown>

-   :fontawesome-brands-bilibili:{ .lg .middle } __ESP32S3__

    ---

    [:octicons-arrow-right-24: <a href="https://documentation.espressif.com/esp32-s3_datasheet_en.html" target="_blank"> Data Sheet ✅  </a>](#)

    [:octicons-arrow-right-24: <a href="https://documentation.espressif.com/esp32-s3_technical_reference_manual_en.pdf" target="_blank"> Technical Reference Manual </a>](#)

-   :fontawesome-brands-bilibili:{ .lg .middle } __ESP32S3-WROOM-1/1U__

    ---

    [:octicons-arrow-right-24: <a href="https://documentation.espressif.com/esp32-s3-wroom-1_wroom-1u_datasheet_en.html" target="_blank"> Data Sheet ✅  </a>](#)

    [:octicons-arrow-right-24: <a href="https://documentation.espressif.com/ESP32-S3-WROOM-1U_V1.4_Reference_Design_0.zip" target="_blank"> Technical Reference Manual </a>](#)

-   :fontawesome-brands-bilibili:{ .lg .middle } __ESP32S3-WROOM-2__

    ---

    [:octicons-arrow-right-24: <a href="https://documentation.espressif.com/esp32-s3-wroom-2_datasheet_en.html" target="_blank"> Data Sheet ✅ </a>](#)

    [:octicons-arrow-right-24: <a href="https://documentation.espressif.com/ESP32-S3-WROOM-2_Reference_Design.zip" target="_blank"> Technical Reference Manual </a>](#)

</div>