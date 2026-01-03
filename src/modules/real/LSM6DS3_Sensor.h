#pragma once

/**
 * @file LSM6DS3_Sensor.h
 * @brief LSM6DS3 倾斜传感器驱动 - 真实硬件实现
 * @note 使用 SparkFun LSM6DS3 库
 */

#include "../../interfaces/ISensor.h"
#include "../../../include/AppConfig.h"
#include <Wire.h>
#include <SparkFunLSM6DS3.h>

class LSM6DS3_Sensor : public ISensor {
private:
    LSM6DS3 imu;
    
public:
    bool init() override {
        DEBUG_PRINTLN("[LSM6DS3] 初始化中...");
        
        // I2C 总线初始化（需要注意与摄像头的冲突）
        Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
        
        // 初始化传感器
        if (imu.begin() != 0) {
            DEBUG_PRINTLN("[LSM6DS3] ❌ 初始化失败！");
            return false;
        }
        
        DEBUG_PRINTLN("[LSM6DS3] ✓ 初始化成功");
        return true;
    }
    
    float readData() override {
        // 读取加速度计数据
        float ax = imu.readFloatAccelX();
        float ay = imu.readFloatAccelY();
        float az = imu.readFloatAccelZ();
        
        // 计算倾斜角度（Pitch 或 Roll，这里计算 Pitch）
        float pitch = atan2(ax, sqrt(ay * ay + az * az)) * 180.0 / PI;
        
        DEBUG_PRINTF("[LSM6DS3] 倾斜角度: %.2f° (Ax=%.2f, Ay=%.2f, Az=%.2f)\n", 
                     pitch, ax, ay, az);
        
        return abs(pitch); // 返回绝对值
    }
    
    void sleep() override {
        // 进入低功耗模式
        // LSM6DS3 可以配置为 Sleep 模式或关闭加速度计
        DEBUG_PRINTLN("[LSM6DS3] 进入低功耗模式");
        
        // 释放 I2C 总线（让摄像头可以使用）
        Wire.end();
    }
    
    const char* getName() override {
        return "LSM6DS3_Sensor";
    }
};
