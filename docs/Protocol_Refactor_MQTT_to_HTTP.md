# 📡 通信协议重构：MQTT → HTTP

## 🎯 重构目标

将通信协议从 **MQTT** 切换为 **HTTP**，以适应低功耗场景和图片传输需求。

---

## ✅ 重构完成内容

### 1. 配置文件更新 (`Settings.h`)

**移除 MQTT 配置：**
```cpp
// 已删除
#define MQTT_SERVER                 "broker.emqx.io"
#define MQTT_PORT                   1883
#define MQTT_TOPIC_ALARM            "pole/alarm"
#define MQTT_TOPIC_STATUS           "pole/status"
```

**新增 HTTP API 配置：**
```cpp
// HTTP 服务器配置
#define HTTP_SERVER_HOST            "api.your-platform.com"
#define HTTP_SERVER_PORT            80
#define HTTP_USE_SSL                false

// API 端点
#define HTTP_API_ALARM              "/api/alarm"
#define HTTP_API_STATUS             "/api/status"
#define HTTP_API_IMAGE              "/api/upload/image"

// 设备标识
#define HTTP_DEVICE_ID              "POLE_001"

// 超时配置
#define HTTP_RESPONSE_TIMEOUT_SEC   60
#define HTTP_DATA_TIMEOUT_SEC       80
#define HTTP_IMAGE_TIMEOUT_SEC      120
```

---

### 2. 接口定义更新 (`IComm.h`)

**旧接口 (MQTT)：**
```cpp
virtual bool sendAlarm(const char* payload) = 0;
virtual bool sendStatus(const char* payload) = 0;
virtual bool subscribeCommand(const char* topic) = 0;
virtual bool receiveCommand(char* outCommand, size_t maxLen) = 0;
```

**新接口 (HTTP)：**
```cpp
// 发送报警（支持获取服务器响应）
virtual bool sendAlarm(const char* payload, 
                       char* outResponse = nullptr, 
                       size_t maxResponseLen = 0) = 0;

// 发送状态（支持获取服务器响应）
virtual bool sendStatus(const char* payload, 
                        char* outResponse = nullptr, 
                        size_t maxResponseLen = 0) = 0;

// 上传图片（新增）
virtual bool uploadImage(const uint8_t* imageData, 
                         size_t imageSize, 
                         const char* metadata = nullptr) = 0;
```

**关键改进：**
- ✅ 移除了 `subscribeCommand()` 和 `receiveCommand()`
- ✅ HTTP 响应直接携带下行指令（"捎带机制"）
- ✅ 新增 `uploadImage()` 专门处理图片上传

---

### 3. EC800K 驱动重构 (`EC800K_Driver.h`)

#### 连接流程

**MQTT 模式（旧）：**
```
1. 激活 PDP
2. 配置 MQTT
3. 连接 Broker
4. 订阅 Topic
→ 保持长连接
```

**HTTP 模式（新）：**
```
1. 激活 PDP
2. 配置 HTTP 上下文
→ 即连即走，无需保持连接
```

#### 核心 AT 指令

| 功能 | AT 指令 | 说明 |
|------|---------|------|
| 设置 URL | `AT+QHTTPURL=<len>,<timeout>` | 设置目标服务器地址 |
| 发送 POST | `AT+QHTTPPOST=<size>,<input_time>,<output_time>` | 发送 JSON 数据或图片 |
| 读取响应 | `AT+QHTTPREAD=<timeout>` | 获取服务器响应（下行指令） |

#### 图片上传实现

```cpp
bool uploadImage(const uint8_t* imageData, size_t imageSize, 
                 const char* metadata) {
    // 1. 构建 URL
    String url = "http://api.server.com/api/upload/image";
    if (metadata) url += "?meta=" + String(metadata);
    
    // 2. 设置 URL
    AT+QHTTPURL=<url_length>,80
    CONNECT
    <send_url>
    OK
    
    // 3. POST 图片数据
    AT+QHTTPPOST=<image_size>,60,120
    CONNECT
    <send_binary_data>  // 直接发送 JPEG 二进制流
    OK
    
    // 4. 等待响应
    +QHTTPPOST: 0,200,50  // 0=成功, 200=HTTP状态码
    
    return true;
}
```

---

### 4. Mock 实现更新 (`MockComm.h`)

**模拟 HTTP 行为：**
```cpp
bool sendAlarm(const char* payload, char* outResponse, size_t maxLen) {
    DEBUG_PRINTLN("╔══════════ HTTP POST 报警 ══════════╗");
    DEBUG_PRINTF("║ URL: http://%s%s\n", HOST, API_ALARM);
    DEBUG_PRINTF("║ Payload: %s\n", payload);
    
    // 10% 概率模拟服务器返回指令
    if (outResponse && random(100) < 10) {
        strcpy(outResponse, "{\"cmd\":\"set_interval\",\"value\":7200}");
    }
    return true;
}

bool uploadImage(const uint8_t* imageData, size_t imageSize, 
                 const char* metadata) {
    DEBUG_PRINTLN("╔══════════ HTTP POST 图片 ══════════╗");
    DEBUG_PRINTF("║ Size: %d bytes\n", imageSize);
    
    // 验证 JPEG 格式
    if (imageData[0] == 0xFF && imageData[1] == 0xD8) {
        DEBUG_PRINTLN("║ Format: ✓ JPEG");
    }
    return true;
}
```

---

## 🔄 业务流程对比

### MQTT 模式（旧）

```
┌─────────────────────────────────────────┐
│ 1. 设备唤醒                              │
│ 2. 连接 4G 网络                          │
│ 3. 连接 MQTT Broker                      │
│ 4. 订阅指令 Topic                        │
│ 5. 拍照 → 切片 → 逐包发布 (复杂!)       │
│ 6. 发送状态 JSON                         │
│ 7. 检查有无下行指令                      │
│ 8. 断开 MQTT                             │
│ 9. 休眠                                  │
└─────────────────────────────────────────┘
```

### HTTP 模式（新）

```
┌─────────────────────────────────────────┐
│ 1. 设备唤醒                              │
│ 2. 连接 4G 网络                          │
│ 3. 拍照 → 直接 POST (简单!)              │
│    └─ 服务器响应携带指令 (捎带机制)      │
│ 4. 发送状态 JSON (POST)                  │
│    └─ 服务器响应携带指令                 │
│ 5. 断开网络                              │
│ 6. 休眠                                  │
└─────────────────────────────────────────┘
```

---

## 💡 HTTP "捎带机制" 实现远程控制

### 设备 → 服务器（上报数据）

```http
POST /api/status HTTP/1.1
Host: api.your-platform.com
Content-Type: application/json

{
  "device_id": "POLE_001",
  "battery": 3.8,
  "tilt": 2.5,
  "gps": {"lat": 22.5, "lon": 113.9}
}
```

### 服务器 → 设备（响应指令）

**情况 A：无指令**
```http
HTTP/1.1 200 OK
Content-Type: application/json

{"status": "ok"}
```

**情况 B：有指令**
```http
HTTP/1.1 200 OK
Content-Type: application/json

{
  "status": "ok",
  "command": "set_interval",
  "value": 7200
}
```

### MCU 处理逻辑

```cpp
char response[256];
if (comm->sendStatus(jsonPayload, response, sizeof(response))) {
    // 解析响应
    if (strstr(response, "\"command\"")) {
        // 执行指令
        if (strstr(response, "set_interval")) {
            // 修改定时器间隔
        } else if (strstr(response, "reboot")) {
            // 重启设备
        }
    }
}
```

---

## 📊 HTTP vs MQTT 对比

| 维度 | MQTT | HTTP | 优胜者 |
|------|------|------|--------|
| **图片传输** | 需要切片（复杂） | 原生支持（简单） | ✅ HTTP |
| **代码复杂度** | 高（维护会话） | 低（即连即走） | ✅ HTTP |
| **功耗** | 需保持连接 | 用完即断 | ✅ HTTP |
| **流量消耗** | 需心跳包 | 无额外开销 | ✅ HTTP |
| **实时控制** | 优秀（推送） | 差（轮询） | ✅ MQTT |
| **适合场景** | 在线设备 | 低功耗/休眠设备 | - |

**结论：对于"5天续航+图片传输"的场景，HTTP 完胜！**

---

## 🚀 下一步：服务器端开发

您需要搭建一个简单的 HTTP 服务器，提供以下 API：

### 1. 接收报警
```
POST /api/alarm
Content-Type: application/json
```

### 2. 接收状态
```
POST /api/status
Content-Type: application/json
```

### 3. 接收图片
```
POST /api/upload/image
Content-Type: image/jpeg
```

**推荐技术栈：**
- Python Flask/FastAPI
- Node.js Express
- PHP Laravel

---

## ✅ 重构检查清单

- [x] Settings.h 配置更新
- [x] IComm.h 接口重构
- [x] EC800K_Driver.h 实现 HTTP 协议
- [x] MockComm.h 模拟 HTTP 行为
- [ ] WorkflowManager.h 业务流程适配（下一步）
- [ ] 测试编译
- [ ] 服务器端 API 开发

---

**完成时间**: 2026年1月3日  
**协议版本**: HTTP/1.1  
**向后兼容**: 无（完全重构）
