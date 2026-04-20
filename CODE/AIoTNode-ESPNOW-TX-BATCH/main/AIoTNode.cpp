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
