
/**
 * Settings.h
 * 业务逻辑参数配置 - 集中管理所有可调参数
 *
 * 设计原则:
 *   - 所有可调参数集中在此文件
 *   - 按模块分类，便于查找和修改
 *   - 硬件适配时只需修改此文件
 */
#pragma once

// ╔══════════════════════════════════════════════════════════════════╗
// ║                    📷 摄像头模块 (OV2640)                          ║
// ╚══════════════════════════════════════════════════════════════════╝
// 参考 project-name/main/camera_module.c 简化配置
#define CAM_XCLK_FREQ_HZ 8000000    // XCLK 主时钟频率 (8MHz, 参考 project-name)
#define CAM_LEDC_TIMER LEDC_TIMER_0 // LEDC 定时器
#define CAM_LEDC_CHANNEL LEDC_CHANNEL_0 // LEDC 通道

// 分辨率和质量 (参考 project-name/main/camera_module.c:45-48)
#define CAM_FRAME_SIZE FRAMESIZE_QVGA // 分辨率: 320x240
#define CAM_JPEG_QUALITY 12           // JPEG 压缩质量 (0-63, 越小越好)
#define CAM_FB_COUNT 1                // 帧缓冲数量 (简化为单缓冲)

// 拍照参数
#define CAM_CAPTURE_RETRY_COUNT 3     // 拍照失败重试次数
#define CAM_INIT_STABILIZE_MS 300     // 初始化后稳定等待时间 (ms)
#define CAM_CAPTURE_RETRY_DELAY_MS 50 // 重试间隔 (ms)

// Mock 摄像头参数 (仅仿真使用)
#define MOCK_CAM_JPEG_MIN_SIZE 2048   // 模拟 JPEG 最小大小 (bytes)
#define MOCK_CAM_JPEG_MAX_SIZE 8192   // 模拟 JPEG 最大大小 (bytes)
#define MOCK_CAM_INIT_DELAY_MS 200    // 模拟初始化延迟 (ms)
#define MOCK_CAM_CAPTURE_DELAY_MS 100 // 模拟拍照延迟 (ms)

// ╔══════════════════════════════════════════════════════════════════╗
// ║                    📡 WiFi & Bemfa Cloud                           ║
// ╚══════════════════════════════════════════════════════════════════╝
#define WIFI_SSID "zhoujiayi"         // WiFi 名称
#define WIFI_PASSWORD "zhoujiayi" // WiFi 密码
#define WIFI_KEEP_ALIVE 1             // 1=保持WiFi连接(测试/热点), 0=用完关闭(省电)

// 巴法云配置 (参考 project-name/main/bemfa_client.c)
#define BEMFA_USER_KEY "e40a3c7f54544bfcb16937f71f3c95e6"
#define BEMFA_TOPIC_IMG "cam"  // 图片主题
#define BEMFA_TOPIC_MSG "data" // 消息/控制主题 (用户指定: data)
#define BEMFA_API_IMG "http://apis.bemfa.com/vb/api/v1/imagesUploadBin"
#define BEMFA_API_MSG "http://apis.bemfa.com/va/sendMessage"

// 移除或保留 4G 配置作为备用
// #define EC800K_BAUD_RATE ...

// ╔══════════════════════════════════════════════════════════════════╗
// ║                    🛰️ GPS 模块 (ATGM336H)                         ║
// ╚══════════════════════════════════════════════════════════════════╝
#define GPS_BAUD_RATE 9600          // GPS 串口波特率
#define GPS_INIT_DELAY_MS 1000      // 模块启动稳定时间 (ms)
#define GPS_TIMEOUT_MS 30000        // 搜星超时时间 (ms)
#define GPS_UPDATE_INTERVAL_MS 1000 // 位置更新间隔 (ms)
#define GPS_UPLOAD_INTERVAL_MS                                                 \
  60000 // GPS 定时上传间隔 (60s, 参考 project-name)
#define TILT_GPS_SKIP_DURATION_MS 30000 // 倾斜后跳过 GPS 上传的时长 (30s)
#define GPS_TIMEOUT_MS 30000            // 搜星超时时间 (ms)
#define GPS_UPDATE_INTERVAL_MS 1000     // 位置更新间隔 (ms)

// ╔══════════════════════════════════════════════════════════════════╗
// ║                    📐 倾斜传感器 (LSM6DS3)                         ║
// ╚══════════════════════════════════════════════════════════════════╝
#define TILT_THRESHOLD 5.0f        // 倾斜报警角度 (度)
#define TILT_DEBOUNCE_COUNT 5      // 防抖采样次数
#define TILT_SAMPLE_INTERVAL_MS 50 // 采样间隔 (ms)

// ╔══════════════════════════════════════════════════════════════════╗
// ║                    🔊 音频传感器 (麦克风 + ADC)                     ║
// ╚══════════════════════════════════════════════════════════════════╝
// 【报警阈值】修改这个值调整灵敏度（单位：分贝 dB）
//   40 dB = 安静房间
//   50 dB = 正常交谈
//   60 dB = 大声说话
//   70 dB = 非常吵闹
//   80 dB = 施工噪音
#define NOISE_THRESHOLD_DB 45        // 噪音报警阈值（分贝）

// 内部参数（无需修改）
#define NOISE_SAMPLE_COUNT 50        // 采样次数
#define NOISE_SAMPLE_INTERVAL_US 200 // 采样间隔（微秒）

// ╔══════════════════════════════════════════════════════════════════╗
// ║                    🔋 电池管理                                     ║
// ╚══════════════════════════════════════════════════════════════════╝
#define BAT_LOW_LIMIT 3.4f      // 低电量保护阈值 (V)
#define BAT_CRITICAL_LIMIT 3.2f // 极低电量阈值 (V)
#define BAT_VOLTAGE_DIV 2.0f    // 电池分压系数 (R16+R17)/R16
#define ADC_REF_VOLTAGE 3.3f    // ESP32 ADC 参考电压

// ╔══════════════════════════════════════════════════════════════════╗
// ║                    💤 休眠策略                                     ║
// ╚══════════════════════════════════════════════════════════════════╝
#if USE_MOCK_HARDWARE
#define HEARTBEAT_INTERVAL_SEC 5  // Wokwi 测试: 5秒快速心跳
#define SLEEP_DURATION_LOW_BAT 10 // 低电量休眠时长 (秒)
#define SLEEP_DURATION_ALARM 3    // 报警后短休眠 (秒)
#else
#define HEARTBEAT_INTERVAL_SEC 3600 // 真实硬件: 1小时心跳 (禁用睡眠时此值用于模拟延迟)
#define SLEEP_DURATION_LOW_BAT 7200 // 低电量休眠: 2小时
#define SLEEP_DURATION_ALARM 60     // 报警后短休眠: 1分钟
#define TEST_LOOP_DELAY_SEC 10      // 测试模式循环延迟 (秒)
#endif

// ╔══════════════════════════════════════════════════════════════════╗
// ║                    🌐 HTTP API 配置                                ║
// ╚══════════════════════════════════════════════════════════════════╝
// 服务器配置 (根据实际部署修改)
#define HTTP_SERVER_HOST "api.your-platform.com" // 服务器域名或IP
#define HTTP_SERVER_PORT 80                      // HTTP 端口 (443 for HTTPS)
#define HTTP_USE_SSL false                       // 是否使用 HTTPS

// API 端点
#define HTTP_API_ALARM "/api/alarm"        // 报警上报接口
#define HTTP_API_STATUS "/api/status"      // 状态心跳接口
#define HTTP_API_IMAGE "/api/upload/image" // 图片上传接口

// 设备标识
#define HTTP_DEVICE_ID "POLE_001" // 设备唯一 ID

// ╔══════════════════════════════════════════════════════════════════╗
// ║                    ⏱️ 通信超时                                     ║
// ╚══════════════════════════════════════════════════════════════════╝
#define NETWORK_CONNECT_TIMEOUT_MS 30000 // 网络连接超时 (ms)
#define HTTP_RESPONSE_TIMEOUT_SEC 60     // HTTP 响应超时 (秒)
#define HTTP_DATA_TIMEOUT_SEC 80         // HTTP 数据传输超时 (秒)
#define HTTP_IMAGE_TIMEOUT_SEC 120       // HTTP 图片上传超时 (秒)
