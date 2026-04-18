# RGBLED 笔记

## 介绍

!!! note
    本章讲解基于 node_rgb 组件的 RGB 灯驱动与控制。当前实现面向 SK6812MINI-C（SK6812 类）单总线 RGB，默认控制引脚为 IO3，通过修改配置同样可用于其他开发板。

## RGBLED灯

![LED](LED.png){ width=800px }

## 电路示意图

![LED_CIRCUIT](LED_CIRCUIT.png){ width=800px }

如图所示，控制LED的GPIO是IO3。

## 效果图

![DEMO](DEMO.jpg){ width=800px }

## 依赖关系

RGB 相关功能依赖 ESP-IDF 组件（RMT + LED Strip 驱动链路）。在组件的 CMakeLists.txt 中至少需要声明：

- driver
- esp_driver_gpio

无需额外安装第三方库，但要确保工程启用了 ESP-IDF 自带的 led_strip 能力。

## 关键函数

| 函数原型 | 说明 | 示例 |
| --- | --- | --- |
| `esp_err_t node_rgb_init(const node_rgb_config_t *cfg)` | 初始化 RGB 设备，支持设置 GPIO、灯珠数量、亮度 | `node_rgb_config_t cfg = NODE_RGB_CONFIG_DEFAULT(); node_rgb_init(&cfg);` |
| `void node_rgb_deinit(void)` | 释放 RGB 设备并清空显示 | `node_rgb_deinit();` |
| `void node_rgb_set_brightness(uint8_t v)` | 设置亮度（0-255） | `node_rgb_set_brightness(128);` |
| `void node_rgb_rgb(uint8_t r, uint8_t g, uint8_t b)` | 直接设置 RGB 颜色并刷新 | `node_rgb_rgb(255, 64, 0);` |
| `esp_err_t node_rgb_str(const char *s)` | 按颜色名称设置颜色（英文不区分大小写，支持中文别名） | `node_rgb_str("cyan");` |
| `void node_rgb_clear(void)` | 熄灭所有灯珠 | `node_rgb_clear();` |
| `void node_rgb_chase_step(uint8_t r, uint8_t g, uint8_t b)` | 跑马灯步进：多灯时移动光点，单灯时按 R→G→B 循环 | `node_rgb_chase_step(255, 0, 0);` |
| `void node_rgb_chase_reset(void)` | 重置跑马灯状态 | `node_rgb_chase_reset();` |

## 默认配置

- 默认 GPIO：IO3（NODE_RGB_GPIO_DEFAULT）
- 默认灯珠数量：1
- 默认亮度：96
- 灯珠数量限制：1 到 256（超出范围会被自动钳制）

## 颜色字符串说明

node_rgb_str() 支持常用颜色名称，例如：

- 英文：red、green、blue、yellow、cyan、magenta、purple、orange、pink、white、black、off
- 中文：红、绿、蓝、黄、青、品红、紫、橙、粉、白、黑、关

输入会自动去除首尾空白；英文颜色名大小写不敏感。

## 返回值建议

- ESP_OK：执行成功
- ESP_ERR_INVALID_ARG：参数非法（如空字符串、配置异常）
- ESP_ERR_INVALID_STATE：设备尚未初始化
- ESP_ERR_NOT_FOUND：颜色字符串未匹配

## 使用建议

- node_rgb 组件本身不包含 FreeRTOS 延时与任务逻辑，动画节拍建议在应用层使用 vTaskDelay 控制。
- 单灯演示可使用颜色名数组轮播（如 red -> orange -> yellow -> ...），与 main.cpp 示例一致。
