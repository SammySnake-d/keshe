#pragma once

/**
 * @file EC800K_Driver.h
 * @brief EC800K 4G模块驱动 - 真实硬件实现
 * @note 使用 AT 指令 + HTTP 协议（适合低功耗和图片传输）
 */

#include "../../interfaces/IComm.h"
#include "../../../include/AppConfig.h"
#include "../../../include/Settings.h"
#include <HardwareSerial.h>

class EC800K_Driver : public IComm {
private:
    HardwareSerial modemSerial;
    bool isNetworkReady = false;
    int pdpContextId = 1;
    
public:
    EC800K_Driver() : modemSerial(1) {}
    
    bool init() override {
        DEBUG_PRINTLN("[EC800K] 初始化中...");
        pinMode(PIN_EC800_DTR, OUTPUT);
        digitalWrite(PIN_EC800_DTR, LOW);
        modemSerial.begin(EC800K_BAUD_RATE, SERIAL_8N1, PIN_EC800_RX, PIN_EC800_TX);
        delay(EC800K_INIT_DELAY_MS);
        if (!sendATCommand("AT", EC800K_AT_TIMEOUT_MS)) {
            DEBUG_PRINTLN("[EC800K] ❌ 初始化失败！");
            return false;
        }
        DEBUG_PRINTLN("[EC800K] ✓ 初始化成功");
        return true;
    }
    
    bool connectNetwork() override {
        DEBUG_PRINTLN("[EC800K] 连接移动网络...");
        if (!sendATCommand("AT+CPIN?", 2000, "+CPIN: READY")) {
            DEBUG_PRINTLN("[EC800K] ❌ SIM 卡未就绪");
            return false;
        }
        DEBUG_PRINTLN("[EC800K] 等待网络注册...");
        for (int i = 0; i < EC800K_NETWORK_RETRY_COUNT; i++) {
            if (sendATCommand("AT+CREG?", EC800K_AT_TIMEOUT_MS, "+CREG: 0,1") ||
                sendATCommand("AT+CREG?", EC800K_AT_TIMEOUT_MS, "+CREG: 0,5")) {
                DEBUG_PRINTLN("[EC800K] ✓ 网络已注册");
                break;
            }
            delay(EC800K_NETWORK_RETRY_DELAY);
        }
        sendATCommand("AT+QICSGP=1,1,\"CMNET\",\"\",\"\",1", 2000);
        DEBUG_PRINTLN("[EC800K] 激活 PDP 上下文...");
        if (!sendATCommand("AT+QIACT=1", 5000)) {
            DEBUG_PRINTLN("[EC800K] ❌ PDP 激活失败");
            return false;
        }
        configureHTTP();
        isNetworkReady = true;
        DEBUG_PRINTLN("[EC800K] ✓ HTTP 网络就绪");
        return true;
    }
    
    bool sendAlarm(const char* payload, char* outResponse = nullptr, size_t maxResponseLen = 0) override {
        if (!isNetworkReady) return false;
        DEBUG_PRINTLN("[EC800K] 发送报警数据...");
        String url = String("http://") + HTTP_SERVER_HOST + HTTP_API_ALARM;
        return httpPost(url.c_str(), payload, strlen(payload), "application/json", outResponse, maxResponseLen);
    }
    
    bool sendStatus(const char* payload, char* outResponse = nullptr, size_t maxResponseLen = 0) override {
        if (!isNetworkReady) return false;
        DEBUG_PRINTLN("[EC800K] 发送状态心跳...");
        String url = String("http://") + HTTP_SERVER_HOST + HTTP_API_STATUS;
        return httpPost(url.c_str(), payload, strlen(payload), "application/json", outResponse, maxResponseLen);
    }
    
    bool uploadImage(const uint8_t* imageData, size_t imageSize, const char* metadata = nullptr) override {
        if (!isNetworkReady) return false;
        DEBUG_PRINTF("[EC800K] 上传图片 (%d bytes)...\n", imageSize);
        String url = String("http://") + HTTP_SERVER_HOST + HTTP_API_IMAGE;
        if (metadata) url += "?meta=" + String(metadata);
        return httpPost(url.c_str(), (const char*)imageData, imageSize, "image/jpeg", nullptr, 0);
    }
    
    void sleep() override {
        DEBUG_PRINTLN("[EC800K] 进入 DTR 休眠模式");
        sendATCommand("AT+QIDEACT=1", 2000);
        modemSerial.println("AT+QSCLK=1");
        delay(100);
        digitalWrite(PIN_EC800_DTR, HIGH);
        isNetworkReady = false;
    }
    
    const char* getName() override {
        return "EC800K_HTTP";
    }
    
private:
    void configureHTTP() {
        DEBUG_PRINTLN("[EC800K] 配置 HTTP 上下文...");
        sendATCommand("AT+QHTTPCFG=\"contextid\",1", 2000);
        sendATCommand("AT+QHTTPCFG=\"responseheader\",1", 2000);
    }
    
    bool httpPost(const char* url, const char* data, size_t dataLen, 
                  const char* contentType, char* outResponse, size_t maxResponseLen) {
        DEBUG_PRINTF("[EC800K] POST %s (%d bytes, %s)\n", url, dataLen, contentType);
        
        // 1. 设置 URL
        String urlCmd = String("AT+QHTTPURL=") + String(strlen(url)) + ",80";
        modemSerial.println(urlCmd);
        if (!waitForResponse("CONNECT", 2000)) {
            DEBUG_PRINTLN("[EC800K] ❌ URL 设置失败");
            return false;
        }
        modemSerial.print(url);
        if (!waitForResponse("OK", 2000)) {
            DEBUG_PRINTLN("[EC800K] ❌ URL 确认失败");
            return false;
        }
        
        // 2. 设置自定义请求头 (Content-Type)
        // 计算头部字符串长度: "Content-Type: image/jpeg" 或 "Content-Type: application/json"
        String headerStr = String("Content-Type: ") + contentType;
        int headerLen = headerStr.length();
        
        String headerCmd = String("AT+QHTTPHEADER=") + String(headerLen);
        modemSerial.println(headerCmd);
        if (!waitForResponse("CONNECT", 2000)) {
            DEBUG_PRINTLN("[EC800K] ❌ Header 设置失败");
            return false;
        }
        modemSerial.print(headerStr);  // 发送 "Content-Type: image/jpeg"
        if (!waitForResponse("OK", 2000)) {
            DEBUG_PRINTLN("[EC800K] ❌ Header 确认失败");
            return false;
        }
        
        // 3. 发送 POST 请求 (纯二进制数据)
        // AT+QHTTPPOST=<data_len>,<input_time>,<output_time>
        String postCmd = String("AT+QHTTPPOST=") + String(dataLen) + ",60,80";
        modemSerial.println(postCmd);
        
        if (!waitForResponse("CONNECT", 2000)) {
            DEBUG_PRINTLN("[EC800K] ❌ POST 准备失败");
            return false;
        }
        
        // 4. 直接发送原始二进制数据 (Raw Binary)
        modemSerial.write((const uint8_t*)data, dataLen);
        
        if (!waitForResponse("OK", 2000)) {
            DEBUG_PRINTLN("[EC800K] ❌ 数据发送失败");
            return false;
        }
        
        // 5. 等待 HTTP 响应 (+QHTTPPOST: <err>,<http_code>,<content_length>)
        String response = "";
        unsigned long start = millis();
        int httpStatus = 0;
        int responseLen = 0;
        
        while (millis() - start < 60000) {
            if (modemSerial.available()) {
                char c = modemSerial.read();
                response += c;
                
                // 解析: +QHTTPPOST: 0,200,123
                if (response.indexOf("+QHTTPPOST:") != -1) {
                    int c1 = response.indexOf(',');
                    int c2 = response.indexOf(',', c1 + 1);
                    if (c1 > 0 && c2 > 0) {
                        int errCode = response.substring(response.indexOf(':') + 1, c1).toInt();
                        httpStatus = response.substring(c1 + 1, c2).toInt();
                        responseLen = response.substring(c2 + 1).toInt();
                        
                        DEBUG_PRINTF("[EC800K] +QHTTPPOST: err=%d, http=%d, len=%d\n", 
                                     errCode, httpStatus, responseLen);
                        break;
                    }
                }
            }
        }
        
        if (httpStatus == 0) {
            DEBUG_PRINTLN("[EC800K] ❌ HTTP 响应超时");
            return false;
        }
        
        // 6. 读取服务器响应内容 (用于"捎带指令")
        if (outResponse && maxResponseLen > 0 && httpStatus == 200 && responseLen > 0) {
            readHTTPResponse(outResponse, maxResponseLen);
        }
        
        return (httpStatus >= 200 && httpStatus < 300);
    }
    
    void readHTTPResponse(char* buffer, size_t maxLen) {
        // 读取 HTTP 响应体 (用于接收服务器下发的指令)
        // AT+QHTTPREAD=<timeout>
        modemSerial.println("AT+QHTTPREAD=80");
        
        if (!waitForResponse("CONNECT", 2000)) {
            DEBUG_PRINTLN("[EC800K] ⚠️ 无响应内容可读");
            buffer[0] = '\0';
            return;
        }
        
        size_t idx = 0;
        unsigned long start = millis();
        bool inData = false;
        
        while (idx < maxLen - 1 && millis() - start < 5000) {
            if (modemSerial.available()) {
                char c = modemSerial.read();
                
                // 跳过 CONNECT 后的换行符
                if (!inData && (c == '\r' || c == '\n')) continue;
                inData = true;
                
                // 遇到 OK 说明读取完成
                if (c == 'O' && modemSerial.peek() == 'K') {
                    break;
                }
                
                buffer[idx++] = c;
            }
        }
        buffer[idx] = '\0';
        waitForResponse("OK", 1000);
        
        DEBUG_PRINTF("[EC800K] 响应内容: %s\n", buffer);
    }
    
    bool sendATCommand(const char* cmd, uint32_t timeout, const char* expected = "OK") {
        modemSerial.println(cmd);
        return waitForResponse(expected, timeout);
    }
    
    bool waitForResponse(const char* expected, uint32_t timeout) {
        unsigned long start = millis();
        String response = "";
        while (millis() - start < timeout) {
            if (modemSerial.available()) {
                char c = modemSerial.read();
                response += c;
                if (response.indexOf(expected) != -1) return true;
            }
        }
        return false;
    }
};
