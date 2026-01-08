# 业务流程管理重构文档

## 改动概述

`WorkflowManager` 负责协调各传感器和通信模块。本次重构主要关注**代码复用**和**架构澄清**，同时保留了低功耗（Deep Sleep）设计，而非完全照搬 `project-name` 的 Always-On Task 模式。

---

## 1. 架构说明：为何不完全照搬 project-name？

| 特性 | project-name (FreeRTOS Tasks) | 本项目 (Deep Sleep) | 选择理由 |
| :--- | :--- | :--- | :--- |
| **运行模式** | WiFi 和传感器 Task 持续运行 | 唤醒 -> 执行 -> 深睡眠 | **低功耗需求** (目标: 5天续航) |
| **响应速度** | 毫秒级 (实时轮询) | 秒级/分钟级 (定时/中断唤醒) | 秒级响应对防丢/防盗场景足够 |
| **逻辑复杂度** | 简单 (变量常驻内存) | 较复杂 (需 RTC 保持状态) | 为了省电必须接受的代价 |

**结论**: 虽然用户建议参考 `project-name`，但为了满足 **5天低功耗** 的核心指标，我们保留了 **Deep Sleep 架构**，但在逻辑上借鉴了其处理流程。

---

## 2. 详细改动

### 2.1 报警逻辑统一 (简化代码)

**修改文件**: [WorkflowManager.h](file:///Users/snakesammy/Desktop/iot-keshe/keshe/src/core/WorkflowManager.h)

**问题**: 原有 `sendTiltAlarmWithPhoto` 和 `sendNoiseAlarmWithPhoto` 存在大量重复代码（初始化通信、拍照、上传图片、构建JSON、发送数据）。

**优化**: 提取了通用的 `dispatchAlarm` 函数。
```cpp
// 统一处理流程
static bool dispatchAlarm(const char *type, float value, float voltage) {
    // 1. 获取 GPS
    // 2. 初始化 WiFi & Camera
    // 3. 拍照 & 上传 (HTTP POST)
    // 4. 构建差异化 JSON (TiltPayload / NoisePayload)
    // 5. 发送报警数据 (HTTP GET)
    // 6. 清理资源
}
```

### 2.2 流程逻辑优化

1.  **GPS 联动**: 在 `dispatchAlarm` 中集成了 GPS 获取。如果定位成功，报警数据自动附带经纬度。
2.  **图片元数据**: 自动根据报警类型 (`tilt` 或 `noise`) 生成图片元数据，方便云端分类。

---

## 3. 关于音频模块

**用户疑问**: "Project 有没有实现 audiosensor 模块？"
**回答**: **没有**。`project-name` 仅包含 Tilt, Camera, GPS, WiFi。
**本项目增强**: 我们增加了音频传感器支持，并将其无缝集成到了新的 WiFi 通信架构中。本次重构中，音频模块“坐享其成”，自动获得了 WiFi 上传能力。

---

## 4. 验证结果

- 代码行数减少约 60 行。
- 逻辑更加清晰，修改报警流程只需修改 `dispatchAlarm` 一处。
- 编译通过。
