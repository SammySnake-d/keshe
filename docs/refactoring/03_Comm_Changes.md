# 通信模块重构文档 (4G 转 WiFi)

## 改动概述

将通信方案从原计划的 EC800 (4G Cat.1) 切换为 ESP32 原生 WiFi，并集成 Bemfa (巴法云) 平台。

---

## 详细改动

### 1. 移除 4G 模块

**修改内容**:
- **删除文件**: `src/modules/real/EC800K_Driver.h`
- **修改引用**: `DeviceFactory.h` 中不再实例化 `EC800K_Driver`。
- **修改引脚**: `PinMap.h` 中注释掉了 EC800 相关的 GPIO 定义 (IO3, IO4, IO5)，确认 `project-name` 未使用这些引脚。
- **清除配置**: `Settings.h` 中移除了 4G 相关的 APN、波特率等配置。

### 2. 新增 WiFi 模块

**新增文件**: [WifiComm.h](file:///Users/snakesammy/Desktop/iot-keshe/keshe/src/modules/real/WifiComm.h)

**实现功能**:
- `init()`: 设置 station 模式，关闭节能以提高稳定性。
- `connectNetwork()`: 连接 `Settings.h` 中配置的 WiFi SSID/Password。
- `sendAlarm()` / `sendStatus()`: 实现了 HTTP GET 请求，适配 Bemfa `sendMessage` 接口 (包含 URL 编码保护)。
- `uploadImage()`: 实现了 HTTP POST 请求，适配 Bemfa `imagesUploadBin` 接口。

### 3. 集成 Bemfa 云平台

**配置更新** ([Settings.h](file:///Users/snakesammy/Desktop/iot-keshe/keshe/include/Settings.h)):
- `BEMFA_USER_KEY`: 您的私钥。
- `BEMFA_TOPIC_IMG`: 图片主题 (cam)。
- `BEMFA_TOPIC_MSG`: 消息主题 (data)。

**工厂模式更新** ([DeviceFactory.h](file:///Users/snakesammy/Desktop/iot-keshe/keshe/src/core/DeviceFactory.h)):
- `createCommModule()` 现在返回 `WifiComm` 实例。

---

## 验证结果

- 4G 相关代码彻底移除，无编译残留。
- WiFi 模块成功编译，使用了 Arduino `WiFi` 和 `HTTPClient` 库。
- 逻辑完全覆盖了 `IComm` 接口，上层业务逻辑 (WorkflowManager) 无需修改即可使用新通道。
