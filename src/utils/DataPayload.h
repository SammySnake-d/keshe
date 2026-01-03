#pragma once

/**
 * @file DataPayload.h
 * @brief 数据传输结构体定义（JSON 序列化）
 * @note 使用 ArduinoJson 库进行对象序列化，替代手动拼接字符串
 */

#include "../../include/AppConfig.h"
#include <Arduino.h>
#include <ArduinoJson.h>

/**
 * @brief 消息类型枚举
 */
enum class PayloadType {
    TILT,        // 倾斜报警
    LOW_BATTERY, // 低电量报警
    STATUS,      // 状态心跳
    FULL_ALARM   // 完整报警（含GPS）
};

/**
 * @brief GPS 坐标结构体
 */
struct GpsLocation {
    double latitude;   // 纬度
    double longitude;  // 经度
    
    GpsLocation() : latitude(0.0), longitude(0.0) {}
    GpsLocation(double lat, double lon) : latitude(lat), longitude(lon) {}
};

/**
 * @brief 倾斜报警数据结构体
 */
struct TiltAlarmPayload {
    float angle;           // 倾斜角度
    float voltage;         // 电池电压
    unsigned long timestamp; // 时间戳
    
    TiltAlarmPayload() : angle(0.0f), voltage(0.0f), timestamp(0) {}
    TiltAlarmPayload(float ang, float vol) 
        : angle(ang), voltage(vol), timestamp(millis()) {}
    
    /**
     * @brief 使用 ArduinoJson 序列化为 JSON 字符串
     */
    String toJson() const {
        StaticJsonDocument<256> doc;
        doc["type"] = "TILT";
        doc["angle"] = serialized(String(angle, 2));      // 保留2位小数
        doc["voltage"] = serialized(String(voltage, 2));  // 保留2位小数
        doc["timestamp"] = timestamp;
        
        String json;
        serializeJson(doc, json);
        return json;
    }
};

/**
 * @brief 低电量报警数据结构体
 */
struct LowBatteryPayload {
    float voltage;         // 电池电压
    unsigned long timestamp; // 时间戳
    
    LowBatteryPayload() : voltage(0.0f), timestamp(0) {}
    explicit LowBatteryPayload(float vol) 
        : voltage(vol), timestamp(millis()) {}
    
    /**
     * @brief 使用 ArduinoJson 序列化为 JSON 字符串
     */
    String toJson() const {
        StaticJsonDocument<256> doc;
        doc["type"] = "LOW_BATTERY";
        doc["voltage"] = serialized(String(voltage, 2));  // 保留2位小数
        doc["timestamp"] = timestamp;
        
        String json;
        serializeJson(doc, json);
        return json;
    }
};

/**
 * @brief 噪音报警数据结构体
 */
struct NoiseAlarmPayload {
    float voltage;          // 电池电压
    bool hasGps;            // 是否包含 GPS
    GpsLocation location;   // GPS 坐标（可选）
    unsigned long timestamp; // 时间戳
    
    NoiseAlarmPayload() : voltage(0.0f), hasGps(false), location(), timestamp(0) {}
    
    NoiseAlarmPayload(float vol) 
        : voltage(vol), hasGps(false), location(), timestamp(millis()) {}
    
    NoiseAlarmPayload(float vol, double lat, double lon) 
        : voltage(vol), hasGps(true), location(lat, lon), timestamp(millis()) {}
    
    /**
     * @brief 使用 ArduinoJson 序列化为 JSON 字符串
     */
    String toJson() const {
        StaticJsonDocument<384> doc;
        doc["type"] = "NOISE";
        doc["voltage"] = serialized(String(voltage, 2));
        doc["timestamp"] = timestamp;
        
        // 如果有 GPS 数据，添加 location 字段
        if (hasGps) {
            JsonObject locObj = doc.createNestedObject("location");
            locObj["lat"] = serialized(String(location.latitude, 6));
            locObj["lon"] = serialized(String(location.longitude, 6));
        }
        
        String json;
        serializeJson(doc, json);
        return json;
    }
};

/**
 * @brief 状态心跳数据结构体
 */
struct StatusPayload {
    float angle;           // 当前倾斜角度
    float voltage;         // 电池电压
    unsigned long uptime;  // 运行时间（秒）
    String version;        // 固件版本
    bool hasGps;           // 是否包含 GPS 数据
    GpsLocation location;  // GPS 坐标（可选）
    
    StatusPayload() : angle(0.0f), voltage(0.0f), uptime(0), 
                      version(FIRMWARE_VERSION), hasGps(false), location() {}
    
    StatusPayload(float ang, float vol) 
        : angle(ang), voltage(vol), uptime(millis() / 1000), 
          version(FIRMWARE_VERSION), hasGps(false), location() {}
    
    StatusPayload(float ang, float vol, double lat, double lon) 
        : angle(ang), voltage(vol), uptime(millis() / 1000), 
          version(FIRMWARE_VERSION), hasGps(true), location(lat, lon) {}
    
    /**
     * @brief 使用 ArduinoJson 序列化为 JSON 字符串
     */
    String toJson() const {
        StaticJsonDocument<512> doc;
        doc["type"] = "STATUS";
        doc["angle"] = serialized(String(angle, 2));      // 保留2位小数
        doc["voltage"] = serialized(String(voltage, 2));  // 保留2位小数
        doc["uptime"] = uptime;
        doc["version"] = version;
        
        // 如果有 GPS 数据，添加 location 字段
        if (hasGps) {
            JsonObject locObj = doc.createNestedObject("location");
            locObj["lat"] = serialized(String(location.latitude, 6));
            locObj["lon"] = serialized(String(location.longitude, 6));
        }
        
        String json;
        serializeJson(doc, json);
        return json;
    }
};

/**
 * @brief 完整报警数据结构体（含GPS坐标）
 */
struct FullAlarmPayload {
    float angle;            // 倾斜角度
    float voltage;          // 电池电压
    GpsLocation location;   // GPS 坐标
    unsigned long timestamp; // 时间戳
    
    FullAlarmPayload() : angle(0.0f), voltage(0.0f), location(), timestamp(0) {}
    FullAlarmPayload(float ang, float vol, double lat, double lon) 
        : angle(ang), voltage(vol), location(lat, lon), timestamp(millis()) {}
    
    /**
     * @brief 使用 ArduinoJson 序列化为 JSON 字符串
     */
    String toJson() const {
        StaticJsonDocument<512> doc;
        doc["type"] = "TILT"; // 保持兼容性，类型仍为 TILT 但包含 location
        doc["angle"] = serialized(String(angle, 2));      // 保留2位小数
        doc["voltage"] = serialized(String(voltage, 2));  // 保留2位小数
        
        JsonObject locObj = doc.createNestedObject("location");
        locObj["lat"] = serialized(String(location.latitude, 6));   // 保留6位小数
        locObj["lon"] = serialized(String(location.longitude, 6));  // 保留6位小数
        
        doc["timestamp"] = timestamp;
        
        String json;
        serializeJson(doc, json);
        return json;
    }
};

