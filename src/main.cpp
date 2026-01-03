/**
 * @file main.cpp
 * @brief 通信电缆杆监测系统 - MVP 主程序
 * @architecture 多唤醒源状态机 (Timer + Audio + [Future: Tilt INT])
 */

#include <Arduino.h>
#include "../include/AppConfig.h"
#include "core/SystemManager.h"
#include "core/DeviceFactory.h"
#include "modules/sensors/AudioSensor.h"
#include "modules/camera/CameraManager.h"
#include "utils/PayloadBuilder.h"

// ==================== 全局变量 ====================
ISensor* tiltSensor = nullptr;
IComm* commModule = nullptr;
esp_sleep_wakeup_cause_t wakeupCause;

// RTC 内存：跨越重启保持
RTC_DATA_ATTR bool hasCalibrated = false;
RTC_DATA_ATTR uint8_t bootCount = 0;

// ==================== 函数声明 ====================
void handleFirstBoot();
void handleTimerWakeup();
void handleAudioWakeup();
bool takePhotoAndUpload(float angle, float voltage);

// ==================== 主程序入口 ====================
void setup() {
    Serial.begin(115200);
    delay(100);
    
    // ========== [1] 系统初始化 ==========
    SystemManager::init();
    
    // 获取唤醒原因
    wakeupCause = SystemManager::getWakeupCause();
    
    // 统计启动次数（调试用）
    bootCount++;
    DEBUG_PRINTF("\n[MAIN] 🔢 启动计数: %d (RTC 内存保持)\n", bootCount);
    
    // ========== [2] 首次启动：零点校准 ==========
    // 关键修复：完全依赖 RTC 内存标志，不依赖硬件唤醒原因
    // 原因：Wokwi 可能无法完全模拟 esp_sleep_get_wakeup_cause()
    bool isFirstBoot = !hasCalibrated;
    
    if (isFirstBoot) {
        hasCalibrated = true;
        handleFirstBoot();
        return; // 校准后进入休眠，下次唤醒执行正常流程
    }
    
    // ========== [3] 电池检查 ==========
    if (!SystemManager::isBatteryHealthy()) {
        DEBUG_PRINTLN("[MAIN] ⚠️ 电量不足，直接休眠");
        SystemManager::deepSleep(SLEEP_DURATION_LOW_BAT);
        return;
    }
    
    // ========== [4] 根据唤醒原因分支处理 ==========
    #if ENABLE_DEEP_SLEEP
        // 真实硬件：根据硬件唤醒原因判断
        switch (wakeupCause) {
            case ESP_SLEEP_WAKEUP_TIMER:
                handleTimerWakeup();
                break;
            case ESP_SLEEP_WAKEUP_EXT0:
                handleAudioWakeup();
                break;
            default:
                DEBUG_PRINTLN("[MAIN] ⚠️ 未知唤醒原因");
                SystemManager::deepSleep(SLEEP_DURATION_NORMAL);
                break;
        }
    #else
        // Wokwi 模式：由于无法获取真实唤醒原因，默认执行定时器唤醒流程
        // 如需测试音频唤醒，可手动切换到 handleAudioWakeup()
        handleTimerWakeup();
    #endif
}

void loop() {
    #if !ENABLE_DEEP_SLEEP
        // Wokwi 模拟模式：等待延时结束后继续执行下一轮
        // deepSleep() 中的 delay() 结束后会回到这里
        delay(100);
        setup(); // 重新进入状态机（模拟唤醒）
    #else
        // 真实硬件：深度睡眠后不会回到这里
        DEBUG_PRINTLN("[MAIN] ⚠️ 不应该到达这里！");
        delay(1000);
    #endif
}

// ==================== 首次启动处理 ====================
void handleFirstBoot() {
    DEBUG_PRINTLN("\n[MAIN] 🔧 首次启动 - 执行零点校准");
    
    // 1. 创建传感器
    tiltSensor = DeviceFactory::createTiltSensor();
    if (!tiltSensor || !tiltSensor->init()) {
        DEBUG_PRINTLN("[MAIN] ❌ 传感器初始化失败");
        SystemManager::deepSleep(SLEEP_DURATION_NORMAL);
        return;
    }
    
    // 2. 读取初始姿态
    float initialAngle = tiltSensor->readData();
    SystemManager::calibrateInitialPose(initialAngle, 0);
    
    // 3. 初始化声音传感器
    AudioSensor::init();
    AudioSensor::enableWakeupInterrupt();
    
    // 4. 清理并进入休眠
    tiltSensor->sleep();
    DeviceFactory::destroy(tiltSensor);
    
    DEBUG_PRINTLN("[MAIN] ✓ 校准完成，进入首次休眠\n");
    SystemManager::deepSleep(SLEEP_DURATION_NORMAL);
}

// ==================== 定时器唤醒：心跳巡检 ====================
void handleTimerWakeup() {
    DEBUG_PRINTLN("\n[MAIN] ⏰ 定时器唤醒 - 心跳巡检");
    
    float batteryVoltage = SystemManager::readBatteryVoltage();
    
    // 1. 创建传感器
    tiltSensor = DeviceFactory::createTiltSensor();
    if (!tiltSensor || !tiltSensor->init()) {
        DEBUG_PRINTLN("[MAIN] ❌ 传感器初始化失败");
        SystemManager::deepSleep(SLEEP_DURATION_NORMAL);
        return;
    }
    
    // 2. 读取倾角
    float currentAngle = tiltSensor->readData();
    float relativeAngle = SystemManager::getRelativeTilt(currentAngle, 0);
    
    DEBUG_PRINTF("[MAIN] 📐 当前倾角: %.2f° (相对初始: %.2f°)\n", currentAngle, relativeAngle);
    
    // 3. 判断是否需要报警
    if (relativeAngle > TILT_THRESHOLD) {
        DEBUG_PRINTLN("\n[MAIN] 🚨 检测到倾斜！启动报警流程");
        
        if (takePhotoAndUpload(relativeAngle, batteryVoltage)) {
            // 报警后短休眠（5分钟后再检查）
            tiltSensor->sleep();
            DeviceFactory::destroy(tiltSensor);
            SystemManager::deepSleep(SLEEP_DURATION_ALARM);
            return;
        }
    }
    
    // 4. 正常状态：上报心跳
    commModule = DeviceFactory::createCommModule();
    if (commModule && commModule->init() && commModule->connectNetwork()) {
        String statusPayload = PayloadBuilder::buildStatusHeartbeat(relativeAngle, batteryVoltage);
        commModule->sendStatus(statusPayload.c_str());
        
        // 5. 检查下行指令
        char command[128] = {0};
        if (commModule->receiveCommand(command, sizeof(command))) {
            DEBUG_PRINTF("[MAIN] 📥 收到指令: %s\n", command);
            // TODO: 解析并执行指令（重启、修改上报间隔等）
        }
        
        commModule->sleep();
    }
    
    // 6. 清理资源
    if (tiltSensor) {
        tiltSensor->sleep();
        DeviceFactory::destroy(tiltSensor);
    }
    if (commModule) {
        DeviceFactory::destroy(commModule);
    }
    
    DEBUG_PRINTLN("[MAIN] ✓ 心跳完成，进入休眠\n");
    SystemManager::deepSleep(SLEEP_DURATION_NORMAL);
}

// ==================== 声音中断唤醒：异常音报警 ====================
void handleAudioWakeup() {
    DEBUG_PRINTLN("\n[MAIN] 🔊 声音中断唤醒 - 异常音检测");
    
    // 确认是否真的是声音触发
    if (!AudioSensor::isNoiseDetected()) {
        DEBUG_PRINTLN("[MAIN] ⚠️ 误触发，返回休眠");
        SystemManager::deepSleep(SLEEP_DURATION_NORMAL);
        return;
    }
    
    float batteryVoltage = SystemManager::readBatteryVoltage();
    
    // 拍照并上传
    DEBUG_PRINTLN("[MAIN] 📸 声音异常 → 拍照取证");
    
    commModule = DeviceFactory::createCommModule();
    if (!commModule || !commModule->init() || !commModule->connectNetwork()) {
        DEBUG_PRINTLN("[MAIN] ❌ 通信模块启动失败");
        SystemManager::deepSleep(SLEEP_DURATION_NORMAL);
        return;
    }
    
    // 拍照
    uint8_t* photoBuffer = nullptr;
    size_t photoSize = 0;
    
    if (CameraManager::init() && CameraManager::capturePhoto(&photoBuffer, &photoSize)) {
        // 构建声音报警 JSON
        String alarmPayload = "{\"type\":\"NOISE\",\"voltage\":" + String(batteryVoltage, 2) + 
                              ",\"photo_size\":" + String(photoSize) + "}";
        
        commModule->sendAlarm(alarmPayload.c_str());
        
        CameraManager::releasePhoto();
        CameraManager::powerOff();
    }
    
    commModule->sleep();
    DeviceFactory::destroy(commModule);
    
    DEBUG_PRINTLN("[MAIN] ✓ 声音报警完成，进入休眠\n");
    SystemManager::deepSleep(SLEEP_DURATION_ALARM);
}

// ==================== 拍照并上传 ====================
bool takePhotoAndUpload(float angle, float voltage) {
    // 1. 初始化通信模块
    commModule = DeviceFactory::createCommModule();
    if (!commModule || !commModule->init() || !commModule->connectNetwork()) {
        DEBUG_PRINTLN("[MAIN] ❌ 通信模块启动失败");
        return false;
    }
    
    // 2. 拍照（需先关闭传感器释放 I2C）
    if (tiltSensor) {
        tiltSensor->sleep();
    }
    
    uint8_t* photoBuffer = nullptr;
    size_t photoSize = 0;
    bool photoSuccess = false;
    
    if (CameraManager::init()) {
        if (CameraManager::capturePhoto(&photoBuffer, &photoSize)) {
            DEBUG_PRINTF("[MAIN] ✓ 拍照成功 (%d bytes)\n", photoSize);
            photoSuccess = true;
        }
        CameraManager::releasePhoto();
        CameraManager::powerOff();
    }
    
    // 3. 发送报警
    String alarmPayload = PayloadBuilder::buildTiltAlarm(angle, voltage);
    bool sendSuccess = commModule->sendAlarm(alarmPayload.c_str());
    
    // 4. 清理
    commModule->sleep();
    DeviceFactory::destroy(commModule);
    
    return sendSuccess;
}

