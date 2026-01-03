#pragma once

/**
 * @file ISensor.h
 * @brief 传感器接口定义 - 契约层
 */

class ISensor {
public:
    virtual ~ISensor() {}

    /**
     * @brief 初始化传感器
     * @return true=成功, false=失败
     */
    virtual bool init() = 0;

    /**
     * @brief 读取传感器数据
     * @return 角度值（度）或其他测量值
     */
    virtual float readData() = 0;

    /**
     * @brief 让传感器进入低功耗模式
     */
    virtual void sleep() = 0;

    /**
     * @brief 获取传感器名称
     * @return 名称字符串
     */
    virtual const char* getName() = 0;
};
