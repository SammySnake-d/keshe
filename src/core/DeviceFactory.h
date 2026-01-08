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
private:
  // 单例实例（测试模式下复用，避免重复初始化）
#if !ENABLE_DEEP_SLEEP
  static ISensor *_tiltSensor;
  static IComm *_commModule;
  static IAudio *_audioSensor;
  // GPS 和 Camera 每次都重新创建（资源占用大）
#endif

public:
  /**
   * @brief 创建倾斜传感器实例
   * @return ISensor* 实例指针
   */
  static ISensor *createTiltSensor() {
#if !ENABLE_DEEP_SLEEP
    // 测试模式：复用已有实例
    if (_tiltSensor != nullptr) {
      DEBUG_PRINTLN("[Factory] 复用传感器: LSM6DS3_Sensor");
      return _tiltSensor;
    }
#endif

    DEBUG_PRINT("[Factory] 创建传感器: ");

#if USE_MOCK_HARDWARE
    DEBUG_PRINTLN("MockTiltSensor");
    auto sensor = new MockTiltSensor();
#else
    DEBUG_PRINTLN("LSM6DS3_Sensor");
    auto sensor = new LSM6DS3_Sensor();
#endif

#if !ENABLE_DEEP_SLEEP
    _tiltSensor = sensor;
#endif
    return sensor;
  }

  /**
   * @brief 创建通信模块实例
   * @return IComm* 实例指针
   */
  static IComm *createCommModule() {
#if !ENABLE_DEEP_SLEEP
    // 测试模式：复用已有实例
    if (_commModule != nullptr) {
      DEBUG_PRINTLN("[Factory] 复用通信模块: WifiComm");
      return _commModule;
    }
#endif

    DEBUG_PRINT("[Factory] 创建通信模块: ");

#if USE_MOCK_HARDWARE
    DEBUG_PRINTLN("MockComm");
    auto comm = new MockComm();
#else
    DEBUG_PRINTLN("WifiComm (Bemfa)");
    auto comm = new WifiComm();
#endif

#if !ENABLE_DEEP_SLEEP
    _commModule = comm;
#endif
    return comm;
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
#if !ENABLE_DEEP_SLEEP
    if (_audioSensor != nullptr) {
      DEBUG_PRINTLN("[Factory] 复用音频传感器: AudioSensor_ADC");
      return _audioSensor;
    }
#endif

    DEBUG_PRINT("[Factory] 创建音频传感器: ");

#if USE_MOCK_HARDWARE
    DEBUG_PRINTLN("MockAudioSensor");
    auto audio = new MockAudioSensor();
#else
    DEBUG_PRINTLN("AudioSensor_ADC");
    auto audio = new AudioSensor_ADC();
#endif

#if !ENABLE_DEEP_SLEEP
    _audioSensor = audio;
#endif
    return audio;
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
#if !ENABLE_DEEP_SLEEP
    // 测试模式：不销毁单例实例（通过地址比较）
    if ((void*)instance == (void*)_tiltSensor || 
        (void*)instance == (void*)_commModule || 
        (void*)instance == (void*)_audioSensor) {
      return;  // 跳过销毁
    }
#endif
    if (instance != nullptr) {
      delete instance;
      instance = nullptr;
    }
  }
};

// 静态成员初始化
#if !ENABLE_DEEP_SLEEP
ISensor *DeviceFactory::_tiltSensor = nullptr;
IComm *DeviceFactory::_commModule = nullptr;
IAudio *DeviceFactory::_audioSensor = nullptr;
#endif
