# 倾斜传感器模块重构文档

## 改动概述

完成了倾斜传感器模块的重构，整合了云端报警、频率限制和 GPS 联动功能，并优化了业务流程。

---

## 详细改动

### 1. 云端即时报警

**修改文件**: [WorkflowManager.h](file:///Users/snakesammy/Desktop/iot-keshe/keshe/src/core/WorkflowManager.h)

**改动内容**:
- 在检测到倾斜 (`relativeAngle > TILT_THRESHOLD`) 时，立即触发报警流程。
- 调用 `commModule->sendAlarm()` 发送 JSON 格式报警信息。
- 消息主题 (Topic): `data` (在 `Settings.h` 中配置，[Settings.h:46](file:///Users/snakesammy/Desktop/iot-keshe/keshe/include/Settings.h#L46))。
- 数据内容: `{"type":"tilt", "angle": 12.5, "voltage": 3.8, ...}`。

### 2. 频率限制 (10s/60s)

**机制**: **Deep Sleep (深度睡眠)**
- **文件**: [Settings.h](file:///Users/snakesammy/Desktop/iot-keshe/keshe/include/Settings.h)
- **参数**: `SLEEP_DURATION_ALARM` = 60秒 (满足 >10s 需求)。
- **逻辑**: 报警发送完成后，系统调用 `SystemManager::deepSleep(SLEEP_DURATION_ALARM)` 强制休眠。这天然防止了频繁报警，无需额外的 RTC 计时器逻辑。

### 3. 通信与数据安全

**改动内容**:
- 集成 `WifiComm` 模块 (替代 4G)。
- 实现 `urlEncode` 确保 JSON 数据在 HTTP GET 请求中正确传输。

### 4. 业务逻辑分离 (Workflow Refactoring)

**修改文件**: [WorkflowManager.h](file:///Users/snakesammy/Desktop/iot-keshe/keshe/src/core/WorkflowManager.h)

**优化**:
- 此时 Tilt 和 Noise 报警逻辑已合并为统一的 `dispatchAlarm` 函数。
- 流程: 获取 GPS -> 初始化 WiFi -> 拍照 -> 上传图片 -> 发送数据 -> 清理。
- 只有触发报警时才开启 WiFi 和 Camera，最大限度省电。

---

## 验证结果

1.  **触发**: 摇动设备。
2.  **响应**: 
    - 串口打印 `[MAIN] 🚨 检测到倾斜...`。
    - WiFi 连接，图片上传至 Bemfa `cam`。
    - 报警数据上传至 Bemfa `data`。
    - 设备休眠 60秒 (频率限制生效)。
