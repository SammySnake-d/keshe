#pragma once

/**
 * @file WorkflowManager.h
 * @brief 业务流程管理器 - 处理各种唤醒场景的业务逻辑
 */

#include "../../include/AppConfig.h"
#include "../interfaces/ISensor.h"
#include "../interfaces/IComm.h"
#include "../interfaces/IGPS.h"
#include "../interfaces/IAudio.h"
#include "../interfaces/ICamera.h"
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
        ISensor* tiltSensor = DeviceFactory::createTiltSensor();
        if (!tiltSensor || !tiltSensor->init()) {
            DEBUG_PRINTLN("[MAIN] ❌ 传感器初始化失败");
            return;
        }
        
        float initialAngle = tiltSensor->readData();
        SystemManager::calibrateInitialPose(initialAngle, 0);
        DEBUG_PRINTF("[SYS] 零点校准完成: Pitch=%.2f°, Roll=0.00°\n", initialAngle);
        
        // 音频传感器初始化
        IAudio* audioSensor = DeviceFactory::createAudioSensor();
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
     * @note 由于声音传感器为模拟信号输出，无法触发硬件中断
     *       因此在每次定时器唤醒时同时检查声音
     */
    static void handleTimerWakeup() {
        DEBUG_PRINTLN("\n[MAIN] ⏰ 定时器唤醒 - 心跳巡检");
        
        float batteryVoltage = SystemManager::readBatteryVoltage();
        
        // 1. 读取倾角
        float relativeAngle = readTiltAngle();
        if (relativeAngle < 0) {
            SystemManager::deepSleep(SLEEP_DURATION_NORMAL);
            return;
        }
        
        // 2. 检查是否超过倾斜阈值触发报警
        if (relativeAngle > TILT_THRESHOLD) {
            DEBUG_PRINTLN("\n[MAIN] 🚨 检测到倾斜！启动报警流程");
            
            if (sendTiltAlarmWithPhoto(relativeAngle, batteryVoltage)) {
                SystemManager::deepSleep(SLEEP_DURATION_ALARM);
                return;
            }
        }
        
        // 3. 检查声音是否超过阈值（模拟信号，软件轮询检测）
        DEBUG_PRINTLN("[MAIN] 🔊 检测环境声音...");
        IAudio* audioSensor = DeviceFactory::createAudioSensor();
        if (audioSensor && audioSensor->init() && audioSensor->isNoiseDetected()) {
            DEBUG_PRINTLN("\n[MAIN] 🚨 检测到异常声音！启动报警流程");
            uint16_t soundLevel = audioSensor->readPeakToPeak();  // 保存声音等级
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
        SystemManager::deepSleep(SLEEP_DURATION_NORMAL);
    }
    
    /**
     * @brief 声音中断唤醒 - 噪音报警流程
     * @note 当前硬件为模拟信号输出，此函数仅在添加外部比较器后使用
     *       正常情况下声音检测由 handleTimerWakeup() 轮询完成
     */
    static void handleAudioWakeup() {
        DEBUG_PRINTLN("\n[MAIN] 🔊 声音中断唤醒 - 异常音检测");
        
        // 确认是否真的是声音触发（二次确认）
        IAudio* audioSensor = DeviceFactory::createAudioSensor();
        if (!audioSensor || !audioSensor->init() || !audioSensor->isNoiseDetected()) {
            DEBUG_PRINTLN("[MAIN] ⚠️ 误触发，返回休眠");
            if (audioSensor) {
                audioSensor->sleep();
                DeviceFactory::destroy(audioSensor);
            }
            SystemManager::deepSleep(SLEEP_DURATION_NORMAL);
            return;
        }
        
        uint16_t soundLevel = audioSensor->readPeakToPeak();
        audioSensor->sleep();
        DeviceFactory::destroy(audioSensor);
        
        float batteryVoltage = SystemManager::readBatteryVoltage();
        
        sendNoiseAlarmWithPhoto(batteryVoltage, soundLevel);
        
        DEBUG_PRINTLN("[MAIN] ✓ 声音报警完成，进入休眠\n");
        SystemManager::deepSleep(SLEEP_DURATION_ALARM);
    }

private:
    /**
     * @brief 读取倾角数据
     * @return 相对倾角，失败返回 -1
     */
    static float readTiltAngle() {
        ISensor* tiltSensor = DeviceFactory::createTiltSensor();
        if (!tiltSensor || !tiltSensor->init()) {
            DEBUG_PRINTLN("[MAIN] ❌ 传感器初始化失败");
            DeviceFactory::destroy(tiltSensor);
            return -1.0f;
        }
        
        float currentAngle = tiltSensor->readData();
        float relativeAngle = SystemManager::getRelativeTilt(currentAngle, 0);
        
        DEBUG_PRINTF("[MAIN] 📐 当前倾角: %.2f° (相对初始: %.2f°)\n", currentAngle, relativeAngle);
        
        tiltSensor->sleep();
        DeviceFactory::destroy(tiltSensor);
        
        return relativeAngle;
    }
    
    /**
     * @brief 获取 GPS 定位数据
     * @param gpsData 输出的 GPS 数据
     * @return true=定位成功, false=定位失败
     */
    static bool getGpsLocation(GpsData& gpsData) {
        IGPS* gps = DeviceFactory::createGpsModule();
        if (!gps || !gps->init()) {
            DeviceFactory::destroy(gps);
            return false;
        }
        
        DEBUG_PRINTLN("[MAIN] 📡 正在获取 GPS 定位...");
        unsigned long gpsTimeout = USE_MOCK_HARDWARE ? 5000 : 30000;
        
        bool success = gps->getLocation(gpsData, gpsTimeout);
        
        if (success) {
            DEBUG_PRINTF("[MAIN] ✓ GPS 定位成功: %.6f, %.6f\n", gpsData.latitude, gpsData.longitude);
        } else {
            DEBUG_PRINTLN("[MAIN] ⚠️  GPS 定位失败");
        }
        
        gps->sleep();
        DeviceFactory::destroy(gps);
        
        return success;
    }
    
    /**
     * @brief 发送倾斜报警（含拍照和 GPS）
     */
    static bool sendTiltAlarmWithPhoto(float angle, float voltage) {
        // 1. 获取 GPS 位置
        GpsData gpsData;
        bool hasGps = getGpsLocation(gpsData);
        
        // 2. 初始化通信模块
        IComm* commModule = DeviceFactory::createCommModule();
        if (!commModule || !commModule->init() || !commModule->connectNetwork()) {
            DEBUG_PRINTLN("[MAIN] ❌ 通信模块启动失败");
            DeviceFactory::destroy(commModule);
            return false;
        }
        
        // 3. 拍照
        uint8_t* photoBuffer = nullptr;
        size_t photoSize = 0;
        
        ICamera* camera = DeviceFactory::createCamera();
        if (camera && camera->init()) {
            if (camera->capturePhoto(&photoBuffer, &photoSize)) {
                DEBUG_PRINTF("[MAIN] ✓ 拍照成功 (%d bytes)\n", photoSize);
                
                // 4. 上传图片（HTTP POST）
                String metadata = String("{\"device_id\":\"") + HTTP_DEVICE_ID + 
                                  String("\",\"type\":\"tilt\",\"angle\":" + String(angle, 2) + "}");
                
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
        
        // 5. 构建并发送报警 JSON
        String alarmJson;
        if (hasGps) {
            FullAlarmPayload alarmData(angle, voltage, gpsData.latitude, gpsData.longitude);
            alarmJson = alarmData.toJson();
            DEBUG_PRINTLN("[MAIN] 📤 发送带 GPS 的倾斜报警");
        } else {
            TiltAlarmPayload alarmData(angle, voltage);
            alarmJson = alarmData.toJson();
            DEBUG_PRINTLN("[MAIN] 📤 发送不带 GPS 的倾斜报警");
        }
        
        char serverResponse[256] = {0};
        bool success = commModule->sendAlarm(alarmJson.c_str(), serverResponse, sizeof(serverResponse));
        
        if (success && strlen(serverResponse) > 0) {
            DEBUG_PRINTF("[MAIN] 📥 服务器响应: %s\n", serverResponse);
        }
        
        commModule->sleep();
        DeviceFactory::destroy(commModule);
        
        return success;
    }
    
    /**
     * @brief 发送噪音报警（含拍照、GPS 和声音等级）
     * @param voltage 电池电压
     * @param soundLevel 声音峰峰值 (0-4095)
     */
    static bool sendNoiseAlarmWithPhoto(float voltage, uint16_t soundLevel) {
        // 1. 获取 GPS 位置
        GpsData gpsData;
        bool hasGps = getGpsLocation(gpsData);
        
        // 2. 初始化通信模块
        IComm* commModule = DeviceFactory::createCommModule();
        if (!commModule || !commModule->init() || !commModule->connectNetwork()) {
            DEBUG_PRINTLN("[MAIN] ❌ 通信模块启动失败");
            DeviceFactory::destroy(commModule);
            return false;
        }
        
        // 3. 拍照
        uint8_t* photoBuffer = nullptr;
        size_t photoSize = 0;
        
        ICamera* camera = DeviceFactory::createCamera();
        if (camera && camera->init()) {
            if (camera->capturePhoto(&photoBuffer, &photoSize)) {
                DEBUG_PRINTF("[MAIN] ✓ 拍照成功 (%d bytes)\n", photoSize);
                
                // 4. 上传图片（HTTP POST）
                String metadata = String("{\"device_id\":\"") + HTTP_DEVICE_ID + 
                                  String("\",\"type\":\"noise\",\"sound_level\":" + String(soundLevel) + "}");
                
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
        
        // 5. 构建并发送报警（包含声音等级）
        String alarmJson;
        if (hasGps) {
            NoiseAlarmPayload alarmData(voltage, soundLevel, gpsData.latitude, gpsData.longitude);
            alarmJson = alarmData.toJson();
            DEBUG_PRINTLN("[MAIN] 📤 发送带 GPS 的噪音报警");
        } else {
            NoiseAlarmPayload alarmData(voltage, soundLevel);
            alarmJson = alarmData.toJson();
            DEBUG_PRINTLN("[MAIN] 📤 发送不带 GPS 的噪音报警");
        }
        
        char serverResponse[256] = {0};
        bool success = commModule->sendAlarm(alarmJson.c_str(), serverResponse, sizeof(serverResponse));
        
        if (success && strlen(serverResponse) > 0) {
            DEBUG_PRINTF("[MAIN] 📥 服务器响应: %s\n", serverResponse);
        }
        
        commModule->sleep();
        DeviceFactory::destroy(commModule);
        
        return success;
    }
    
    /**
     * @brief 发送状态心跳（含 GPS）
     */
    static void sendStatusHeartbeat(float angle, float voltage) {
        // 1. 获取 GPS
        GpsData gpsData;
        bool hasGps = getGpsLocation(gpsData);
        
        // 2. 初始化通信模块
        IComm* commModule = DeviceFactory::createCommModule();
        if (!commModule || !commModule->init() || !commModule->connectNetwork()) {
            DEBUG_PRINTLN("[MAIN] ❌ 通信模块启动失败");
            DeviceFactory::destroy(commModule);
            return;
        }
        
        // 3. 构建并发送心跳
        StatusPayload statusData;
        if (hasGps) {
            statusData = StatusPayload(angle, voltage, gpsData.latitude, gpsData.longitude);
        } else {
            statusData = StatusPayload(angle, voltage);
        }
        
        String statusJson = statusData.toJson();
        
        // 4. 发送状态并接收服务器响应（HTTP 捎带下行指令）
        char serverResponse[256] = {0};
        if (commModule->sendStatus(statusJson.c_str(), serverResponse, sizeof(serverResponse))) {
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
