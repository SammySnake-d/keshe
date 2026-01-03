#pragma once

/**
 * @file ICamera.h
 * @brief 摄像头接口定义
 * @note 定义拍照模块的统一接口
 */

#include <cstdint>
#include <cstddef>

/**
 * @brief 摄像头接口
 */
class ICamera {
public:
    virtual ~ICamera() = default;
    
    /**
     * @brief 初始化摄像头
     * @return true=成功, false=失败
     */
    virtual bool init() = 0;
    
    /**
     * @brief 拍摄照片
     * @param outBuffer 输出 JPEG 数据指针
     * @param outSize 输出图片大小
     * @return true=成功, false=失败
     */
    virtual bool capturePhoto(uint8_t** outBuffer, size_t* outSize) = 0;
    
    /**
     * @brief 释放照片缓冲区
     */
    virtual void releasePhoto() = 0;
    
    /**
     * @brief 关闭摄像头电源
     */
    virtual void powerOff() = 0;
    
    /**
     * @brief 检查是否已初始化
     */
    virtual bool isReady() const = 0;
};
