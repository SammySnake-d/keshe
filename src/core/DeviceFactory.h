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
  static IGPS *_gpsModule;
  static ICamera *_camera;
#endif

public:
  /**
   * @brief 创建倾斜传感器实例
   */
  static ISensor *createTiltSensor() {
#if !ENABLE_DEEP_SLEEP
    if (_tiltSensor != nullptr) {
      return _tiltSensor;
    }
#endif

#if USE_MOCK_HARDWARE
    auto sensor = new MockTiltSensor();
#else
    auto sensor = new LSM6DS3_Sensor();
#endif

#if !ENABLE_DEEP_SLEEP
    _tiltSensor = sensor;
#endif
    return sensor;
  }

  /**
   * @brief 创建通信模块实例
   */
  static IComm *createCommModule() {
#if !ENABLE_DEEP_SLEEP
    if (_commModule != nullptr) {
      return _commModule;
    }
#endif

#if USE_MOCK_HARDWARE
    auto comm = new MockComm();
#else
    auto comm = new WifiComm();
#endif

#if !ENABLE_DEEP_SLEEP
    _commModule = comm;
#endif
    return comm;
  }

  /**
   * @brief 创建 GPS 模块实例
   */
  static IGPS *createGpsModule() {
#if !ENABLE_DEEP_SLEEP
    if (_gpsModule != nullptr) {
      return _gpsModule;
    }
#endif

#if USE_MOCK_HARDWARE
    auto gps = new MockGPS();
#else
    auto gps = new ATGM336H_Driver();
#endif

#if !ENABLE_DEEP_SLEEP
    _gpsModule = gps;
#endif
    return gps;
  }

  /**
   * @brief 创建音频传感器实例
   */
  static IAudio *createAudioSensor() {
#if !ENABLE_DEEP_SLEEP
    if (_audioSensor != nullptr) {
      return _audioSensor;
    }
#endif

#if USE_MOCK_HARDWARE
    auto audio = new MockAudioSensor();
#else
    auto audio = new AudioSensor_ADC();
#endif

#if !ENABLE_DEEP_SLEEP
    _audioSensor = audio;
#endif
    return audio;
  }

  /**
   * @brief 创建摄像头实例（单例模式，避免 esp_camera_deinit 破坏 ADC）
   */
  static ICamera *createCamera() {
#if !ENABLE_DEEP_SLEEP
    if (_camera != nullptr) {
      return _camera;
    }
#endif

#if USE_MOCK_HARDWARE
    auto camera = new MockCamera();
#else
    auto camera = new OV2640_Camera();
#endif

#if !ENABLE_DEEP_SLEEP
    _camera = camera;
#endif
    return camera;
  }

  /**
   * @brief 销毁实例
   */
  template <typename T> static void destroy(T *instance) {
#if !ENABLE_DEEP_SLEEP
    // 测试模式：不销毁单例实例
    if ((void*)instance == (void*)_tiltSensor || 
        (void*)instance == (void*)_commModule || 
        (void*)instance == (void*)_audioSensor ||
        (void*)instance == (void*)_gpsModule ||
        (void*)instance == (void*)_camera) {
      return;
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
IGPS *DeviceFactory::_gpsModule = nullptr;
ICamera *DeviceFactory::_camera = nullptr;
#endif
