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
    DEBUG_PRINTLN("\n[MAIN] 🔧 首次启动 - 执行零点校准");

    // 倾斜传感器校准
    ISensor *tiltSensor = DeviceFactory::createTiltSensor();
    if (!tiltSensor || !tiltSensor->init()) {
      DEBUG_PRINTLN("[MAIN] ❌ 传感器初始化失败");
      return;
    }

    // 读取初始角度（绝对值）
    LSM6DS3_Sensor *lsm = static_cast<LSM6DS3_Sensor *>(tiltSensor);
    float initialPitch = lsm->getAbsolutePitch();
    float initialRoll = 0.0f; // 简化处理，仅校准 Pitch

    // 保存到 RTC 内存和传感器对象
    SystemManager::calibrateInitialPose(initialPitch, initialRoll);
    lsm->calibrate(initialPitch, initialRoll);

    DEBUG_PRINTF("[SYS] 零点校准完成: Pitch=%.2f°, Roll=%.2f°\n", initialPitch,
                 initialRoll);

    // 音频传感器初始化
    IAudio *audioSensor = DeviceFactory::createAudioSensor();
    if (audioSensor && audioSensor->init()) {
      DEBUG_PRINTLN("[Audio] 声音传感器初始化完成");
    }
    DeviceFactory::destroy(audioSensor);

    tiltSensor->sleep();
    DeviceFactory::destroy(tiltSensor);

    DEBUG_PRINTLN("[MAIN] ✓ 校准完成，进入首次休眠\n");
  }

  /**
   * @brief 定时器唤醒 - 心跳巡检流程
   * @note 每次唤醒时检查倾斜和声音，判断是否超过阈值
   */
  static void handleTimerWakeup() {
    DEBUG_PRINTLN("\n[MAIN] ⏰ 定时器唤醒 - 心跳巡检");

    // 读取并显示电池状态
    float batteryVoltage = SystemManager::readBatteryVoltage();
    int batteryPercent = SystemManager::getBatteryPercentage();
    DEBUG_PRINTF("[MAIN] 🔋 电池状态: %.2fV (%d%%)\n", batteryVoltage,
                 batteryPercent);

    // 1. 读取倾角（相对于初始位置的偏移）
    float relativeAngle = readTiltAngle();
    if (relativeAngle < 0) {
      SystemManager::deepSleep(HEARTBEAT_INTERVAL_SEC);
      return;
    }

    // 2. 检查是否超过 5° 倾斜阈值（软件判断）
    if (relativeAngle > TILT_THRESHOLD) {
      DEBUG_PRINTF("\n[MAIN] 🚨 检测到倾斜 %.2f° > %.2f° 阈值！启动报警流程\n",
                   relativeAngle, TILT_THRESHOLD);

      // 更新倾斜触发时间 (用于 GPS 联动)
      g_last_tilt_trigger_ms = millis();

      if (sendTiltAlarmWithPhoto(relativeAngle, batteryVoltage)) {
        SystemManager::deepSleep(SLEEP_DURATION_ALARM);
        return;
      }
    }

    // 3. 检查声音是否超过阈值（模拟信号，软件轮询检测）
    DEBUG_PRINTLN("[MAIN] 🔊 检测环境声音...");
    IAudio *audioSensor = DeviceFactory::createAudioSensor();
    if (audioSensor && audioSensor->init() && audioSensor->isNoiseDetected()) {
      DEBUG_PRINTLN("\n[MAIN] 🚨 检测到异常声音！启动报警流程");
      uint16_t soundLevel = audioSensor->readPeakToPeak(); // 保存声音等级
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

    // 4. 正常心跳上报
    sendStatusHeartbeat(relativeAngle, batteryVoltage);

    DEBUG_PRINTLN("[MAIN] ✓ 心跳完成，进入休眠\n");
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
    if (!tiltSensor || !tiltSensor->init()) {
      DEBUG_PRINTLN("[MAIN] ❌ 传感器初始化失败");
      DeviceFactory::destroy(tiltSensor);
      return -1.0f;
    }

    // 恢复零点校准值（从 RTC 内存读取）
    LSM6DS3_Sensor *lsm = static_cast<LSM6DS3_Sensor *>(tiltSensor);
    float initialPitch = SystemManager::getInitialPitch();
    float initialRoll = SystemManager::getInitialRoll();
    lsm->calibrate(initialPitch, initialRoll);

    // 读取相对倾角（已经在 readData 中计算相对值）
    float relativeAngle = tiltSensor->readData();

    DEBUG_PRINTF("[MAIN] 📐 相对倾角: %.2f°\n", relativeAngle);

    tiltSensor->sleep();
    DeviceFactory::destroy(tiltSensor);

    return relativeAngle;
  }

  /**
   * @brief 获取 GPS 定位数据
   * @param gpsData 输出的 GPS 数据
   * @return true=定位成功, false=定位失败
   */
  static bool getGpsLocation(GpsData &gpsData) {
    IGPS *gps = DeviceFactory::createGpsModule();
    if (!gps || !gps->init()) {
      DeviceFactory::destroy(gps);
      return false;
    }

    DEBUG_PRINTLN("[MAIN] 📡 正在获取 GPS 定位...");
    unsigned long gpsTimeout = USE_MOCK_HARDWARE ? 5000 : 30000;

    bool success = gps->getLocation(gpsData, gpsTimeout);

    if (success) {
      DEBUG_PRINTF("[MAIN] ✓ GPS 定位成功: %.6f, %.6f\n", gpsData.latitude,
                   gpsData.longitude);
    } else {
      DEBUG_PRINTLN("[MAIN] ⚠️  GPS 定位失败");
    }

    gps->sleep();
    DeviceFactory::destroy(gps);

    return success;
  }

  /**
   * @brief 统一报警处理流程 (拍照 -> 上传图片 -> 发送数据)
   * @param type 报警类型 ("tilt" 或 "noise")
   * @param value 报警数值 (角度或声音等级)
   * @param voltage 电池电压
   */
  static bool dispatchAlarm(const char *type, float value, float voltage) {
    // 1. 获取 GPS 位置
    GpsData gpsData;
    bool hasGps = getGpsLocation(gpsData);

    // 2. 初始化通信模块
    IComm *commModule = DeviceFactory::createCommModule();
    if (!commModule || !commModule->init() || !commModule->connectNetwork()) {
      DEBUG_PRINTLN("[MAIN] ❌ 通信模块启动失败");
      DeviceFactory::destroy(commModule);
      return false;
    }

    // 3. 拍照并上传
    ICamera *camera = DeviceFactory::createCamera();
    if (camera && camera->init()) {
      uint8_t *photoBuffer = nullptr;
      size_t photoSize = 0;
      if (camera->capturePhoto(&photoBuffer, &photoSize)) {
        DEBUG_PRINTF("[MAIN] ✓ 拍照成功 (%d bytes)\n", photoSize);

        // 构建图片元数据
        String metadata;
        if (strcmp(type, "tilt") == 0) {
          metadata =
              String("{\"device_id\":\"") + HTTP_DEVICE_ID +
              String("\",\"type\":\"tilt\",\"angle\":" + String(value, 2) +
                     "}");
        } else {
          metadata = String("{\"device_id\":\"") + HTTP_DEVICE_ID +
                     String("\",\"type\":\"noise\",\"sound_level\":" +
                            String((int)value) + "}");
        }

        if (commModule->uploadImage(photoBuffer, photoSize, metadata.c_str())) {
          DEBUG_PRINTLN("[MAIN] ✓ 图片上传成功");
        } else {
          DEBUG_PRINTLN("[MAIN] ⚠️ 图片上传失败");
        }
      }
      camera->releasePhoto();
      camera->powerOff();
      DeviceFactory::destroy(camera);
    }

    // 4. 构建报警 JSON
    String alarmJson;
    if (strcmp(type, "tilt") == 0) {
      if (hasGps) {
        alarmJson = FullAlarmPayload(value, voltage, gpsData.latitude,
                                     gpsData.longitude)
                        .toJson();
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

    DEBUG_PRINTF("[MAIN] 📤 发送 %s 报警数据\n", type);
    DEBUG_PRINTF("[MAIN] 📦 上报内容: %s\n", alarmJson.c_str());

    // 5. 发送数据
    char serverResponse[256] = {0};
    bool success = commModule->sendAlarm(alarmJson.c_str(), serverResponse,
                                         sizeof(serverResponse));

    if (success && strlen(serverResponse) > 0) {
      DEBUG_PRINTF("[MAIN] 📥 服务器响应: %s\n", serverResponse);
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
   * @brief 发送状态心跳（含 GPS）
   */
  static void sendStatusHeartbeat(float angle, float voltage) {
    // 1. 获取 GPS
    GpsData gpsData;
    bool hasGps = getGpsLocation(gpsData);

    // 2. 初始化通信模块
    IComm *commModule = DeviceFactory::createCommModule();
    if (!commModule || !commModule->init() || !commModule->connectNetwork()) {
      DEBUG_PRINTLN("[MAIN] ❌ 通信模块启动失败");
      DeviceFactory::destroy(commModule);
      return;
    }

    // 3. 构建并发送心跳
    StatusPayload statusData;
    if (hasGps) {
      statusData =
          StatusPayload(angle, voltage, gpsData.latitude, gpsData.longitude);
    } else {
      statusData = StatusPayload(angle, voltage);
    }

    String statusJson = statusData.toJson();
    DEBUG_PRINTLN("[MAIN] 📤 发送心跳数据");
    DEBUG_PRINTF("[MAIN] 📦 上报内容: %s\n", statusJson.c_str());

    // 4. 发送状态并接收服务器响应（HTTP 捎带下行指令）
    char serverResponse[256] = {0};

    // 插入: 检查是否需要单独上传 GPS (参考 project-name 的 60s 定时上传)
    uploadGpsIfNeeded(commModule);

    if (commModule->sendStatus(statusJson.c_str(), serverResponse,
                               sizeof(serverResponse))) {
      DEBUG_PRINTLN("[MAIN] ✓ 心跳发送成功");

      // 解析服务器响应中的指令
      if (strlen(serverResponse) > 0) {
        DEBUG_PRINTF("[MAIN] 📥 服务器响应: %s\n", serverResponse);

        // 简单的 JSON 解析（查找 "command" 字段）
        if (strstr(serverResponse, "\"command\"")) {
          if (strstr(serverResponse, "set_interval")) {
            DEBUG_PRINTLN("[MAIN] 🔧 执行指令: 修改上报间隔");
            // TODO: 解析 value 并修改定时器
          } else if (strstr(serverResponse, "reboot")) {
            DEBUG_PRINTLN("[MAIN] 🔧 执行指令: 重启设备");
            ESP.restart();
          } else if (strstr(serverResponse, "capture")) {
            DEBUG_PRINTLN("[MAIN] 🔧 执行指令: 立即拍照");
            // TODO: 触发拍照流程
          }
        }
      }
    } else {
      DEBUG_PRINTLN("[MAIN] ⚠️ 心跳发送失败");
    }

    commModule->sleep();
    DeviceFactory::destroy(commModule);
  }
};
