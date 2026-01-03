#pragma once
#include <Arduino.h>

/**
 * @file IComm.h
 * @brief 通信模块接口定义 - 契约层
 */

class IComm {
public:
    virtual ~IComm() {}

    /**
     * @brief 初始化通信模块
     * @return true=成功, false=失败
     */
    virtual bool init() = 0;

    /**
     * @brief 连接网络（移动网络/MQTT）
     * @return true=连接成功, false=连接失败
     */
    virtual bool connectNetwork() = 0;

    /**
     * @brief 发送报警数据（JSON 格式）
     * @param payload JSON 字符串
     * @return true=发送成功, false=发送失败
     */
    virtual bool sendAlarm(const char* payload) = 0;

    /**
     * @brief 发送状态心跳（JSON 格式）
     * @param payload JSON 字符串
     * @return true=发送成功, false=发送失败
     */
    virtual bool sendStatus(const char* payload) = 0;
    
    /**
     * @brief 订阅下行指令主题
     * @param topic 订阅的主题名称
     * @return true=订阅成功, false=订阅失败
     */
    virtual bool subscribeCommand(const char* topic) = 0;
    
    /**
     * @brief 检查并接收下行指令
     * @param outCommand 输出指令内容
     * @param maxLen 最大长度
     * @return true=收到指令, false=无指令
     */
    virtual bool receiveCommand(char* outCommand, size_t maxLen) = 0;

    /**
     * @brief 让通信模块进入低功耗模式
     */
    virtual void sleep() = 0;

    /**
     * @brief 获取模块名称
     * @return 名称字符串
     */
    virtual const char* getName() = 0;
};
