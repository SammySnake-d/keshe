#pragma once

/**
 * @file OV2640_Camera.h
 * @brief OV2640 摄像头真实硬件驱动 (支持 PSRAM 缓存)
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
 * PSRAM 内存管理:
 *   - 使用 heap_caps_malloc(MALLOC_CAP_SPIRAM) 在 PSRAM 中分配缓冲区
 *   - 图片数据存储在 PSRAM 中，避免占用宝贵的 SRAM
 *   - 支持 ESP32-S3-WROOM-1-N8R2 的 2MB PSRAM
 */

#include "../../interfaces/ICamera.h"
#include "../../../include/AppConfig.h"
#include "../../../include/PinMap.h"

#if ENABLE_CAMERA
    #include "esp_camera.h"
    #include "esp_heap_caps.h"  // PSRAM 内存管理
#endif

class OV2640_Camera : public ICamera {
private:
    bool initialized = false;
    uint32_t captureCount = 0;           // 拍照计数
    uint32_t lastCaptureTime = 0;        // 上次拍照时间
    
    // PSRAM 缓冲区管理
    uint8_t* psramBuffer = nullptr;      // PSRAM 中的图片缓冲区
    size_t psramBufferSize = 0;          // 缓冲区大小
    
public:
    OV2640_Camera() : initialized(false), psramBuffer(nullptr), psramBufferSize(0) {}
    
    ~OV2640_Camera() {
        #if ENABLE_CAMERA
            // 释放 PSRAM 缓冲区
            if (psramBuffer != nullptr) {
                heap_caps_free(psramBuffer);
                psramBuffer = nullptr;
            }
            
            // 反初始化相机
            if (initialized) {
                esp_camera_deinit();
            }
        #endif
    }
    
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
            
            // 1. 电源控制序列（参考 STM32 示例的时序）
            //    PWDN: HIGH -> LOW 完成上电
            pinMode(PIN_CAM_PWDN, OUTPUT);
            digitalWrite(PIN_CAM_PWDN, HIGH);  // 先断电
            delay(10);
            digitalWrite(PIN_CAM_PWDN, LOW);   // 再上电
            delay(100);                         // 等待电源稳定
            
            DEBUG_PRINTLN("[OV2640] 电源已开启，等待稳定...");
            
            // 2. 配置相机参数
            camera_config_t config = {};
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
            // grab_mode 在下面根据 PSRAM 情况设置
            
            // 根据 PSRAM 选择分辨率
            bool hasPsram = psramFound();
            if (hasPsram) {
                config.frame_size = FRAMESIZE_UXGA;  // 1600x1200
                config.jpeg_quality = CAM_JPEG_QUALITY_HIGH;
                config.fb_count = 2;  // 双缓冲，提高连续拍照性能
                config.fb_location = CAMERA_FB_IN_PSRAM;
                config.grab_mode = CAMERA_GRAB_LATEST;  // 单次拍照用 LATEST，获取最新帧
                DEBUG_PRINTLN("[OV2640] 检测到 PSRAM，使用高分辨率模式");
            } else {
                config.frame_size = FRAMESIZE_SVGA;  // 800x600
                config.jpeg_quality = CAM_JPEG_QUALITY_LOW;
                config.fb_count = 1;
                config.fb_location = CAMERA_FB_IN_DRAM;
                config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
                DEBUG_PRINTLN("[OV2640] 无 PSRAM，使用低分辨率模式");
            }
            
            // 3. 初始化驱动
            esp_err_t err = esp_camera_init(&config);
            if (err != ESP_OK) {
                DEBUG_PRINTF("[OV2640] ❌ 初始化失败: 0x%x\n", err);
                DEBUG_PRINTLN("  常见原因: XCLK未连接GPIO2, I2C连线错误, 电源不足");
                powerOff();
                return false;
            }
            
            // 4. 丢弃前几帧，等待自动曝光稳定
            //    参考 STM32 示例：配置完成后需要等待 100-200ms
            DEBUG_PRINTLN("[OV2640] 等待自动曝光稳定...");
            delay(200);  // 等待传感器内部配置生效
            
            for (int i = 0; i < 5; i++) {  // 丢弃前 5 帧
                camera_fb_t* fb = esp_camera_fb_get();
                if (fb) {
                    DEBUG_PRINTF("[OV2640] 丢弃预热帧 #%d (%d bytes)\n", i+1, fb->len);
                    esp_camera_fb_return(fb);
                }
                delay(50);  // 帧间延时
            }
            
            // 5. 调整传感器设置
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
     * @brief 拍摄照片并存储到 PSRAM
     * @param outBuffer 输出 JPEG 数据指针 (指向 PSRAM)
     * @param outSize 输出图片大小 (通常 50KB-150KB)
     * @return true=成功, false=失败
     * 
     * @note JPEG 数据格式:
     *       - 文件头: 0xFF, 0xD8 (SOI - Start Of Image)
     *       - 文件尾: 0xFF, 0xD9 (EOI - End Of Image)
     *       数据存储在 PSRAM 中，可重复上传
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
            
            // 清空旧的缓冲帧，确保获取最新图像
            esp_camera_return_all();
            
            // 带重试机制的拍照 (应对时序不稳定)
            camera_fb_t* fb = nullptr;
            for (int retry = 0; retry < CAM_CAPTURE_RETRY_COUNT; retry++) {
                fb = esp_camera_fb_get();
                
                if (fb && fb->len > 0) {
                    // 验证 JPEG 格式
                    if (validateJpegData(fb->buf, fb->len)) {
                        break;  // 拍照成功
                    } else {
                        DEBUG_PRINTF("[OV2640] ⚠️ JPEG 数据无效，重试 %d/%d\n", 
                                     retry + 1, CAM_CAPTURE_RETRY_COUNT);
                        esp_camera_fb_return(fb);
                        fb = nullptr;
                    }
                } else {
                    DEBUG_PRINTF("[OV2640] ⚠️ 获取帧失败，重试 %d/%d\n", 
                                 retry + 1, CAM_CAPTURE_RETRY_COUNT);
                    if (fb) {
                        esp_camera_fb_return(fb);
                        fb = nullptr;
                    }
                }
                delay(CAM_CAPTURE_RETRY_DELAY_MS);
            }
            
            if (!fb) {
                DEBUG_PRINTLN("[OV2640] ❌ 拍照失败（已重试）");
                return false;
            }
            
            // 释放旧的 PSRAM 缓冲区
            if (psramBuffer != nullptr) {
                heap_caps_free(psramBuffer);
                psramBuffer = nullptr;
            }
            
            // 在 PSRAM 中分配新缓冲区
            psramBuffer = static_cast<uint8_t*>(heap_caps_malloc(fb->len, MALLOC_CAP_SPIRAM));
            if (psramBuffer == nullptr) {
                DEBUG_PRINTLN("[OV2640] ❌ PSRAM 分配失败");
                esp_camera_fb_return(fb);
                return false;
            }
            
            // 复制图片数据到 PSRAM
            memcpy(psramBuffer, fb->buf, fb->len);
            psramBufferSize = fb->len;
            
            // 释放相机帧缓冲
            esp_camera_fb_return(fb);
            
            // 更新统计信息
            captureCount++;
            lastCaptureTime = millis();
            
            *outBuffer = psramBuffer;
            *outSize = psramBufferSize;
            
            DEBUG_PRINTF("[OV2640] ✓ 拍照成功 #%lu (%d bytes, 已存入 PSRAM)\n", 
                         captureCount, psramBufferSize);
            
            return true;
        #endif
    }
    
    void releasePhoto() override {
        #if ENABLE_CAMERA
            if (psramBuffer != nullptr) {
                heap_caps_free(psramBuffer);
                psramBuffer = nullptr;
                psramBufferSize = 0;
                DEBUG_PRINTLN("[OV2640] 释放 PSRAM 缓冲区");
            }
        #endif
    }
    
    void powerOff() override {
        DEBUG_PRINTLN("[OV2640] 关闭摄像头电源");
        
        #if ENABLE_CAMERA
            // 先释放缓冲区
            releasePhoto();
            
            // 反初始化驱动
            if (initialized) {
                esp_camera_deinit();
            }
            
            // PWDN 拉高断电
            digitalWrite(PIN_CAM_PWDN, HIGH);
        #endif
        
        initialized = false;
    }
    
    bool isReady() const override {
        return initialized;
    }
    
    // ========== 辅助方法 ==========
    
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
     * JPEG 格式 (参考 STM32 OV2640 示例):
     *   - 必须以 0xFF 0xD8 (SOI - Start Of Image) 开头
     *   - 必须以 0xFF 0xD9 (EOI - End Of Image) 结尾
     *   - 最小有效 JPEG 大小约 1KB
     */
    bool validateJpegData(const uint8_t* data, size_t len) {
        // 最小 JPEG 大小检查
        if (len < 1024) {
            DEBUG_PRINTF("[OV2640] JPEG 太小: %d bytes (最小 1024)\n", len);
            return false;
        }
        
        // 检查 SOI (Start Of Image): 0xFF 0xD8
        if (data[0] != 0xFF || data[1] != 0xD8) {
            DEBUG_PRINTF("[OV2640] JPEG SOI 标记缺失 (got: 0x%02X 0x%02X)\n", 
                         data[0], data[1]);
            return false;
        }
        
        // 检查 EOI (End Of Image): 0xFF 0xD9
        // 参考 STM32 示例：从尾部向前搜索 EOI 标记
        bool foundEOI = false;
        for (size_t i = len - 2; i > len - 100 && i > 2; i--) {
            if (data[i] == 0xFF && data[i + 1] == 0xD9) {
                foundEOI = true;
                if (i != len - 2) {
                    DEBUG_PRINTF("[OV2640] EOI 在偏移 %d (尾部有 %d 字节填充)\n", 
                                 i, len - i - 2);
                }
                break;
            }
        }
        
        if (!foundEOI) {
            DEBUG_PRINTLN("[OV2640] JPEG EOI 标记缺失");
            return false;
        }
        
        // 检查 JPEG 标记 APP0 或 APP1 (可选但推荐)
        // 0xFF 0xE0 = APP0 (JFIF), 0xFF 0xE1 = APP1 (EXIF)
        if (data[2] == 0xFF && (data[3] == 0xE0 || data[3] == 0xE1)) {
            DEBUG_PRINTLN("[OV2640] JPEG 格式验证通过");
        }
        
        return true;
    }
};
