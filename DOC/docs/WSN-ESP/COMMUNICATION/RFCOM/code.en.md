# RFCOM CODE

## Component Architecture

```plaintext
- driver
    - node_espnow
        - include
            - node_espnow.h
        - node_espnow.c
        - CMakeLists.txt
- main
    - AIoTNode.cpp
```

Referenced projects:

- TX: `CODE/AIoTNode-ESPNOW-TX-BATCH`
- RX: `CODE/AIoTNode-ESPNOW-RX-BATCH`

## driver/node_espnow/CMakeLists.txt

```cmake
idf_component_register(SRCS "node_espnow.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_wifi esp_event esp_netif esp_timer)
```

## node_espnow.h

```c
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    NODE_ESPNOW_QOS0 = 0,
    NODE_ESPNOW_QOS1 = 1,
    NODE_ESPNOW_QOS2 = 2,
} node_espnow_qos_t;

typedef struct
{
    uint8_t channel;
    uint16_t chunk_payload_bytes;
    uint8_t tx_window_size;
    uint32_t ack_timeout_ms;
    uint8_t max_retries;
    uint32_t session_timeout_ms;
    uint32_t max_batch_bytes;
    node_espnow_qos_t qos_default;
    uint16_t worker_stack_size;
    uint8_t worker_priority;
    uint8_t event_queue_len;
} node_espnow_config_t;

typedef struct
{
    uint32_t transfer_id;
    uint32_t total_bytes;
    uint16_t total_chunks;
    uint16_t acked_chunks;
    uint16_t tx_attempts;
    uint16_t tx_success_frames;
    uint16_t tx_failed_frames;
    uint32_t duration_ms;
    esp_err_t fail_reason;
} node_espnow_tx_result_t;

typedef struct
{
    uint32_t transfer_id;
    uint32_t total_bytes;
    uint16_t total_chunks;
    uint16_t received_chunks;
    uint32_t duration_ms;
    const uint8_t *payload;
    size_t payload_len;
} node_espnow_rx_batch_t;

typedef void (*node_espnow_tx_result_cb_t)(const uint8_t peer_mac[6],
                                           const node_espnow_tx_result_t *result,
                                           void *user_ctx);

typedef void (*node_espnow_rx_batch_cb_t)(const uint8_t peer_mac[6],
                                          const node_espnow_rx_batch_t *batch,
                                          void *user_ctx);

typedef struct
{
    node_espnow_tx_result_cb_t tx_result_cb;
    void *user_ctx;
} node_espnow_handlers_t;

/**
 * @brief Fill a config with safe defaults.
 */
void node_espnow_default_config(node_espnow_config_t *config);

/**
 * @brief Initialize ESPNOW TX engine with config and callbacks.
 */
esp_err_t node_espnow_init(const node_espnow_config_t *config,
                           const node_espnow_handlers_t *handlers);

/**
 * @brief Send one batch byte-stream to a target peer.
 *
 * @param payload Pointer to any memory block (array/string/struct/serialized bytes).
 *                The pointer value itself is NOT transmitted, only payload bytes are sent.
 * @note The library copies payload internally, caller can free its buffer after return.
 */
esp_err_t node_espnow_send_to(const uint8_t peer_mac[6], const void *payload, size_t payload_len);

/**
 * @brief Register RX complete-batch callback.
 *
 * @note Callback runs in node_espnow worker task context.
 */
esp_err_t node_espnow_set_rx_batch_cb(node_espnow_rx_batch_cb_t cb, void *user_ctx);

/**
 * @brief Get current default qos.
 */
node_espnow_qos_t node_espnow_get_qos(void);

/**
 * @brief Deinitialize driver and release resources.
 */
void node_espnow_deinit(void);

#ifdef __cplusplus
}
#endif

```

## node_espnow.c

```c
#include "node_espnow.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_now.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#define NODE_ESPNOW_MAGIC 0x4E45
#define NODE_ESPNOW_VERSION 1
#define NODE_ESPNOW_WORKER_NAME "node_espnow"
#define NODE_ESPNOW_TASK_WAIT_MS 50
#define NODE_ESPNOW_BROADCAST_MAC "\xFF\xFF\xFF\xFF\xFF\xFF"
#define NODE_ESPNOW_MAX_RX_SESSIONS 4
#define NODE_ACK_FLAG_ALL_ACKED 0x01U
#define NODE_TX_INTER_CHUNK_DELAY_MS 2
#define NODE_QOS2_COMPLETED_CACHE_SIZE 8
#define NODE_QOS2_COMPLETED_TTL_MS 60000

typedef enum
{
    NODE_FRAME_START = 1,
    NODE_FRAME_DATA = 2,
    NODE_FRAME_END = 3,
    NODE_FRAME_ACK = 4,
} node_espnow_frame_type_t;

typedef enum
{
    NODE_EVENT_SEND_REQUEST = 1,
    NODE_EVENT_SEND_DONE = 2,
    NODE_EVENT_RX_FRAME = 3,
} node_espnow_event_type_t;

typedef struct __attribute__((packed))
{
    uint16_t magic;
    uint8_t version;
    uint8_t type;
    uint32_t transfer_id;
    uint32_t total_len;
    uint16_t chunk_idx;
    uint16_t chunk_total;
    uint16_t payload_len;
    uint8_t flags;
    uint8_t reserved;
} node_espnow_frame_header_t;

typedef struct
{
    node_espnow_event_type_t type;
    union
    {
        struct
        {
            uint8_t peer_mac[6];
            uint8_t *payload;
            size_t payload_len;
        } send_req;
        struct
        {
            uint8_t peer_mac[6];
            esp_now_send_status_t status;
        } send_done;
        struct
        {
            uint8_t peer_mac[6];
            uint16_t len;
            uint8_t data[ESP_NOW_MAX_DATA_LEN];
        } rx;
    } data;
} node_espnow_event_t;

/* [STATE] TX lifecycle for one logical batch transfer. */
typedef struct
{
    bool active;
    bool sent_start;
    bool sent_end;
    uint8_t peer_mac[6];
    uint8_t *payload;
    size_t payload_len;
    uint32_t transfer_id;
    uint16_t chunk_total;
    uint16_t next_chunk_to_send;
    uint16_t acked_chunks;
    uint16_t tx_attempts;
    uint16_t tx_success_frames;
    uint16_t tx_failed_frames;
    uint8_t retry_count;
    int64_t started_us;
    int64_t last_tx_us;
    int64_t last_progress_us;
    uint8_t *acked_bitmap;
} node_espnow_tx_session_t;

/* [STATE] RX reassembly state for one peer + transfer_id. */
typedef struct
{
    bool active;
    bool got_end;
    uint8_t peer_mac[6];
    uint32_t transfer_id;
    uint32_t total_len;
    uint16_t chunk_total;
    uint16_t received_chunks;
    uint8_t *payload;
    uint8_t *received_bitmap;
    int64_t started_us;
    int64_t last_activity_us;
} node_espnow_rx_session_t;

typedef struct
{
    bool valid;
    uint8_t peer_mac[6];
    uint32_t transfer_id;
    int64_t completed_us;
} node_espnow_completed_transfer_t;

typedef struct
{
    bool inited;
    node_espnow_config_t cfg;
    node_espnow_handlers_t handlers;
    node_espnow_rx_batch_cb_t rx_batch_cb;
    void *rx_user_ctx;
    QueueHandle_t event_queue;
    TaskHandle_t worker_task;
    node_espnow_tx_session_t tx_session;
    node_espnow_rx_session_t rx_sessions[NODE_ESPNOW_MAX_RX_SESSIONS];
    node_espnow_completed_transfer_t completed_transfers[NODE_QOS2_COMPLETED_CACHE_SIZE];
    uint32_t transfer_seed;
} node_espnow_ctx_t;

static const char *TAG = "NODE_ESPNOW";
static node_espnow_ctx_t g_ctx = {0};

static void node_espnow_worker_task(void *arg);
static void node_espnow_handle_event(const node_espnow_event_t *evt);
static void node_espnow_handle_rx_frame(const uint8_t peer_mac[6], const uint8_t *data, uint16_t len);
static void node_espnow_handle_ack_frame(const uint8_t peer_mac[6], const node_espnow_frame_header_t *hdr, uint16_t payload_len);
static void node_espnow_handle_data_frame(const uint8_t peer_mac[6], const node_espnow_frame_header_t *hdr,
                                          const uint8_t *payload, uint16_t payload_len);
static esp_err_t node_espnow_send_tx_frame(node_espnow_frame_type_t type, uint16_t chunk_idx,
                                           const uint8_t *payload, uint16_t payload_len, uint8_t flags);
static esp_err_t node_espnow_send_ack_frame(const uint8_t peer_mac[6], const node_espnow_frame_header_t *rx_hdr,
                                            uint16_t chunk_idx, uint8_t flags);
static void node_espnow_try_send_more(void);
static void node_espnow_finish_tx_session(esp_err_t reason);
static void node_espnow_cleanup_rx_sessions(int64_t now_us, bool force);
static void node_espnow_release_rx_session(node_espnow_rx_session_t *session);
static void node_espnow_maybe_complete_rx_session(node_espnow_rx_session_t *session);
static node_espnow_rx_session_t *node_espnow_find_rx_session(const uint8_t peer_mac[6], uint32_t transfer_id);
static node_espnow_rx_session_t *node_espnow_alloc_rx_session(const uint8_t peer_mac[6], uint32_t transfer_id,
                                                              uint32_t total_len, uint16_t chunk_total);
static bool node_espnow_is_bit_set(const uint8_t *bitmap, uint16_t idx);
static void node_espnow_set_bit(uint8_t *bitmap, uint16_t idx);
static void node_espnow_mark_ack(uint16_t chunk_idx);
static bool node_espnow_chunk_is_acked(uint16_t chunk_idx);
static bool node_espnow_all_chunks_acked(void);
static esp_err_t node_espnow_ensure_peer(const uint8_t peer_mac[6]);
static uint16_t node_espnow_max_chunk_payload(void);
static bool node_espnow_validate_data_bounds(const node_espnow_frame_header_t *hdr, uint16_t payload_len);
static void node_espnow_cleanup_completed_cache(int64_t now_us);
static bool node_espnow_qos2_is_completed(const uint8_t peer_mac[6], uint32_t transfer_id, int64_t now_us);
static void node_espnow_qos2_mark_completed(const uint8_t peer_mac[6], uint32_t transfer_id, int64_t now_us);

void node_espnow_default_config(node_espnow_config_t *config)
{
    if (config == NULL)
    {
        return;
    }

    memset(config, 0, sizeof(*config));
    config->channel = 1;
    config->chunk_payload_bytes = 180;
    config->tx_window_size = 4;
    config->ack_timeout_ms = 400;
    config->max_retries = 4;
    config->session_timeout_ms = 5000;
    config->max_batch_bytes = 64 * 1024;
    config->qos_default = NODE_ESPNOW_QOS1;
    config->worker_stack_size = 4096;
    config->worker_priority = 5;
    config->event_queue_len = 24;
}

static void node_espnow_send_cb(const wifi_tx_info_t *info, esp_now_send_status_t status)
{
    if (!g_ctx.inited || info == NULL || g_ctx.event_queue == NULL)
    {
        return;
    }

    node_espnow_event_t evt = {0};
    evt.type = NODE_EVENT_SEND_DONE;
    memcpy(evt.data.send_done.peer_mac, info->des_addr, 6);
    evt.data.send_done.status = status;
    (void)xQueueSend(g_ctx.event_queue, &evt, 0);
}

static void node_espnow_recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len)
{
    if (!g_ctx.inited || recv_info == NULL || data == NULL || len <= 0 || g_ctx.event_queue == NULL)
    {
        return;
    }
    if (len > ESP_NOW_MAX_DATA_LEN)
    {
        return;
    }

    node_espnow_event_t evt = {0};
    evt.type = NODE_EVENT_RX_FRAME;
    memcpy(evt.data.rx.peer_mac, recv_info->src_addr, 6);
    evt.data.rx.len = (uint16_t)len;
    memcpy(evt.data.rx.data, data, (size_t)len);
    (void)xQueueSend(g_ctx.event_queue, &evt, 0);
}

esp_err_t node_espnow_init(const node_espnow_config_t *config,
                           const node_espnow_handlers_t *handlers)
{
    if (g_ctx.inited)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (config == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    memset(&g_ctx, 0, sizeof(g_ctx));
    g_ctx.cfg = *config;
    if (handlers != NULL)
    {
        g_ctx.handlers.tx_result_cb = handlers->tx_result_cb;
        g_ctx.handlers.user_ctx = handlers->user_ctx;
    }

    if (g_ctx.cfg.event_queue_len == 0 || g_ctx.cfg.worker_stack_size < 2048)
    {
        return ESP_ERR_INVALID_ARG;
    }

    uint16_t max_chunk = node_espnow_max_chunk_payload();
    if (g_ctx.cfg.chunk_payload_bytes == 0 || g_ctx.cfg.chunk_payload_bytes > max_chunk)
    {
        g_ctx.cfg.chunk_payload_bytes = max_chunk;
    }
    if (g_ctx.cfg.tx_window_size == 0)
    {
        g_ctx.cfg.tx_window_size = 1;
    }
    if (g_ctx.cfg.ack_timeout_ms == 0)
    {
        g_ctx.cfg.ack_timeout_ms = 300;
    }
    if (g_ctx.cfg.session_timeout_ms < g_ctx.cfg.ack_timeout_ms)
    {
        g_ctx.cfg.session_timeout_ms = g_ctx.cfg.ack_timeout_ms * 4;
    }

    ESP_ERROR_CHECK(esp_netif_init());
    esp_err_t ret = esp_event_loop_create_default();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE)
    {
        return ret;
    }

    wifi_init_config_t wifi_cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_channel(g_ctx.cfg.channel, WIFI_SECOND_CHAN_NONE));
    ESP_ERROR_CHECK(esp_now_init());

    ESP_ERROR_CHECK(esp_now_register_send_cb(node_espnow_send_cb));
    ESP_ERROR_CHECK(esp_now_register_recv_cb(node_espnow_recv_cb));

    g_ctx.event_queue = xQueueCreate(g_ctx.cfg.event_queue_len, sizeof(node_espnow_event_t));
    if (g_ctx.event_queue == NULL)
    {
        return ESP_ERR_NO_MEM;
    }

    BaseType_t created = xTaskCreate(node_espnow_worker_task,
                                     NODE_ESPNOW_WORKER_NAME,
                                     g_ctx.cfg.worker_stack_size,
                                     NULL,
                                     g_ctx.cfg.worker_priority,
                                     &g_ctx.worker_task);
    if (created != pdPASS)
    {
        vQueueDelete(g_ctx.event_queue);
        g_ctx.event_queue = NULL;
        return ESP_ERR_NO_MEM;
    }

    g_ctx.inited = true;
    ESP_LOGI(TAG, "Unified library initialized (qos=%d, chunk=%u, window=%u)",
             g_ctx.cfg.qos_default, g_ctx.cfg.chunk_payload_bytes, g_ctx.cfg.tx_window_size);
    return ESP_OK;
}

esp_err_t node_espnow_set_rx_batch_cb(node_espnow_rx_batch_cb_t cb, void *user_ctx)
{
    if (!g_ctx.inited)
    {
        return ESP_ERR_INVALID_STATE;
    }
    g_ctx.rx_batch_cb = cb;
    g_ctx.rx_user_ctx = user_ctx;
    return ESP_OK;
}

node_espnow_qos_t node_espnow_get_qos(void)
{
    return g_ctx.cfg.qos_default;
}

esp_err_t node_espnow_send_to(const uint8_t peer_mac[6], const void *payload, size_t payload_len)
{
    if (!g_ctx.inited || peer_mac == NULL || payload == NULL || payload_len == 0)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (g_ctx.tx_session.active)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (payload_len > g_ctx.cfg.max_batch_bytes)
    {
        return ESP_ERR_INVALID_SIZE;
    }

    uint8_t *copied = (uint8_t *)malloc(payload_len);
    if (copied == NULL)
    {
        return ESP_ERR_NO_MEM;
    }
    memcpy(copied, payload, payload_len);

    node_espnow_event_t evt = {0};
    evt.type = NODE_EVENT_SEND_REQUEST;
    memcpy(evt.data.send_req.peer_mac, peer_mac, 6);
    evt.data.send_req.payload = copied;
    evt.data.send_req.payload_len = payload_len;

    if (xQueueSend(g_ctx.event_queue, &evt, pdMS_TO_TICKS(100)) != pdTRUE)
    {
        free(copied);
        return ESP_ERR_TIMEOUT;
    }

    return ESP_OK;
}

void node_espnow_deinit(void)
{
    if (!g_ctx.inited)
    {
        return;
    }

    if (g_ctx.worker_task != NULL)
    {
        vTaskDelete(g_ctx.worker_task);
    }
    if (g_ctx.event_queue != NULL)
    {
        vQueueDelete(g_ctx.event_queue);
    }

    node_espnow_finish_tx_session(ESP_ERR_INVALID_STATE);
    node_espnow_cleanup_rx_sessions(0, true);

    esp_now_unregister_send_cb();
    esp_now_unregister_recv_cb();
    esp_now_deinit();
    esp_wifi_stop();
    esp_wifi_deinit();
    memset(&g_ctx, 0, sizeof(g_ctx));
}

static void node_espnow_worker_task(void *arg)
{
    (void)arg;
    node_espnow_event_t evt;

    while (1)
    {
        /* [FLOW] ISR/driver callbacks only enqueue events; state changes happen here. */
        if (xQueueReceive(g_ctx.event_queue, &evt, pdMS_TO_TICKS(NODE_ESPNOW_TASK_WAIT_MS)) == pdTRUE)
        {
            node_espnow_handle_event(&evt);
        }

        int64_t now_us = esp_timer_get_time();
        node_espnow_cleanup_rx_sessions(now_us, false);
        node_espnow_cleanup_completed_cache(now_us);

        if (g_ctx.tx_session.active)
        {
            int64_t ack_deadline_us = (int64_t)g_ctx.cfg.ack_timeout_ms * 1000;
            int64_t session_deadline_us = (int64_t)g_ctx.cfg.session_timeout_ms * 1000;

            if (g_ctx.tx_session.last_tx_us > 0 && (now_us - g_ctx.tx_session.last_tx_us) >= ack_deadline_us)
            {
                if (node_espnow_all_chunks_acked())
                {
                    node_espnow_finish_tx_session(ESP_OK);
                }
                else if (g_ctx.tx_session.retry_count >= g_ctx.cfg.max_retries)
                {
                    node_espnow_finish_tx_session(ESP_ERR_TIMEOUT);
                }
                else
                {
                    /* [RELIABILITY] Timeout path: restart from START and resend pending chunks. */
                    g_ctx.tx_session.retry_count++;
                    g_ctx.tx_session.sent_start = false;
                    g_ctx.tx_session.sent_end = false;
                    g_ctx.tx_session.next_chunk_to_send = 0;
                    ESP_LOGW(TAG, "transfer=%lu retry=%u",
                             (unsigned long)g_ctx.tx_session.transfer_id,
                             (unsigned)g_ctx.tx_session.retry_count);
                    node_espnow_try_send_more();
                }
            }

            if ((now_us - g_ctx.tx_session.started_us) >= session_deadline_us)
            {
                node_espnow_finish_tx_session(ESP_ERR_TIMEOUT);
            }
        }
    }
}

static void node_espnow_handle_event(const node_espnow_event_t *evt)
{
    if (evt == NULL)
    {
        return;
    }

    switch (evt->type)
    {
    case NODE_EVENT_SEND_REQUEST:
    {
        if (g_ctx.tx_session.active)
        {
            free(evt->data.send_req.payload);
            break;
        }

        if (node_espnow_ensure_peer(evt->data.send_req.peer_mac) != ESP_OK)
        {
            free(evt->data.send_req.payload);
            break;
        }

        node_espnow_tx_session_t *s = &g_ctx.tx_session;
        memset(s, 0, sizeof(*s));
        s->active = true;
        memcpy(s->peer_mac, evt->data.send_req.peer_mac, 6);
        s->payload = evt->data.send_req.payload;
        s->payload_len = evt->data.send_req.payload_len;
        s->transfer_id = ++g_ctx.transfer_seed;

        /* [FLOW] Split one logical payload into DATA chunks. */
        size_t chunk_total = (s->payload_len + g_ctx.cfg.chunk_payload_bytes - 1U) / g_ctx.cfg.chunk_payload_bytes;
        if (chunk_total == 0 || chunk_total > UINT16_MAX)
        {
            node_espnow_finish_tx_session(ESP_ERR_INVALID_SIZE);
            break;
        }
        s->chunk_total = (uint16_t)chunk_total;
        s->started_us = esp_timer_get_time();
        s->last_progress_us = s->started_us;
        s->last_tx_us = 0;

        size_t bitmap_bytes = (s->chunk_total + 7U) / 8U;
        s->acked_bitmap = (uint8_t *)calloc(bitmap_bytes, 1);
        if (s->acked_bitmap == NULL)
        {
            node_espnow_finish_tx_session(ESP_ERR_NO_MEM);
            break;
        }

        ESP_LOGI(TAG, "TX transfer=%lu start bytes=%u chunks=%u",
                 (unsigned long)s->transfer_id,
                 (unsigned)s->payload_len,
                 (unsigned)s->chunk_total);
        node_espnow_try_send_more();
        break;
    }

    case NODE_EVENT_SEND_DONE:
    {
        if (g_ctx.tx_session.active &&
            memcmp(g_ctx.tx_session.peer_mac, evt->data.send_done.peer_mac, 6) == 0)
        {
            if (evt->data.send_done.status == ESP_NOW_SEND_SUCCESS)
            {
                g_ctx.tx_session.tx_success_frames++;
            }
            else
            {
                g_ctx.tx_session.tx_failed_frames++;
            }
        }
        break;
    }

    case NODE_EVENT_RX_FRAME:
        node_espnow_handle_rx_frame(evt->data.rx.peer_mac, evt->data.rx.data, evt->data.rx.len);
        break;

    default:
        break;
    }
}

static void node_espnow_handle_rx_frame(const uint8_t peer_mac[6], const uint8_t *data, uint16_t len)
{
    if (len < (uint16_t)sizeof(node_espnow_frame_header_t))
    {
        ESP_LOGW(TAG, "drop short frame len=%u", (unsigned)len);
        return;
    }

    node_espnow_frame_header_t hdr = {0};
    memcpy(&hdr, data, sizeof(hdr));
    uint16_t payload_len = (uint16_t)(len - sizeof(hdr));
    const uint8_t *payload = data + sizeof(hdr);

    if (hdr.magic != NODE_ESPNOW_MAGIC || hdr.version != NODE_ESPNOW_VERSION)
    {
        ESP_LOGW(TAG, "drop invalid header magic=0x%04X version=%u", hdr.magic, (unsigned)hdr.version);
        return;
    }
    if (hdr.payload_len != payload_len)
    {
        ESP_LOGW(TAG, "drop payload mismatch hdr=%u actual=%u", (unsigned)hdr.payload_len, (unsigned)payload_len);
        return;
    }

    switch ((node_espnow_frame_type_t)hdr.type)
    {
    case NODE_FRAME_ACK:
        node_espnow_handle_ack_frame(peer_mac, &hdr, payload_len);
        break;
    case NODE_FRAME_START:
    case NODE_FRAME_DATA:
    case NODE_FRAME_END:
        node_espnow_handle_data_frame(peer_mac, &hdr, payload, payload_len);
        break;
    default:
        ESP_LOGW(TAG, "drop unknown frame type=%u", (unsigned)hdr.type);
        break;
    }
}

static void node_espnow_handle_ack_frame(const uint8_t peer_mac[6], const node_espnow_frame_header_t *hdr, uint16_t payload_len)
{
    if (payload_len != 0U)
    {
        ESP_LOGW(TAG, "drop ACK with payload len=%u", (unsigned)payload_len);
        return;
    }

    node_espnow_tx_session_t *s = &g_ctx.tx_session;
    if (!s->active)
    {
        return;
    }
    if (memcmp(s->peer_mac, peer_mac, 6) != 0 || hdr->transfer_id != s->transfer_id)
    {
        return;
    }

    /* [RELIABILITY] ACK marks one chunk, or all chunks via ALL_ACKED flag. */
    if ((hdr->flags & NODE_ACK_FLAG_ALL_ACKED) != 0U)
    {
        for (uint16_t i = 0; i < s->chunk_total; i++)
        {
            node_espnow_mark_ack(i);
        }
        s->acked_chunks = s->chunk_total;
    }
    else if (hdr->chunk_idx < s->chunk_total && !node_espnow_chunk_is_acked(hdr->chunk_idx))
    {
        node_espnow_mark_ack(hdr->chunk_idx);
        s->acked_chunks++;
    }

    s->last_progress_us = esp_timer_get_time();
    if (node_espnow_all_chunks_acked())
    {
        node_espnow_finish_tx_session(ESP_OK);
    }
    else
    {
        node_espnow_try_send_more();
    }
}

static void node_espnow_handle_data_frame(const uint8_t peer_mac[6], const node_espnow_frame_header_t *hdr,
                                          const uint8_t *payload, uint16_t payload_len)
{
    if (hdr->total_len == 0 || hdr->total_len > g_ctx.cfg.max_batch_bytes || hdr->chunk_total == 0)
    {
        ESP_LOGW(TAG, "drop invalid data header transfer=%lu total=%lu chunks=%u",
                 (unsigned long)hdr->transfer_id, (unsigned long)hdr->total_len, (unsigned)hdr->chunk_total);
        return;
    }

    node_espnow_rx_session_t *s = node_espnow_find_rx_session(peer_mac, hdr->transfer_id);

    if ((node_espnow_frame_type_t)hdr->type == NODE_FRAME_START)
    {
        if (payload_len != 0U)
        {
            ESP_LOGW(TAG, "drop START with payload transfer=%lu", (unsigned long)hdr->transfer_id);
            return;
        }
        if (s == NULL)
        {
            if (g_ctx.cfg.qos_default == NODE_ESPNOW_QOS2 &&
                node_espnow_qos2_is_completed(peer_mac, hdr->transfer_id, esp_timer_get_time()))
            {
                /* [RELIABILITY] QoS2 duplicate START: ack all and skip app re-delivery. */
                (void)node_espnow_send_ack_frame(peer_mac, hdr, hdr->chunk_total, NODE_ACK_FLAG_ALL_ACKED);
                ESP_LOGI(TAG, "QoS2 duplicate transfer=%lu ignored (already delivered)",
                         (unsigned long)hdr->transfer_id);
                return;
            }

            s = node_espnow_alloc_rx_session(peer_mac, hdr->transfer_id, hdr->total_len, hdr->chunk_total);
            if (s == NULL)
            {
                ESP_LOGW(TAG, "drop START no resources transfer=%lu", (unsigned long)hdr->transfer_id);
                return;
            }
            ESP_LOGI(TAG, "RX transfer=%lu start from %02X:%02X:%02X:%02X:%02X:%02X bytes=%lu chunks=%u",
                     (unsigned long)s->transfer_id,
                     s->peer_mac[0], s->peer_mac[1], s->peer_mac[2],
                     s->peer_mac[3], s->peer_mac[4], s->peer_mac[5],
                     (unsigned long)s->total_len, (unsigned)s->chunk_total);
        }
        s->last_activity_us = esp_timer_get_time();
        return;
    }

    if (s == NULL)
    {
        ESP_LOGW(TAG, "drop frame no session type=%u transfer=%lu", (unsigned)hdr->type, (unsigned long)hdr->transfer_id);
        return;
    }

    if (s->total_len != hdr->total_len || s->chunk_total != hdr->chunk_total)
    {
        ESP_LOGW(TAG, "drop transfer mismatch transfer=%lu", (unsigned long)hdr->transfer_id);
        return;
    }

    if ((node_espnow_frame_type_t)hdr->type == NODE_FRAME_END)
    {
        if (payload_len != 0U)
        {
            ESP_LOGW(TAG, "drop END with payload transfer=%lu", (unsigned long)hdr->transfer_id);
            return;
        }
        s->got_end = true;
        s->last_activity_us = esp_timer_get_time();
        /* [FLOW] Completion requires END frame and all DATA chunks present. */
        node_espnow_maybe_complete_rx_session(s);
        return;
    }

    if (!node_espnow_validate_data_bounds(hdr, payload_len))
    {
        ESP_LOGW(TAG, "drop DATA out-of-range transfer=%lu idx=%u payload=%u",
                 (unsigned long)hdr->transfer_id, (unsigned)hdr->chunk_idx, (unsigned)payload_len);
        return;
    }

    if (!node_espnow_is_bit_set(s->received_bitmap, hdr->chunk_idx))
    {
        uint32_t offset = (uint32_t)hdr->chunk_idx * g_ctx.cfg.chunk_payload_bytes;
        memcpy(s->payload + offset, payload, payload_len);
        node_espnow_set_bit(s->received_bitmap, hdr->chunk_idx);
        s->received_chunks++;
    }

    s->last_activity_us = esp_timer_get_time();
    /* [RELIABILITY] ACK each DATA chunk for QoS1 progress tracking. */
    (void)node_espnow_send_ack_frame(peer_mac, hdr, hdr->chunk_idx, 0U);
    node_espnow_maybe_complete_rx_session(s);
}

static void node_espnow_try_send_more(void)
{
    node_espnow_tx_session_t *s = &g_ctx.tx_session;
    if (!s->active)
    {
        return;
    }

    if (!s->sent_start)
    {
        if (node_espnow_send_tx_frame(NODE_FRAME_START, 0, NULL, 0, 0) == ESP_OK)
        {
            s->sent_start = true;
            s->tx_attempts++;
            s->last_tx_us = esp_timer_get_time();
        }
        else
        {
            node_espnow_finish_tx_session(ESP_FAIL);
            return;
        }
    }

    uint8_t sent_in_burst = 0;
    /* [FLOW] Sliding-window send: emit next unacked chunks up to tx_window_size. */
    while (s->next_chunk_to_send < s->chunk_total && sent_in_burst < g_ctx.cfg.tx_window_size)
    {
        uint16_t idx = s->next_chunk_to_send++;
        if (node_espnow_chunk_is_acked(idx))
        {
            continue;
        }

        uint32_t offset = (uint32_t)idx * g_ctx.cfg.chunk_payload_bytes;
        uint16_t remaining = (uint16_t)(s->payload_len - offset);
        uint16_t chunk_len = remaining > g_ctx.cfg.chunk_payload_bytes ? g_ctx.cfg.chunk_payload_bytes : remaining;

        if (node_espnow_send_tx_frame(NODE_FRAME_DATA, idx, s->payload + offset, chunk_len, 0) != ESP_OK)
        {
            node_espnow_finish_tx_session(ESP_FAIL);
            return;
        }

        s->tx_attempts++;
        sent_in_burst++;
        s->last_tx_us = esp_timer_get_time();
        /* [RELIABILITY] Small pacing improves large-transfer stability on noisy links. */
        if (NODE_TX_INTER_CHUNK_DELAY_MS > 0)
        {
            vTaskDelay(pdMS_TO_TICKS(NODE_TX_INTER_CHUNK_DELAY_MS));
        }
    }

    if (s->next_chunk_to_send >= s->chunk_total && !s->sent_end)
    {
        if (node_espnow_send_tx_frame(NODE_FRAME_END, s->chunk_total, NULL, 0, 0) == ESP_OK)
        {
            s->tx_attempts++;
            s->sent_end = true;
            s->last_tx_us = esp_timer_get_time();
        }
        else
        {
            node_espnow_finish_tx_session(ESP_FAIL);
            return;
        }
    }

    if (g_ctx.cfg.qos_default == NODE_ESPNOW_QOS0 && s->sent_end)
    {
        node_espnow_finish_tx_session(ESP_OK);
    }
}

static esp_err_t node_espnow_send_tx_frame(node_espnow_frame_type_t type, uint16_t chunk_idx,
                                           const uint8_t *payload, uint16_t payload_len, uint8_t flags)
{
    node_espnow_tx_session_t *s = &g_ctx.tx_session;
    if (!s->active)
    {
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t frame[ESP_NOW_MAX_DATA_LEN] = {0};
    node_espnow_frame_header_t hdr = {0};
    hdr.magic = NODE_ESPNOW_MAGIC;
    hdr.version = NODE_ESPNOW_VERSION;
    hdr.type = (uint8_t)type;
    hdr.transfer_id = s->transfer_id;
    hdr.total_len = (uint32_t)s->payload_len;
    hdr.chunk_idx = chunk_idx;
    hdr.chunk_total = s->chunk_total;
    hdr.payload_len = payload_len;
    hdr.flags = flags;

    size_t hdr_len = sizeof(hdr);
    if ((hdr_len + payload_len) > sizeof(frame))
    {
        return ESP_ERR_INVALID_SIZE;
    }

    memcpy(frame, &hdr, hdr_len);
    if (payload_len > 0U)
    {
        memcpy(frame + hdr_len, payload, payload_len);
    }

    return esp_now_send(s->peer_mac, frame, hdr_len + payload_len);
}

static esp_err_t node_espnow_send_ack_frame(const uint8_t peer_mac[6], const node_espnow_frame_header_t *rx_hdr,
                                            uint16_t chunk_idx, uint8_t flags)
{
    if (node_espnow_ensure_peer(peer_mac) != ESP_OK)
    {
        return ESP_FAIL;
    }

    node_espnow_frame_header_t ack = {0};
    ack.magic = NODE_ESPNOW_MAGIC;
    ack.version = NODE_ESPNOW_VERSION;
    ack.type = NODE_FRAME_ACK;
    ack.transfer_id = rx_hdr->transfer_id;
    ack.total_len = rx_hdr->total_len;
    ack.chunk_idx = chunk_idx;
    ack.chunk_total = rx_hdr->chunk_total;
    ack.payload_len = 0;
    ack.flags = flags;
    return esp_now_send(peer_mac, (const uint8_t *)&ack, sizeof(ack));
}

static void node_espnow_finish_tx_session(esp_err_t reason)
{
    node_espnow_tx_session_t *s = &g_ctx.tx_session;
    if (!s->active)
    {
        if (s->payload != NULL)
        {
            free(s->payload);
            s->payload = NULL;
        }
        if (s->acked_bitmap != NULL)
        {
            free(s->acked_bitmap);
            s->acked_bitmap = NULL;
        }
        return;
    }

    node_espnow_tx_result_t result = {0};
    result.transfer_id = s->transfer_id;
    result.total_bytes = s->payload_len;
    result.total_chunks = s->chunk_total;
    result.acked_chunks = s->acked_chunks;
    result.tx_attempts = s->tx_attempts;
    result.tx_success_frames = s->tx_success_frames;
    result.tx_failed_frames = s->tx_failed_frames;
    result.duration_ms = (uint32_t)((esp_timer_get_time() - s->started_us) / 1000);
    result.fail_reason = reason;

    if (reason == ESP_OK)
    {
        ESP_LOGI(TAG, "TX transfer=%lu done in %lu ms (attempts=%u, retries=%u)",
                 (unsigned long)s->transfer_id,
                 (unsigned long)result.duration_ms,
                 (unsigned)result.tx_attempts,
                 (unsigned)s->retry_count);
    }
    else
    {
        ESP_LOGE(TAG, "TX transfer=%lu failed (%s), acked=%u/%u, retries=%u",
                 (unsigned long)s->transfer_id,
                 esp_err_to_name(reason),
                 (unsigned)result.acked_chunks,
                 (unsigned)result.total_chunks,
                 (unsigned)s->retry_count);
    }

    if (g_ctx.handlers.tx_result_cb != NULL)
    {
        g_ctx.handlers.tx_result_cb(s->peer_mac, &result, g_ctx.handlers.user_ctx);
    }

    if (s->payload != NULL)
    {
        free(s->payload);
    }
    if (s->acked_bitmap != NULL)
    {
        free(s->acked_bitmap);
    }
    memset(s, 0, sizeof(*s));
}

static void node_espnow_cleanup_rx_sessions(int64_t now_us, bool force)
{
    for (size_t i = 0; i < NODE_ESPNOW_MAX_RX_SESSIONS; i++)
    {
        node_espnow_rx_session_t *s = &g_ctx.rx_sessions[i];
        if (!s->active)
        {
            continue;
        }

        if (!force && now_us > 0)
        {
            int64_t timeout_us = (int64_t)g_ctx.cfg.session_timeout_ms * 1000;
            if ((now_us - s->last_activity_us) < timeout_us)
            {
                continue;
            }

            ESP_LOGW(TAG, "RX transfer=%lu timeout, received=%u/%u end=%d",
                     (unsigned long)s->transfer_id,
                     (unsigned)s->received_chunks,
                     (unsigned)s->chunk_total,
                     (int)s->got_end);
        }

        node_espnow_release_rx_session(s);
    }
}

static void node_espnow_release_rx_session(node_espnow_rx_session_t *session)
{
    if (session == NULL)
    {
        return;
    }
    if (session->payload != NULL)
    {
        free(session->payload);
    }
    if (session->received_bitmap != NULL)
    {
        free(session->received_bitmap);
    }
    memset(session, 0, sizeof(*session));
}

static void node_espnow_maybe_complete_rx_session(node_espnow_rx_session_t *session)
{
    if (session == NULL || !session->active || !session->got_end || session->received_chunks < session->chunk_total)
    {
        return;
    }

    /* [FLOW] Notify TX full assembly via ALL_ACKED, then deliver payload to app callback. */
    node_espnow_frame_header_t rx_hdr = {0};
    rx_hdr.transfer_id = session->transfer_id;
    rx_hdr.total_len = session->total_len;
    rx_hdr.chunk_total = session->chunk_total;
    (void)node_espnow_send_ack_frame(session->peer_mac, &rx_hdr, session->chunk_total, NODE_ACK_FLAG_ALL_ACKED);

    node_espnow_rx_batch_t batch = {0};
    batch.transfer_id = session->transfer_id;
    batch.total_bytes = session->total_len;
    batch.total_chunks = session->chunk_total;
    batch.received_chunks = session->received_chunks;
    batch.duration_ms = (uint32_t)((esp_timer_get_time() - session->started_us) / 1000);
    batch.payload = session->payload;
    batch.payload_len = session->total_len;

    ESP_LOGI(TAG, "RX transfer=%lu completed in %lu ms (%u chunks)",
             (unsigned long)batch.transfer_id,
             (unsigned long)batch.duration_ms,
             (unsigned)batch.total_chunks);

    if (g_ctx.rx_batch_cb != NULL)
    {
        g_ctx.rx_batch_cb(session->peer_mac, &batch, g_ctx.rx_user_ctx);
    }

    if (g_ctx.cfg.qos_default == NODE_ESPNOW_QOS2)
    {
        node_espnow_qos2_mark_completed(session->peer_mac, session->transfer_id, esp_timer_get_time());
    }

    node_espnow_release_rx_session(session);
}

static void node_espnow_cleanup_completed_cache(int64_t now_us)
{
    if (now_us <= 0 || g_ctx.cfg.qos_default != NODE_ESPNOW_QOS2)
    {
        return;
    }

    int64_t ttl_us = (int64_t)NODE_QOS2_COMPLETED_TTL_MS * 1000;
    for (size_t i = 0; i < NODE_QOS2_COMPLETED_CACHE_SIZE; i++)
    {
        node_espnow_completed_transfer_t *c = &g_ctx.completed_transfers[i];
        if (!c->valid)
        {
            continue;
        }
        if ((now_us - c->completed_us) >= ttl_us)
        {
            memset(c, 0, sizeof(*c));
        }
    }
}

static bool node_espnow_qos2_is_completed(const uint8_t peer_mac[6], uint32_t transfer_id, int64_t now_us)
{
    if (peer_mac == NULL || g_ctx.cfg.qos_default != NODE_ESPNOW_QOS2)
    {
        return false;
    }

    for (size_t i = 0; i < NODE_QOS2_COMPLETED_CACHE_SIZE; i++)
    {
        node_espnow_completed_transfer_t *c = &g_ctx.completed_transfers[i];
        if (c->valid && c->transfer_id == transfer_id && memcmp(c->peer_mac, peer_mac, 6) == 0)
        {
            if (now_us > 0)
            {
                c->completed_us = now_us;
            }
            return true;
        }
    }
    return false;
}

static void node_espnow_qos2_mark_completed(const uint8_t peer_mac[6], uint32_t transfer_id, int64_t now_us)
{
    if (peer_mac == NULL || g_ctx.cfg.qos_default != NODE_ESPNOW_QOS2)
    {
        return;
    }

    size_t replace_idx = 0;
    int64_t oldest_us = INT64_MAX;

    for (size_t i = 0; i < NODE_QOS2_COMPLETED_CACHE_SIZE; i++)
    {
        node_espnow_completed_transfer_t *c = &g_ctx.completed_transfers[i];
        if (c->valid && c->transfer_id == transfer_id && memcmp(c->peer_mac, peer_mac, 6) == 0)
        {
            c->completed_us = now_us;
            return;
        }
        if (!c->valid)
        {
            replace_idx = i;
            oldest_us = -1;
            break;
        }
        if (c->completed_us < oldest_us)
        {
            oldest_us = c->completed_us;
            replace_idx = i;
        }
    }

    node_espnow_completed_transfer_t *dst = &g_ctx.completed_transfers[replace_idx];
    memset(dst, 0, sizeof(*dst));
    dst->valid = true;
    memcpy(dst->peer_mac, peer_mac, 6);
    dst->transfer_id = transfer_id;
    dst->completed_us = now_us;
}

static node_espnow_rx_session_t *node_espnow_find_rx_session(const uint8_t peer_mac[6], uint32_t transfer_id)
{
    for (size_t i = 0; i < NODE_ESPNOW_MAX_RX_SESSIONS; i++)
    {
        node_espnow_rx_session_t *s = &g_ctx.rx_sessions[i];
        if (s->active && s->transfer_id == transfer_id && memcmp(s->peer_mac, peer_mac, 6) == 0)
        {
            return s;
        }
    }
    return NULL;
}

static node_espnow_rx_session_t *node_espnow_alloc_rx_session(const uint8_t peer_mac[6], uint32_t transfer_id,
                                                              uint32_t total_len, uint16_t chunk_total)
{
    for (size_t i = 0; i < NODE_ESPNOW_MAX_RX_SESSIONS; i++)
    {
        if (!g_ctx.rx_sessions[i].active)
        {
            node_espnow_rx_session_t *s = &g_ctx.rx_sessions[i];
            size_t bitmap_bytes = (chunk_total + 7U) / 8U;
            uint8_t *payload = (uint8_t *)malloc(total_len);
            uint8_t *bitmap = (uint8_t *)calloc(bitmap_bytes, 1);
            if (payload == NULL || bitmap == NULL)
            {
                free(payload);
                free(bitmap);
                return NULL;
            }

            memset(s, 0, sizeof(*s));
            s->active = true;
            memcpy(s->peer_mac, peer_mac, 6);
            s->transfer_id = transfer_id;
            s->total_len = total_len;
            s->chunk_total = chunk_total;
            s->payload = payload;
            s->received_bitmap = bitmap;
            s->started_us = esp_timer_get_time();
            s->last_activity_us = s->started_us;
            return s;
        }
    }
    return NULL;
}

static bool node_espnow_is_bit_set(const uint8_t *bitmap, uint16_t idx)
{
    if (bitmap == NULL)
    {
        return false;
    }
    return (bitmap[idx / 8U] & (1U << (idx % 8U))) != 0U;
}

static void node_espnow_set_bit(uint8_t *bitmap, uint16_t idx)
{
    if (bitmap == NULL)
    {
        return;
    }
    bitmap[idx / 8U] |= (1U << (idx % 8U));
}

static bool node_espnow_validate_data_bounds(const node_espnow_frame_header_t *hdr, uint16_t payload_len)
{
    if (hdr->type != NODE_FRAME_DATA || payload_len == 0U || hdr->chunk_idx >= hdr->chunk_total)
    {
        return false;
    }

    uint32_t offset = (uint32_t)hdr->chunk_idx * g_ctx.cfg.chunk_payload_bytes;
    if (offset >= hdr->total_len)
    {
        return false;
    }
    return (offset + payload_len) <= hdr->total_len;
}

static bool node_espnow_chunk_is_acked(uint16_t chunk_idx)
{
    node_espnow_tx_session_t *s = &g_ctx.tx_session;
    if (s->acked_bitmap == NULL || chunk_idx >= s->chunk_total)
    {
        return false;
    }
    return (s->acked_bitmap[chunk_idx / 8U] & (1U << (chunk_idx % 8U))) != 0U;
}

static void node_espnow_mark_ack(uint16_t chunk_idx)
{
    node_espnow_tx_session_t *s = &g_ctx.tx_session;
    if (s->acked_bitmap == NULL || chunk_idx >= s->chunk_total)
    {
        return;
    }
    s->acked_bitmap[chunk_idx / 8U] |= (1U << (chunk_idx % 8U));
}

static bool node_espnow_all_chunks_acked(void)
{
    node_espnow_tx_session_t *s = &g_ctx.tx_session;
    if (!s->active)
    {
        return false;
    }
    return s->acked_chunks >= s->chunk_total;
}

static esp_err_t node_espnow_ensure_peer(const uint8_t peer_mac[6])
{
    if (esp_now_is_peer_exist(peer_mac))
    {
        return ESP_OK;
    }

    esp_now_peer_info_t peer = {0};
    memcpy(peer.peer_addr, peer_mac, 6);
    peer.channel = g_ctx.cfg.channel;
    peer.ifidx = WIFI_IF_STA;
    peer.encrypt = false;

    if (memcmp(peer_mac, NODE_ESPNOW_BROADCAST_MAC, 6) == 0)
    {
        peer.channel = 0;
    }

    return esp_now_add_peer(&peer);
}

static uint16_t node_espnow_max_chunk_payload(void)
{
    size_t hdr_len = sizeof(node_espnow_frame_header_t);
    if (ESP_NOW_MAX_DATA_LEN <= hdr_len)
    {
        return 1;
    }
    return (uint16_t)(ESP_NOW_MAX_DATA_LEN - hdr_len);
}

```


## TX main (`AIoTNode.cpp`)

```cpp
/**
 * @file AIoTNode.cpp
 * @brief TX application based on node_espnow library.
 */

#include <string.h>

#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_log.h"
#include "esp_psram.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "node_espnow.h"
#include "node_exit.h"
#include "node_led.h"
#include "node_rtc.h"
#include "node_sdcard.h"
#include "node_spi.h"
#include "node_timer.h"

#ifdef __cplusplus
extern "C" {
#endif

static const char *TAG = "ESP_NOW_TX_APP";
static const uint32_t kTxPeriodMs = 3000U;
static const uint16_t kFrameMagic = 0x4E44U;
static const uint8_t kFrameVersion = 1U;
static const size_t kPrintableArrayCount = 128U;
static const size_t kPrintablePayloadBytes = kPrintableArrayCount * sizeof(uint16_t);
static const size_t kMediumPayloadBytes = 600U;
static const size_t kRawPayloadBytes = 300U;
static const size_t kLargeArrayCount = 4096U;
static const size_t kLargePayloadBytes = kLargeArrayCount * sizeof(uint16_t);
static const size_t kMaxBatchBytes = 16384U;
static const size_t kMaxStringBytes = 128U;
static const uint8_t kTotalCases = 6U;
static const uint8_t kTotalQosRounds = 3U;
static const uint8_t kCaseDiscoveryReq = 0xF0U;
static const uint8_t kCaseDiscoveryResp = 0xF1U;
static const uint32_t kDiscoveryIntervalMs = 1200U;

static const uint8_t kBroadcastMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static uint8_t s_target_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static uint8_t s_tx_frame_buf[kMaxBatchBytes];
static uint16_t s_printable_array[kPrintableArrayCount];
static uint8_t s_medium_pattern[kMediumPayloadBytes];
static char s_string_buf[kMaxStringBytes];
static uint8_t s_raw_bytes[kRawPayloadBytes];
static uint16_t s_large_array[kLargeArrayCount];
static volatile bool s_tx_busy = false;
static uint32_t s_tx_round = 0;
static uint8_t s_last_case_id = 0U;
static uint32_t s_last_case_seq = 0U;
static size_t s_last_case_bytes = 0U;
static bool s_case_done[kTotalCases + 1] = {0};
static bool s_case_pass[kTotalCases + 1] = {0};
static bool s_summary_printed = false;
static bool s_matrix_done[kTotalQosRounds][kTotalCases + 1] = {0};
static bool s_matrix_pass[kTotalQosRounds][kTotalCases + 1] = {0};
static bool s_matrix_summary_printed = false;
static bool s_peer_discovered = false;
static uint32_t s_discovery_seq = 0U;
static int64_t s_last_discovery_req_ms = 0;
static uint8_t s_qos_round_idx = 0U;
static bool s_all_rounds_finished = false;

typedef struct __attribute__((packed))
{
    uint16_t magic;
    uint8_t version;
    uint8_t case_id;
    uint32_t seq;
    uint32_t payload_len;
    uint32_t checksum;
} test_frame_header_t;

enum
{
    TEST_CASE_PRINTABLE_ARRAY = 1,
    TEST_CASE_MEDIUM = 2,
    TEST_CASE_STRING = 3,
    TEST_CASE_STRUCT = 4,
    TEST_CASE_RAW_BYTES = 5,
    TEST_CASE_LARGE = 6,
};

typedef struct __attribute__((packed))
{
    uint32_t tick_s;
    float temperature_c;
    int16_t accel_x;
    int16_t accel_y;
    int16_t accel_z;
    uint8_t status_flags;
    char tag[8];
} demo_sensor_packet_t;

static demo_sensor_packet_t s_struct_packet;
static const node_espnow_qos_t kQosPlan[kTotalQosRounds] = {
    NODE_ESPNOW_QOS0,
    NODE_ESPNOW_QOS1,
    NODE_ESPNOW_QOS2,
};
static void on_tx_rx_batch(const uint8_t peer_mac[6], const node_espnow_rx_batch_t *batch, void *user_ctx);
static void tx_result_callback(const uint8_t peer_mac[6], const node_espnow_tx_result_t *result, void *user_ctx);

static const char *qos_name(node_espnow_qos_t qos)
{
    switch (qos)
    {
    case NODE_ESPNOW_QOS0:
        return "QOS0";
    case NODE_ESPNOW_QOS1:
        return "QOS1";
    case NODE_ESPNOW_QOS2:
        return "QOS2";
    default:
        return "UNKNOWN_QOS";
    }
}

static node_espnow_qos_t current_qos(void)
{
    return kQosPlan[s_qos_round_idx];
}

static const char *result_name(bool done, bool pass)
{
    if (!done)
    {
        return "NOT_RUN";
    }
    return pass ? "PASS" : "FAIL";
}

static const char *test_case_name(uint8_t case_id)
{
    switch (case_id)
    {
    case TEST_CASE_PRINTABLE_ARRAY:
        return "PRINTABLE_LONG_ARRAY";
    case TEST_CASE_MEDIUM:
        return "MEDIUM_MULTI_CHUNK";
    case TEST_CASE_STRING:
        return "STRING_TEXT";
    case TEST_CASE_STRUCT:
        return "STRUCT_PACKET";
    case TEST_CASE_RAW_BYTES:
        return "RAW_BINARY";
    case TEST_CASE_LARGE:
        return "LARGE_VECTOR_0_TO_4095";
    default:
        return "UNKNOWN";
    }
}

static bool is_broadcast_mac(const uint8_t mac[6])
{
    return mac != NULL &&
           mac[0] == 0xFF && mac[1] == 0xFF && mac[2] == 0xFF &&
           mac[3] == 0xFF && mac[4] == 0xFF && mac[5] == 0xFF;
}

static const char *test_case_goal(uint8_t case_id)
{
    switch (case_id)
    {
    case TEST_CASE_PRINTABLE_ARRAY:
        return "Verify array payload can be reconstructed and printed";
    case TEST_CASE_MEDIUM:
        return "Verify medium payload split/reassemble path";
    case TEST_CASE_STRING:
        return "Verify null-terminated string transmission";
    case TEST_CASE_STRUCT:
        return "Verify packed struct field integrity";
    case TEST_CASE_RAW_BYTES:
        return "Verify arbitrary binary blob transmission";
    case TEST_CASE_LARGE:
        return "Verify large payload multi-chunk reliability";
    default:
        return "Unknown goal";
    }
}

static bool all_cases_done(void)
{
    for (uint8_t i = 1U; i <= kTotalCases; i++)
    {
        if (!s_case_done[i])
        {
            return false;
        }
    }
    return true;
}

static uint8_t next_pending_case(void)
{
    for (uint8_t i = 1U; i <= kTotalCases; i++)
    {
        if (!s_case_done[i])
        {
            return i;
        }
    }
    return 0U;
}

static void log_tx_summary_once(void)
{
    if (s_summary_printed)
    {
        return;
    }

    ESP_LOGI(TAG, "========== TX TEST SUMMARY (%s) ==========", qos_name(current_qos()));
    for (uint8_t i = 1U; i <= kTotalCases; i++)
    {
        s_matrix_done[s_qos_round_idx][i] = s_case_done[i];
        s_matrix_pass[s_qos_round_idx][i] = s_case_pass[i];
        ESP_LOGI(TAG, "Stage-%u %-22s : %s",
                 (unsigned)i,
                 test_case_name(i),
                 s_case_pass[i] ? "PASS" : "FAIL");
    }
    ESP_LOGI(TAG, "TX one-shot test finished for %s.", qos_name(current_qos()));
    s_summary_printed = true;
}

static void log_tx_matrix_summary_once(void)
{
    if (s_matrix_summary_printed)
    {
        return;
    }

    ESP_LOGI(TAG, "========== TX FINAL MATRIX SUMMARY ==========");
    for (uint8_t q = 0U; q < kTotalQosRounds; q++)
    {
        ESP_LOGI(TAG, "-- %s --", qos_name(kQosPlan[q]));
        for (uint8_t c = 1U; c <= kTotalCases; c++)
        {
            ESP_LOGI(TAG, "Stage-%u %-22s : %s",
                     (unsigned)c,
                     test_case_name(c),
                     result_name(s_matrix_done[q][c], s_matrix_pass[q][c]));
        }
    }
    s_matrix_summary_printed = true;
}

static void reset_round_state(void)
{
    memset(s_case_done, 0, sizeof(s_case_done));
    memset(s_case_pass, 0, sizeof(s_case_pass));
    s_summary_printed = false;
    s_peer_discovered = false;
    memset(s_target_mac, 0xFF, sizeof(s_target_mac));
    s_tx_round = 0U;
    s_last_case_id = 0U;
    s_last_case_seq = 0U;
    s_last_case_bytes = 0U;
    s_discovery_seq = 0U;
    s_last_discovery_req_ms = 0;
}

static bool start_node_espnow_with_qos(node_espnow_qos_t qos)
{
    node_espnow_config_t cfg;
    node_espnow_default_config(&cfg);
    cfg.channel = 1;
    cfg.qos_default = qos;
    cfg.chunk_payload_bytes = 160;
    cfg.tx_window_size = 1;
    cfg.ack_timeout_ms = 1200;
    cfg.max_retries = 8;
    cfg.session_timeout_ms = 30000;
    cfg.max_batch_bytes = kMaxBatchBytes;

    node_espnow_handlers_t handlers;
    handlers.tx_result_cb = tx_result_callback;
    handlers.user_ctx = NULL;

    esp_err_t ret = node_espnow_init(&cfg, &handlers);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "node_espnow_init failed for %s: %s", qos_name(qos), esp_err_to_name(ret));
        return false;
    }

    ret = node_espnow_set_rx_batch_cb(on_tx_rx_batch, NULL);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "node_espnow_set_rx_batch_cb failed for %s: %s", qos_name(qos), esp_err_to_name(ret));
        node_espnow_deinit();
        return false;
    }

    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
    ESP_LOGI(TAG, "node_espnow started for %s", qos_name(qos));
    return true;
}

static bool switch_to_next_qos_round(void)
{
    if (s_qos_round_idx + 1U >= kTotalQosRounds)
    {
        s_all_rounds_finished = true;
        return false;
    }

    s_qos_round_idx++;
    ESP_LOGI(TAG, "Switching to next round: %s", qos_name(current_qos()));
    node_espnow_deinit();
    reset_round_state();
    if (!start_node_espnow_with_qos(current_qos()))
    {
        s_all_rounds_finished = true;
        return false;
    }
    return true;
}

static uint32_t calc_checksum32(const uint8_t *data, size_t len)
{
    uint32_t sum = 0U;
    for (size_t i = 0; i < len; i++)
    {
        sum += data[i];
    }
    return sum;
}

static void fill_pattern(uint8_t *data, size_t len, uint8_t case_id, uint32_t seq)
{
    for (size_t i = 0; i < len; i++)
    {
        data[i] = (uint8_t)((seq + case_id + (uint32_t)i) & 0xFFU);
    }
}

static uint16_t make_printable_value(uint32_t seq, uint32_t idx)
{
    return (uint16_t)((seq * 10U + idx) & 0xFFFFU);
}

static void fill_large_array(void)
{
    for (uint32_t i = 0; i < kLargeArrayCount; i++)
    {
        s_large_array[i] = (uint16_t)i;
    }
}

static bool prepare_case_object(uint8_t case_id, uint32_t seq, const void **out_obj_ptr, size_t *out_obj_len)
{
    if (out_obj_ptr == NULL || out_obj_len == NULL)
    {
        return false;
    }

    switch (case_id)
    {
    case TEST_CASE_PRINTABLE_ARRAY:
        for (uint32_t i = 0; i < kPrintableArrayCount; i++)
        {
            s_printable_array[i] = make_printable_value(seq, i);
        }
        *out_obj_ptr = s_printable_array;
        *out_obj_len = sizeof(s_printable_array);
        return true;
    case TEST_CASE_MEDIUM:
        fill_pattern(s_medium_pattern, sizeof(s_medium_pattern), case_id, seq);
        *out_obj_ptr = s_medium_pattern;
        *out_obj_len = sizeof(s_medium_pattern);
        return true;
    case TEST_CASE_STRING:
    {
        static const char *messages[] = {
            "hello from TX: ESPNOW string payload",
            "NexNode test: pointers send pointed bytes",
            "CASE_STRING: printable text over node_espnow",
        };
        const char *msg = messages[seq % (sizeof(messages) / sizeof(messages[0]))];
        size_t need = strlen(msg) + 1U;
        if (need > sizeof(s_string_buf))
        {
            return false;
        }
        memcpy(s_string_buf, msg, need);
        *out_obj_ptr = s_string_buf;
        *out_obj_len = need;
        return true;
    }
    case TEST_CASE_STRUCT:
        memset(&s_struct_packet, 0, sizeof(s_struct_packet));
        s_struct_packet.tick_s = seq * 3U;
        s_struct_packet.temperature_c = 23.5f + (float)(seq % 7U) * 0.25f;
        s_struct_packet.accel_x = (int16_t)(100 + (int16_t)seq);
        s_struct_packet.accel_y = (int16_t)(-50 - (int16_t)seq);
        s_struct_packet.accel_z = (int16_t)(1024 + (int16_t)(seq % 32U));
        s_struct_packet.status_flags = (uint8_t)(seq & 0x0FU);
        memcpy(s_struct_packet.tag, "NEXNODE", 8);
        *out_obj_ptr = &s_struct_packet;
        *out_obj_len = sizeof(s_struct_packet);
        return true;
    case TEST_CASE_RAW_BYTES:
        fill_pattern(s_raw_bytes, sizeof(s_raw_bytes), case_id, seq);
        *out_obj_ptr = s_raw_bytes;
        *out_obj_len = sizeof(s_raw_bytes);
        return true;
    case TEST_CASE_LARGE:
        *out_obj_ptr = s_large_array;
        *out_obj_len = sizeof(s_large_array);
        return true;
    default:
        return false;
    }
}

static bool build_case_frame(uint8_t case_id,
                             uint32_t seq,
                             const void *obj_ptr,
                             size_t obj_len,
                             uint8_t *frame,
                             size_t frame_cap,
                             size_t *out_len)
{
    if (obj_ptr == NULL || obj_len == 0U || frame == NULL || out_len == NULL || frame_cap < sizeof(test_frame_header_t))
    {
        return false;
    }

    if ((sizeof(test_frame_header_t) + obj_len) > frame_cap)
    {
        return false;
    }

    uint8_t *payload = frame + sizeof(test_frame_header_t);
    memcpy(payload, obj_ptr, obj_len);

    test_frame_header_t hdr;
    memset(&hdr, 0, sizeof(hdr));
    hdr.magic = kFrameMagic;
    hdr.version = kFrameVersion;
    hdr.case_id = case_id;
    hdr.seq = seq;
    hdr.payload_len = (uint32_t)obj_len;
    hdr.checksum = calc_checksum32(payload, obj_len);
    memcpy(frame, &hdr, sizeof(hdr));

    *out_len = sizeof(test_frame_header_t) + obj_len;
    return true;
}

static bool build_discovery_frame(uint8_t case_id, uint32_t seq, uint8_t *frame, size_t frame_cap, size_t *out_len)
{
    static const char *kDiscoveryReqText = "DISCOVERY_REQ";
    static const char *kDiscoveryRespText = "DISCOVERY_RESP";
    const char *payload = NULL;

    if (case_id == kCaseDiscoveryReq)
    {
        payload = kDiscoveryReqText;
    }
    else if (case_id == kCaseDiscoveryResp)
    {
        payload = kDiscoveryRespText;
    }
    else
    {
        return false;
    }

    return build_case_frame(case_id, seq, payload, strlen(payload) + 1U, frame, frame_cap, out_len);
}

static void log_local_mac(void)
{
    uint8_t local_mac[6] = {0};
    if (esp_wifi_get_mac(WIFI_IF_STA, local_mac) == ESP_OK)
    {
        ESP_LOGI(TAG, "Local MAC: %02X:%02X:%02X:%02X:%02X:%02X",
                 local_mac[0], local_mac[1], local_mac[2],
                 local_mac[3], local_mac[4], local_mac[5]);
    }
}

static void on_tx_rx_batch(const uint8_t peer_mac[6], const node_espnow_rx_batch_t *batch, void *user_ctx)
{
    (void)user_ctx;
    if (batch == NULL || batch->payload == NULL || batch->payload_len < sizeof(test_frame_header_t))
    {
        return;
    }

    test_frame_header_t hdr;
    memset(&hdr, 0, sizeof(hdr));
    memcpy(&hdr, batch->payload, sizeof(hdr));
    const uint8_t *payload = batch->payload + sizeof(hdr);
    size_t payload_len = batch->payload_len - sizeof(hdr);

    bool header_ok = (hdr.magic == kFrameMagic) &&
                     (hdr.version == kFrameVersion) &&
                     (hdr.payload_len == payload_len) &&
                     (hdr.checksum == calc_checksum32(payload, payload_len));
    if (!header_ok || hdr.case_id != kCaseDiscoveryResp)
    {
        return;
    }

    if (!s_peer_discovered)
    {
        memcpy(s_target_mac, peer_mac, 6);
        s_peer_discovered = true;
        ESP_LOGI(TAG,
                 "Discovery locked peer MAC: %02X:%02X:%02X:%02X:%02X:%02X (seq=%lu)",
                 s_target_mac[0], s_target_mac[1], s_target_mac[2],
                 s_target_mac[3], s_target_mac[4], s_target_mac[5],
                 (unsigned long)hdr.seq);
    }
}

static void tx_result_callback(const uint8_t peer_mac[6],
                               const node_espnow_tx_result_t *result,
                               void *user_ctx)
{
    (void)user_ctx;
    s_tx_busy = false;
    led_toggle();

    if (result->fail_reason == ESP_OK)
    {
        if (s_last_case_id >= 1U && s_last_case_id <= kTotalCases)
        {
            s_case_done[s_last_case_id] = true;
            s_case_pass[s_last_case_id] = true;
        }
        ESP_LOGI(TAG,
                 "TX PASS | qos=%s case=%s seq=%lu bytes=%lu | peer=%02X:%02X:%02X:%02X:%02X:%02X | transfer=%lu chunks=%u acked=%u duration=%lums",
                 qos_name(current_qos()),
                 test_case_name(s_last_case_id),
                 (unsigned long)s_last_case_seq,
                 (unsigned long)s_last_case_bytes,
                 peer_mac[0], peer_mac[1], peer_mac[2], peer_mac[3], peer_mac[4], peer_mac[5],
                 (unsigned long)result->transfer_id,
                 (unsigned)result->total_chunks,
                 (unsigned)result->acked_chunks,
                 (unsigned long)result->duration_ms);
    }
    else
    {
        if (s_last_case_id >= 1U && s_last_case_id <= kTotalCases)
        {
            s_case_done[s_last_case_id] = true;
            s_case_pass[s_last_case_id] = false;
        }
        ESP_LOGW(TAG,
                 "TX FAIL | qos=%s case=%s seq=%lu bytes=%lu | peer=%02X:%02X:%02X:%02X:%02X:%02X | transfer=%lu reason=%s acked=%u/%u attempts=%u",
                 qos_name(current_qos()),
                 test_case_name(s_last_case_id),
                 (unsigned long)s_last_case_seq,
                 (unsigned long)s_last_case_bytes,
                 peer_mac[0], peer_mac[1], peer_mac[2], peer_mac[3], peer_mac[4], peer_mac[5],
                 (unsigned long)result->transfer_id,
                 esp_err_to_name(result->fail_reason),
                 (unsigned)result->acked_chunks,
                 (unsigned)result->total_chunks,
                 (unsigned)result->tx_attempts);
    }
}

static void tx_test_task(void *pvParameters)
{
    (void)pvParameters;
    ESP_LOGI(TAG, "TX test task started");
    ESP_LOGI(TAG, "Discovery mode: broadcast request, then lock peer MAC for unicast tests");
    ESP_LOGI(TAG, "Test mode: matrix run across QOS0/QOS1/QOS2 (each case once per QoS)");
    for (uint8_t i = 1U; i <= kTotalCases; i++)
    {
        ESP_LOGI(TAG, "  Stage-%u %-22s | Goal: %s",
                 (unsigned)i,
                 test_case_name(i),
                 test_case_goal(i));
    }
    ESP_LOGI(TAG, "Target peer: %02X:%02X:%02X:%02X:%02X:%02X",
             s_target_mac[0], s_target_mac[1], s_target_mac[2],
             s_target_mac[3], s_target_mac[4], s_target_mac[5]);

    while (1)
    {
        if (s_all_rounds_finished)
        {
            log_tx_matrix_summary_once();
            ESP_LOGI(TAG, "All QoS rounds finished.");
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }

        if (!s_tx_busy)
        {
            if (!s_peer_discovered)
            {
                int64_t now_ms = esp_timer_get_time() / 1000;
                if ((now_ms - s_last_discovery_req_ms) >= (int64_t)kDiscoveryIntervalMs)
                {
                    size_t discover_len = 0U;
                    s_discovery_seq++;
                    if (build_discovery_frame(kCaseDiscoveryReq, s_discovery_seq, s_tx_frame_buf, sizeof(s_tx_frame_buf), &discover_len))
                    {
                        ESP_LOGI(TAG, "========== TX DISCOVERY ==========");
                        ESP_LOGI(TAG, "Broadcast DISCOVERY_REQ qos=%s seq=%lu bytes=%lu",
                                 qos_name(current_qos()),
                                 (unsigned long)s_discovery_seq,
                                 (unsigned long)discover_len);
                        esp_err_t ret = node_espnow_send_to(kBroadcastMac, s_tx_frame_buf, discover_len);
                        if (ret == ESP_OK)
                        {
                            s_tx_busy = true;
                            s_last_case_id = 0U;
                            s_last_case_seq = s_discovery_seq;
                            s_last_case_bytes = discover_len;
                            s_last_discovery_req_ms = now_ms;
                        }
                        else
                        {
                            ESP_LOGW(TAG, "DISCOVERY_REQ send failed: %s", esp_err_to_name(ret));
                        }
                    }
                }
                vTaskDelay(pdMS_TO_TICKS(200));
                continue;
            }

            if (all_cases_done())
            {
                log_tx_summary_once();
                if (!switch_to_next_qos_round())
                {
                    ESP_LOGI(TAG, "No next QoS round, TX matrix test completed.");
                }
                vTaskDelay(pdMS_TO_TICKS(1000));
                continue;
            }

            uint32_t case_seq = s_tx_round + 1U;
            uint8_t case_id = next_pending_case();
            const void *obj_ptr = NULL;
            size_t obj_len = 0U;
            size_t tx_len = 0U;

            if (!prepare_case_object(case_id, case_seq, &obj_ptr, &obj_len))
            {
                ESP_LOGW(TAG, "Prepare object failed: case=%s seq=%lu", test_case_name(case_id), (unsigned long)case_seq);
                vTaskDelay(pdMS_TO_TICKS(kTxPeriodMs));
                continue;
            }

            if (!build_case_frame(case_id, case_seq, obj_ptr, obj_len, s_tx_frame_buf, sizeof(s_tx_frame_buf), &tx_len))
            {
                ESP_LOGW(TAG, "Build case frame failed: case=%s seq=%lu", test_case_name(case_id), (unsigned long)case_seq);
                vTaskDelay(pdMS_TO_TICKS(kTxPeriodMs));
                continue;
            }

            ESP_LOGI(TAG, "========== TX STAGE %u/%u ==========", (unsigned)case_id, (unsigned)kTotalCases);
            ESP_LOGI(TAG, "QoS round: %s", qos_name(current_qos()));
            ESP_LOGI(TAG, "Case: %s", test_case_name(case_id));
            ESP_LOGI(TAG, "Goal: %s", test_case_goal(case_id));
            ESP_LOGI(TAG, "Plan: seq=%lu tx_bytes=%lu", (unsigned long)case_seq, (unsigned long)tx_len);
            ESP_LOGI(TAG, "Object pointer=%p object_bytes=%lu", obj_ptr, (unsigned long)obj_len);
            esp_err_t ret = node_espnow_send_to(s_target_mac, s_tx_frame_buf, tx_len);
            if (ret == ESP_OK)
            {
                s_tx_busy = true;
                s_tx_round = case_seq;
                s_last_case_id = case_id;
                s_last_case_seq = case_seq;
                s_last_case_bytes = tx_len;
                ESP_LOGI(TAG, "Queued OK. Waiting callback for case=%s seq=%lu", test_case_name(case_id), (unsigned long)case_seq);
            }
            else
            {
                ESP_LOGW(TAG, "Queue TX request failed for case=%s seq=%lu: %s",
                         test_case_name(case_id),
                         (unsigned long)case_seq,
                         esp_err_to_name(ret));
            }
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void app_main(void)
{
    esp_err_t ret;
    uint32_t flash_size;
    esp_chip_info_t chip_info;

    ESP_LOGI(TAG, "========== System Initialization ==========");
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    esp_flash_get_size(NULL, &flash_size);
    esp_chip_info(&chip_info);
    ESP_LOGI(TAG, "CPU cores: %d", chip_info.cores);
    ESP_LOGI(TAG, "Flash size: %ld MB", flash_size / (1024 * 1024));
    ESP_LOGI(TAG, "PSRAM size: %d bytes", esp_psram_get_size());

    ESP_LOGI(TAG, "========== Hardware Initialization ==========");
    led_init();
    exit_init();
    spi2_init();

    fill_large_array();

    ESP_LOGI(TAG, "========== node_espnow Initialization ==========");
    memset(s_matrix_done, 0, sizeof(s_matrix_done));
    memset(s_matrix_pass, 0, sizeof(s_matrix_pass));
    s_matrix_summary_printed = false;
    reset_round_state();
    s_qos_round_idx = 0U;
    if (!start_node_espnow_with_qos(current_qos()))
    {
        while (1)
        {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
    log_local_mac();
    xTaskCreate(tx_test_task, "tx_test_task", 4096, NULL, 5, NULL);
    ESP_LOGI(TAG, "TX app started. Initial target=%02X:%02X:%02X:%02X:%02X:%02X",
             s_target_mac[0], s_target_mac[1], s_target_mac[2],
             s_target_mac[3], s_target_mac[4], s_target_mac[5]);
    ESP_LOGI(TAG, "Will broadcast discovery first, then switch to unicast target MAC.");
    ESP_LOGI(TAG, "QoS plan: QOS0 -> QOS1 -> QOS2");
    ESP_LOGI(TAG, "Batch limit: %u bytes (binary-aligned)", (unsigned)kMaxBatchBytes);

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(5000));
        ESP_LOGI(TAG, "Heartbeat... qos=%s tx_busy=%d", qos_name(current_qos()), (int)s_tx_busy);
    }
}

#ifdef __cplusplus
}
#endif

```

Send API call in TX flow:

```cpp
esp_err_t ret = node_espnow_send_to(target_mac, tx_buffer, tx_len);
```

## RX main (`AIoTNode.cpp`)

```cpp
/**
 * @file AIoTNode.cpp
 * @brief RX application based on unified node_espnow library.
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_log.h"
#include "esp_psram.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "node_espnow.h"
#include "node_exit.h"
#include "node_led.h"
#include "node_rtc.h"
#include "node_sdcard.h"
#include "node_spi.h"
#include "node_timer.h"

#ifdef __cplusplus
extern "C" {
#endif

static const char *TAG = "ESP_NOW_RX_APP";
static const uint16_t kFrameMagic = 0x4E44U;
static const uint8_t kFrameVersion = 1U;
static const size_t kPrintableArrayCount = 128U;
static const size_t kPrintablePayloadBytes = kPrintableArrayCount * sizeof(uint16_t);
static const size_t kMediumPayloadBytes = 600U;
static const size_t kRawPayloadBytes = 300U;
static const size_t kLargeArrayCount = 4096U;
static const size_t kLargePayloadBytes = kLargeArrayCount * sizeof(uint16_t);
static const uint64_t kLargeExpectedSum = 8386560ULL;
static const size_t kMaxBatchBytes = 16384U;
static const uint8_t kTotalCases = 6U;
static const uint8_t kTotalQosRounds = 3U;
static const uint8_t kCaseDiscoveryReq = 0xF0U;
static const uint8_t kCaseDiscoveryResp = 0xF1U;

static uint32_t s_rx_total = 0;
static uint32_t s_rx_ok = 0;
static uint32_t s_rx_fail = 0;
static uint32_t s_rx_printable_ok = 0;
static uint32_t s_rx_medium_ok = 0;
static uint32_t s_rx_string_ok = 0;
static uint32_t s_rx_struct_ok = 0;
static uint32_t s_rx_raw_ok = 0;
static uint32_t s_rx_large_ok = 0;
static bool s_case_seen[kTotalCases + 1] = {0};
static bool s_case_pass[kTotalCases + 1] = {0};
static bool s_summary_printed = false;
static bool s_matrix_done[kTotalQosRounds][kTotalCases + 1] = {0};
static bool s_matrix_pass[kTotalQosRounds][kTotalCases + 1] = {0};
static bool s_matrix_summary_printed = false;
static uint8_t s_qos_round_idx = 0U;
static bool s_round_complete_pending = false;
static bool s_all_rounds_finished = false;

typedef struct __attribute__((packed))
{
    uint16_t magic;
    uint8_t version;
    uint8_t case_id;
    uint32_t seq;
    uint32_t payload_len;
    uint32_t checksum;
} test_frame_header_t;

enum
{
    TEST_CASE_PRINTABLE_ARRAY = 1,
    TEST_CASE_MEDIUM = 2,
    TEST_CASE_STRING = 3,
    TEST_CASE_STRUCT = 4,
    TEST_CASE_RAW_BYTES = 5,
    TEST_CASE_LARGE = 6,
};

typedef struct __attribute__((packed))
{
    uint32_t tick_s;
    float temperature_c;
    int16_t accel_x;
    int16_t accel_y;
    int16_t accel_z;
    uint8_t status_flags;
    char tag[8];
} demo_sensor_packet_t;
static const node_espnow_qos_t kQosPlan[kTotalQosRounds] = {
    NODE_ESPNOW_QOS0,
    NODE_ESPNOW_QOS1,
    NODE_ESPNOW_QOS2,
};

static const char *qos_name(node_espnow_qos_t qos)
{
    switch (qos)
    {
    case NODE_ESPNOW_QOS0:
        return "QOS0";
    case NODE_ESPNOW_QOS1:
        return "QOS1";
    case NODE_ESPNOW_QOS2:
        return "QOS2";
    default:
        return "UNKNOWN_QOS";
    }
}

static node_espnow_qos_t current_qos(void)
{
    return kQosPlan[s_qos_round_idx];
}

static const char *result_name(bool done, bool pass)
{
    if (!done)
    {
        return "NOT_RUN";
    }
    return pass ? "PASS" : "FAIL";
}

static const char *test_case_name(uint8_t case_id)
{
    switch (case_id)
    {
    case TEST_CASE_PRINTABLE_ARRAY:
        return "PRINTABLE_LONG_ARRAY";
    case TEST_CASE_MEDIUM:
        return "MEDIUM_MULTI_CHUNK";
    case TEST_CASE_STRING:
        return "STRING_TEXT";
    case TEST_CASE_STRUCT:
        return "STRUCT_PACKET";
    case TEST_CASE_RAW_BYTES:
        return "RAW_BINARY";
    case TEST_CASE_LARGE:
        return "LARGE_VECTOR_0_TO_4095";
    default:
        return "UNKNOWN";
    }
}

static const char *test_case_goal(uint8_t case_id)
{
    switch (case_id)
    {
    case TEST_CASE_PRINTABLE_ARRAY:
        return "Array payload can be reconstructed and printed";
    case TEST_CASE_MEDIUM:
        return "Medium payload split/reassemble path works";
    case TEST_CASE_STRING:
        return "Null-terminated string transmitted correctly";
    case TEST_CASE_STRUCT:
        return "Packed struct fields are preserved";
    case TEST_CASE_RAW_BYTES:
        return "Arbitrary binary payload is intact";
    case TEST_CASE_LARGE:
        return "Large payload multi-chunk transfer succeeds";
    default:
        return "Unknown goal";
    }
}

static bool all_cases_seen(void)
{
    for (uint8_t i = 1U; i <= kTotalCases; i++)
    {
        if (!s_case_seen[i])
        {
            return false;
        }
    }
    return true;
}

static void log_rx_summary_once(void)
{
    if (s_summary_printed)
    {
        return;
    }

    ESP_LOGI(TAG, "========== RX TEST SUMMARY (%s) ==========", qos_name(current_qos()));
    for (uint8_t i = 1U; i <= kTotalCases; i++)
    {
        s_matrix_done[s_qos_round_idx][i] = s_case_seen[i];
        s_matrix_pass[s_qos_round_idx][i] = s_case_pass[i];
        ESP_LOGI(TAG, "Stage-%u %-22s : %s",
                 (unsigned)i,
                 test_case_name(i),
                 s_case_pass[i] ? "PASS" : "FAIL");
    }
    ESP_LOGI(TAG, "RX one-shot verification finished for %s.", qos_name(current_qos()));
    s_summary_printed = true;
}

static void log_rx_matrix_summary_once(void)
{
    if (s_matrix_summary_printed)
    {
        return;
    }

    ESP_LOGI(TAG, "========== RX FINAL MATRIX SUMMARY ==========");
    for (uint8_t q = 0U; q < kTotalQosRounds; q++)
    {
        ESP_LOGI(TAG, "-- %s --", qos_name(kQosPlan[q]));
        for (uint8_t c = 1U; c <= kTotalCases; c++)
        {
            ESP_LOGI(TAG, "Stage-%u %-22s : %s",
                     (unsigned)c,
                     test_case_name(c),
                     result_name(s_matrix_done[q][c], s_matrix_pass[q][c]));
        }
    }
    s_matrix_summary_printed = true;
}

static void reset_round_state(void)
{
    s_rx_total = 0;
    s_rx_ok = 0;
    s_rx_fail = 0;
    s_rx_printable_ok = 0;
    s_rx_medium_ok = 0;
    s_rx_string_ok = 0;
    s_rx_struct_ok = 0;
    s_rx_raw_ok = 0;
    s_rx_large_ok = 0;
    memset(s_case_seen, 0, sizeof(s_case_seen));
    memset(s_case_pass, 0, sizeof(s_case_pass));
    s_summary_printed = false;
    s_round_complete_pending = false;
}

static uint32_t calc_checksum32(const uint8_t *data, size_t len)
{
    uint32_t sum = 0U;
    for (size_t i = 0; i < len; i++)
    {
        sum += data[i];
    }
    return sum;
}

static bool build_control_frame(uint8_t case_id, uint32_t seq, const char *text, uint8_t *frame, size_t frame_cap, size_t *out_len)
{
    if (text == NULL || frame == NULL || out_len == NULL || frame_cap < sizeof(test_frame_header_t))
    {
        return false;
    }

    size_t payload_len = strlen(text) + 1U;
    if ((sizeof(test_frame_header_t) + payload_len) > frame_cap)
    {
        return false;
    }

    uint8_t *payload = frame + sizeof(test_frame_header_t);
    memcpy(payload, text, payload_len);

    test_frame_header_t hdr;
    memset(&hdr, 0, sizeof(hdr));
    hdr.magic = kFrameMagic;
    hdr.version = kFrameVersion;
    hdr.case_id = case_id;
    hdr.seq = seq;
    hdr.payload_len = (uint32_t)payload_len;
    hdr.checksum = calc_checksum32(payload, payload_len);
    memcpy(frame, &hdr, sizeof(hdr));

    *out_len = sizeof(test_frame_header_t) + payload_len;
    return true;
}

static bool validate_pattern(const uint8_t *payload, size_t payload_len, uint8_t case_id, uint32_t seq)
{
    for (size_t i = 0; i < payload_len; i++)
    {
        uint8_t expected = (uint8_t)((seq + case_id + (uint32_t)i) & 0xFFU);
        if (payload[i] != expected)
        {
            return false;
        }
    }
    return true;
}

static const char *expected_string_for_seq(uint32_t seq)
{
    static const char *messages[] = {
        "hello from TX: ESPNOW string payload",
        "NexNode test: pointers send pointed bytes",
        "CASE_STRING: printable text over node_espnow",
    };
    return messages[seq % (sizeof(messages) / sizeof(messages[0]))];
}

static uint16_t make_printable_value(uint32_t seq, uint32_t idx)
{
    return (uint16_t)((seq * 10U + idx) & 0xFFFFU);
}

static bool validate_printable_array(const uint16_t *arr, size_t count, uint32_t seq)
{
    if (arr == NULL || count != kPrintableArrayCount)
    {
        return false;
    }
    for (size_t i = 0; i < count; i++)
    {
        if (arr[i] != make_printable_value(seq, i))
        {
            return false;
        }
    }
    return true;
}

static void log_printable_array(const uint16_t *arr, size_t count, uint32_t seq)
{
    ESP_LOGI(TAG, "Printable array content (seq=%lu, count=%u):",
             (unsigned long)seq,
             (unsigned)count);
    for (size_t i = 0; i < count; i += 16U)
    {
        size_t end = (i + 16U < count) ? (i + 16U) : count;
        char line[160];
        int pos = snprintf(line, sizeof(line), "[%03u..%03u] ",
                           (unsigned)i,
                           (unsigned)(end - 1U));
        for (size_t j = i; j < end && pos > 0 && (size_t)pos < sizeof(line); j++)
        {
            int wrote = snprintf(line + pos, sizeof(line) - (size_t)pos, "%u ", (unsigned)arr[j]);
            if (wrote <= 0)
            {
                break;
            }
            pos += wrote;
        }
        ESP_LOGI(TAG, "%s", line);
    }
}

static void log_printable_array_compare(const uint16_t *arr, size_t count, uint32_t seq)
{
    if (arr == NULL || count == 0U)
    {
        return;
    }

    size_t show = count < 8U ? count : 8U;
    char expected_line[192];
    char actual_line[192];
    int ep = snprintf(expected_line, sizeof(expected_line), "Expected first[%u]:", (unsigned)show);
    int ap = snprintf(actual_line, sizeof(actual_line), "Received first[%u]:", (unsigned)show);
    for (size_t i = 0; i < show; i++)
    {
        if (ep > 0 && (size_t)ep < sizeof(expected_line))
        {
            ep += snprintf(expected_line + ep, sizeof(expected_line) - (size_t)ep, " %u",
                           (unsigned)make_printable_value(seq, (uint32_t)i));
        }
        if (ap > 0 && (size_t)ap < sizeof(actual_line))
        {
            ap += snprintf(actual_line + ap, sizeof(actual_line) - (size_t)ap, " %u", (unsigned)arr[i]);
        }
    }
    ESP_LOGI(TAG, "%s", expected_line);
    ESP_LOGI(TAG, "%s", actual_line);
}

static bool validate_large_array(const uint8_t *payload, size_t payload_len)
{
    if (payload == NULL || payload_len != kLargePayloadBytes)
    {
        return false;
    }

    const uint16_t *vec = (const uint16_t *)payload;
    if (vec[0] != 0U || vec[kLargeArrayCount - 1U] != (uint16_t)(kLargeArrayCount - 1U))
    {
        return false;
    }

    uint64_t sum = 0U;
    for (size_t i = 0; i < kLargeArrayCount; i++)
    {
        sum += vec[i];
    }
    return sum == kLargeExpectedSum;
}

static bool validate_struct_case(const uint8_t *payload, size_t payload_len, uint32_t seq)
{
    if (payload == NULL || payload_len != sizeof(demo_sensor_packet_t))
    {
        return false;
    }

    demo_sensor_packet_t pkt;
    memcpy(&pkt, payload, sizeof(pkt));

    if (pkt.tick_s != (seq * 3U))
    {
        return false;
    }

    if (pkt.accel_x != (int16_t)(100 + (int16_t)seq) ||
        pkt.accel_y != (int16_t)(-50 - (int16_t)seq) ||
        pkt.accel_z != (int16_t)(1024 + (int16_t)(seq % 32U)) ||
        pkt.status_flags != (uint8_t)(seq & 0x0FU))
    {
        return false;
    }

    return memcmp(pkt.tag, "NEXNODE", 8) == 0;
}

static void log_struct_case(const uint8_t *payload)
{
    demo_sensor_packet_t pkt;
    memcpy(&pkt, payload, sizeof(pkt));
    ESP_LOGI(TAG,
             "Struct fields: tick_s=%lu temp=%.2f accel=[%d,%d,%d] status=0x%02X tag=%.8s",
             (unsigned long)pkt.tick_s,
             (double)pkt.temperature_c,
             (int)pkt.accel_x,
             (int)pkt.accel_y,
             (int)pkt.accel_z,
             (unsigned)pkt.status_flags,
             pkt.tag);
}

static void log_struct_case_compare(const uint8_t *payload, uint32_t seq)
{
    demo_sensor_packet_t actual;
    demo_sensor_packet_t expected;
    memcpy(&actual, payload, sizeof(actual));
    memset(&expected, 0, sizeof(expected));
    expected.tick_s = seq * 3U;
    expected.temperature_c = 23.5f + (float)(seq % 7U) * 0.25f;
    expected.accel_x = (int16_t)(100 + (int16_t)seq);
    expected.accel_y = (int16_t)(-50 - (int16_t)seq);
    expected.accel_z = (int16_t)(1024 + (int16_t)(seq % 32U));
    expected.status_flags = (uint8_t)(seq & 0x0FU);
    memcpy(expected.tag, "NEXNODE", 8);

    ESP_LOGI(TAG,
             "Expected struct: tick_s=%lu temp=%.2f accel=[%d,%d,%d] status=0x%02X tag=%.8s",
             (unsigned long)expected.tick_s,
             (double)expected.temperature_c,
             (int)expected.accel_x,
             (int)expected.accel_y,
             (int)expected.accel_z,
             (unsigned)expected.status_flags,
             expected.tag);
    ESP_LOGI(TAG,
             "Received struct: tick_s=%lu temp=%.2f accel=[%d,%d,%d] status=0x%02X tag=%.8s",
             (unsigned long)actual.tick_s,
             (double)actual.temperature_c,
             (int)actual.accel_x,
             (int)actual.accel_y,
             (int)actual.accel_z,
             (unsigned)actual.status_flags,
             actual.tag);
}

static bool validate_string_case(const uint8_t *payload, size_t payload_len)
{
    if (payload == NULL || payload_len == 0U)
    {
        return false;
    }
    return payload[payload_len - 1U] == '\0';
}

static void log_raw_preview(const uint8_t *payload, size_t payload_len)
{
    size_t show = payload_len < 16U ? payload_len : 16U;
    char line[128];
    int pos = snprintf(line, sizeof(line), "RAW preview:");
    for (size_t i = 0; i < show && pos > 0 && (size_t)pos < sizeof(line); i++)
    {
        int wrote = snprintf(line + pos, sizeof(line) - (size_t)pos, " %02X", payload[i]);
        if (wrote <= 0)
        {
            break;
        }
        pos += wrote;
    }
    ESP_LOGI(TAG, "%s", line);
}

static void log_pattern_compare_preview(const uint8_t *payload, size_t payload_len, uint8_t case_id, uint32_t seq, const char *label)
{
    size_t show = payload_len < 16U ? payload_len : 16U;
    char exp_line[192];
    char act_line[192];
    int ep = snprintf(exp_line, sizeof(exp_line), "%s expected[%u]:", label, (unsigned)show);
    int ap = snprintf(act_line, sizeof(act_line), "%s received[%u]:", label, (unsigned)show);
    for (size_t i = 0; i < show; i++)
    {
        uint8_t expected = (uint8_t)((seq + case_id + (uint32_t)i) & 0xFFU);
        if (ep > 0 && (size_t)ep < sizeof(exp_line))
        {
            ep += snprintf(exp_line + ep, sizeof(exp_line) - (size_t)ep, " %02X", expected);
        }
        if (ap > 0 && (size_t)ap < sizeof(act_line))
        {
            ap += snprintf(act_line + ap, sizeof(act_line) - (size_t)ap, " %02X", payload[i]);
        }
    }
    ESP_LOGI(TAG, "%s", exp_line);
    ESP_LOGI(TAG, "%s", act_line);
}

static void log_large_array_compare(const uint8_t *payload, size_t payload_len)
{
    if (payload == NULL || payload_len < sizeof(uint16_t))
    {
        return;
    }
    const uint16_t *vec = (const uint16_t *)payload;
    size_t count = payload_len / sizeof(uint16_t);
    uint64_t sum = 0U;
    for (size_t i = 0; i < count; i++)
    {
        sum += vec[i];
    }
    ESP_LOGI(TAG,
             "Large expected: first=0 last=%u sum=%llu bytes=%u",
             (unsigned)(kLargeArrayCount - 1U),
             (unsigned long long)kLargeExpectedSum,
             (unsigned)kLargePayloadBytes);
    ESP_LOGI(TAG,
             "Large received: first=%u last=%u sum=%llu bytes=%u",
             (unsigned)vec[0],
             (unsigned)vec[count - 1U],
             (unsigned long long)sum,
             (unsigned)payload_len);
}

static void log_local_mac(void)
{
    uint8_t local_mac[6] = {0};
    if (esp_wifi_get_mac(WIFI_IF_STA, local_mac) == ESP_OK)
    {
        ESP_LOGI(TAG, "Local MAC: %02X:%02X:%02X:%02X:%02X:%02X",
                 local_mac[0], local_mac[1], local_mac[2],
                 local_mac[3], local_mac[4], local_mac[5]);
    }
}

static void on_rx_batch(const uint8_t peer_mac[6], const node_espnow_rx_batch_t *batch, void *user_ctx)
{
    (void)user_ctx;
    s_rx_total++;

    bool valid = false;
    uint8_t case_id = 0U;
    uint32_t case_seq = 0U;
    size_t case_payload_len = 0U;

    if (batch->payload != NULL && batch->payload_len >= sizeof(test_frame_header_t))
    {
        test_frame_header_t hdr;
        memset(&hdr, 0, sizeof(hdr));
        memcpy(&hdr, batch->payload, sizeof(hdr));
        const uint8_t *case_payload = batch->payload + sizeof(hdr);
        size_t case_payload_bytes = batch->payload_len - sizeof(hdr);
        uint32_t checksum = calc_checksum32(case_payload, case_payload_bytes);

        case_id = hdr.case_id;
        case_seq = hdr.seq;
        case_payload_len = case_payload_bytes;
        valid = (hdr.magic == kFrameMagic) &&
                (hdr.version == kFrameVersion) &&
                (hdr.payload_len == case_payload_bytes) &&
                (hdr.checksum == checksum);

        if (valid && hdr.case_id == kCaseDiscoveryReq)
        {
            uint8_t reply[96];
            size_t reply_len = 0U;
            if (build_control_frame(kCaseDiscoveryResp, hdr.seq, "DISCOVERY_RESP", reply, sizeof(reply), &reply_len))
            {
                esp_err_t ret = node_espnow_send_to(peer_mac, reply, reply_len);
                ESP_LOGI(TAG,
                         "Discovery request from %02X:%02X:%02X:%02X:%02X:%02X seq=%lu -> reply %s",
                         peer_mac[0], peer_mac[1], peer_mac[2], peer_mac[3], peer_mac[4], peer_mac[5],
                         (unsigned long)hdr.seq,
                         ret == ESP_OK ? "queued" : esp_err_to_name(ret));
            }
            return;
        }

        if (valid && hdr.case_id == kCaseDiscoveryResp)
        {
            return;
        }

        if (valid && hdr.case_id == TEST_CASE_PRINTABLE_ARRAY)
        {
            log_printable_array_compare((const uint16_t *)case_payload,
                                        case_payload_bytes / sizeof(uint16_t),
                                        hdr.seq);
            valid = (case_payload_bytes == kPrintablePayloadBytes) &&
                    ((case_payload_bytes % sizeof(uint16_t)) == 0U) &&
                    validate_printable_array((const uint16_t *)case_payload,
                                             case_payload_bytes / sizeof(uint16_t),
                                             hdr.seq);
            if (valid)
            {
                s_rx_printable_ok++;
                log_printable_array((const uint16_t *)case_payload,
                                    case_payload_bytes / sizeof(uint16_t),
                                    hdr.seq);
            }
        }
        else if (valid && hdr.case_id == TEST_CASE_MEDIUM)
        {
            log_pattern_compare_preview(case_payload, case_payload_bytes, hdr.case_id, hdr.seq, "MEDIUM");
            valid = (case_payload_bytes == kMediumPayloadBytes) &&
                    validate_pattern(case_payload, case_payload_bytes, hdr.case_id, hdr.seq);
            if (valid)
            {
                s_rx_medium_ok++;
            }
        }
        else if (valid && hdr.case_id == TEST_CASE_STRING)
        {
            const char *expected = expected_string_for_seq(hdr.seq);
            ESP_LOGI(TAG, "Expected string: %s", expected);
            ESP_LOGI(TAG, "Received string: %s", (const char *)case_payload);
            valid = validate_string_case(case_payload, case_payload_bytes);
            valid = valid && (strcmp((const char *)case_payload, expected) == 0);
            if (valid)
            {
                s_rx_string_ok++;
            }
        }
        else if (valid && hdr.case_id == TEST_CASE_STRUCT)
        {
            log_struct_case_compare(case_payload, hdr.seq);
            valid = validate_struct_case(case_payload, case_payload_bytes, hdr.seq);
            if (valid)
            {
                s_rx_struct_ok++;
                log_struct_case(case_payload);
            }
        }
        else if (valid && hdr.case_id == TEST_CASE_RAW_BYTES)
        {
            log_pattern_compare_preview(case_payload, case_payload_bytes, hdr.case_id, hdr.seq, "RAW");
            valid = (case_payload_bytes == kRawPayloadBytes) &&
                    validate_pattern(case_payload, case_payload_bytes, hdr.case_id, hdr.seq);
            if (valid)
            {
                s_rx_raw_ok++;
                log_raw_preview(case_payload, case_payload_bytes);
            }
        }
        else if (valid && hdr.case_id == TEST_CASE_LARGE)
        {
            log_large_array_compare(case_payload, case_payload_bytes);
            valid = validate_large_array(case_payload, case_payload_bytes);
            if (valid)
            {
                s_rx_large_ok++;
            }
        }
        else
        {
            valid = false;
        }
    }

    if (valid)
    {
        s_rx_ok++;
        led_toggle();
    }
    else
    {
        s_rx_fail++;
    }

    ESP_LOGI(TAG,
             "RX %s | qos=%s case=%s seq=%lu payload_bytes=%lu | from=%02X:%02X:%02X:%02X:%02X:%02X transfer=%lu chunks=%u/%u duration=%lums",
             valid ? "PASS" : "FAIL",
             qos_name(current_qos()),
             test_case_name(case_id),
             (unsigned long)case_seq,
             (unsigned long)case_payload_len,
             peer_mac[0], peer_mac[1], peer_mac[2], peer_mac[3], peer_mac[4], peer_mac[5],
             (unsigned long)batch->transfer_id,
             (unsigned)batch->received_chunks,
             (unsigned)batch->total_chunks,
             (unsigned long)batch->duration_ms);

    if (case_id >= 1U && case_id <= kTotalCases && !s_case_seen[case_id])
    {
        s_case_seen[case_id] = true;
        s_case_pass[case_id] = valid;
        ESP_LOGI(TAG, "========== RX STAGE %u/%u ==========", (unsigned)case_id, (unsigned)kTotalCases);
        ESP_LOGI(TAG, "QoS round: %s", qos_name(current_qos()));
        ESP_LOGI(TAG, "Case: %s", test_case_name(case_id));
        ESP_LOGI(TAG, "Goal: %s", test_case_goal(case_id));
        ESP_LOGI(TAG, "Result: %s", valid ? "PASS" : "FAIL");
    }
    else if (case_id >= 1U && case_id <= kTotalCases)
    {
        ESP_LOGI(TAG, "Duplicate case data ignored for stage summary: case=%s", test_case_name(case_id));
    }

    if (all_cases_seen())
    {
        log_rx_summary_once();
        s_round_complete_pending = true;
    }

    if (!valid && case_id == TEST_CASE_LARGE && batch->payload_len >= sizeof(test_frame_header_t))
    {
        const uint8_t *raw = batch->payload + sizeof(test_frame_header_t);
        const uint16_t *vec = (const uint16_t *)raw;
        size_t count = (batch->payload_len - sizeof(test_frame_header_t)) / sizeof(uint16_t);
        ESP_LOGW(TAG, "Vector check fail: len=%u, first=%u, last=%u, expected_last=%u, expected_sum=%llu",
                 (unsigned)(batch->payload_len - sizeof(test_frame_header_t)),
                 (unsigned)vec[0],
                 (unsigned)vec[count - 1U],
                 (unsigned)(kLargeArrayCount - 1U),
                 (unsigned long long)kLargeExpectedSum);
    }
}

static bool start_node_espnow_with_qos(node_espnow_qos_t qos)
{
    node_espnow_config_t cfg;
    node_espnow_default_config(&cfg);
    cfg.channel = 1;
    cfg.qos_default = qos;
    cfg.chunk_payload_bytes = 160;
    cfg.tx_window_size = 1;
    cfg.ack_timeout_ms = 1200;
    cfg.max_retries = 8;
    cfg.session_timeout_ms = 30000;
    cfg.max_batch_bytes = kMaxBatchBytes;

    node_espnow_handlers_t handlers = {
        .tx_result_cb = NULL,
        .user_ctx = NULL,
    };

    esp_err_t ret = node_espnow_init(&cfg, &handlers);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "node_espnow_init failed for %s: %s", qos_name(qos), esp_err_to_name(ret));
        return false;
    }

    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));

    ret = node_espnow_set_rx_batch_cb(on_rx_batch, NULL);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "node_espnow_set_rx_batch_cb failed for %s: %s", qos_name(qos), esp_err_to_name(ret));
        node_espnow_deinit();
        return false;
    }

    ESP_LOGI(TAG, "node_espnow started for %s", qos_name(qos));
    return true;
}

static bool switch_to_next_qos_round(void)
{
    if (s_qos_round_idx + 1U >= kTotalQosRounds)
    {
        s_all_rounds_finished = true;
        return false;
    }

    s_qos_round_idx++;
    ESP_LOGI(TAG, "Switching to next round: %s", qos_name(current_qos()));
    node_espnow_deinit();
    reset_round_state();
    if (!start_node_espnow_with_qos(current_qos()))
    {
        s_all_rounds_finished = true;
        return false;
    }
    return true;
}

void app_main(void)
{
    esp_err_t ret;
    uint32_t flash_size;
    esp_chip_info_t chip_info;

    ESP_LOGI(TAG, "========== System Initialization ==========");
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    esp_flash_get_size(NULL, &flash_size);
    esp_chip_info(&chip_info);
    ESP_LOGI(TAG, "CPU cores: %d", chip_info.cores);
    ESP_LOGI(TAG, "Flash size: %ld MB", flash_size / (1024 * 1024));
    ESP_LOGI(TAG, "PSRAM size: %d bytes", esp_psram_get_size());

    ESP_LOGI(TAG, "========== Hardware Initialization ==========");
    led_init();
    exit_init();
    spi2_init();

    ESP_LOGI(TAG, "========== node_espnow Initialization ==========");
    memset(s_matrix_done, 0, sizeof(s_matrix_done));
    memset(s_matrix_pass, 0, sizeof(s_matrix_pass));
    s_matrix_summary_printed = false;
    reset_round_state();
    s_qos_round_idx = 0U;
    if (!start_node_espnow_with_qos(current_qos()))
    {
        while (1)
        {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    log_local_mac();
    ESP_LOGI(TAG, "RX app started. Waiting application test frames...");
    ESP_LOGI(TAG, "QoS plan: QOS0 -> QOS1 -> QOS2");
    ESP_LOGI(TAG, "RX discovery mode: reply DISCOVERY_REQ with DISCOVERY_RESP (unicast).");
    ESP_LOGI(TAG, "Validation mode: one-shot structured verification (each case once)");
    for (uint8_t i = 1U; i <= kTotalCases; i++)
    {
        ESP_LOGI(TAG, "  Stage-%u %-22s | Goal: %s",
                 (unsigned)i,
                 test_case_name(i),
                 test_case_goal(i));
    }
    ESP_LOGI(TAG, "Large-vector spec: bytes=%u, first=0, last=%u, sum=%llu",
             (unsigned)kLargePayloadBytes,
             (unsigned)(kLargeArrayCount - 1U),
             (unsigned long long)kLargeExpectedSum);
    while (1)
    {
        if (s_round_complete_pending && !s_all_rounds_finished)
        {
            s_round_complete_pending = false;
            if (!switch_to_next_qos_round())
            {
                ESP_LOGI(TAG, "No next QoS round, RX matrix test completed.");
            }
        }
        else if (s_all_rounds_finished)
        {
            log_rx_matrix_summary_once();
        }

        vTaskDelay(pdMS_TO_TICKS(5000));
        ESP_LOGI(TAG, "Heartbeat... qos=%s rx_total=%lu rx_ok=%lu rx_fail=%lu [print=%lu medium=%lu string=%lu struct=%lu raw=%lu large=%lu]",
                 qos_name(current_qos()),
                 (unsigned long)s_rx_total,
                 (unsigned long)s_rx_ok,
                 (unsigned long)s_rx_fail,
                 (unsigned long)s_rx_printable_ok,
                 (unsigned long)s_rx_medium_ok,
                 (unsigned long)s_rx_string_ok,
                 (unsigned long)s_rx_struct_ok,
                 (unsigned long)s_rx_raw_ok,
                 (unsigned long)s_rx_large_ok);
    }
}

#ifdef __cplusplus
}
#endif
```

## Main Communication Logic in These Projects

1. TX sends discovery request via broadcast.
2. RX replies discovery response via unicast.
3. TX locks target MAC.
4. TX runs staged payload transmission.
5. RX validates payload by case and logs summary.
6. Both sides run QoS matrix rounds (`QOS0`, `QOS1`, `QOS2`).

## Notes

- `node_espnow_send_to()` copies payload internally, so caller buffer can be reused after return.
- Worker task is the state-owner for TX/RX sessions; ISR callbacks only enqueue events.
- For large payloads, tune:
  - `chunk_payload_bytes`
  - `tx_window_size`
  - `ack_timeout_ms`
  - `max_retries`
  - `session_timeout_ms`

## Quick Start Guide (Usage-First)

This section is a usage-oriented summary for readers who want to run the communication stack quickly without reading all source details first.

### 1) Minimal bring-up order

Use this fixed order on both TX and RX:

1. Initialize NVS and board peripherals.
2. Build `node_espnow_config_t` from `node_espnow_default_config`.
3. Set channel/QoS and reliability parameters.
4. Call `node_espnow_init(...)`.
5. Register RX complete callback (`node_espnow_set_rx_batch_cb`).
6. TX side starts sending with `node_espnow_send_to(...)`.

If this order is changed (for example, send before callback registration), troubleshooting becomes harder.

### 2) Minimal sender recipe

Sender side practical sequence:

1. Optionally run discovery by broadcast.
2. Lock peer MAC from RX response.
3. Build one logical payload (`buffer + length`).
4. Call `node_espnow_send_to(peer_mac, buffer, length)`.
5. Wait for TX result callback.
6. On timeout/failure, adjust ACK/retry/chunk settings before expanding application logic.

Important: only one active TX session is allowed by the current implementation; parallel sends return `ESP_ERR_INVALID_STATE`.

### 3) Minimal receiver recipe

Receiver side practical sequence:

1. Start `node_espnow` with same channel and compatible QoS profile.
2. Register `node_espnow_set_rx_batch_cb(...)`.
3. In callback:
   - parse your application header
   - validate payload length/checksum
   - process business payload
4. Keep callback lightweight; hand off heavy work to your own task if needed.

The callback receives fully reassembled batch payload, not individual chunks.

### 4) Parameter selection cheat sheet

| Situation | Recommended direction |
|----------|------------------------|
| Clean link, short payload | Larger `chunk_payload_bytes`, lower retries, lower timeout. |
| Noisy link or distance increase | Smaller `chunk_payload_bytes`, higher `ack_timeout_ms`, higher `max_retries`. |
| High latency spikes | Keep `tx_window_size` low and increase `session_timeout_ms`. |
| Memory-limited node | Lower `max_batch_bytes`, avoid too many concurrent RX sessions. |
| Throughput-first test | Increase window carefully, then observe fail ratio. |

Start from the project baseline used in `AIoTNode.cpp` (`chunk=160`, `window=1`, `ack=1200ms`, `retries=8`) and tune one parameter at a time.

### 5) Log reading map

Fast interpretation of common logs:

- `TX ... start bytes=... chunks=...`  
  TX session opened and chunk planning completed.
- `retry=...`  
  ACK timeout occurred; sender restarted from START for pending chunks.
- `RX transfer=... completed ...`  
  RX reassembly is complete; callback delivery is expected.
- `drop ...` warnings  
  Frame validation failed (header mismatch, payload mismatch, bounds issue, or unknown type).

If TX logs keep retrying and RX has no valid completion logs, first verify channel and peer MAC lock flow.

### 6) Common failure patterns and fixes

1. `node_espnow_init failed`  
   - Check Wi-Fi/ESP-NOW init order and dependency components.
2. `ESP_ERR_INVALID_STATE` from `node_espnow_send_to`  
   - Previous TX session still active; wait callback or adjust application pacing.
3. Frequent timeout with large payloads  
   - Reduce `chunk_payload_bytes`, increase `ack_timeout_ms`, keep window at 1 first.
4. Discovery works but data stage fails  
   - Check whether target MAC is updated after discovery and whether both sides keep same channel.
5. RX receives but validation fails  
   - Re-check application-level frame format, checksum scope, and packed struct layout.

### 7) Practical progression for new users

For fastest onboarding, validate in this order:

1. Discovery only.
2. One tiny payload (string).
3. Medium payload (multi-chunk).
4. Large payload (stress path).
5. Repeat steps 2-4 across `QOS0`, `QOS1`, `QOS2`.

This progression matches how the TX/RX demo apps are structured, and it isolates failures early.
