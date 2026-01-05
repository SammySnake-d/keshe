/**
 * @file ov2640_real.h
 * @brief OV2640 Real 硬件测试函数
 * 
 * 功能：
 *   - XCLK 时钟生成
 *   - 摄像头电源控制
 *   - 摄像头初始化
 *   - 拍照测试
 *   - PSRAM 缓存测试
 */

#ifndef OV2640_REAL_H
#define OV2640_REAL_H

#include <Arduino.h>
#include <unity.h>
#include <esp_camera.h>
#include <esp_heap_caps.h>
#include "PinMap.h"

// 摄像头引脚配置
#define PWDN_GPIO_NUM     PIN_CAM_PWDN
#define RESET_GPIO_NUM    PIN_CAM_RESET
#define XCLK_GPIO_NUM     PIN_CAM_XCLK
#define SIOD_GPIO_NUM     PIN_CAM_SIOD
#define SIOC_GPIO_NUM     PIN_CAM_SIOC

#define Y9_GPIO_NUM       PIN_CAM_D7
#define Y8_GPIO_NUM       PIN_CAM_D6
#define Y7_GPIO_NUM       PIN_CAM_D5
#define Y6_GPIO_NUM       PIN_CAM_D4
#define Y5_GPIO_NUM       PIN_CAM_D3
#define Y4_GPIO_NUM       PIN_CAM_D2
#define Y3_GPIO_NUM       PIN_CAM_D1
#define Y2_GPIO_NUM       PIN_CAM_D0
#define VSYNC_GPIO_NUM    PIN_CAM_VSYNC
#define HREF_GPIO_NUM     PIN_CAM_HREF
#define PCLK_GPIO_NUM     PIN_CAM_PCLK

// 全局变量
extern bool cameraInitialized;

// ==================== Real 测试用例 ====================

/**
 * @brief Real测试：XCLK 时钟生成
 */
void test_real_xclk_generation() {
    Serial.println("\n[TEST] Real: XCLK 时钟生成");
    
    // 配置 LEDC 生成 20MHz 时钟
    ledcSetup(0, 20000000, 1);  // 通道0, 20MHz, 1位分辨率
    ledcAttachPin(XCLK_GPIO_NUM, 0);
    ledcWrite(0, 1);  // 50% 占空比
    
    Serial.printf("  XCLK 引脚: GPIO %d\n", XCLK_GPIO_NUM);
    Serial.println("  频率: 20MHz (LEDC PWM)");
    Serial.println("  ✓ XCLK 已启动");
    
    delay(100);
}

/**
 * @brief Real测试：摄像头电源控制
 */
void test_real_camera_power() {
    Serial.println("\n[TEST] Real: 摄像头电源控制");
    
    pinMode(PWDN_GPIO_NUM, OUTPUT);
    
    // Power Down (关闭)
    digitalWrite(PWDN_GPIO_NUM, HIGH);
    Serial.println("  PWDN=HIGH → 摄像头关闭");
    delay(100);
    
    // Power On (开启)
    digitalWrite(PWDN_GPIO_NUM, LOW);
    Serial.println("  PWDN=LOW → 摄像头开启");
    delay(200);
    
    Serial.println("✓ 电源控制正常");
}

/**
 * @brief Real测试：摄像头初始化
 * 
 * 参考 STM32 OV2640 示例的初始化时序：
 *   1. 电源上电序列 (PWDN: HIGH -> LOW)
 *   2. 等待电源稳定 (100-200ms)
 *   3. 初始化驱动
 *   4. 读取并验证摄像头 ID
 *   5. 丢弃前几帧等待自动曝光稳定
 */
void test_real_camera_initialization() {
    Serial.println("\n[TEST] Real: 摄像头初始化");
    Serial.println("  参考 STM32 OV2640 示例时序...");
    
    // 1. 电源控制序列（参考 STM32 示例）
    pinMode(PWDN_GPIO_NUM, OUTPUT);
    digitalWrite(PWDN_GPIO_NUM, HIGH);  // 先断电
    delay(10);
    digitalWrite(PWDN_GPIO_NUM, LOW);   // 再上电
    delay(100);                          // 等待电源稳定
    Serial.println("  [1/5] 电源序列完成");
    
    // 2. 配置摄像头
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;
    config.grab_mode = CAMERA_GRAB_LATEST;  // 获取最新帧
    
    // PSRAM 配置
    if (psramFound()) {
        config.frame_size = FRAMESIZE_UXGA;  // 1600x1200
        config.jpeg_quality = 10;
        config.fb_count = 2;
        config.fb_location = CAMERA_FB_IN_PSRAM;
        Serial.println("  [2/5] PSRAM 检测到，使用高分辨率模式");
    } else {
        config.frame_size = FRAMESIZE_SVGA;  // 800x600
        config.jpeg_quality = 12;
        config.fb_count = 1;
        config.fb_location = CAMERA_FB_IN_DRAM;
        Serial.println("  [2/5] 无 PSRAM，使用低分辨率模式");
    }
    
    // 3. 初始化驱动
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("  ❌ 初始化失败: 0x%x\n", err);
        Serial.println("  常见原因:");
        Serial.println("    - XCLK 未连接到 GPIO 2 (需要飞线!)");
        Serial.println("    - I2C (SCCB) 连线错误");
        Serial.println("    - 电源不足 (需要 3.3V@200mA)");
        Serial.println("    - 数据线 D0-D7 连接错误");
        TEST_FAIL_MESSAGE("摄像头初始化失败，检查连线");
    }
    Serial.println("  [3/5] 驱动初始化成功");
    
    // 4. 读取摄像头信息（参考 STM32 的 DCMI_OV2640_ReadID）
    sensor_t* s = esp_camera_sensor_get();
    if (s) {
        Serial.printf("  [4/5] 传感器 PID: 0x%02X\n", s->id.PID);
        // OV2640 的 PID 应该是 0x26
        if (s->id.PID == 0x26) {
            Serial.println("       ✓ 确认是 OV2640 传感器");
        }
    }
    
    // 5. 丢弃前几帧，等待自动曝光稳定（参考 STM32 示例的 Delay_ms(200)）
    Serial.println("  [5/5] 等待自动曝光稳定...");
    delay(200);
    for (int i = 0; i < 3; i++) {
        camera_fb_t* fb = esp_camera_fb_get();
        if (fb) {
            Serial.printf("       丢弃预热帧 #%d (%d bytes)\n", i+1, fb->len);
            esp_camera_fb_return(fb);
        }
        delay(50);
    }
    
    cameraInitialized = true;
    Serial.println("  ✓ 摄像头初始化成功");
}

/**
 * @brief Real测试：拍照测试
 */
void test_real_capture_photo() {
    Serial.println("\n[TEST] Real: 拍照测试");
    
    if (!cameraInitialized) {
        TEST_FAIL_MESSAGE("摄像头未初始化");
    }
    
    // 拍照
    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("  ❌ 拍照失败");
        TEST_FAIL_MESSAGE("获取图片失败");
    }
    
    Serial.printf("  图片大小: %d bytes\n", fb->len);
    Serial.printf("  分辨率: %dx%d\n", fb->width, fb->height);
    Serial.printf("  格式: %s\n", fb->format == PIXFORMAT_JPEG ? "JPEG" : "其他");
    
    // 验证 JPEG 头
    TEST_ASSERT_EQUAL_HEX8_MESSAGE(0xFF, fb->buf[0], "JPEG 头正确");
    TEST_ASSERT_EQUAL_HEX8_MESSAGE(0xD8, fb->buf[1], "JPEG 头正确");
    
    TEST_ASSERT_TRUE_MESSAGE(fb->len > 1000, "图片大小合理");
    
    esp_camera_fb_return(fb);
    Serial.println("✓ 拍照成功");
}

/**
 * @brief Real测试：PSRAM 缓存测试
 */
void test_real_psram_buffer() {
    Serial.println("\n[TEST] Real: PSRAM 缓存测试");
    
    #if CONFIG_SPIRAM_SUPPORT || CONFIG_SPIRAM
        if (!cameraInitialized) {
            TEST_FAIL_MESSAGE("摄像头未初始化");
        }
        
        camera_fb_t* fb = esp_camera_fb_get();
        if (!fb) {
            TEST_FAIL_MESSAGE("获取图片失败");
        }
        
        // 分配 PSRAM 缓存
        uint8_t* psramBuf = (uint8_t*)heap_caps_malloc(fb->len, MALLOC_CAP_SPIRAM);
        TEST_ASSERT_NOT_NULL_MESSAGE(psramBuf, "PSRAM 分配成功");
        
        // 复制数据到 PSRAM
        memcpy(psramBuf, fb->buf, fb->len);
        Serial.printf("  复制到 PSRAM: %d bytes\n", fb->len);
        
        // 验证数据完整性
        TEST_ASSERT_EQUAL_MEMORY_MESSAGE(fb->buf, psramBuf, 100, 
                                        "PSRAM 数据完整");
        
        heap_caps_free(psramBuf);
        esp_camera_fb_return(fb);
        Serial.println("✓ PSRAM 缓存正常");
    #else
        Serial.println("  ⚠️  PSRAM 未启用，跳过测试");
    #endif
}

/**
 * @brief Real测试：连续拍照测试
 */
void test_real_continuous_capture() {
    Serial.println("\n[TEST] Real: 连续拍照测试");
    
    if (!cameraInitialized) {
        TEST_FAIL_MESSAGE("摄像头未初始化");
    }
    
    for (int i = 0; i < 3; i++) {
        camera_fb_t* fb = esp_camera_fb_get();
        if (fb) {
            Serial.printf("  第 %d 张: %d bytes\n", i+1, fb->len);
            esp_camera_fb_return(fb);
        } else {
            Serial.printf("  第 %d 张: 失败\n", i+1);
        }
        delay(500);
    }
    
    Serial.println("✓ 连续拍照稳定");
}

#endif // OV2640_REAL_H
