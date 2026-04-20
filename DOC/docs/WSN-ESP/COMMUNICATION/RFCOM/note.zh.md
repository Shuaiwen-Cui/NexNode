# RFCOM 节点间通信（ESP-NOW 批量传输）

## 介绍

本节文档说明仓库中用于节点间数据通信的 RFCOM 模块。

实现参考：

- 发送端项目：`CODE/AIoTNode-ESPNOW-TX-BATCH`
- 接收端项目：`CODE/AIoTNode-ESPNOW-RX-BATCH`
- 核心库：`driver/node_espnow`

该通信引擎基于 ESP-NOW，在单帧之上实现了分帧、分块、重组与回调交付，面向中大负载批量数据传输。

## 架构

整体分两层：

1. `node_espnow` 库层  
   负责帧协议、分块发送、ACK、重试、会话状态管理与完成回调。
2. 应用层（`main/AIoTNode.cpp`）  
   负责构造业务/测试负载、选择 QoS 配置、处理发送结果与接收验证。

概念流程：

```plaintext
应用负载
   -> START 帧
   -> DATA 帧 0..N-1
   -> END 帧
   -> 接收端重组 + ACK
   -> 完整批次回调
```

## `node_espnow` 协议模型

库中定义 4 种帧类型：

- `NODE_FRAME_START`
- `NODE_FRAME_DATA`
- `NODE_FRAME_END`
- `NODE_FRAME_ACK`

每帧包含头字段，例如：

- `transfer_id`
- `total_len`
- `chunk_idx`
- `chunk_total`
- `payload_len`
- `flags`

该模型可稳定承载超过单帧长度的逻辑消息。

## QoS 模式

库支持三种 QoS：

- `NODE_ESPNOW_QOS0`  
  近似无确认发送，发送完数据与结束帧后完成会话。
- `NODE_ESPNOW_QOS1`  
  基于 ACK 的分块可靠传输。
- `NODE_ESPNOW_QOS2`  
  在接收侧增加已完成传输缓存，抑制重复 START 导致的重复交付。

## 关键运行行为

1. 发送侧
   - 按 `chunk_payload_bytes` 对负载分块
   - 用 `tx_window_size` 控制窗口
   - 按 `ack_timeout_ms` 与 `max_retries` 做重试
2. 接收侧
   - 以 peer + `transfer_id` 维护重组会话
   - 用位图跟踪 chunk 到达
   - 满足 END + 全部分块后触发完整回调
3. 会话安全
   - 按 `session_timeout_ms` 清理超时会话
   - 自动释放 TX/RX 动态内存

## TX/RX 主程序中的实现方式

两个 `AIoTNode.cpp` 都遵循同一模式：

1. 系统与硬件初始化。
2. 以默认配置构造 `node_espnow_config_t`。
3. 配置分块、窗口、超时、重试、最大批量。
4. 调用 `node_espnow_init(...)`。
5. 注册接收完整批次回调。
6. 进入周期通信逻辑。

发送端工程还实现了：

- 先广播发现请求，再锁定目标 MAC 的单播传输。
- 多类型载荷阶段验证（数组、中等负载、字符串、结构体、原始字节、大向量）。
- `QOS0 -> QOS1 -> QOS2` 轮次化测试。

接收端工程还实现了：

- 发现响应。
- 按 case 做数据校验与矩阵汇总。

## 适用场景

RFCOM 模块适用于：

- 节点间结构化遥测数据交换
- 采样数组的批量传输
- 噪声链路下的可靠二进制数据传输
- 联调阶段的分层通信验证

## 依赖关系

本模块主要依赖：

- `esp_wifi`
- `esp_now`
- `esp_event`
- `esp_netif`
- `esp_timer`
- FreeRTOS queue/task
