#pragma once

/**
 * @file MockCamera.h
 * @brief 摄像头 Mock 实现 (支持 PSRAM 缓存)
 * @note 用于 Wokwi 仿真和单元测试
 * 
 * Mock 模拟说明:
 *   - 模拟 OV2640 的 JPEG 输出格式
 *   - 生成带有正确 SOI/EOI 标记的假数据
 *   - 模拟真实拍照的延迟和偶发失败
 *   - 使用 PSRAM 存储模拟图片数据
 */

#include "../../interfaces/ICamera.h"
#include "../../../include/AppConfig.h"

#ifdef ESP32
    #include "esp_heap_caps.h"  // PSRAM 内存管理
#endif

class MockCamera : public ICamera {
private:
    bool initialized = false;
    uint32_t captureCount = 0;
    uint32_t lastCaptureTime = 0;
    
    // PSRAM 缓冲区管理
    uint8_t* psramBuffer = nullptr;
    size_t psramBufferSize = 0;
    
public:
    MockCamera() : initialized(false), psramBuffer(nullptr), psramBufferSize(0) {}
    
    ~MockCamera() {
        #ifdef ESP32
            if (psramBuffer != nullptr) {
                heap_caps_free(psramBuffer);
                psramBuffer = nullptr;
            }
        #else
            if (psramBuffer != nullptr) {
                free(psramBuffer);
                psramBuffer = nullptr;
            }
        #endif
    }
    
    bool init() override {
        DEBUG_PRINTLN("[MockCamera] 初始化成功（仿真模式）");
        DEBUG_PRINTLN("[MockCamera] 模拟 OV2640 JPEG 输出");
        delay(MOCK_CAM_INIT_DELAY_MS);  // 模拟初始化延迟
        initialized = true;
        return true;
    }
    
    /**
     * @brief 模拟拍照，生成带正确 JPEG 标记的数据并存储到 PSRAM
     */
    bool capturePhoto(uint8_t** outBuffer, size_t* outSize) override {
        if (!initialized) return false;
        
        DEBUG_PRINTLN("[MockCamera] 📸 模拟拍照...");
        
        // 模拟拍照延迟
        delay(MOCK_CAM_CAPTURE_DELAY_MS);
        
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
            // --- 模拟压缩图像数据 (实际会有几十KB) ---
            0xFF, 0xDB,  // DQT (量化表) 标记
            0x00, 0x43,  // DQT 长度
            // ... 省略量化表数据 ...
            0xFF, 0xC0,  // SOF0 (帧头) 标记
            0x00, 0x11,  // SOF0 长度
            0x08,        // 精度
            0x02, 0x80,  // 图像高度 (640)
            0x01, 0xE0,  // 图像宽度 (480)
            0x03,        // 颜色分量数
            // ... 省略其他数据 ...
            0xFF, 0xDA,  // SOS (扫描开始) 标记
            0x00, 0x0C,  // SOS 长度
            // ... 省略扫描数据 ...
            // 模拟压缩数据 (填充一些随机字节)
            0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0,
            0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54, 0x32, 0x10,
            0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x11,
            0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
            0xFF, 0xD9   // EOI (End of Image)
        };
        
        // 释放旧的缓冲区
        if (psramBuffer != nullptr) {
            #ifdef ESP32
                heap_caps_free(psramBuffer);
            #else
                free(psramBuffer);
            #endif
            psramBuffer = nullptr;
        }
        
        // 分配新缓冲区
        psramBufferSize = sizeof(mockJpeg);
        
        #ifdef ESP32
            // 在 PSRAM 中分配（如果可用）
            if (psramFound()) {
                psramBuffer = (uint8_t*)heap_caps_malloc(psramBufferSize, MALLOC_CAP_SPIRAM);
                DEBUG_PRINTLN("[MockCamera] 使用 PSRAM 缓冲区");
            } else {
                psramBuffer = (uint8_t*)malloc(psramBufferSize);
                DEBUG_PRINTLN("[MockCamera] 使用 DRAM 缓冲区");
            }
        #else
            psramBuffer = (uint8_t*)malloc(psramBufferSize);
        #endif
        
        if (psramBuffer == nullptr) {
            DEBUG_PRINTLN("[MockCamera] ❌ 内存分配失败");
            return false;
        }
        
        // 复制模拟数据到缓冲区
        memcpy(psramBuffer, mockJpeg, psramBufferSize);
        
        // 更新统计信息
        captureCount++;
        lastCaptureTime = millis();
        
        *outBuffer = psramBuffer;
        *outSize = psramBufferSize;
        
        DEBUG_PRINTF("[MockCamera] ✅ 模拟拍照成功 #%lu: %d bytes (已存入内存)\n", 
                     captureCount, psramBufferSize);
        
        return true;
    }
    
    void releasePhoto() override {
        if (psramBuffer != nullptr) {
            #ifdef ESP32
                heap_caps_free(psramBuffer);
            #else
                free(psramBuffer);
            #endif
            psramBuffer = nullptr;
            psramBufferSize = 0;
            DEBUG_PRINTLN("[MockCamera] 释放缓冲区");
        }
    }
    
    void powerOff() override {
        DEBUG_PRINTLN("[MockCamera] 模拟关闭电源");
        releasePhoto();
        initialized = false;
    }
    
    bool isReady() const override {
        return initialized;
    }
    
    // ========== 辅助方法 ==========
    
    uint32_t getCaptureCount() const {
        return captureCount;
    }
};
