# RFCOM Node-to-Node Communication (ESP-NOW Batch)

## Introduction

This section documents the RFCOM communication module used for node-to-node data exchange in this repository.

Implementation reference:

- TX project: `CODE/AIoTNode-ESPNOW-TX-BATCH`
- RX project: `CODE/AIoTNode-ESPNOW-RX-BATCH`
- Core library: `driver/node_espnow`

The communication engine is built on ESP-NOW and wraps large logical payloads into a framed, chunked, and reassembled batch flow.

## Architecture

The design has two layers:

1. `node_espnow` library layer  
   Provides framing, chunking, ACK handling, retries, session state, and callback delivery.
2. Application layer (`main/AIoTNode.cpp`)  
   Builds test/business payloads, starts library with selected QoS profile, and processes TX/RX results.

Conceptual flow:

```plaintext
Application payload
   -> START frame
   -> DATA frame 0..N-1
   -> END frame
   -> RX reassembly + ACKs
   -> complete-batch callback
```

## Protocol Model Used by `node_espnow`

`node_espnow` uses four frame types:

- `NODE_FRAME_START`
- `NODE_FRAME_DATA`
- `NODE_FRAME_END`
- `NODE_FRAME_ACK`

Each frame carries a compact header with fields such as:

- `transfer_id`
- `total_len`
- `chunk_idx`
- `chunk_total`
- `payload_len`
- `flags`

This gives deterministic batch transfer for medium/large payloads beyond single-frame use.

## QoS Modes

The library exposes three QoS modes:

- `NODE_ESPNOW_QOS0`  
  Fire-and-forget style. Session completes after data/end are sent.
- `NODE_ESPNOW_QOS1`  
  ACK-based reliability for chunk progress and completion.
- `NODE_ESPNOW_QOS2`  
  Adds duplicate-completion suppression cache on RX side for repeated START retransmissions.

## Key Runtime Behaviors

1. TX side
   - Splits one payload into chunks by `chunk_payload_bytes`
   - Uses `tx_window_size` to control in-flight chunks
   - Retries when ACK timeout is reached (`ack_timeout_ms`, `max_retries`)
2. RX side
   - Maintains reassembly sessions indexed by peer + `transfer_id`
   - Uses bitmap to track received chunks
   - Delivers complete payload by callback when END + all chunks are present
3. Session safety
   - Session timeout cleanup (`session_timeout_ms`)
   - Memory cleanup for TX/RX buffers

## Application-Level Pattern in TX/RX Projects

Both `AIoTNode.cpp` applications use the same pattern:

1. Initialize system/NVS/hardware.
2. Build `node_espnow_config_t` from defaults.
3. Tune chunk/window/timeout/retry/max-batch.
4. Call `node_espnow_init(...)`.
5. Register RX complete-batch callback.
6. Run periodic communication logic.

The TX project additionally implements:

- Discovery request broadcast, then peer MAC lock.
- Multi-stage payload matrix (array, medium blob, string, struct, raw bytes, large vector).
- QoS round-robin verification (`QOS0 -> QOS1 -> QOS2`).

The RX project additionally implements:

- Discovery response.
- Per-case payload validation and summary matrix.

## Typical Use in Node Communication

This RFCOM module is suitable for:

- Structured telemetry packets between edge nodes
- Batch transfer of sampled arrays
- Reliable transfer of binary payloads under noisy link conditions
- Multi-stage protocol tests during bring-up

## Dependencies

Main dependencies in this module:

- `esp_wifi`
- `esp_now`
- `esp_event`
- `esp_netif`
- `esp_timer`
- FreeRTOS queue/task
