#pragma once

/**
 * @file IAudio.h
 * @brief 音频传感器接口定义
 * 
 * 功能要求 (5): 环境音监测功能，当声音大于设定的阈值时，
 *              发出警报信息，并拍照上传到物联网系统
 */

#include "../../include/AppConfig.h"
#include "../../include/Settings.h"

/**
 * @brief 音频传感器接口
 */
class IAudio {
public:
    virtual ~IAudio() = default;
    
    /**
     * @brief 初始化音频传感器
     * @return true=成功, false=失败
     */
    virtual bool init() = 0;
    
    /**
     * @brief 读取当前声音峰峰值
     * @return 峰峰值 (0-4095)
     */
    virtual uint16_t readPeakToPeak() = 0;
    
    /**
     * @brief 检测是否超过噪音阈值
     * @return true=超过阈值, false=正常
     */
    virtual bool isNoiseDetected() = 0;
    
    /**
     * @brief 获取声音强度百分比
     * @return 0-100
     */
    virtual uint8_t getSoundPercent() = 0;
    
    /**
     * @brief 进入低功耗模式
     */
    virtual void sleep() = 0;
};
