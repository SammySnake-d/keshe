#pragma once

/**
 * @file MockTiltSensor.h
 * @brief 模拟倾斜传感器 - 用于开发测试
 * @note 第5次调用返回超阈值数据
 */

#include "../../interfaces/ISensor.h"
#include "../../../include/AppConfig.h"

// RTC 内存变量：跨越重启保持
RTC_DATA_ATTR static uint8_t rtc_readCount = 0;

class MockTiltSensor : public ISensor {
public:
    bool init() override {
        DEBUG_PRINTLN("[MockTilt] 初始化成功 (仿真模式)");
        return true;
    }

    float readData() override {
        rtc_readCount++;
        
        // 前4次返回正常值，第5次触发报警，第6次后重置
        float angle = (rtc_readCount == 5) ? 10.5f : 2.0f;
        
        DEBUG_PRINTF("[MockTilt] 读取角度: %.2f° (第%d次调用)\n", angle, rtc_readCount);
        
        // 第6次之后重置（模拟修复后恢复正常）
        if (rtc_readCount >= 6) {
            rtc_readCount = 0;
            DEBUG_PRINTLN("[MockTilt] 计数器已重置");
        }
        
        return angle;
    }

    void sleep() override {
        DEBUG_PRINTLN("[MockTilt] 进入休眠模式");
    }

    const char* getName() override {
        return "MockTiltSensor";
    }
};
