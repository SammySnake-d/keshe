# GPS 模块重构改动文档

## 改动概述

参考 project-name 的 GPS 模块实现，同步引脚配置并添加定时上传和倾斜联动功能。

---

## 详细改动

### 1. 引脚配置同步

**修改文件**: [PinMap.h](file:///Users/snakesammy/Desktop/iot-keshe/keshe/include/PinMap.h)

**修改前**:
```cpp
#define PIN_GPS_PWR 1   // U1.22 -> IO1 (MOSFET 控制 GPS 电源)
```

**修改后**:
尝试改为 46 (参考 project-name)，但最终确认保留为 1 (用户确认)。
```cpp
#define PIN_GPS_PWR 1  // 用户确认保留为 1，虽然 GPIO 46 在参考项目中用于电源
```

### 2. 配置参数添加

**修改文件**: [Settings.h](file:///Users/snakesammy/Desktop/iot-keshe/keshe/include/Settings.h)

**新增内容**:
```cpp
#define GPS_UPLOAD_INTERVAL_MS      60000       // GPS 定时上传间隔 (60s)
#define TILT_GPS_SKIP_DURATION_MS   30000       // 倾斜后跳过 GPS 上传的时长 (30s)
```

### 3. 核心逻辑实现

**修改文件**: [WorkflowManager.h](file:///Users/snakesammy/Desktop/iot-keshe/keshe/src/core/WorkflowManager.h)

#### 3.1 RTC 变量支持
添加了 `RTC_DATA_ATTR` 变量以在深睡眠期间保持状态：
```cpp
static RTC_DATA_ATTR uint32_t lastGpsUploadTime;
static RTC_DATA_ATTR uint32_t g_last_tilt_trigger_ms;
```

#### 3.2 倾斜联动
在 `handleTimerWakeup` 中检测到倾斜时更新触发时间：
```cpp
if (relativeAngle > TILT_THRESHOLD) {
    g_last_tilt_trigger_ms = millis();
    // ...
}
```

#### 3.3 定时上传逻辑
实现了 `uploadGpsIfNeeded` 函数，并在 `sendStatusHeartbeat` 中调用（复用 HTTP 连接）：
1. **倾斜联动**: 如果最近 30s 内发生过倾斜报警，跳过本次 GPS 上传（避免覆盖报警状态）。
2. **时间间隔**: 检查距离上次上传是否超过 60s。
3. **上传格式**: 发送 `GPS:Lat:...,Lon:...` 字符串。

---

## 验证结果

1.  **引脚**: 确认 `PIN_GPS_PWR` 为 1。
2.  **功能**: 
    - 正常心跳包含 GPS 坐标 (StatusPayload)。
    - 每 60s 可能会触发一次独立 GPS 上传（如果有必要，虽然目前逻辑主要依赖心跳携带）。
    - 倾斜报警后，短期内不会触发独立 GPS 上传。
