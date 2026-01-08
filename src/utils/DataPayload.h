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
    GpsLocation location;  // GPS 坐标（无效时为 0,0）
    unsigned long timestamp; // 时间戳
    
    TiltAlarmPayload() : angle(0.0f), voltage(0.0f), location(), timestamp(0) {}
    TiltAlarmPayload(float ang, float vol) 
        : angle(ang), voltage(vol), location(), timestamp(millis()) {}
    TiltAlarmPayload(float ang, float vol, double lat, double lon) 
        : angle(ang), voltage(vol), location(lat, lon), timestamp(millis()) {}
    
    bool hasValidGps() const { return location.latitude != 0.0 || location.longitude != 0.0; }
    
    String toJson() const {
        StaticJsonDocument<256> doc;
        doc["type"] = "TILT";
        doc["angle"] = serialized(String(angle, 2));
        doc["voltage"] = serialized(String(voltage, 2));
        doc["timestamp"] = timestamp;
        
        if (hasValidGps()) {
            JsonObject locObj = doc.createNestedObject("location");
            locObj["lat"] = serialized(String(location.latitude, 6));
            locObj["lon"] = serialized(String(location.longitude, 6));
        } else {
            doc["location"] = nullptr;
        }
        
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
    GpsLocation location;  // GPS 坐标
    unsigned long timestamp; // 时间戳
    
    LowBatteryPayload() : voltage(0.0f), location(), timestamp(0) {}
    explicit LowBatteryPayload(float vol) 
        : voltage(vol), location(), timestamp(millis()) {}
    LowBatteryPayload(float vol, double lat, double lon) 
        : voltage(vol), location(lat, lon), timestamp(millis()) {}
    
    bool hasValidGps() const { return location.latitude != 0.0 || location.longitude != 0.0; }
    
    String toJson() const {
        StaticJsonDocument<256> doc;
        doc["type"] = "LOW_BATTERY";
        doc["voltage"] = serialized(String(voltage, 2));
        doc["timestamp"] = timestamp;
        
        if (hasValidGps()) {
            JsonObject locObj = doc.createNestedObject("location");
            locObj["lat"] = serialized(String(location.latitude, 6));
            locObj["lon"] = serialized(String(location.longitude, 6));
        } else {
            doc["location"] = nullptr;
        }
        
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
    uint16_t soundLevel;    // 声音峰峰值 (0-4095)
    uint8_t soundPercent;   // 声音强度百分比 (0-100)
    GpsLocation location;   // GPS 坐标
    unsigned long timestamp; // 时间戳
    
    NoiseAlarmPayload() : voltage(0.0f), soundLevel(0), soundPercent(0), 
                          location(), timestamp(0) {}
    
    NoiseAlarmPayload(float vol, uint16_t level = 0) 
        : voltage(vol), soundLevel(level), soundPercent(map(level, 0, 4095, 0, 100)),
          location(), timestamp(millis()) {}
    
    NoiseAlarmPayload(float vol, uint16_t level, double lat, double lon) 
        : voltage(vol), soundLevel(level), soundPercent(map(level, 0, 4095, 0, 100)),
          location(lat, lon), timestamp(millis()) {}
    
    bool hasValidGps() const { return location.latitude != 0.0 || location.longitude != 0.0; }
    
    String toJson() const {
        StaticJsonDocument<384> doc;
        doc["type"] = "NOISE";
        doc["voltage"] = serialized(String(voltage, 2));
        doc["soundLevel"] = soundLevel;
        doc["soundPercent"] = soundPercent;
        doc["timestamp"] = timestamp;
        
        if (hasValidGps()) {
            JsonObject locObj = doc.createNestedObject("location");
            locObj["lat"] = serialized(String(location.latitude, 6));
            locObj["lon"] = serialized(String(location.longitude, 6));
        } else {
            doc["location"] = nullptr;
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
    GpsLocation location;  // GPS 坐标
    
    StatusPayload() : angle(0.0f), voltage(0.0f), uptime(0), 
                      version(FIRMWARE_VERSION), location() {}
    
    StatusPayload(float ang, float vol) 
        : angle(ang), voltage(vol), uptime(millis() / 1000), 
          version(FIRMWARE_VERSION), location() {}
    
    StatusPayload(float ang, float vol, double lat, double lon) 
        : angle(ang), voltage(vol), uptime(millis() / 1000), 
          version(FIRMWARE_VERSION), location(lat, lon) {}
    
    bool hasValidGps() const { return location.latitude != 0.0 || location.longitude != 0.0; }
    
    String toJson() const {
        StaticJsonDocument<512> doc;
        doc["type"] = "STATUS";
        doc["angle"] = serialized(String(angle, 2));
        doc["voltage"] = serialized(String(voltage, 2));
        doc["uptime"] = uptime;
        doc["version"] = version;
        
        if (hasValidGps()) {
            JsonObject locObj = doc.createNestedObject("location");
            locObj["lat"] = serialized(String(location.latitude, 6));
            locObj["lon"] = serialized(String(location.longitude, 6));
        } else {
            doc["location"] = nullptr;
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

