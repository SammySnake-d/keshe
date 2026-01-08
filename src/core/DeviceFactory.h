#pragma once

/**
 * @file DeviceFactory.h
 * @brief 工厂模式：根据配置生产 Mock 或 Real 实例
 */

#include "../../include/AppConfig.h"
#include "../interfaces/IAudio.h"
#include "../interfaces/ICamera.h"
#include "../interfaces/IComm.h"
#include "../interfaces/IGPS.h"
#include "../interfaces/ISensor.h"

// Mock 实现
#if USE_MOCK_HARDWARE
#include "../modules/mock/MockAudioSensor.h"
#include "../modules/mock/MockCamera.h"
#include "../modules/mock/MockComm.h"
#include "../modules/mock/MockGPS.h"
#include "../modules/mock/MockTiltSensor.h"
// Real 实现
#else
#include "../modules/real/ATGM336H_Driver.h"
#include "../modules/real/AudioSensor_ADC.h"
// #include "../modules/real/EC800K_Driver.h" (已移除)
#include "../modules/real/LSM6DS3_Sensor.h"
#include "../modules/real/OV2640_Camera.h"
#include "../modules/real/WifiComm.h"
#endif

class DeviceFactory {
public:
  /**
   * @brief 创建倾斜传感器实例
   * @return ISensor* 实例指针
   */
  static ISensor *createTiltSensor() {
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
  static IComm *createCommModule() {
    DEBUG_PRINT("[Factory] 创建通信模块: ");

#if USE_MOCK_HARDWARE
    DEBUG_PRINTLN("MockComm");
    return new MockComm();
#else
    // 切换为 WiFi 通信模块
    // 切换为 WiFi 通信模块
    DEBUG_PRINTLN("WifiComm (Bemfa)");
    return new WifiComm();
    // DEBUG_PRINTLN("EC800K_Driver");
    // return new EC800K_Driver();
#endif
  }

  /**
   * @brief 创建 GPS 模块实例
   * @return IGPS* 实例指针
   */
  static IGPS *createGpsModule() {
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
   * @brief 创建音频传感器实例
   * @return IAudio* 实例指针
   */
  static IAudio *createAudioSensor() {
    DEBUG_PRINT("[Factory] 创建音频传感器: ");

#if USE_MOCK_HARDWARE
    DEBUG_PRINTLN("MockAudioSensor");
    return new MockAudioSensor();
#else
    DEBUG_PRINTLN("AudioSensor_ADC");
    return new AudioSensor_ADC();
#endif
  }

  /**
   * @brief 创建摄像头实例
   * @return ICamera* 实例指针
   */
  static ICamera *createCamera() {
    DEBUG_PRINT("[Factory] 创建摄像头: ");

#if USE_MOCK_HARDWARE
    DEBUG_PRINTLN("MockCamera");
    return new MockCamera();
#else
    DEBUG_PRINTLN("OV2640_Camera");
    return new OV2640_Camera();
#endif
  }

  /**
   * @brief 销毁实例
   */
  template <typename T> static void destroy(T *instance) {
    if (instance != nullptr) {
      delete instance;
      instance = nullptr;
    }
  }
};
