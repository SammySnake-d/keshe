#pragma once

/**
 * @file IGPS.h
 * @brief GPS 模块接口定义
 */

struct GpsData {
    double latitude;       // 纬度（十进制度数）
    double longitude;      // 经度（十进制度数）
    float altitude;        // 海拔高度（米）
    float speed;           // 速度（km/h）
    float course;          // 航向（度）
    uint8_t satellites;    // 卫星数量
    float hdop;            // 水平精度因子
    bool isValid;          // 定位是否有效
    unsigned long timestamp; // 数据时间戳
    
    GpsData() : latitude(0.0), longitude(0.0), altitude(0.0f), 
                speed(0.0f), course(0.0f), satellites(0), 
                hdop(99.9f), isValid(false), timestamp(0) {}
};

class IGPS {
public:
    virtual ~IGPS() {}
    
    /**
     * @brief 初始化 GPS 模块
     * @return true=成功, false=失败
     */
    virtual bool init() = 0;
    
    /**
     * @brief 获取定位数据（带超时）
     * @param data 输出的 GPS 数据
     * @param timeoutMs 超时时间（毫秒）
     * @return true=获取成功, false=超时或失败
     */
    virtual bool getLocation(GpsData& data, unsigned long timeoutMs = 30000) = 0;
    
    /**
     * @brief 进入低功耗模式
     */
    virtual void sleep() = 0;
    
    /**
     * @brief 获取模块名称
     */
    virtual const char* getName() = 0;
};
