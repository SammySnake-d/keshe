#pragma once

/**
 * @file WorkflowManager.h
 * @brief 业务流程管理器 - 处理各种唤醒场景的业务逻辑
 */

#include "../../include/AppConfig.h"
#include "../interfaces/ISensor.h"
#include "../interfaces/IComm.h"
#include "../interfaces/IGPS.h"
#include "../utils/DataPayload.h"
#include "DeviceFactory.h"
#include "SystemManager.h"
#include "../modules/sensors/AudioSensor.h"
#include "../modules/camera/CameraManager.h"

class WorkflowManager {
public:
    /**
     * @brief 首次启动校准流程
     */
    static void handleFirstBoot() {
        DEBUG_PRINTLN("\n[MAIN] 🔧 首次启动 - 执行零点校准");
        
        ISensor* tiltSensor = DeviceFactory::createTiltSensor();
        if (!tiltSensor || !tiltSensor->init()) {
            DEBUG_PRINTLN("[MAIN] ❌ 传感器初始化失败");
            return;
        }
        
        float initialAngle = tiltSensor->readData();
        SystemManager::calibrateInitialPose(initialAngle, 0);
        DEBUG_PRINTF("[SYS] 零点校准完成: Pitch=%.2f°, Roll=0.00°\n", initialAngle);
        
        AudioSensor::init();
        DEBUG_PRINTLN("[Audio] 声音传感器初始化完成");
        
        tiltSensor->sleep();
        DeviceFactory::destroy(tiltSensor);
        
        DEBUG_PRINTLN("[MAIN] ✓ 校准完成，进入首次休眠\n");
    }
    
    /**
     * @brief 定时器唤醒 - 心跳巡检流程
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
        
        // 2. 检查是否超过阈值触发报警
        if (relativeAngle > TILT_THRESHOLD) {
            DEBUG_PRINTLN("\n[MAIN] 🚨 检测到倾斜！启动报警流程");
            
            if (sendTiltAlarmWithPhoto(relativeAngle, batteryVoltage)) {
                SystemManager::deepSleep(SLEEP_DURATION_ALARM);
                return;
            }
        }
        
        // 3. 正常心跳上报
        sendStatusHeartbeat(relativeAngle, batteryVoltage);
        
        DEBUG_PRINTLN("[MAIN] ✓ 心跳完成，进入休眠\n");
        SystemManager::deepSleep(SLEEP_DURATION_NORMAL);
    }
    
    /**
     * @brief 声音中断唤醒 - 噪音报警流程
     */
    static void handleAudioWakeup() {
        DEBUG_PRINTLN("\n[MAIN] 🔊 声音中断唤醒 - 异常音检测");
        
        // 确认是否真的是声音触发
        if (!AudioSensor::isNoiseDetected()) {
            DEBUG_PRINTLN("[MAIN] ⚠️ 误触发，返回休眠");
            SystemManager::deepSleep(SLEEP_DURATION_NORMAL);
            return;
        }
        
        float batteryVoltage = SystemManager::readBatteryVoltage();
        
        sendNoiseAlarmWithPhoto(batteryVoltage);
        
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
     * @brief 拍照
     * @param photoSize 输出照片大小
     * @return true=拍照成功, false=拍照失败
     */
    static bool takePhoto(size_t& photoSize) {
        uint8_t* photoBuffer = nullptr;
        photoSize = 0;
        
        if (!CameraManager::init()) {
            return false;
        }
        
        bool success = CameraManager::capturePhoto(&photoBuffer, &photoSize);
        
        if (success) {
            DEBUG_PRINTF("[MAIN] ✓ 拍照成功 (%d bytes)\n", photoSize);
        }
        
        CameraManager::releasePhoto();
        CameraManager::powerOff();
        
        return success;
    }
    
    /**
     * @brief 发送倾斜报警（含拍照和 GPS）
     */
    static bool sendTiltAlarmWithPhoto(float angle, float voltage) {
        // 1. 获取 GPS
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
        size_t photoSize = 0;
        takePhoto(photoSize);
        
        // 4. 构建并发送报警
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
        
        bool success = commModule->sendAlarm(alarmJson.c_str());
        
        commModule->sleep();
        DeviceFactory::destroy(commModule);
        
        return success;
    }
    
    /**
     * @brief 发送噪音报警（含拍照和 GPS）
     */
    static bool sendNoiseAlarmWithPhoto(float voltage) {
        // 1. 获取 GPS
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
        size_t photoSize = 0;
        takePhoto(photoSize);
        
        // 4. 构建并发送报警
        String alarmJson;
        if (hasGps) {
            NoiseAlarmPayload alarmData(voltage, gpsData.latitude, gpsData.longitude);
            alarmJson = alarmData.toJson();
            DEBUG_PRINTLN("[MAIN] 📤 发送带 GPS 的噪音报警");
        } else {
            NoiseAlarmPayload alarmData(voltage);
            alarmJson = alarmData.toJson();
            DEBUG_PRINTLN("[MAIN] 📤 发送不带 GPS 的噪音报警");
        }
        
        bool success = commModule->sendAlarm(alarmJson.c_str());
        
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
        commModule->sendStatus(statusJson.c_str());
        
        // 4. 检查下行指令
        char command[128] = {0};
        if (commModule->receiveCommand(command, sizeof(command))) {
            DEBUG_PRINTF("[MAIN] 📥 收到指令: %s\n", command);
            // TODO: 解析并执行指令（重启、修改上报间隔等）
        }
        
        commModule->sleep();
        DeviceFactory::destroy(commModule);
    }
};
