/**
 * @file node_rgb.c
 * @brief SK6812MINI-C via espressif/led_strip (RMT); no RTOS usage inside this module.
 */

#include "node_rgb.h"
#include "led_strip.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "node_rgb";

static led_strip_handle_t s_strip = NULL;
static int s_num_leds = 1;
static uint8_t s_brightness = 96;
static uint32_t s_chase_pos = 0;
static uint8_t s_single_phase = 0;
static bool s_inited = false;

static inline uint8_t scale(uint8_t c)
{
    return (uint8_t)(((uint16_t)c * s_brightness) / 255);
}

static void apply_fill(uint8_t r, uint8_t g, uint8_t b)
{
    if (!s_strip) {
        return;
    }
    r = scale(r);
    g = scale(g);
    b = scale(b);
    for (int i = 0; i < s_num_leds; i++) {
        led_strip_set_pixel(s_strip, (uint32_t)i, r, g, b);
    }
    led_strip_refresh(s_strip);
}

esp_err_t node_rgb_init(const node_rgb_config_t *cfg)
{
    if (s_inited) {
        return ESP_OK;
    }
    if (!cfg || cfg->gpio_num < 0) {
        return ESP_ERR_INVALID_ARG;
    }

    s_num_leds = cfg->num_leds;
    if (s_num_leds < 1) {
        s_num_leds = 1;
    }
    if (s_num_leds > 256) {
        s_num_leds = 256;
    }
    s_brightness = cfg->brightness ? cfg->brightness : 96;

    led_strip_config_t strip_cfg = {
        .strip_gpio_num = cfg->gpio_num,
        .max_leds = (uint32_t)s_num_leds,
        .led_pixel_format = LED_PIXEL_FORMAT_GRB,
        .led_model = LED_MODEL_SK6812,
        .flags.invert_out = false,
    };

    led_strip_rmt_config_t rmt_cfg = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000,
        .flags.with_dma = false,
    };

    esp_err_t err = led_strip_new_rmt_device(&strip_cfg, &rmt_cfg, &s_strip);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "led_strip_new_rmt_device: %s", esp_err_to_name(err));
        return err;
    }

    led_strip_clear(s_strip);
    s_chase_pos = 0;
    s_single_phase = 0;
    s_inited = true;
    ESP_LOGI(TAG, "SK6812 GPIO%d x %d, brightness %u", cfg->gpio_num, s_num_leds, (unsigned)s_brightness);
    return ESP_OK;
}

void node_rgb_deinit(void)
{
    if (s_strip) {
        led_strip_clear(s_strip);
        led_strip_del(s_strip);
        s_strip = NULL;
    }
    s_inited = false;
}

bool node_rgb_is_initialized(void)
{
    return s_inited;
}

void node_rgb_set_brightness(uint8_t v)
{
    s_brightness = v;
}

void node_rgb_fill(uint8_t r, uint8_t g, uint8_t b)
{
    apply_fill(r, g, b);
}

void node_rgb_clear(void)
{
    if (s_strip) {
        led_strip_clear(s_strip);
    }
}

void node_rgb_chase_reset(void)
{
    s_chase_pos = 0;
    s_single_phase = 0;
}

void node_rgb_chase_step(uint8_t r, uint8_t g, uint8_t b)
{
    if (!s_strip) {
        return;
    }

    if (s_num_leds <= 1) {
        uint8_t rr = 0, gg = 0, bb = 0;
        switch (s_single_phase % 3) {
            case 0:
                rr = r;
                break;
            case 1:
                gg = g;
                break;
            default:
                bb = b;
                break;
        }
        s_single_phase++;
        apply_fill(rr, gg, bb);
        return;
    }

    uint8_t R = scale(r);
    uint8_t G = scale(g);
    uint8_t B = scale(b);

    for (int i = 0; i < s_num_leds; i++) {
        led_strip_set_pixel(s_strip, (uint32_t)i, 0, 0, 0);
    }

    int idx = (int)(s_chase_pos % (uint32_t)s_num_leds);
    led_strip_set_pixel(s_strip, (uint32_t)idx, R, G, B);

    int t1 = (idx - 1 + s_num_leds) % s_num_leds;
    int t2 = (idx - 2 + s_num_leds) % s_num_leds;
    led_strip_set_pixel(s_strip, (uint32_t)t1, R / 4, G / 4, B / 4);
    led_strip_set_pixel(s_strip, (uint32_t)t2, R / 16, G / 16, B / 16);

    s_chase_pos++;
    led_strip_refresh(s_strip);
}
