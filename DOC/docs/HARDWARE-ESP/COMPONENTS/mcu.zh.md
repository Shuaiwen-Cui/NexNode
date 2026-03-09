# 微控制器

出于IoT功能和边缘AI处理的需求，我们选择了ESP32S3作为主控芯片。ESP32S3是Espressif推出的一款高性能、低功耗的Wi-Fi和蓝牙双模SoC，具有强大的处理能力和丰富的外设接口，非常适合用于AIoT节点。而为了简化硬件设计，我们选择了Esspressif官方的ESP32-S3-WROOM模组作为开发基础，其内置了ESP32S3芯片，不同规格的Flash和PSRAM满足不同应用需求。

目前该模组一共有一下两代产品，其中第一代分为内置天线和无内置天线两种版本，第二代则只有内置天线版本：

| 模组    | 天线类型     | FLASH 大小     | PSRAM 大小   |
| ----------------| ---------------- | --------- | ------- |
| ESP32S3-WROOM-1/1U | PCB Antenna/IPEX Antenna | 16MB      | 8MB    |
| ESP32S3-WROOM-2 | PCB Antenna      | 32MB      | 16MB   |

![](MODULE.png)

对于SHM应用，我们主要考虑采用WROOM-1U版本，接外置天线以获得更好的无线性能和更远的通信距离；而对于室内边缘AI计算，我们则考虑基于WROOM-2版本，因为其提供更大的Flash和PSRAM容量，可以更好地支持复杂的AI模型和算法。


<div class="grid cards" markdown>

-   :fontawesome-brands-bilibili:{ .lg .middle } __ESP32S3__

    ---

    [:octicons-arrow-right-24: <a href="https://documentation.espressif.com/esp32-s3_datasheet_cn.html" target="_blank"> 技术规格书 ✅  </a>](#)

    [:octicons-arrow-right-24: <a href="https://documentation.espressif.com/esp32-s3_technical_reference_manual_cn.pdf" target="_blank"> 技术参考手册 </a>](#)

-   :fontawesome-brands-bilibili:{ .lg .middle } __ESP32S3-WROOM-1/1U__

    ---

    [:octicons-arrow-right-24: <a href="https://documentation.espressif.com/esp32-s3-wroom-1_wroom-1u_datasheet_cn.html" target="_blank"> 技术说明书 ✅  </a>](#)

    [:octicons-arrow-right-24: <a href="https://documentation.espressif.com/ESP32-S3-WROOM-1U_V1.4_Reference_Design.zip" target="_blank"> 技术参考手册 </a>](#)

-   :fontawesome-brands-bilibili:{ .lg .middle } __ESP32S3-WROOM-2__

    ---

    [:octicons-arrow-right-24: <a href="https://documentation.espressif.com/esp32-s3-wroom-2_datasheet_cn.html" target="_blank"> 技术说明书 ✅ </a>](#)

    [:octicons-arrow-right-24: <a href="https://www.espressif.com/sites/default/files/documentation/ESP32-S3-WROOM-2_Reference_Design.zip" target="_blank"> 技术参考手册 </a>](#)
    
</div>