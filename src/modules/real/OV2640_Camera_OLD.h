#pragma once

/**
 * @file OV2640_Camera.h
 * @brief OV2640 摄像头真实硬件驱动
 * 
 * 硬件时序说明 (基于 OV2640 数据手册):
 *   - VSYNC: 帧同步信号，标识一帧图像的开始/结束
 *   - HREF:  行参考信号，高电平时数据有效
 *   - PCLK:  像素时钟，上升沿采集 D0-D7 数据
 *   - XCLK:  主时钟输入 (ESP32 提供 20MHz)
 * 
 * ESP32 Camera 驱动封装了所有底层时序控制:
 *   - 使用 I2S/LCD_CAM 外设 + DMA 自动采集
 *   - JPEG 模式下自动检测 0xFFD8 (SOI) 和 0xFFD9 (EOI)
 *   - 无需手动处理 VSYNC/HREF/PCLK 时序
 * 
 * 低功耗控制:
 *   - PWDN 拉高: Power Down 模式 (< 15µA)
 *   - PWDN 拉低: 正常工作模式
 * 
 * @note 使用 ESP32 Camera 驱动，通过 SCCB (I2C) 配置寄存器
 */

#include "../../interfaces/ICamera.h"
#include "../../../include/AppConfig.h"
#include "../../../include/PinMap.h"

#if ENABLE_CAMERA
    #include "esp_camera.h"
#endif

class OV2640_Camera : public ICamera {
private:
    bool initialized = false;
    camera_fb_t* currentFrame = nullptr;
    uint32_t captureCount = 0;           // 拍照计数
    uint32_t lastCaptureTime = 0;        // 上次拍照时间
    
public:
    OV2640_Camera() : initialized(false), psramBuffer(nullptr), psramBufferSize(0) {}
    
    ~OV2640_Camera() {
        if (psramBuffer != nullptr) {
            heap_caps_free(psramBuffer);
            psramBuffer = nullptr;
        }
        if (initialized) {
            esp_camera_deinit();
        }
    }
    
    bool begin() override {
    OV2640_Camera() : initialized(false), psramBuffer(nullptr), psramBufferSize(0) {}
    
    /**
     * @brief 初始化摄像头
     * @warning 调用前必须确保 LSM6DS3 已休眠并释放 I2C
     */
    bool init() override {
        #if !ENABLE_CAMERA
            DEBUG_PRINTLN("[OV2640] 摄像头功能未启用 (ENABLE_CAMERA=0)");
            return false;
        #else
            DEBUG_PRINTLN("[OV2640] 正在初始化...");
            
            // 1. 上电（PWDN 拉低）
            pinMode(PIN_CAM_PWDN, OUTPUT);
            digitalWrite(PIN_CAM_PWDN, LOW);
            delay(100);
            
            // 2. 配置相机参数
            camera_config_t config;
            config.ledc_channel = CAM_LEDC_CHANNEL;
            config.ledc_timer = CAM_LEDC_TIMER;
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
            config.xclk_freq_hz = CAM_XCLK_FREQ_HZ;
            config.pixel_format = PIXFORMAT_JPEG;
            config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
            
            // 根据 PSRAM 选择分辨率
            if (psramFound()) {
                config.frame_size = FRAMESIZE_UXGA;  // 1600x1200
                config.jpeg_quality = CAM_JPEG_QUALITY_HIGH;
                config.fb_count = 2;
                config.fb_location = CAMERA_FB_IN_PSRAM;
                DEBUG_PRINTLN("[OV2640] 检测到 PSRAM，使用高分辨率模式");
            } else {
                config.frame_size = FRAMESIZE_SVGA;  // 800x600
                config.jpeg_quality = CAM_JPEG_QUALITY_LOW;
                config.fb_count = 1;
                config.fb_location = CAMERA_FB_IN_DRAM;
                DEBUG_PRINTLN("[OV2640] 无 PSRAM，使用低分辨率模式");
            }
            
            // 3. 初始化驱动
            esp_err_t err = esp_camera_init(&config);
            if (err != ESP_OK) {
                DEBUG_PRINTF("[OV2640] ❌ 初始化失败: 0x%x\n", err);
                powerOff();
                return false;
            }
            
            // 4. 可选：调整传感器设置
            sensor_t* s = esp_camera_sensor_get();
            if (s) {
                s->set_brightness(s, 0);     // 亮度 (-2 ~ 2)
                s->set_contrast(s, 0);       // 对比度 (-2 ~ 2)
                s->set_saturation(s, 0);     // 饱和度 (-2 ~ 2)
                s->set_whitebal(s, 1);       // 自动白平衡
                s->set_awb_gain(s, 1);       // 自动白平衡增益
                s->set_exposure_ctrl(s, 1);  // 自动曝光
                s->set_aec2(s, 0);           // AEC DSP
                s->set_gain_ctrl(s, 1);      // 自动增益
            }
            
            DEBUG_PRINTLN("[OV2640] ✓ 初始化成功");
            initialized = true;
            return true;
        #endif
    }
    
    /**
     * @brief 拍摄照片
     * @param outBuffer 输出 JPEG 数据指针
     * @param outSize 输出图片大小 (通常 50KB-150KB)
     * @return true=成功, false=失败
     * 
     * @note JPEG 数据格式:
     *       - 文件头: 0xFF, 0xD8 (SOI - Start Of Image)
     *       - 文件尾: 0xFF, 0xD9 (EOI - End Of Image)
     *       ESP32 驱动自动检测这些标记确定帧边界
     */
    bool capturePhoto(uint8_t** outBuffer, size_t* outSize) override {
        #if !ENABLE_CAMERA
            DEBUG_PRINTLN("[OV2640] ❌ 摄像头功能未启用");
            return false;
        #else
            if (!initialized) {
                DEBUG_PRINTLN("[OV2640] ❌ 摄像头未初始化");
                return false;
            }
            
            DEBUG_PRINTLN("[OV2640] 正在拍照...");
            
            // 释放之前的帧（如果有）
            if (currentFrame) {
                esp_camera_fb_return(currentFrame);
                currentFrame = nullptr;
            }
            
            // 带重试机制的拍照 (应对时序不稳定)
            for (int retry = 0; retry < CAM_CAPTURE_RETRY_COUNT; retry++) {
                // esp_camera_fb_get() 内部:
                // 1. 等待 VSYNC 信号 (帧开始)
                // 2. 通过 DMA 采集 PCLK 同步的 D0-D7 数据
                // 3. 检测 JPEG EOI (0xFFD9) 确定帧结束
                currentFrame = esp_camera_fb_get();
                
                if (currentFrame && currentFrame->len > 0) {
                    // 验证 JPEG 格式 (检查 SOI 和 EOI 标记)
                    if (validateJpegData(currentFrame->buf, currentFrame->len)) {
                        break;  // 拍照成功
                    } else {
                        DEBUG_PRINTF("[OV2640] ⚠️ JPEG 数据无效，重试 %d/%d\n", 
                                     retry + 1, CAM_CAPTURE_RETRY_COUNT);
                        esp_camera_fb_return(currentFrame);
                        currentFrame = nullptr;
                    }
                } else {
                    DEBUG_PRINTF("[OV2640] ⚠️ 获取帧失败，重试 %d/%d\n", 
                                 retry + 1, CAM_CAPTURE_RETRY_COUNT);
                }
                delay(CAM_CAPTURE_RETRY_DELAY_MS);  // 等待传感器稳定
            }
            
            if (!currentFrame) {
                DEBUG_PRINTLN("[OV2640] ❌ 拍照失败（已重试）");
                return false;
            }
            
            *outBuffer = currentFrame->buf;
            *outSize = currentFrame->len;
            captureCount++;
            lastCaptureTime = millis();
            
            DEBUG_PRINTF("[OV2640] ✓ 拍照成功 #%lu (%d bytes, %dx%d)\n", 
                         captureCount, currentFrame->len, 
                         currentFrame->width, currentFrame->height);
            
            return true;
        #endif
    }
    
    void releasePhoto() override {
        #if ENABLE_CAMERA
            if (currentFrame) {
                esp_camera_fb_return(currentFrame);
                currentFrame = nullptr;
                DEBUG_PRINTLN("[OV2640] 释放照片缓冲区");
            }
        #endif
    }
    
    void powerOff() override {
        DEBUG_PRINTLN("[OV2640] 关闭摄像头电源");
        
        #if ENABLE_CAMERA
            // 先释放当前帧
            releasePhoto();
            
            // 反初始化驱动
            esp_camera_deinit();
            
            // PWDN 拉高断电
            digitalWrite(PIN_CAM_PWDN, HIGH);
        #endif
        
        initialized = false;
    }
    
    bool isReady() const override {
        return initialized;
    }
    
    // ========== 辅助方法 ==========
    
    /**
     * @brief 获取拍照统计信息
     */
    uint32_t getCaptureCount() const {
        return captureCount;
    }
    
private:
    /**
     * @brief 验证 JPEG 数据完整性
     * @param data JPEG 数据指针
     * @param len 数据长度
     * @return true=有效, false=无效
     * 
     * JPEG 格式:
     *   - 必须以 0xFF 0xD8 (SOI) 开头
     *   - 必须以 0xFF 0xD9 (EOI) 结尾
     */
    bool validateJpegData(const uint8_t* data, size_t len) {
        if (len < 4) return false;
        
        // 检查 SOI (Start Of Image)
        if (data[0] != 0xFF || data[1] != 0xD8) {
            DEBUG_PRINTLN("[OV2640] JPEG SOI 标记缺失");
            return false;
        }
        
        // 检查 EOI (End Of Image)
        if (data[len - 2] != 0xFF || data[len - 1] != 0xD9) {
            DEBUG_PRINTLN("[OV2640] JPEG EOI 标记缺失");
            return false;
        }
        
        return true;
    }
};
