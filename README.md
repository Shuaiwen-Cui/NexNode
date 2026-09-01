# NexNode

**Decisions where the data is.** A wireless edge-AI sensing node taken end to end — schematic and
PCB, mechanical enclosure, real-time firmware, wireless stack — so the node can sense, decide and
report on its own instead of streaming raw data somewhere else.

[![build](https://github.com/Shuaiwen-Cui/NexNode/actions/workflows/build.yml/badge.svg)](https://github.com/Shuaiwen-Cui/NexNode/actions/workflows/build.yml)
[![license](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![docs](https://img.shields.io/badge/docs-EN%20%7C%20ZH-brightgreen.svg)](https://shuaiwen-cui.github.io/NexNode/)

"Nex" comes from *Next* and *Nexus* — the next generation of IoT devices, and the link between the
physical and the digital world. NexNode is the hardware half of a pair: it runs
[TinyAuton](https://github.com/Shuaiwen-Cui/TinyAuton) for on-device intelligence and reports to
[NexHub](https://github.com/Shuaiwen-Cui/NexHub) in the cloud.

---

## The node

A two-board stack — a main control board carrying the MCU, power and radio, and a swappable
extension board carrying the sensing front end — in a 3D-printed enclosure.

| Subsystem | What is on the board |
|---|---|
| Control | ESP32-S3 (dual-core, Wi-Fi + BLE, external antenna), microSD storage |
| Perception | Dual accelerometer: an ultra-low-power sentinel (ADXL367) that watches continuously and wakes a 20-bit acquisition sensor (ADXL355) only on motion |
| Communication | Wi-Fi/MQTT uplink and ESP-NOW peer-to-peer over one 2.4 GHz radio |
| Power | 18650 cell with solar MPPT charging |
| Actuation | LED / RGB indicators, expansion headers |

Schematics, PCB layout, mechanical design, the bill of materials and the component-selection
rationale are all published in the [hardware documentation](https://shuaiwen-cui.github.io/NexNode/).

---

## Firmware

Nine ESP-IDF projects, each buildable on its own, layered as
**drivers → middleware → application**:

| Project | What it demonstrates |
|---|---|
| `AIoTNode-ZERO` | Minimal bring-up; the smallest thing that runs |
| `AIoTNode-CORE-BLANK` | The driver layer with no application on top |
| `AIoTNode-CORE` | Full driver set: I2C, SPI, timer, RTC, RNG, SD card, LED/RGB, Wi-Fi, MQTT |
| `AIoTNode-ADXL367` | Sentinel accelerometer — always-on motion detection and wake |
| `AIoTNode-ADXL355` | High-resolution acquisition path |
| `AIoTNode-ESPNOW-TX` / `-RX` | Peer-to-peer link between nodes |
| `AIoTNode-ESPNOW-TX-BATCH` / `-RX-BATCH` | RFCOM: chunked transport with three QoS levels, for payloads larger than a single ESP-NOW frame |

`RFCOM` is a small transport layer written for this node: ESP-NOW gives a 250-byte frame and no
delivery guarantee, so RFCOM fragments, reassembles and — depending on the QoS level chosen —
acknowledges and retries.

```bash
cd CODE/AIoTNode-CORE
idf.py set-target esp32s3
idf.py build flash monitor
```

Requires **ESP-IDF v6.0** — the projects' `sdkconfig` files are generated against it, and older
toolchains will fail before reaching the compiler.

---

## Documentation

A bilingual (English / 中文) site covers the hardware and the firmware in parallel: system and
software architecture, component selection, schematic, PCB layout, mechanical design, BOM, a
per-peripheral driver reference (note / code / integration for each), the wireless stack, and
release notes.

- **[shuaiwen-cui.github.io/NexNode](https://shuaiwen-cui.github.io/NexNode/)**
- Mirror: [cuishuaiwen.com:9100](http://www.cuishuaiwen.com:9100/) (personal server)

```bash
cd DOC
mkdocs serve      # http://localhost:8000
```

---

## Repository layout

```
CODE/              nine ESP-IDF projects (drivers, sensing, wireless)
DOC/               MkDocs sources for the bilingual documentation
RESOURCES-ESP32/   ESP32-S3 datasheets and board references
RESOURCES-STM32/   datasheets from the earlier STM32 prototype
DEPRECATED/        superseded boards and firmware, kept for provenance
```

The node began as an STM32 breadboard prototype before moving to the ESP32-S3 for its integrated
radio and PSRAM; that earlier work is kept under `DEPRECATED/` and `RESOURCES-STM32/`.

---

## Background

NexNode is the general-purpose branch of hardware built during my Ph.D. at Nanyang Technological
University on edge intelligence for wireless sensing. Domain-specific firmware is maintained
separately and is not open-sourced. Related publications: [my site](https://www.cuishuaiwen.com).

---

## License

MIT — see [LICENSE](LICENSE).
