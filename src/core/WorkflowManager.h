#pragma once

/**
 * @file WorkflowManager.h
 * @brief 业务流程管理器 - 处理各种唤醒场景的业务逻辑
 */

#include "../../include/AppConfig.h"
#include "../interfaces/IAudio.h"
#include "../interfaces/ICamera.h"
#include "../interfaces/IComm.h"
#include "../interfaces/IGPS.h"
#include "../interfaces/ISensor.h"
#include "../modules/real/LSM6DS3_Sensor.h"
#include "../utils/DataPayload.h"
#include "DeviceFactory.h"
#include "SystemManager.h"

class WorkflowManager {
public:
  /**
   * @brief 首次启动校准流程
   */
  static void handleFirstBoot() {
    DEBUG_PRINTLN("[系统] 首次启动 - 零点校准");

    ISensor *tiltSensor = DeviceFactory::createTiltSensor();
    if (!tiltSensor || !tiltSensor->init()) {
      DEBUG_PRINTLN("[传感器] ❌ 初始化失败");
      return;
    }

    LSM6DS3_Sensor *lsm = static_cast<LSM6DS3_Sensor *>(tiltSensor);
    float initialPitch = lsm->getAbsolutePitch();
    float initialRoll = lsm->getAbsoluteRoll();

    SystemManager::calibrateInitialPose(initialPitch, initialRoll);
    lsm->calibrate(initialPitch, initialRoll);

    DEBUG_PRINTLN("[系统] ✓ 零点校准完成");

    IAudio *audioSensor = DeviceFactory::createAudioSensor();
    if (audioSensor) audioSensor->init();
    DeviceFactory::destroy(audioSensor);

    tiltSensor->sleep();
    DeviceFactory::destroy(tiltSensor);
  }

  /**
   * @brief 定时器唤醒 - 心跳巡检流程
   */
  static void handleTimerWakeup() {
    float batteryVoltage = SystemManager::readBatteryVoltage();
    int batteryPercent = SystemManager::getBatteryPercentage();
    DEBUG_PRINTF("[巡检] 电池: %.2fV (%d%%)\n", batteryVoltage, batteryPercent);

    // 1. 读取倾角
    float relativeAngle = readTiltAngle();
    if (relativeAngle < 0) {
      SystemManager::deepSleep(HEARTBEAT_INTERVAL_SEC);
      return;
    }
    DEBUG_PRINTF("[巡检] 倾角: %.2f°\n", relativeAngle);

    // 2. 检查倾斜阈值
    if (relativeAngle > TILT_THRESHOLD) {
      DEBUG_PRINTF("[报警] 🚨 倾斜: %.2f° > %.2f°\n", relativeAngle, TILT_THRESHOLD);
      g_last_tilt_trigger_ms = millis();

      if (sendTiltAlarmWithPhoto(relativeAngle, batteryVoltage)) {
        SystemManager::deepSleep(SLEEP_DURATION_ALARM);
        return;
      }
    }

    // 3. 检查声音
    IAudio *audioSensor = DeviceFactory::createAudioSensor();
    if (audioSensor && audioSensor->init() && audioSensor->isNoiseDetected()) {
      DEBUG_PRINTLN("[报警] 🚨 异常噪音");
      uint16_t soundLevel = audioSensor->readPeakToPeak();
      audioSensor->sleep();
      DeviceFactory::destroy(audioSensor);

      if (sendNoiseAlarmWithPhoto(batteryVoltage, soundLevel)) {
        SystemManager::deepSleep(SLEEP_DURATION_ALARM);
        return;
      }
    } else {
      if (audioSensor) {
        audioSensor->sleep();
        DeviceFactory::destroy(audioSensor);
      }
    }

    // 4. 正常心跳
    sendStatusHeartbeat(relativeAngle, batteryVoltage);
    SystemManager::deepSleep(HEARTBEAT_INTERVAL_SEC);
  }

  /**
   * @brief 声音中断唤醒 - 噪音报警流程
   * @note 当前硬件为模拟信号输出，此函数仅在添加外部比较器后使用
   *       正常情况下声音检测由 handleTimerWakeup() 轮询完成
   */
  static void handleAudioWakeup() {
    DEBUG_PRINTLN("\n[MAIN] 🔊 声音中断唤醒 - 异常音检测");

    // 读取并显示电池状态
    float batteryVoltage = SystemManager::readBatteryVoltage();
    int batteryPercent = SystemManager::getBatteryPercentage();
    DEBUG_PRINTF("[MAIN] 🔋 电池状态: %.2fV (%d%%)\n", batteryVoltage,
                 batteryPercent);

    // 确认是否真的是声音触发（二次确认）
    IAudio *audioSensor = DeviceFactory::createAudioSensor();
    if (!audioSensor || !audioSensor->init() ||
        !audioSensor->isNoiseDetected()) {
      DEBUG_PRINTLN("[MAIN] ⚠️ 误触发，返回休眠");
      if (audioSensor) {
        audioSensor->sleep();
        DeviceFactory::destroy(audioSensor);
      }
      SystemManager::deepSleep(HEARTBEAT_INTERVAL_SEC);
      return;
    }

    uint16_t soundLevel = audioSensor->readPeakToPeak();
    audioSensor->sleep();
    DeviceFactory::destroy(audioSensor);

    sendNoiseAlarmWithPhoto(batteryVoltage, soundLevel);

    DEBUG_PRINTLN("[MAIN] ✓ 声音报警完成，进入休眠\n");
    SystemManager::deepSleep(SLEEP_DURATION_ALARM);
  }

  // ==========================================
  // 🌍 GPS 状态管理 (RTC 内存)
  // ==========================================
public:
  static uint32_t lastGpsUploadTime;      // 上次 GPS 上传时间
  static uint32_t g_last_tilt_trigger_ms; // 上次倾斜触发时间 (用于联动)

  static uint32_t getLastTiltTime() { return g_last_tilt_trigger_ms; }

private:
  /**
   * @brief 检查并执行定时 GPS 上传
   * @param commModule 已初始化的通信模块指针
   * @note 参考 project-name/main/main.c:163-173
   */
  static void uploadGpsIfNeeded(IComm *commModule) {
    uint32_t now = millis();

    // 1. 检查倾斜联动 (倾斜后 30s 内不上传，避免覆盖报警状态)
    if (now - g_last_tilt_trigger_ms < TILT_GPS_SKIP_DURATION_MS) {
      DEBUG_PRINTLN("[GPS] ⚠️ 近期有倾斜报警，跳过本次 GPS 定时上传");
      return;
    }

    // 2. 检查时间间隔 (60s 上传一次)
    if (now - lastGpsUploadTime > GPS_UPLOAD_INTERVAL_MS) {
      DEBUG_PRINTLN("[GPS] ⏰ 执行定时 GPS 上传...");

      GpsData gpsData;
      if (getGpsLocation(gpsData)) {
        // 构建并发送 GPS 消息 (参考 project-name 格式: "GPS:Lat:...,Lon:...")
        char gpsMsg[64];
        snprintf(gpsMsg, sizeof(gpsMsg), "GPS:Lat:%.6f,Lon:%.6f",
                 gpsData.latitude, gpsData.longitude);

        char serverResponse[64];
        commModule->sendStatus(gpsMsg, serverResponse, sizeof(serverResponse));

        DEBUG_PRINTF("[GPS] 📤 发送: %s\n", gpsMsg);
        lastGpsUploadTime = now;
      }
    }
  }

private:
  /**
   * @brief 读取倾角数据（相对于初始位置）
   * @return 相对倾角，失败返回 -1
   */
  static float readTiltAngle() {
    ISensor *tiltSensor = DeviceFactory::createTiltSensor();
    if (!tiltSensor) {
      DEBUG_PRINTLN("[MAIN] ❌ 传感器创建失败");
      return -1.0f;
    }
    
    // 检查是否需要初始化（首次或深度睡眠后）
    static bool sensorInitialized = false;
#if !ENABLE_DEEP_SLEEP
    if (!sensorInitialized) {
#endif
      if (!tiltSensor->init()) {
        DEBUG_PRINTLN("[MAIN] ❌ 传感器初始化失败");
        DeviceFactory::destroy(tiltSensor);
        return -1.0f;
      }
      sensorInitialized = true;
#if !ENABLE_DEEP_SLEEP
    }
#endif

    // 恢复零点校准值（从 RTC 内存读取）
    LSM6DS3_Sensor *lsm = static_cast<LSM6DS3_Sensor *>(tiltSensor);
    float initialPitch = SystemManager::getInitialPitch();
    float initialRoll = SystemManager::getInitialRoll();
    lsm->calibrate(initialPitch, initialRoll);

    // 读取相对倾角（已经在 readData 中计算相对值）
    float relativeAngle = tiltSensor->readData();

    DEBUG_PRINTF("[MAIN] 📐 相对倾角: %.2f°\n", relativeAngle);

#if ENABLE_DEEP_SLEEP
    tiltSensor->sleep();
#endif
    DeviceFactory::destroy(tiltSensor);

    return relativeAngle;
  }

  /**
   * @brief 获取 GPS 定位数据
   */
  static bool getGpsLocation(GpsData &gpsData) {
    IGPS *gps = DeviceFactory::createGpsModule();
    if (!gps || !gps->init()) {
      DeviceFactory::destroy(gps);
      return false;
    }

    unsigned long gpsTimeout = USE_MOCK_HARDWARE ? 5000 : 30000;
    bool success = gps->getLocation(gpsData, gpsTimeout);

    if (!success) {
      DEBUG_PRINTLN("[GPS] ⚠️ 定位失败");
    }

    gps->sleep();
    DeviceFactory::destroy(gps);
    return success;
  }

  /**
   * @brief 统一报警处理流程
   */
  static bool dispatchAlarm(const char *type, float value, float voltage) {
    // 1. 获取 GPS
    GpsData gpsData;
    bool hasGps = getGpsLocation(gpsData);

    // 2. 初始化通信
    IComm *commModule = DeviceFactory::createCommModule();
    if (!commModule || !commModule->init() || !commModule->connectNetwork()) {
      DEBUG_PRINTLN("[通信] ❌ 连接失败");
      DeviceFactory::destroy(commModule);
      return false;
    }

    // 3. 拍照上传
    ICamera *camera = DeviceFactory::createCamera();
    if (camera && camera->init()) {
      uint8_t *photoBuffer = nullptr;
      size_t photoSize = 0;
      if (camera->capturePhoto(&photoBuffer, &photoSize)) {
        DEBUG_PRINTF("[上报] 📷 图片: %d bytes\n", photoSize);
        String metadata = String("{\"device_id\":\"") + HTTP_DEVICE_ID +
                         "\",\"type\":\"" + type + "\"}";
        if (commModule->uploadImage(photoBuffer, photoSize, metadata.c_str())) {
          DEBUG_PRINTLN("[上报] ✓ 图片上传成功");
        } else {
          DEBUG_PRINTLN("[上报] ⚠️ 图片上传失败");
        }
      }
      camera->releasePhoto();
      camera->powerOff();
      DeviceFactory::destroy(camera);
    }

    // 4. 构建并发送报警
    String alarmJson;
    if (strcmp(type, "tilt") == 0) {
      if (hasGps) {
        alarmJson = TiltAlarmPayload(value, voltage, gpsData.latitude,
                                     gpsData.longitude).toJson();
      } else {
        alarmJson = TiltAlarmPayload(value, voltage).toJson();
      }
    } else {
      if (hasGps) {
        alarmJson = NoiseAlarmPayload(voltage, (uint16_t)value,
                                      gpsData.latitude, gpsData.longitude)
                        .toJson();
      } else {
        alarmJson = NoiseAlarmPayload(voltage, (uint16_t)value).toJson();
      }
    }

    DEBUG_PRINTF("[上报] 📤 %s报警: %s\n", 
                 strcmp(type, "tilt") == 0 ? "倾斜" : "噪音", 
                 alarmJson.c_str());

    char serverResponse[256] = {0};
    bool success = commModule->sendAlarm(alarmJson.c_str(), serverResponse,
                                         sizeof(serverResponse));
    if (success) {
      DEBUG_PRINTLN("[上报] ✓ 发送成功");
    }

    commModule->sleep();
    DeviceFactory::destroy(commModule);
    return success;
  }

  static bool sendTiltAlarmWithPhoto(float angle, float voltage) {
    return dispatchAlarm("tilt", angle, voltage);
  }

  static bool sendNoiseAlarmWithPhoto(float voltage, uint16_t soundLevel) {
    return dispatchAlarm("noise", (float)soundLevel, voltage);
  }

  /**
   * @brief 发送状态心跳
   */
  static void sendStatusHeartbeat(float angle, float voltage) {
    GpsData gpsData;
    bool hasGps = getGpsLocation(gpsData);

    IComm *commModule = DeviceFactory::createCommModule();
    if (!commModule || !commModule->init() || !commModule->connectNetwork()) {
      DEBUG_PRINTLN("[通信] ❌ 连接失败");
      DeviceFactory::destroy(commModule);
      return;
    }

    StatusPayload statusData;
    if (hasGps) {
      statusData = StatusPayload(angle, voltage, gpsData.latitude, gpsData.longitude);
    } else {
      statusData = StatusPayload(angle, voltage);
    }

    String statusJson = statusData.toJson();
    DEBUG_PRINTF("[上报] 📤 心跳: %s\n", statusJson.c_str());

    char serverResponse[256] = {0};
    uploadGpsIfNeeded(commModule);

    if (commModule->sendStatus(statusJson.c_str(), serverResponse, sizeof(serverResponse))) {
      DEBUG_PRINTLN("[上报] ✓ 发送成功");
      // 解析服务器指令
      if (strlen(serverResponse) > 0 && strstr(serverResponse, "\"command\"")) {
        if (strstr(serverResponse, "reboot")) {
          DEBUG_PRINTLN("[系统] 执行重启指令");
          ESP.restart();
        }
      }
    }

    commModule->sleep();
    DeviceFactory::destroy(commModule);
  }
};
