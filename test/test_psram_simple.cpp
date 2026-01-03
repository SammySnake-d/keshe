/**
 * @file test_psram_simple.cpp
 * @brief ESP32-S3 PSRAM 快速验证测试（简化版）
 * 
 * 使用方法:
 *   1. 将 src/main.cpp 重命名为 src/main.cpp.bak
 *   2. 将此文件复制为 src/main.cpp
 *   3. 编译上传
 *   4. 打开串口监视器（115200 波特率）
 * 
 * 预期结果 (ESP32-S3-WROOM-1-N8R2):
 *   Total PSRAM: 2097152 bytes (2.00 MB)
 *   SUCCESS: PSRAM is enabled and working!
 *   Memory allocation test: OK
 */

#include <Arduino.h>

void setup() {
    Serial.begin(115200);
    delay(3000); // 等待串口稳定

    Serial.println("\n--- Testing PSRAM ---");
    
    // 1. 获取 PSRAM 总大小
    size_t psram_size = ESP.getPsramSize();
    Serial.printf("Total PSRAM: %d bytes (%.2f MB)\n", 
                  psram_size, psram_size / 1024.0 / 1024.0);

    // 2. 获取剩余可用 PSRAM
    size_t free_psram = ESP.getFreePsram();
    Serial.printf("Free PSRAM: %d bytes (%.2f MB)\n", 
                  free_psram, free_psram / 1024.0 / 1024.0);

    if (psram_size > 0) {
        Serial.println("SUCCESS: PSRAM is enabled and working!");
        
        // 3. 尝试申请一块大内存 (例如 100KB)
        // 注意：要用 ps_malloc 而不是普通的 malloc
        byte* big_buffer = (byte*)ps_malloc(100 * 1024); 
        
        if (big_buffer != NULL) {
            Serial.println("Memory allocation test: OK");
            
            // 写入测试数据
            Serial.println("Writing test pattern...");
            for (int i = 0; i < 1024; i++) {
                big_buffer[i] = i % 256;
            }
            
            // 读回验证
            Serial.println("Verifying data...");
            bool ok = true;
            for (int i = 0; i < 1024; i++) {
                if (big_buffer[i] != (i % 256)) {
                    ok = false;
                    break;
                }
            }
            
            if (ok) {
                Serial.println("Data verification: PASSED ✅");
            } else {
                Serial.println("Data verification: FAILED ❌");
            }
            
            // 用完记得释放
            free(big_buffer);
            Serial.println("Memory released");
        } else {
            Serial.println("Memory allocation test: FAILED");
        }
    } else {
        Serial.println("ERROR: PSRAM not found. Check Tools -> PSRAM settings.");
        Serial.println("Required platformio.ini settings:");
        Serial.println("  board_build.arduino.memory_type = qio_qspi");
        Serial.println("  build_flags = -DBOARD_HAS_PSRAM");
    }
    
    Serial.println("\n--- Test Complete ---");
}

void loop() {
    // 每 5 秒显示一次内存状态
    static unsigned long last = 0;
    if (millis() - last > 5000) {
        last = millis();
        Serial.printf("Free PSRAM: %d KB | Free Heap: %d KB\n", 
                      ESP.getFreePsram() / 1024, 
                      ESP.getFreeHeap() / 1024);
    }
}
