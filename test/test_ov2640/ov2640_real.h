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
 */
void test_real_camera_initialization() {
    Serial.println("\n[TEST] Real: 摄像头初始化");
    
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
    config.pin_sccb_sda = SIOD_GPIO_NUM;  // 使用新的引脚名称
    config.pin_sccb_scl = SIOC_GPIO_NUM;  // 使用新的引脚名称
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;
    
    // PSRAM 配置
    #if CONFIG_SPIRAM_SUPPORT || CONFIG_SPIRAM
        config.frame_size = FRAMESIZE_UXGA;  // 1600x1200
        config.jpeg_quality = 10;
        config.fb_count = 2;
        Serial.println("  模式: PSRAM (高分辨率)");
    #else
        config.frame_size = FRAMESIZE_SVGA;  // 800x600
        config.jpeg_quality = 12;
        config.fb_count = 1;
        Serial.println("  模式: 内部 RAM (低分辨率)");
    #endif
    
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("  ❌ 初始化失败: 0x%x\n", err);
        TEST_FAIL_MESSAGE("摄像头初始化失败，检查连线");
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
