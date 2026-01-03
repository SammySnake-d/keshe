#pragma once

/**
 * @file CameraManager.h
 * @brief 摄像头管理模块 - 拍照、上传、I2C 仲裁
 * @note OV2640 通过 I2C 配置，需要与 LSM6DS3 互斥使用
 */

#include "../../../include/AppConfig.h"

#if ENABLE_CAMERA && !USE_MOCK_HARDWARE
    #include "esp_camera.h"
#endif

class CameraManager {
private:
    static bool isInitialized;
    
public:
    /**
     * @brief 初始化摄像头（会占用 I2C 总线）
     * @warning 调用前必须确保 LSM6DS3 已休眠并释放 I2C
     */
    static bool init() {
        #if !ENABLE_CAMERA
            DEBUG_PRINTLN("[Camera] 摄像头功能未启用");
            return false;
        #endif
        
        #if USE_MOCK_HARDWARE
            DEBUG_PRINTLN("[Camera] Mock 模式：模拟摄像头初始化");
            delay(200);
            isInitialized = true;
            return true;
        #else
            DEBUG_PRINTLN("[Camera] 正在初始化 OV2640...");
            
            // 1. 上电（PWDN 拉低）
            pinMode(PIN_CAM_PWDN, OUTPUT);
            digitalWrite(PIN_CAM_PWDN, LOW);
            delay(100);
            
            // 2. 配置相机参数
            camera_config_t config;
            config.ledc_channel = LEDC_CHANNEL_0;
            config.ledc_timer = LEDC_TIMER_0;
            config.pin_d0 = PIN_CAM_D0;
            config.pin_d1 = PIN_CAM_D1;
            config.pin_d2 = PIN_CAM_D2;
            config.pin_d3 = PIN_CAM_D3;
            config.pin_d4 = PIN_CAM_D4;
            config.pin_d5 = PIN_CAM_D5;
            config.pin_d6 = PIN_CAM_D6;
            config.pin_d7 = PIN_CAM_D7;
            config.pin_xclk = PIN_CAM_XCLK;
            config.pin_pclk = PIN_CAM_PCLK;
            config.pin_vsync = PIN_CAM_VSYNC;
            config.pin_href = PIN_CAM_HREF;
            config.pin_sccb_sda = PIN_CAM_SIOD;
            config.pin_sccb_scl = PIN_CAM_SIOC;
            config.pin_pwdn = PIN_CAM_PWDN;
            config.pin_reset = PIN_CAM_RESET;
            config.xclk_freq_hz = 20000000;
            config.pixel_format = PIXFORMAT_JPEG;
            
            // 根据 PSRAM 选择分辨率
            if (psramFound()) {
                config.frame_size = FRAMESIZE_UXGA; // 1600x1200
                config.jpeg_quality = 10;
                config.fb_count = 2;
            } else {
                config.frame_size = FRAMESIZE_SVGA; // 800x600
                config.jpeg_quality = 12;
                config.fb_count = 1;
            }
            
            // 3. 初始化驱动
            esp_err_t err = esp_camera_init(&config);
            if (err != ESP_OK) {
                DEBUG_PRINTF("[Camera] ❌ 初始化失败: 0x%x\n", err);
                powerOff();
                return false;
            }
            
            DEBUG_PRINTLN("[Camera] ✓ 初始化成功");
            isInitialized = true;
            return true;
        #endif
    }
    
    /**
     * @brief 拍摄照片
     * @param outBuffer 输出 JPEG 数据指针
     * @param outSize 输出图片大小
     * @return true=成功, false=失败
     */
    static bool capturePhoto(uint8_t** outBuffer, size_t* outSize) {
        #if USE_MOCK_HARDWARE
            // Mock: 返回一个小的测试数据
            static uint8_t mockData[] = {0xFF, 0xD8, 0xFF, 0xE0, 0x00, 0x10}; // JPEG 头部
            *outBuffer = mockData;
            *outSize = sizeof(mockData);
            DEBUG_PRINTLN("[Camera] Mock 模式：返回模拟图片数据 (6 bytes)");
            return true;
        #else
            if (!isInitialized) {
                DEBUG_PRINTLN("[Camera] ❌ 摄像头未初始化");
                return false;
            }
            
            DEBUG_PRINTLN("[Camera] 正在拍照...");
            camera_fb_t* fb = esp_camera_fb_get();
            
            if (!fb) {
                DEBUG_PRINTLN("[Camera] ❌ 拍照失败");
                return false;
            }
            
            *outBuffer = fb->buf;
            *outSize = fb->len;
            
            DEBUG_PRINTF("[Camera] ✓ 拍照成功 (%d bytes)\n", fb->len);
            
            // 注意：调用者需要调用 releasePhoto() 释放
            return true;
        #endif
    }
    
    /**
     * @brief 释放照片缓冲区
     */
    static void releasePhoto() {
        #if !USE_MOCK_HARDWARE
            camera_fb_t* fb = esp_camera_fb_get();
            if (fb) {
                esp_camera_fb_return(fb);
            }
        #endif
    }
    
    /**
     * @brief 断电摄像头（释放 I2C 总线）
     */
    static void powerOff() {
        DEBUG_PRINTLN("[Camera] 关闭摄像头电源");
        
        #if USE_MOCK_HARDWARE
            delay(50);
        #else
            // PWDN 拉高断电
            digitalWrite(PIN_CAM_PWDN, HIGH);
            esp_camera_deinit();
        #endif
        
        isInitialized = false;
    }
};

// 静态成员初始化
bool CameraManager::isInitialized = false;
