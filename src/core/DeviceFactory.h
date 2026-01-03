#pragma once

/**
 * @file DeviceFactory.h
 * @brief 工厂模式：根据配置生产 Mock 或 Real 实例
 */

#include "../../include/AppConfig.h"
#include "../interfaces/ISensor.h"
#include "../interfaces/IComm.h"
#include "../interfaces/IGPS.h"

// Mock 实现
#if USE_MOCK_HARDWARE
    #include "../modules/mock/MockTiltSensor.h"
    #include "../modules/mock/MockComm.h"
    #include "../modules/mock/MockGPS.h"
// Real 实现
#else
    #include "../modules/real/LSM6DS3_Sensor.h"
    #include "../modules/real/EC800K_Driver.h"
    #include "../modules/real/ATGM336H_Driver.h"
#endif

class DeviceFactory {
public:
    /**
     * @brief 创建倾斜传感器实例
     * @return ISensor* 实例指针
     */
    static ISensor* createTiltSensor() {
        DEBUG_PRINT("[Factory] 创建传感器: ");
        
        #if USE_MOCK_HARDWARE
            DEBUG_PRINTLN("MockTiltSensor");
            return new MockTiltSensor();
        #else
            DEBUG_PRINTLN("LSM6DS3_Sensor");
            return new LSM6DS3_Sensor();
        #endif
    }

    /**
     * @brief 创建通信模块实例
     * @return IComm* 实例指针
     */
    static IComm* createCommModule() {
        DEBUG_PRINT("[Factory] 创建通信模块: ");
        
        #if USE_MOCK_HARDWARE
            DEBUG_PRINTLN("MockComm");
            return new MockComm();
        #else
            DEBUG_PRINTLN("EC800K_Driver");
            return new EC800K_Driver();
        #endif
    }

    /**
     * @brief 创建 GPS 模块实例
     * @return IGPS* 实例指针
     */
    static IGPS* createGpsModule() {
        DEBUG_PRINT("[Factory] 创建 GPS 模块: ");
        
        #if USE_MOCK_HARDWARE
            DEBUG_PRINTLN("MockGPS");
            return new MockGPS();
        #else
            DEBUG_PRINTLN("ATGM336H_Driver");
            return new ATGM336H_Driver();
        #endif
    }

    /**
     * @brief 销毁实例
     */
    template<typename T>
    static void destroy(T* instance) {
        if (instance != nullptr) {
            delete instance;
            instance = nullptr;
        }
    }
};
