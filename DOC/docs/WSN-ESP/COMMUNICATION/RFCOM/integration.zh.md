# 集成

## 介绍

本节说明如何将 RFCOM 通信组件（`node_espnow`）集成到 ESP-IDF 项目中，写法与以下参考工程一致：

- `CODE/AIoTNode-ESPNOW-TX-BATCH`
- `CODE/AIoTNode-ESPNOW-RX-BATCH`

## 创建新组件

!!! warning
    在 `driver` 下创建组件前，请确认项目级 `CMakeLists.txt` 已将 `driver` 目录加入 `EXTRA_COMPONENT_DIRS`。

在 VSCode 终端执行：

```bash
get_idf
idf.py -C driver create-component node_espnow
```

然后补齐：

- `driver/node_espnow/include/node_espnow.h`
- `driver/node_espnow/node_espnow.c`
- `driver/node_espnow/CMakeLists.txt`

## 替换组件代码

将模板文件替换为参考工程中的 RFCOM 实现：

- 头文件/API：`driver/node_espnow/include/node_espnow.h`
- 核心实现：`driver/node_espnow/node_espnow.c`
- 构建文件：`driver/node_espnow/CMakeLists.txt`

组件依赖：

- `esp_wifi`
- `esp_event`
- `esp_netif`
- `esp_timer`

## 项目层集成

在 `main/AIoTNode.cpp` 中：

1. 引入 `node_espnow.h`。
2. 从默认配置构造 `node_espnow_config_t`。
3. 按链路情况调整参数。
4. 调用 `node_espnow_init(...)`。
5. 通过 `node_espnow_set_rx_batch_cb(...)` 注册接收完成回调。
6. 使用 `node_espnow_send_to(...)` 发送批量数据。

最小启动骨架（TX/RX 通用）：

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

## TX/RX 联调部署方式

使用两块板：

- A 板烧录 TX 程序（`AIoTNode-ESPNOW-TX-BATCH`）
- B 板烧录 RX 程序（`AIoTNode-ESPNOW-RX-BATCH`）

流程：

1. RX 等待并响应发现请求。
2. TX 广播发现并锁定响应端 MAC。
3. TX 进入阶段化 payload 发送。
4. RX 按 case 校验并输出结果。
5. 双端执行 QoS 轮次测试。

## 编译与烧录

在每个工程目录执行：

```bash
idf.py build flash monitor
```

## 验证清单

1. 双端启动后能打印本机 MAC。
2. 日志中出现发现请求/响应。
3. TX 日志出现排队成功与传输完成信息。
4. RX 日志出现各 case 的 PASS/FAIL。
5. 最终输出 `QOS0`、`QOS1`、`QOS2` 的矩阵汇总。

## 常用调参建议

- 丢包偏高时：
  - 增大 `ack_timeout_ms`
  - 增大 `max_retries`
  - 降低 `chunk_payload_bytes`
  - 降低 `tx_window_size`
- 内存压力较大时：
  - 降低 `max_batch_bytes`
  - 减少并发 RX 会话
- 确保 TX/RX 使用同一信道（`cfg.channel`）。
