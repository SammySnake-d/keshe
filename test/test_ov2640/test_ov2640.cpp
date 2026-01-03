/**
 * @file test_ov2640.cpp
 * @brief OV2640 摄像头模块测试
 * 
 * 测试目标：
 *   1. 摄像头初始化
 *   2. XCLK 时钟生成（LEDC PWM 20MHz）
 *   3. I2C 通信（SCCB）
 *   4. 图片采集
 *   5. PSRAM 缓存
 * 
 * 硬件连接：
 *   XCLK → GPIO 2 (LEDC 生成)
 *   SIOD → GPIO 17 (I2C SDA)
 *   SIOC → GPIO 18 (I2C SCL)
 *   VSYNC → GPIO 48
 *   HREF → GPIO 47
 *   PCLK → GPIO 45
 *   D0-D7 → GPIO 16,38,15,39,41,40,42,21
 *   PWDN → GPIO 46
 */

#include <Arduino.h>
#include <unity.h>
#include <esp_camera.h>
#include "PinMap.h"

// 引入 Mock 和 Real 测试函数
#include "ov2640_mock.h"
#include "ov2640_real.h"

// 全局变量
bool cameraInitialized = false;

// ==================== 测试主程序 ====================

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("\n");
    Serial.println("╔═══════════════════════════════════════╗");
    Serial.println("║   OV2640 摄像头模块 - 单元测试       ║");
    Serial.println("╚═══════════════════════════════════════╝");
    
    UNITY_BEGIN();
    
#if USE_MOCK_HARDWARE == 1
    // Mock 模式测试
    Serial.println("\n========== Mock 测试模式 ==========");
    RUN_TEST(test_mock_jpeg_generation);
    RUN_TEST(test_mock_psram_allocation);
#else
    // Real 硬件测试
    Serial.println("\n========== Real 硬件测试模式 ==========");
    Serial.println("提示: 确保 OV2640 连线正确，XCLK 需要飞线到 GPIO2\n");
    
    RUN_TEST(test_real_xclk_generation);
    RUN_TEST(test_real_camera_power);
    RUN_TEST(test_real_camera_initialization);
    RUN_TEST(test_real_capture_photo);
    RUN_TEST(test_real_psram_buffer);
    RUN_TEST(test_real_continuous_capture);
#endif
    
    UNITY_END();
    
    Serial.println("\n========== 测试完成 ==========");
}

void loop() {
    delay(5000);
    
    if (cameraInitialized) {
        Serial.println("\n[MONITOR] 拍摄测试照片...");
        camera_fb_t* fb = esp_camera_fb_get();
        if (fb) {
            Serial.printf("  图片: %d bytes (%dx%d)\n", fb->len, fb->width, fb->height);
            esp_camera_fb_return(fb);
        } else {
            Serial.println("  拍照失败");
        }
    }
}
