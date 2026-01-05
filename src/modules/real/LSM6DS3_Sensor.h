#pragma once

/**
 * @file LSM6DS3_Sensor.h
 * @brief LSM6DS3 倾斜传感器驱动 - 数据就绪中断 + 软件角度计算
 * 
 * 功能说明:
 *   1. 使用 SparkFun 库读取加速度计数据
 *   2. 配置数据就绪中断（INT1_DRDY_XL）节省功耗
 *   3. ESP32-S3 侧计算倾斜角度，支持任意阈值（如 5°）
 *   4. 支持零点校准，计算相对于初始位置的角度变化
 * 
 * 工作原理:
 *   - 加速度计以 26 Hz 采样（38.5 ms/次）
 *   - 每次数据准备好时，INT1 拉高触发中断
 *   - ESP32 读取 X/Y/Z 轴数据，用 atan2 计算角度
 *   - 对比初始角度，判断是否超过设定阈值（如 5°）
 * 
 * @note 使用 SparkFun LSM6DS3 库
 */

#include "../../interfaces/ISensor.h"
#include "../../../include/AppConfig.h"
#include "../../../include/PinMap.h"
#include <Wire.h>
#include <SparkFunLSM6DS3.h>

// LSM6DS3 关键寄存器地址
#define LSM6DS3_CTRL1_XL        0x10  // 加速度计控制寄存器
#define LSM6DS3_INT1_CTRL       0x0D  // 中断 1 路由控制寄存器
#define LSM6DS3_STATUS_REG      0x1E  // 状态寄存器（数据就绪标志）

// 寄存器位掩码
#define INT1_DRDY_XL_BIT        0x01  // INT1_CTRL[0]: 加速度数据就绪中断
#define XLDA_BIT                0x01  // STATUS_REG[0]: 加速度数据就绪标志

class LSM6DS3_Sensor : public ISensor {
private:
    LSM6DS3 imu;
    float initialPitch = 0.0f;  // 零点校准的初始角度
    float initialRoll = 0.0f;
    
    /**
     * @brief 写寄存器（I2C）
     */
    bool writeRegister(uint8_t reg, uint8_t value) {
        Wire.beginTransmission(0x6A);  // LSM6DS3 I2C 地址（SDO=0）
        Wire.write(reg);
        Wire.write(value);
        uint8_t error = Wire.endTransmission();
        delay(5);
        return (error == 0);  // 0 = 成功
    }
    
    /**
     * @brief 读寄存器（I2C）
     */
    uint8_t readRegister(uint8_t reg) {
        Wire.beginTransmission(0x6A);
        Wire.write(reg);
        Wire.endTransmission(false);
        Wire.requestFrom(0x6A, 1);
        return Wire.read();
    }
    
    /**
     * @brief 配置数据就绪中断
     */
    bool enableDataReadyInterrupt() {
        DEBUG_PRINTLN("[LSM6DS3] 配置数据就绪中断...");
        
        // 启动加速度计（26 Hz, ±2g）
        if (!writeRegister(LSM6DS3_CTRL1_XL, 0x20)) {
            DEBUG_PRINTLN("[LSM6DS3]   ❌ 加速度计配置失败");
            return false;
        }
        DEBUG_PRINTLN("[LSM6DS3]   ✓ 加速度计已启动 (26Hz, ±2g)");
        
        // 路由数据就绪信号到 INT1
        if (!writeRegister(LSM6DS3_INT1_CTRL, INT1_DRDY_XL_BIT)) {
            DEBUG_PRINTLN("[LSM6DS3]   ❌ 中断配置失败");
            return false;
        }
        DEBUG_PRINTLN("[LSM6DS3]   ✓ 数据就绪中断已配置到 INT1");
        
        return true;
    }
    
public:
    bool init() override {
        DEBUG_PRINTLN("[LSM6DS3] 初始化中...");
        
        // I2C 总线初始化
        Wire.begin(PIN_LSM_SDA, PIN_LSM_SCL);
        Wire.setClock(400000);  // 400 kHz
        
        // 初始化传感器
        if (imu.begin() != 0) {
            DEBUG_PRINTLN("[LSM6DS3] ❌ 初始化失败！");
            return false;
        }
        
        DEBUG_PRINTLN("[LSM6DS3] ✓ 基本初始化成功");
        
        // 配置数据就绪中断
        if (!enableDataReadyInterrupt()) {
            DEBUG_PRINTLN("[LSM6DS3] ⚠️  数据就绪中断配置失败");
            return false;
        }
        
        // 配置 INT1 引脚
        pinMode(PIN_LSM6DS3_INT1, INPUT);
        DEBUG_PRINTF("[LSM6DS3] INT1 引脚: GPIO %d\n", PIN_LSM6DS3_INT1);
        
        return true;
    }
    
    /**
     * @brief 设置零点校准角度（首次启动时调用）
     */
    void calibrate(float pitch, float roll) {
        initialPitch = pitch;
        initialRoll = roll;
        DEBUG_PRINTF("[LSM6DS3] 零点校准: Pitch=%.2f°, Roll=%.2f°\n", pitch, roll);
    }
    
    /**
     * @brief 读取当前倾斜角度（相对于初始位置）
     * @return 相对倾斜角度（绝对值）
     */
    float readData() override {
        // 读取加速度计数据（使用 SparkFun 库）
        float ax = imu.readFloatAccelX();
        float ay = imu.readFloatAccelY();
        float az = imu.readFloatAccelZ();
        
        // 计算当前 Pitch 和 Roll 角度
        float currentPitch = atan2(ax, sqrt(ay * ay + az * az)) * 180.0 / PI;
        float currentRoll = atan2(ay, sqrt(ax * ax + az * az)) * 180.0 / PI;
        
        // 计算相对于初始位置的偏移量
        float deltaPitch = abs(currentPitch - initialPitch);
        float deltaRoll = abs(currentRoll - initialRoll);
        
        // 返回最大偏移量
        float maxTilt = max(deltaPitch, deltaRoll);
        
        DEBUG_PRINTF("[LSM6DS3] Pitch=%.2f°, Roll=%.2f° | 相对偏移=%.2f°\n", 
                     currentPitch, currentRoll, maxTilt);
        
        return maxTilt;
    }
    
    /**
     * @brief 获取绝对角度（不考虑校准）
     */
    float getAbsolutePitch() {
        float ax = imu.readFloatAccelX();
        float ay = imu.readFloatAccelY();
        float az = imu.readFloatAccelZ();
        return atan2(ax, sqrt(ay * ay + az * az)) * 180.0 / PI;
    }
    
    /**
     * @brief 检查数据是否就绪
     */
    bool isDataReady() {
        uint8_t status = readRegister(LSM6DS3_STATUS_REG);
        return (status & XLDA_BIT) != 0;
    }
    
    /**
     * @brief 检查 INT1 引脚状态
     */
    bool isInterruptActive() {
        return digitalRead(PIN_LSM6DS3_INT1) == HIGH;
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
