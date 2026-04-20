# RELEASE - VERSION 1

## CODE REPOSITORY

<div class="grid cards" markdown>

-   :simple-github:{ .lg .middle } __NexNode__

    ---

    Version branches and project variants can be found under the `CODE` folder.

    [:octicons-arrow-right-24: <a href="https://github.com/Shuaiwen-Cui/NexNode.git" target="_blank"> Portal </a>](#)

</div>

## VERSION LIST (CODE)

| Version | Driver Folder | Notes |
| ------- | ------------- | ----- |
| AIoTNode-CORE | `CODE/AIoTNode-CORE/driver` | Base capability set for node control, communication, and common peripherals |
| AIoTNode-ADXL355 | `CODE/AIoTNode-ADXL355/driver` | Adds ADXL sensor-related driver modules on top of the base set |
| AIoTNode-ADXL367 | `CODE/AIoTNode-ADXL367/driver` | Adds ADXL sensor-related driver modules on top of the base set |
| AIoTNode-ESPNOW-TX | `CODE/AIoTNode-ESPNOW-TX/driver` | TX-oriented variant with the common node driver set |
| AIoTNode-ESPNOW-RX | `CODE/AIoTNode-ESPNOW-RX/driver` | RX-oriented variant with the common node driver set |
| AIoTNode-ESPNOW-TX-BATCH | `CODE/AIoTNode-ESPNOW-TX-BATCH/driver` | TX batch variant; includes additional `node_espnow` library |
| AIoTNode-ESPNOW-RX-BATCH | `CODE/AIoTNode-ESPNOW-RX-BATCH/driver` | RX batch variant; includes additional `node_espnow` library |

## DRIVER LIBRARY MAPPING

| Library | CORE | ADXL355 | ADXL367 | ESPNOW-TX | ESPNOW-RX | ESPNOW-TX-BATCH | ESPNOW-RX-BATCH |
| ------- | :--: | :-----: | :-----: | :-------: | :-------: | :-------------: | :-------------: |
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









