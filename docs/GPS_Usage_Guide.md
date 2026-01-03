# GPS 模块使用指南

## 硬件：ATGM336H-5N GPS/北斗双模模块

### 1. 硬件连接（已在 PCB 上完成）

| 功能 | ESP32 引脚 | ATGM336H 引脚 | 说明 |
|------|-----------|--------------|------|
| 电源控制 | GPIO1 (PIN_GPS_PWR) | VCC (通过 P-MOS) | LOW=上电, HIGH=断电 |
| UART RX | GPIO6 (PIN_GPS_TX) | TXD | ESP32 接收 GPS 数据 |
| UART TX | GPIO7 (PIN_GPS_RX) | RXD | ESP32 发送配置命令 |
| GND | GND | GND | 接地 |

**注意：** 原理图使用 P-MOS (SI2309) 高侧开关控制电源，拉低 GPIO1 才能给模块供电。

### 2. 软件架构

```
IGPS (接口)
  ├── ATGM336H_Driver (真实硬件)
  │     └── 使用 TinyGPS++ 解析 NMEA-0183
  └── MockGPS (仿真模式)
        └── 返回固定坐标（深圳南山区）
```

### 3. 使用方法

#### 基础示例

```cpp
#include "core/DeviceFactory.h"
#include "interfaces/IGPS.h"

void example() {
    // 1. 创建 GPS 实例
    IGPS* gps = DeviceFactory::createGpsModule();
    
    // 2. 初始化
    if (!gps->init()) {
        DEBUG_PRINTLN("GPS 初始化失败");
        return;
    }
    
    // 3. 获取定位（超时 30 秒）
    GpsData data;
    if (gps->getLocation(data, 30000)) {
        DEBUG_PRINTF("纬度: %.6f°\n", data.latitude);
        DEBUG_PRINTF("经度: %.6f°\n", data.longitude);
        DEBUG_PRINTF("海拔: %.1fm\n", data.altitude);
        DEBUG_PRINTF("卫星数: %u\n", data.satellites);
        
        // 4. 使用数据构建报警
        FullAlarmPayload alarm(15.5f, 3.7f, data.latitude, data.longitude);
        String json = alarm.toJson();
        // 发送 json...
    } else {
        DEBUG_PRINTLN("定位失败（可能室内或天线位置不佳）");
    }
    
    // 5. 关闭 GPS 省电
    gps->sleep();
    DeviceFactory::destroy(gps);
}
```

#### 集成到报警流程

```cpp
bool takePhotoAndUploadWithGPS(float angle, float voltage) {
    // 1. 初始化 GPS
    IGPS* gps = DeviceFactory::createGpsModule();
    if (!gps->init()) {
        return false;
    }
    
    // 2. 获取定位（限时 30 秒）
    GpsData gpsData;
    bool hasGps = gps->getLocation(gpsData, 30000);
    gps->sleep();
    DeviceFactory::destroy(gps);
    
    // 3. 初始化通信模块
    IComm* commModule = DeviceFactory::createCommModule();
    if (!commModule || !commModule->init() || !commModule->connectNetwork()) {
        return false;
    }
    
    // 4. 拍照（省略细节）
    // ...
    
    // 5. 构建报警数据
    String alarmJson;
    if (hasGps && gpsData.isValid) {
        // 带 GPS 的完整报警
        FullAlarmPayload alarm(angle, voltage, gpsData.latitude, gpsData.longitude);
        alarmJson = alarm.toJson();
    } else {
        // 不带 GPS 的普通报警
        TiltAlarmPayload alarm(angle, voltage);
        alarmJson = alarm.toJson();
    }
    
    // 6. 发送
    bool success = commModule->sendAlarm(alarmJson.c_str());
    
    // 7. 清理
    commModule->sleep();
    DeviceFactory::destroy(commModule);
    
    return success;
}
```

### 4. 数据结构

#### GpsData 结构体

```cpp
struct GpsData {
    double latitude;       // 纬度（十进制度数，如 22.542900）
    double longitude;      // 经度（十进制度数，如 114.053990）
    float altitude;        // 海拔高度（米）
    float speed;           // 速度（km/h）
    float course;          // 航向（度）
    uint8_t satellites;    // 卫星数量
    float hdop;            // 水平精度因子（越小越好）
    bool isValid;          // 定位是否有效
    unsigned long timestamp; // 数据时间戳
};
```

#### JSON 输出格式

**带 GPS 的报警：**
```json
{
  "type": "TILT",
  "angle": "15.50",
  "voltage": "3.70",
  "location": {
    "lat": "22.542900",
    "lon": "114.053990"
  },
  "timestamp": 123456
}
```

**不带 GPS 的报警：**
```json
{
  "type": "TILT",
  "angle": "15.50",
  "voltage": "3.70",
  "timestamp": 123456
}
```

### 5. 注意事项

#### ⚠️ 定位条件
1. **室内无法定位**：GPS 信号无法穿透混凝土，必须在室外或窗边测试
2. **天线方向**：陶瓷天线面朝上，不能被金属遮挡
3. **冷启动慢**：首次定位需要 30-35 秒，热启动只需 1 秒
4. **最少卫星数**：代码要求至少 4 颗卫星才认为定位有效

#### 🔧 调试技巧
1. **查看原始数据**：在 `AppConfig.h` 中定义 `#define GPS_DEBUG_RAW` 可打印 NMEA 数据
2. **检查卫星数**：定位失败时会输出当前卫星数量
3. **Mock 模式测试**：使用 `USE_MOCK_HARDWARE = 1` 可跳过真实定位

#### 💡 优化建议
1. **异步定位**：GPS 搜星很慢，不要在主循环中阻塞
2. **超时控制**：建议超时时间 20-30 秒，避免长时间等待
3. **降级策略**：定位失败时发送不带 GPS 的报警，不影响核心功能
4. **省电策略**：定位完成后立即调用 `sleep()` 断电

### 6. 故障排查

| 问题 | 可能原因 | 解决方案 |
|------|---------|---------|
| 未收到任何数据 | 1. 天线未连接<br>2. 串口接线错误<br>3. 电源未上电 | 1. 检查天线连接<br>2. 确认 TX/RX 交叉连接<br>3. 测量 +3V3_GPS 电压 |
| 定位超时 | 1. 室内环境<br>2. 天线位置不佳<br>3. 卫星数量不足 | 1. 移到室外<br>2. 天线朝上<br>3. 延长超时时间 |
| 坐标乱跳 | 1. 信号弱<br>2. 卫星数量少 | 1. 改善天线位置<br>2. 检查 HDOP 值 |

### 7. 手册参考

- **波特率**：9600 bps (8N1)
- **协议**：NMEA-0183
- **冷启动时间**：≤35 秒
- **热启动时间**：≤1 秒
- **定位精度**：2.5m CEP (单点定位)
- **电源**：3.3V ±10%, <25mA
