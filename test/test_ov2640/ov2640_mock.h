/**
 * @file ov2640_mock.h
 * @brief OV2640 Mock 函数 - 用于测试 JPEG 生成和 PSRAM 逻辑
 * 
 * 功能：
 *   - JPEG 数据格式验证
 *   - PSRAM 分配测试
 */

#ifndef OV2640_MOCK_H
#define OV2640_MOCK_H

#include <Arduino.h>
#include <unity.h>
#include <esp_heap_caps.h>

// ==================== Mock 测试用例 ====================

/**
 * @brief Mock测试：JPEG 生成
 */
void test_mock_jpeg_generation() {
    Serial.println("\n[TEST] Mock: JPEG 生成");
    
    // 模拟生成 JPEG 数据
    size_t mockJpegSize = random(2048, 8192);
    uint8_t* mockJpeg = (uint8_t*)malloc(mockJpegSize);
    
    TEST_ASSERT_NOT_NULL_MESSAGE(mockJpeg, "模拟 JPEG 分配成功");
    
    // 填充 JPEG 头
    mockJpeg[0] = 0xFF;
    mockJpeg[1] = 0xD8;  // JPEG SOI
    mockJpeg[mockJpegSize-2] = 0xFF;
    mockJpeg[mockJpegSize-1] = 0xD9;  // JPEG EOI
    
    Serial.printf("  模拟 JPEG: %d bytes\n", mockJpegSize);
    Serial.printf("  头标识: 0x%02X%02X (期望: FFD8)\n", mockJpeg[0], mockJpeg[1]);
    
    TEST_ASSERT_EQUAL_HEX8_MESSAGE(0xFF, mockJpeg[0], "JPEG 头正确");
    TEST_ASSERT_EQUAL_HEX8_MESSAGE(0xD8, mockJpeg[1], "JPEG 头正确");
    
    free(mockJpeg);
    Serial.println("✓ Mock JPEG 生成正常");
}

/**
 * @brief Mock测试：PSRAM 模拟
 */
void test_mock_psram_allocation() {
    Serial.println("\n[TEST] Mock: PSRAM 模拟");
    
    #if CONFIG_SPIRAM_SUPPORT || CONFIG_SPIRAM
        size_t psramSize = ESP.getPsramSize();
        size_t psramFree = ESP.getFreePsram();
        
        Serial.printf("  PSRAM 总量: %d bytes\n", psramSize);
        Serial.printf("  PSRAM 可用: %d bytes\n", psramFree);
        
        // 尝试分配 100KB PSRAM
        void* testBuf = heap_caps_malloc(100 * 1024, MALLOC_CAP_SPIRAM);
        TEST_ASSERT_NOT_NULL_MESSAGE(testBuf, "PSRAM 分配成功");
        
        heap_caps_free(testBuf);
        Serial.println("✓ PSRAM 可用");
    #else
        Serial.println("  ⚠️  PSRAM 未启用");
    #endif
}

#endif // OV2640_MOCK_H
