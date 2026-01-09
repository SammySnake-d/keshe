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

#include "../../../include/AppConfig.h"
#include "../../../include/PinMap.h"
#include "../../interfaces/ICamera.h"

#if ENABLE_CAMERA
#include "esp_camera.h"
#include "esp_heap_caps.h" // PSRAM 内存管理
#endif

class OV2640_Camera : public ICamera {
private:
  bool initialized = false;
  uint32_t captureCount = 0;    // 拍照计数
  uint32_t lastCaptureTime = 0; // 上次拍照时间

  // 简化: 直接保存帧缓冲指针 (参考 project-name/main/camera_module.c)
  camera_fb_t *currentFrame = nullptr;

public:
  OV2640_Camera() : initialized(false), currentFrame(nullptr) {}

  ~OV2640_Camera() {
#if ENABLE_CAMERA
    // 简化: 释放当前帧 (参考 project-name)
    releasePhoto();

    // 反初始化相机
    if (initialized) {
      esp_camera_deinit();
    }
#endif
  }

  /**
   * @brief 初始化摄像头
   * @note 复用 IMU 已初始化的 I2C 总线（Port 0）
   */
  bool init() override {
#if !ENABLE_CAMERA
    return false;
#else
    // 如果已经初始化过，直接返回成功
    if (initialized) {
      return true;
    }
    
    // 1. 电源控制
    pinMode(PIN_CAM_PWDN, OUTPUT);
    digitalWrite(PIN_CAM_PWDN, HIGH);
    delay(10);
    digitalWrite(PIN_CAM_PWDN, LOW);
    delay(100);

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
    config.pin_xclk = -1;
    config.pin_pclk = PIN_CAM_PCLK;
    config.pin_vsync = PIN_CAM_VSYNC;
    config.pin_href = PIN_CAM_HREF;
    
    // 关键：复用已有的 I2C 总线，不重新初始化
    // 参考 project-name/main/camera_module.c
    config.sccb_i2c_port = 0;      // 使用 I2C Port 0（与 IMU 共用）
    config.pin_sccb_sda = -1;      // -1 表示使用已初始化的总线
    config.pin_sccb_scl = -1;
    
    config.pin_pwdn = PIN_CAM_PWDN;
    config.pin_reset = -1;
    config.xclk_freq_hz = CAM_XCLK_FREQ_HZ;
    config.pixel_format = PIXFORMAT_JPEG;
    config.frame_size = CAM_FRAME_SIZE;
    config.jpeg_quality = CAM_JPEG_QUALITY;
    config.fb_count = CAM_FB_COUNT;
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.grab_mode = CAMERA_GRAB_LATEST;

    // 3. 初始化驱动
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
      DEBUG_PRINTF("[相机] ❌ 初始化失败: 0x%x\n", err);
      digitalWrite(PIN_CAM_PWDN, HIGH);
      return false;
    }

    // 4. 丢弃前几帧，等待自动曝光稳定
    delay(200);
    for (int i = 0; i < 5; i++) {
      camera_fb_t *fb = esp_camera_fb_get();
      if (fb) esp_camera_fb_return(fb);
      delay(50);
    }

    // 5. 调整传感器设置
    sensor_t *s = esp_camera_sensor_get();
    if (s) {
      s->set_brightness(s, 0);
      s->set_contrast(s, 0);
      s->set_saturation(s, 0);
      s->set_whitebal(s, 1);
      s->set_awb_gain(s, 1);
      s->set_exposure_ctrl(s, 1);
      s->set_aec2(s, 0);
      s->set_gain_ctrl(s, 1);
    }

    DEBUG_PRINTLN("[相机] ✓ 初始化成功");
    initialized = true;
    return true;
#endif
  }

  /**
   * @brief 拍摄照片 (简化版，参考 project-name/main/camera_module.c)
   * @param outBuffer 输出 JPEG 数据指针
   * @param outSize 输出图片大小
   * @return true=成功, false=失败
   *
   * @note 简化: 直接返回帧缓冲数据，调用者使用完毕后需调用 releasePhoto()
   */
  bool capturePhoto(uint8_t **outBuffer, size_t *outSize) override {
#if !ENABLE_CAMERA
    return false;
#else
    if (!initialized) {
      DEBUG_PRINTLN("[相机] ❌ 未初始化");
      return false;
    }

    releasePhoto();
    currentFrame = esp_camera_fb_get();

    if (!currentFrame) {
      DEBUG_PRINTLN("[相机] ❌ 拍照失败");
      return false;
    }

    captureCount++;
    lastCaptureTime = millis();

    *outBuffer = currentFrame->buf;
    *outSize = currentFrame->len;

    return true;
#endif
  }

  void releasePhoto() override {
#if ENABLE_CAMERA
    if (currentFrame != nullptr) {
      esp_camera_fb_return(currentFrame);
      currentFrame = nullptr;
    }
#endif
  }

  void powerOff() override {
#if ENABLE_CAMERA
    releasePhoto();
    // 注意：不关闭摄像头电源，因为会破坏 ADC
    // 摄像头保持初始化状态，只释放帧缓冲
#endif
  }

  bool isReady() const override { return initialized; }

  // ========== 辅助方法 ==========

  uint32_t getCaptureCount() const { return captureCount; }

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
  bool validateJpegData(const uint8_t *data, size_t len) {
    // 最小 JPEG 大小检查
    if (len < 1024) {
      DEBUG_PRINTF("[OV2640] JPEG 太小: %d bytes (最小 1024)\n", len);
      return false;
    }

    // 检查 SOI (Start Of Image): 0xFF 0xD8
    if (data[0] != 0xFF || data[1] != 0xD8) {
      DEBUG_PRINTF("[OV2640] JPEG SOI 标记缺失 (got: 0x%02X 0x%02X)\n", data[0],
                   data[1]);
      return false;
    }

    // 检查 EOI (End Of Image): 0xFF 0xD9
    // 参考 STM32 示例：从尾部向前搜索 EOI 标记
    bool foundEOI = false;
    for (size_t i = len - 2; i > len - 100 && i > 2; i--) {
      if (data[i] == 0xFF && data[i + 1] == 0xD9) {
        foundEOI = true;
        if (i != len - 2) {
          DEBUG_PRINTF("[OV2640] EOI 在偏移 %d (尾部有 %d 字节填充)\n", i,
                       len - i - 2);
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
