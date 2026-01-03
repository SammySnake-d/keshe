#pragma once

/**
 * @file MockGPS.h
 * @brief 模拟 GPS 模块 - 用于开发测试
 * @note 输出固定位置或模拟的 NMEA 数据流
 */

#include "../../interfaces/IGPS.h"
#include "../../../include/AppConfig.h"

class MockGPS : public IGPS {
private:
    // 模拟深圳某位置（可修改为你的测试位置）
    const double MOCK_LAT = 22.542900;   // 深圳市南山区
    const double MOCK_LON = 114.053990;
    const float MOCK_ALT = 50.0f;        // 海拔 50 米
    
public:
    bool init() override {
        DEBUG_PRINTLN("[MockGPS] 初始化成功（仿真模式）");
        DEBUG_PRINTLN("[MockGPS] 模拟位置: 深圳市南山区");
        return true;
    }
    
    bool getLocation(GpsData& data, unsigned long timeoutMs = 30000) override {
        DEBUG_PRINTLN("[MockGPS] 模拟定位中...");
        
        // 模拟搜星延迟（1-3秒）
        delay(random(1000, 3000));
        
        // 填充模拟数据
        data.latitude = MOCK_LAT + (random(-100, 100) / 1000000.0);  // 添加微小随机偏移
        data.longitude = MOCK_LON + (random(-100, 100) / 1000000.0);
        data.altitude = MOCK_ALT + random(-5, 5);
        data.speed = 0.0f;
        data.course = 0.0f;
        data.satellites = 8;  // 模拟 8 颗卫星
        data.hdop = 1.2f;     // 良好精度
        data.isValid = true;
        data.timestamp = millis();
        
        DEBUG_PRINTLN("[MockGPS] ✓ 模拟定位成功！");
        DEBUG_PRINTF("  纬度: %.6f°\n", data.latitude);
        DEBUG_PRINTF("  经度: %.6f°\n", data.longitude);
        DEBUG_PRINTF("  海拔: %.1fm\n", data.altitude);
        DEBUG_PRINTF("  卫星数: %u\n", data.satellites);
        
        return true;
    }
    
    void sleep() override {
        DEBUG_PRINTLN("[MockGPS] 进入休眠模式（仿真）");
    }
    
    const char* getName() override {
        return "MockGPS";
    }
};
