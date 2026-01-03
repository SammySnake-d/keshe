#pragma once
#include <Arduino.h>

/**
 * @file IComm.h
 * @brief 通信模块接口定义 - 契约层
 * @note 使用 HTTP 协议，适合低功耗场景和大文件传输
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
     * @brief 连接移动网络并激活 PDP 上下文
     * @return true=连接成功, false=连接失败
     */
    virtual bool connectNetwork() = 0;

    /**
     * @brief 发送报警数据（HTTP POST JSON）
     * @param payload JSON 字符串
     * @param outResponse 服务器响应内容（可选，用于接收下行指令）
     * @param maxResponseLen 响应缓冲区最大长度
     * @return true=发送成功, false=发送失败
     */
    virtual bool sendAlarm(const char* payload, char* outResponse = nullptr, size_t maxResponseLen = 0) = 0;

    /**
     * @brief 发送状态心跳（HTTP POST JSON）
     * @param payload JSON 字符串
     * @param outResponse 服务器响应内容（可选，用于接收下行指令）
     * @param maxResponseLen 响应缓冲区最大长度
     * @return true=发送成功, false=发送失败
     */
    virtual bool sendStatus(const char* payload, char* outResponse = nullptr, size_t maxResponseLen = 0) = 0;
    
    /**
     * @brief 上传图片（HTTP POST 二进制）
     * @param imageData JPEG 图片数据指针
     * @param imageSize 图片大小（字节）
     * @param metadata 元数据 JSON（可选，如经纬度、时间戳等）
     * @return true=上传成功, false=上传失败
     */
    virtual bool uploadImage(const uint8_t* imageData, size_t imageSize, const char* metadata = nullptr) = 0;

    /**
     * @brief 让通信模块进入低功耗模式（断开网络，DTR 休眠）
     */
    virtual void sleep() = 0;

    /**
     * @brief 获取模块名称
     * @return 名称字符串
     */
    virtual const char* getName() = 0;
};
