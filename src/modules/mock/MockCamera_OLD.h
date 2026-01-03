#pragma once

/**
 * @file MockCamera.h
 * @brief 摄像头 Mock 实现
 * @note 用于 Wokwi 仿真和单元测试
 * 
 * Mock 模拟说明:
 *   - 模拟 OV2640 的 JPEG 输出格式
 *   - 生成带有正确 SOI/EOI 标记的假数据
 *   - 模拟真实拍照的延迟和偶发失败
 */

#include "../../interfaces/ICamera.h"
#include "../../../include/AppConfig.h"

class MockCamera : public ICamera {
private:
    bool initialized = false;
    uint32_t captureCount = 0;
    uint32_t lastCaptureTime = 0;
    uint8_t* mockBuffer = nullptr;
    size_t mockBufferSize = 0;
    
public:
    public:
    MockCamera() : initialized(false), psramBuffer(nullptr), psramBufferSize(0) {}
    
    ~MockCamera() {
        if (psramBuffer != nullptr) {
            heap_caps_free(psramBuffer);
            psramBuffer = nullptr;
        }
    }
    
    ~MockCamera() {
        if (mockBuffer) {
            free(mockBuffer);
            mockBuffer = nullptr;
        }
    }
    
    bool init() override {
        DEBUG_PRINTLN("[MockCamera] 初始化成功（仿真模式）");
        DEBUG_PRINTLN("[MockCamera] 模拟 OV2640 JPEG 输出");
        delay(MOCK_CAM_INIT_DELAY_MS);  // 模拟初始化延迟
        initialized = true;
        return true;
    }
    
    /**
     * @brief 模拟拍照，生成带正确 JPEG 标记的数据
     */
    bool capturePhoto(uint8_t** outBuffer, size_t* outSize, uint32_t timeout = 5000) override {
        if (!initialized) return false;
        
        DEBUG_PRINTLN("[MockCamera] 📸 模拟拍照...");
        
        // 模拟 JPEG 图片数据（真实场景下会更大）
        // JPEG 文件格式: 0xFFD8 开头, 0xFFD9 结尾
        static const uint8_t mockJpeg[] = {
            0xFF, 0xD8,  // SOI (Start of Image)
            0xFF, 0xE0,  // APP0 标记
            0x00, 0x10,  // APP0 长度
            'J', 'F', 'I', 'F', 0x00,  // JFIF 标识
            0x01, 0x01,  // 版本
            0x00,        // 密度单位
            0x00, 0x01, 0x00, 0x01,  // 密度
            0x00, 0x00,  // 缩略图尺寸
            // ... (省略中间数据) ...
            0xFF, 0xD9   // EOI (End of Image)
        };
        
        // 释放旧的 PSRAM 缓冲区
        if (psramBuffer != nullptr) {
            heap_caps_free(psramBuffer);
            psramBuffer = nullptr;
        }
        
        // 在 PSRAM 中分配缓冲区
        psramBufferSize = sizeof(mockJpeg);
        psramBuffer = (uint8_t*)heap_caps_malloc(psramBufferSize, MALLOC_CAP_SPIRAM);
        if (psramBuffer == nullptr) {
            DEBUG_PRINTLN("[MockCamera] ❌ PSRAM 分配失败");
            return false;
        }
        
        // 复制模拟数据到 PSRAM
        memcpy(psramBuffer, mockJpeg, psramBufferSize);
        
        *outBuffer = psramBuffer;
        *outSize = psramBufferSize;
        
        DEBUG_PRINTF("[MockCamera] ✅ 模拟拍照成功: %d bytes (已存入 PSRAM)\n", *outSize);
        return true;
    }
    
    void releasePhoto() override {
        if (mockBuffer) {
            free(mockBuffer);
            mockBuffer = nullptr;
            mockBufferSize = 0;
            DEBUG_PRINTLN("[MockCamera] 释放照片缓冲区");
        }
    }
    
    void powerOff() override {
        releasePhoto();
        DEBUG_PRINTLN("[MockCamera] 关闭电源（仿真）");
        delay(50);
        initialized = false;
    }
    
    bool isReady() const override {
        return initialized;
    }
    
    // ========== Mock 辅助方法 ==========
    
    uint32_t getCaptureCount() const {
        return captureCount;
    }
    
private:
    /**
     * @brief 生成模拟 JPEG 数据
     * @note 填充正确的 SOI 和 EOI 标记，中间填充随机数据
     */
    void generateMockJpeg(uint8_t* buffer, size_t size) {
        // JPEG 文件头 (SOI + APP0 JFIF)
        const uint8_t header[] = {
            0xFF, 0xD8,                         // SOI (Start Of Image)
            0xFF, 0xE0, 0x00, 0x10,             // APP0 标记
            0x4A, 0x46, 0x49, 0x46, 0x00,       // "JFIF\0"
            0x01, 0x01,                         // 版本 1.1
            0x00,                               // 像素密度单位
            0x00, 0x01, 0x00, 0x01,             // 密度 1x1
            0x00, 0x00                          // 缩略图 0x0
        };
        
        // 复制头部
        memcpy(buffer, header, sizeof(header));
        
        // 中间填充随机数据 (模拟压缩图像数据)
        for (size_t i = sizeof(header); i < size - 2; i++) {
            buffer[i] = random(0, 256);
            // 避免产生假的 EOI 标记
            if (buffer[i] == 0xFF && i + 1 < size - 2) {
                buffer[++i] = random(0, 0xD0);  // 避免 0xD9
            }
        }
        
        // JPEG 文件尾 (EOI)
        buffer[size - 2] = 0xFF;
        buffer[size - 1] = 0xD9;  // EOI (End Of Image)
    }
};
