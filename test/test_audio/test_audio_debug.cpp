/**
 * @file test_audio_debug.cpp
 * @brief 音频 ADC 调试测试 - 排查为什么 ADC 读取为 0
 */

#include <Arduino.h>
#include <unity.h>

// 引脚定义
#define PIN_MIC_ANALOG 8   // GPIO8 - ADC1_CH7
#define PIN_MIC_INT    19  // GPIO19 - 比较器输出 INT1

// 采样参数
#define SAMPLE_COUNT 50
#define SAMPLE_INTERVAL_US 200

void setUp() {}
void tearDown() {}

/**
 * @brief 读取峰峰值
 */
uint16_t readPeakToPeak() {
    uint16_t minVal = 4095;
    uint16_t maxVal = 0;
    uint32_t sum = 0;
    
    for (int i = 0; i < SAMPLE_COUNT; i++) {
        uint16_t sample = analogRead(PIN_MIC_ANALOG);
        sum += sample;
        if (sample > maxVal) maxVal = sample;
        if (sample < minVal) minVal = sample;
        delayMicroseconds(SAMPLE_INTERVAL_US);
    }
    
    Serial.printf("  ADC: min=%d, max=%d, avg=%d, pp=%d\n", 
                  minVal, maxVal, sum/SAMPLE_COUNT, maxVal-minVal);
    return maxVal - minVal;
}

/**
 * @brief 测试1：基础 ADC 读取
 */
void test_basic_adc_read() {
    Serial.println("\n========== 测试1: 基础 ADC 读取 ==========");
    
    // 配置 ADC
    pinMode(PIN_MIC_ANALOG, INPUT);
    analogReadResolution(12);
    analogSetPinAttenuation(PIN_MIC_ANALOG, ADC_11db);
    
    Serial.println("连续读取 10 次（每次间隔 500ms）：");
    for (int i = 0; i < 10; i++) {
        Serial.printf("第 %d 次: ", i + 1);
        readPeakToPeak();
        delay(500);
    }
    
    TEST_PASS();
}

/**
 * @brief 测试2：检查 GPIO8 是否被占用
 */
void test_gpio8_status() {
    Serial.println("\n========== 测试2: GPIO8 状态检查 ==========");
    
    // 直接读取单个值
    Serial.println("直接 analogRead 10 次：");
    for (int i = 0; i < 10; i++) {
        int val = analogRead(PIN_MIC_ANALOG);
        Serial.printf("  读取 %d: %d\n", i + 1, val);
        delay(100);
    }
    
    // 检查引脚模式
    Serial.printf("\nGPIO8 当前状态: digitalRead=%d\n", digitalRead(PIN_MIC_ANALOG));
    
    TEST_PASS();
}

/**
 * @brief 测试3：WiFi 对 ADC 的影响
 */
void test_wifi_adc_conflict() {
    Serial.println("\n========== 测试3: WiFi 对 ADC 的影响 ==========");
    Serial.println("注意: WiFi 使用 ADC2，麦克风使用 ADC1_CH7 (GPIO8)");
    Serial.println("理论上不应该冲突...\n");
    
    // WiFi 启动前
    Serial.println("WiFi 启动前：");
    for (int i = 0; i < 3; i++) {
        Serial.printf("  第 %d 次: ", i + 1);
        readPeakToPeak();
        delay(200);
    }
    
    // 启动 WiFi
    Serial.println("\n启动 WiFi...");
    #include <WiFi.h>
    WiFi.mode(WIFI_STA);
    WiFi.begin("zhoujiayi", "zhoujiayi");
    
    int timeout = 0;
    while (WiFi.status() != WL_CONNECTED && timeout < 20) {
        delay(500);
        Serial.print(".");
        timeout++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("\nWiFi 已连接: %s\n", WiFi.localIP().toString().c_str());
    } else {
        Serial.println("\nWiFi 连接失败");
    }
    
    // WiFi 启动后
    Serial.println("\nWiFi 启动后：");
    for (int i = 0; i < 5; i++) {
        Serial.printf("  第 %d 次: ", i + 1);
        readPeakToPeak();
        delay(500);
    }
    
    // 断开 WiFi
    Serial.println("\n断开 WiFi...");
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    delay(100);
    
    // WiFi 断开后
    Serial.println("\nWiFi 断开后：");
    for (int i = 0; i < 3; i++) {
        Serial.printf("  第 %d 次: ", i + 1);
        readPeakToPeak();
        delay(200);
    }
    
    TEST_PASS();
}

/**
 * @brief 测试4：I2C 对 ADC 的影响
 */
void test_i2c_adc_conflict() {
    Serial.println("\n========== 测试4: I2C 对 ADC 的影响 ==========");
    
    #include <Wire.h>
    
    // I2C 启动前
    Serial.println("I2C 启动前：");
    for (int i = 0; i < 3; i++) {
        Serial.printf("  第 %d 次: ", i + 1);
        readPeakToPeak();
        delay(200);
    }
    
    // 启动 I2C
    Serial.println("\n启动 I2C (SDA=17, SCL=18)...");
    Wire.begin(17, 18, 100000);
    delay(100);
    
    // I2C 启动后
    Serial.println("\nI2C 启动后：");
    for (int i = 0; i < 3; i++) {
        Serial.printf("  第 %d 次: ", i + 1);
        readPeakToPeak();
        delay(200);
    }
    
    // 扫描 I2C 设备
    Serial.println("\nI2C 设备扫描：");
    for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            Serial.printf("  发现设备: 0x%02X\n", addr);
        }
    }
    
    // I2C 操作后再读取
    Serial.println("\nI2C 操作后：");
    for (int i = 0; i < 3; i++) {
        Serial.printf("  第 %d 次: ", i + 1);
        readPeakToPeak();
        delay(200);
    }
    
    TEST_PASS();
}

/**
 * @brief 测试5：摄像头对 ADC 的影响
 */
void test_camera_adc_conflict() {
    Serial.println("\n========== 测试5: 摄像头对 ADC 的影响 ==========");
    
    #include "esp_camera.h"
    
    // 摄像头启动前
    Serial.println("摄像头启动前：");
    for (int i = 0; i < 3; i++) {
        Serial.printf("  第 %d 次: ", i + 1);
        readPeakToPeak();
        delay(200);
    }
    
    // 配置摄像头
    Serial.println("\n初始化摄像头...");
    
    // 电源控制
    pinMode(46, OUTPUT);
    digitalWrite(46, LOW);  // 开启摄像头电源
    delay(100);
    
    camera_config_t config = {};
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = 16;
    config.pin_d1 = 38;
    config.pin_d2 = 15;
    config.pin_d3 = 39;
    config.pin_d4 = 41;
    config.pin_d5 = 40;
    config.pin_d6 = 42;
    config.pin_d7 = 21;
    config.pin_xclk = -1;
    config.pin_pclk = 45;
    config.pin_vsync = 48;
    config.pin_href = 47;
    config.sccb_i2c_port = 0;
    config.pin_sccb_sda = -1;
    config.pin_sccb_scl = -1;
    config.pin_pwdn = 46;
    config.pin_reset = -1;
    config.xclk_freq_hz = 8000000;
    config.pixel_format = PIXFORMAT_JPEG;
    config.frame_size = FRAMESIZE_QVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.grab_mode = CAMERA_GRAB_LATEST;
    
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("摄像头初始化失败: 0x%x\n", err);
    } else {
        Serial.println("摄像头初始化成功");
    }
    
    // 摄像头启动后
    Serial.println("\n摄像头启动后：");
    for (int i = 0; i < 3; i++) {
        Serial.printf("  第 %d 次: ", i + 1);
        readPeakToPeak();
        delay(200);
    }
    
    // 拍照
    Serial.println("\n拍照...");
    camera_fb_t *fb = esp_camera_fb_get();
    if (fb) {
        Serial.printf("拍照成功: %d bytes\n", fb->len);
        esp_camera_fb_return(fb);
    }
    
    // 拍照后
    Serial.println("\n拍照后：");
    for (int i = 0; i < 3; i++) {
        Serial.printf("  第 %d 次: ", i + 1);
        readPeakToPeak();
        delay(200);
    }
    
    // 关闭摄像头
    Serial.println("\n关闭摄像头...");
    esp_camera_deinit();
    digitalWrite(46, HIGH);  // 关闭摄像头电源
    
    // 关闭后
    Serial.println("\n摄像头关闭后：");
    for (int i = 0; i < 3; i++) {
        Serial.printf("  第 %d 次: ", i + 1);
        readPeakToPeak();
        delay(200);
    }
    
    TEST_PASS();
}

/**
 * @brief 测试6：重新初始化 ADC
 */
void test_adc_reinit() {
    Serial.println("\n========== 测试6: ADC 重新初始化 ==========");
    
    Serial.println("当前状态：");
    for (int i = 0; i < 3; i++) {
        Serial.printf("  第 %d 次: ", i + 1);
        readPeakToPeak();
        delay(200);
    }
    
    // 重新配置 ADC
    Serial.println("\n重新配置 ADC...");
    pinMode(PIN_MIC_ANALOG, INPUT);
    analogReadResolution(12);
    analogSetPinAttenuation(PIN_MIC_ANALOG, ADC_11db);
    delay(10);
    
    Serial.println("\n重新配置后：");
    for (int i = 0; i < 3; i++) {
        Serial.printf("  第 %d 次: ", i + 1);
        readPeakToPeak();
        delay(200);
    }
    
    TEST_PASS();
}

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("\n");
    Serial.println("╔════════════════════════════════════════════╗");
    Serial.println("║      音频 ADC 调试测试                      ║");
    Serial.println("║      GPIO8 (ADC1_CH7) 麦克风                ║");
    Serial.println("╚════════════════════════════════════════════╝");
    
    UNITY_BEGIN();
    
    RUN_TEST(test_basic_adc_read);
    RUN_TEST(test_gpio8_status);
    RUN_TEST(test_i2c_adc_conflict);
    RUN_TEST(test_camera_adc_conflict);
    RUN_TEST(test_adc_reinit);
    // RUN_TEST(test_wifi_adc_conflict);  // WiFi 测试单独运行
    
    UNITY_END();
}

void loop() {
    delay(1000);
}
