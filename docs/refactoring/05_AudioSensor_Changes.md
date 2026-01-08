# 音频传感器模块重构文档 (通信链路迁移)

## 改动概述

音频传感器（麦克风）模块本身的数据采集逻辑（ADC 采样、峰峰值计算）保持不变，但其**报警上报通道**已随着通信模块的重构从 4G 迁移至 WiFi (Bemfa)。

---

## 详细改动

### 1. 通信链路迁移

**核心变化**: 
音频模块检测到噪音时触发的 `sendNoiseAlarmWithPhoto` 函数，现在底层调用的是 `WifiComm` 类，而不是 `EC800K_Driver`。

**文件**: [WorkflowManager.h](file:///Users/snakesammy/Desktop/iot-keshe/keshe/src/core/WorkflowManager.h)

**逻辑流程**:
1.  **噪音检测**: `handleTimerWakeup` 中检测到 `readPeakToPeak()` > `NOISE_THRESHOLD_HIGH`。
2.  **触发报警**: 调用 `sendNoiseAlarmWithPhoto`。
3.  **拍照上传**: `commModule->uploadImage()` -> HTTP POST (Bemfa `imagesUploadBin`) via WiFi.
4.  **数据上报**: `commModule->sendAlarm()` -> HTTP GET (Bemfa `sendMessage`) via WiFi.
    - 消息 Topic: `data` (与倾斜报警共享)
    - 消息内容: `{"type":"noise", "sound_level":1234, ...}`

### 2. 依赖项更新

由于 `DeviceFactory` 的更新，音频模块逻辑无需修改代码即可自动获得以下能力：
- **WiFi 连接**: 使用 `Settings.h` 中的 SSID/Password。
- **URL 编码**: 报警 JSON 数据会自动进行 URL 编码传输。
- **云端集成**: 直接对接 Bemfa 云平台。

---

## 验证方法

1.  **触发噪音**: 在设备旁制造大声噪音（持续时间需覆盖采样窗口）。
2.  **观察日志**:
    - 串口应打印 `[MAIN] 🚨 检测到异常声音！`。
    - 随后进行拍照和上传: `[Bemfa] 上传图片...` / `[Bemfa] 发送请求...`。
3.  **云端检查**:
    - 确认 Bemfa `cam` 主题收到图片。
    - 确认 Bemfa `data` 主题收到 `type: noise` 的 JSON 消息。
