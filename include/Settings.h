
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
// XCLK 时钟配置 (通过 ESP32 LEDC PWM 外设生成)
#define CAM_XCLK_FREQ_HZ            20000000    // XCLK 主时钟频率 (20MHz)
#define CAM_LEDC_TIMER              LEDC_TIMER_0    // LEDC 定时器
#define CAM_LEDC_CHANNEL            LEDC_CHANNEL_0  // LEDC 通道

// 拍照参数
#define CAM_CAPTURE_RETRY_COUNT     3           // 拍照失败重试次数
#define CAM_INIT_STABILIZE_MS       300         // 初始化后稳定等待时间 (ms)
#define CAM_CAPTURE_RETRY_DELAY_MS  50          // 重试间隔 (ms)
#define CAM_JPEG_QUALITY_HIGH       10          // JPEG 质量 (1-63, 越小越好)
#define CAM_JPEG_QUALITY_LOW        12          // 无 PSRAM 时的 JPEG 质量

// Mock 摄像头参数 (仅仿真使用)
#define MOCK_CAM_JPEG_MIN_SIZE      2048        // 模拟 JPEG 最小大小 (bytes)
#define MOCK_CAM_JPEG_MAX_SIZE      8192        // 模拟 JPEG 最大大小 (bytes)
#define MOCK_CAM_INIT_DELAY_MS      200         // 模拟初始化延迟 (ms)
#define MOCK_CAM_CAPTURE_DELAY_MS   100         // 模拟拍照延迟 (ms)

// ╔══════════════════════════════════════════════════════════════════╗
// ║                    📡 4G 通信模块 (EC800K)                         ║
// ╚══════════════════════════════════════════════════════════════════╝
#define EC800K_BAUD_RATE            115200      // 串口波特率
#define EC800K_INIT_DELAY_MS        1000        // 模块启动等待时间 (ms)
#define EC800K_AT_TIMEOUT_MS        1000        // AT 指令默认超时 (ms)
#define EC800K_NETWORK_RETRY_COUNT  30          // 网络注册重试次数
#define EC800K_NETWORK_RETRY_DELAY  1000        // 网络注册重试间隔 (ms)

// ╔══════════════════════════════════════════════════════════════════╗
// ║                    🛰️ GPS 模块 (ATGM336H)                         ║
// ╚══════════════════════════════════════════════════════════════════╝
#define GPS_BAUD_RATE               9600        // GPS 串口波特率
#define GPS_INIT_DELAY_MS           1000        // 模块启动稳定时间 (ms)
#define GPS_TIMEOUT_MS              30000       // 搜星超时时间 (ms)
#define GPS_UPDATE_INTERVAL_MS      1000        // 位置更新间隔 (ms)

// ╔══════════════════════════════════════════════════════════════════╗
// ║                    📐 倾斜传感器 (LSM6DS3)                         ║
// ╚══════════════════════════════════════════════════════════════════╝
#define TILT_THRESHOLD              5.0f        // 倾斜报警角度 (度)
#define TILT_DEBOUNCE_COUNT         5           // 防抖采样次数
#define TILT_SAMPLE_INTERVAL_MS     50          // 采样间隔 (ms)

// ╔══════════════════════════════════════════════════════════════════╗
// ║                    🔊 音频传感器 (麦克风 + ADC)                     ║
// ╚══════════════════════════════════════════════════════════════════╝
// 原理: 麦克风 → 运放放大 → ADC采样 → 计算峰峰值 → 与阈值比较
#define NOISE_THRESHOLD_HIGH        2500        // 噪音报警阈值 (0-4095, 峰峰值)
#define NOISE_THRESHOLD_LOW         500         // 安静环境基准值
#define NOISE_SAMPLE_COUNT          50          // 采样次数 (计算峰峰值)
#define NOISE_SAMPLE_INTERVAL_US    200         // 采样间隔 (微秒)

// ╔══════════════════════════════════════════════════════════════════╗
// ║                    🔋 电池管理                                     ║
// ╚══════════════════════════════════════════════════════════════════╝
#define BAT_LOW_LIMIT               3.4f        // 低电量保护阈值 (V)
#define BAT_CRITICAL_LIMIT          3.2f        // 极低电量阈值 (V)
#define BAT_VOLTAGE_DIV             2.0f        // 电池分压系数 (R16+R17)/R16
#define ADC_REF_VOLTAGE             3.3f        // ESP32 ADC 参考电压

// ╔══════════════════════════════════════════════════════════════════╗
// ║                    💤 休眠策略                                     ║
// ╚══════════════════════════════════════════════════════════════════╝
#if USE_MOCK_HARDWARE
    #define HEARTBEAT_INTERVAL_SEC      5       // Wokwi 测试: 5秒快速心跳
    #define SLEEP_DURATION_LOW_BAT      10      // 低电量休眠时长 (秒)
    #define SLEEP_DURATION_ALARM        3       // 报警后短休眠 (秒)
#else
    #define HEARTBEAT_INTERVAL_SEC      3600    // 真实硬件: 1小时心跳
    #define SLEEP_DURATION_LOW_BAT      7200    // 低电量休眠: 2小时
    #define SLEEP_DURATION_ALARM        60      // 报警后短休眠: 1分钟
#endif

// ╔══════════════════════════════════════════════════════════════════╗
// ║                    🌐 HTTP API 配置                                ║
// ╚══════════════════════════════════════════════════════════════════╝
// 服务器配置 (根据实际部署修改)
#define HTTP_SERVER_HOST            "api.your-platform.com"  // 服务器域名或IP
#define HTTP_SERVER_PORT            80                       // HTTP 端口 (443 for HTTPS)
#define HTTP_USE_SSL                false                    // 是否使用 HTTPS

// API 端点
#define HTTP_API_ALARM              "/api/alarm"            // 报警上报接口
#define HTTP_API_STATUS             "/api/status"           // 状态心跳接口
#define HTTP_API_IMAGE              "/api/upload/image"     // 图片上传接口

// 设备标识
#define HTTP_DEVICE_ID              "POLE_001"              // 设备唯一 ID

// ╔══════════════════════════════════════════════════════════════════╗
// ║                    ⏱️ 通信超时                                     ║
// ╚══════════════════════════════════════════════════════════════════╝
#define NETWORK_CONNECT_TIMEOUT_MS  30000       // 网络连接超时 (ms)
#define HTTP_RESPONSE_TIMEOUT_SEC   60          // HTTP 响应超时 (秒)
#define HTTP_DATA_TIMEOUT_SEC       80          // HTTP 数据传输超时 (秒)
#define HTTP_IMAGE_TIMEOUT_SEC      120         // HTTP 图片上传超时 (秒)
