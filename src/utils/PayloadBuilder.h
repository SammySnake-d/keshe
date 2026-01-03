#pragma once

/**
 * @file PayloadBuilder.h
 * @brief JSON 数据封装工具类
 * @note 用于构建标准的 MQTT 消息格式
 */

#include "../../include/AppConfig.h"
#include <Arduino.h>

class PayloadBuilder {
public:
    /**
     * @brief 构建倾斜报警 JSON
     * @param angle 倾斜角度
     * @param voltage 电池电压
     * @return JSON 字符串
     */
    static String buildTiltAlarm(float angle, float voltage) {
        String json = "{";
        json += "\"type\":\"TILT\",";
        json += "\"angle\":" + String(angle, 2) + ",";
        json += "\"voltage\":" + String(voltage, 2) + ",";
        json += "\"timestamp\":" + String(millis());
        json += "}";
        return json;
    }
    
    /**
     * @brief 构建低电量报警 JSON
     * @param voltage 电池电压
     * @return JSON 字符串
     */
    static String buildLowBatteryAlarm(float voltage) {
        String json = "{";
        json += "\"type\":\"LOW_BATTERY\",";
        json += "\"voltage\":" + String(voltage, 2) + ",";
        json += "\"timestamp\":" + String(millis());
        json += "}";
        return json;
    }
    
    /**
     * @brief 构建状态心跳 JSON
     * @param angle 当前倾斜角度
     * @param voltage 电池电压
     * @return JSON 字符串
     */
    static String buildStatusHeartbeat(float angle, float voltage) {
        String json = "{";
        json += "\"type\":\"STATUS\",";
        json += "\"angle\":" + String(angle, 2) + ",";
        json += "\"voltage\":" + String(voltage, 2) + ",";
        json += "\"uptime\":" + String(millis() / 1000) + ",";
        json += "\"version\":\"" FIRMWARE_VERSION "\"";
        json += "}";
        return json;
    }
    
    /**
     * @brief 构建完整报警 JSON（包含 GPS 坐标）
     * @param angle 倾斜角度
     * @param voltage 电池电压
     * @param lat 纬度
     * @param lon 经度
     * @return JSON 字符串
     */
    static String buildFullAlarm(float angle, float voltage, double lat, double lon) {
        String json = "{";
        json += "\"type\":\"TILT\",";
        json += "\"angle\":" + String(angle, 2) + ",";
        json += "\"voltage\":" + String(voltage, 2) + ",";
        json += "\"location\":{";
        json += "\"lat\":" + String(lat, 6) + ",";
        json += "\"lon\":" + String(lon, 6);
        json += "},";
        json += "\"timestamp\":" + String(millis());
        json += "}";
        return json;
    }
};
