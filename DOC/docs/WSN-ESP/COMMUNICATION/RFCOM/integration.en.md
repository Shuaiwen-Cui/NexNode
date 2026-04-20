# INTEGRATION

## Introduction

This section explains how to integrate the RFCOM communication component (`node_espnow`) into your ESP-IDF project, following the implementation style used in:

- `CODE/AIoTNode-ESPNOW-TX-BATCH`
- `CODE/AIoTNode-ESPNOW-RX-BATCH`

## Create a New Component

!!! warning
    Before creating components in `driver`, ensure your project-level `CMakeLists.txt` includes the `driver` path in `EXTRA_COMPONENT_DIRS`.

In VSCode terminal:

```bash
get_idf
idf.py -C driver create-component node_espnow
```

Then create:

- `driver/node_espnow/include/node_espnow.h`
- `driver/node_espnow/node_espnow.c`
- `driver/node_espnow/CMakeLists.txt`

## Replace Component Code

Replace the generated files with the RFCOM implementation from the reference projects:

- Header/API: `driver/node_espnow/include/node_espnow.h`
- Core logic: `driver/node_espnow/node_espnow.c`
- Build file: `driver/node_espnow/CMakeLists.txt`

Required component dependencies:

- `esp_wifi`
- `esp_event`
- `esp_netif`
- `esp_timer`

## Project-Level Integration

In `main/AIoTNode.cpp`:

1. Include `node_espnow.h`.
2. Prepare `node_espnow_config_t` from defaults.
3. Tune communication parameters.
4. Call `node_espnow_init(...)`.
5. Register RX callback via `node_espnow_set_rx_batch_cb(...)`.
6. Use `node_espnow_send_to(...)` for batch transfer.

Minimum TX/RX startup skeleton:

```cpp
node_espnow_config_t cfg;
node_espnow_default_config(&cfg);
cfg.channel = 1;
cfg.qos_default = NODE_ESPNOW_QOS1;
cfg.chunk_payload_bytes = 160;
cfg.tx_window_size = 1;
cfg.ack_timeout_ms = 1200;
cfg.max_retries = 8;
cfg.session_timeout_ms = 30000;
cfg.max_batch_bytes = 16384;

node_espnow_handlers_t handlers = {
    .tx_result_cb = tx_result_cb_or_null,
    .user_ctx = NULL,
};

ESP_ERROR_CHECK(node_espnow_init(&cfg, &handlers));
ESP_ERROR_CHECK(node_espnow_set_rx_batch_cb(rx_batch_cb, NULL));
```

## TX/RX Deployment Pattern

Use two boards:

- Board A: TX build (`AIoTNode-ESPNOW-TX-BATCH`)
- Board B: RX build (`AIoTNode-ESPNOW-RX-BATCH`)

Behavior:

1. RX waits and responds to discovery request.
2. TX broadcasts discovery and locks peer MAC from response.
3. TX enters staged payload transmission.
4. RX validates each received case and prints result.
5. Both iterate QoS rounds.

## Compile and Flash

For each project terminal:

```bash
idf.py build flash monitor
```

## Verification Checklist

1. Both sides print local MAC and start logs.
2. Discovery request/response appears in logs.
3. TX shows queued/complete transfer logs.
4. RX shows per-case PASS/FAIL validation logs.
5. Final matrix summary is printed for `QOS0`, `QOS1`, `QOS2`.

## Common Tuning Notes

- If packet loss is high:
  - increase `ack_timeout_ms`
  - increase `max_retries`
  - reduce `chunk_payload_bytes`
  - reduce `tx_window_size`
- If memory pressure appears:
  - reduce `max_batch_bytes`
  - reduce RX concurrent sessions
- Keep TX and RX on the same Wi-Fi channel (`cfg.channel`).
