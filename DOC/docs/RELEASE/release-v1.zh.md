# 发布 - 版本 1

## 代码仓库

<div class="grid cards" markdown>

-   :simple-github:{ .lg .middle } __NexNode__

    ---

    各版本分支和项目变体可在仓库的 `CODE` 目录下查看。

    [:octicons-arrow-right-24: <a href="https://github.com/Shuaiwen-Cui/NexNode.git" target="_blank"> 入口 </a>](#)

</div>

## 版本列表（CODE）

| 版本 | Driver 目录 | 说明 |
| ---- | ----------- | ---- |
| AIoTNode-CORE | `CODE/AIoTNode-CORE/driver` | 基础能力集合，覆盖控制、通信和常用外设 |
| AIoTNode-ADXL355 | `CODE/AIoTNode-ADXL355/driver` | 在基础能力上增加 ADXL 传感器相关驱动模块 |
| AIoTNode-ADXL367 | `CODE/AIoTNode-ADXL367/driver` | 在基础能力上增加 ADXL 传感器相关驱动模块 |
| AIoTNode-ESPNOW-TX | `CODE/AIoTNode-ESPNOW-TX/driver` | 面向 TX 的变体，包含通用 node 驱动集合 |
| AIoTNode-ESPNOW-RX | `CODE/AIoTNode-ESPNOW-RX/driver` | 面向 RX 的变体，包含通用 node 驱动集合 |
| AIoTNode-ESPNOW-TX-BATCH | `CODE/AIoTNode-ESPNOW-TX-BATCH/driver` | TX 批处理变体，额外包含 `node_espnow` 库 |
| AIoTNode-ESPNOW-RX-BATCH | `CODE/AIoTNode-ESPNOW-RX-BATCH/driver` | RX 批处理变体，额外包含 `node_espnow` 库 |

## Driver 库对应关系

| 库名 | CORE | ADXL355 | ADXL367 | ESPNOW-TX | ESPNOW-RX | ESPNOW-TX-BATCH | ESPNOW-RX-BATCH |
| ---- | :--: | :-----: | :-----: | :-------: | :-------: | :-------------: | :-------------: |
| `node_led` | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| `espressif__led_strip` | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| `node_rgb` | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| `node_exit` | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| `node_timer` | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| `node_rtc` | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| `node_rng` | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| `node_i2c` | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| `node_spi` | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| `node_sdcard` | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| `node_wifi` | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| `mqtt` | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| `node_mqtt` | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| `node_acc_adxl367` | 🚫 | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| `node_acc_adxl355` | 🚫 | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| `node_espnow` | 🚫 | 🚫 | 🚫 | 🚫 | 🚫 | ✅ | ✅ |