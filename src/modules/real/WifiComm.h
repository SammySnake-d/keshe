#pragma once

#include "../../../include/AppConfig.h"
#include "../../interfaces/IComm.h"
#include "esp_wifi.h"
#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFi.h>

/**
 * @file WifiComm.h
 * @brief WiFi 通信模块实现 (参考 project-name/main/wifi_manager.c 和
 * bemfa_client.c)
 * @note 使用 Arduino 框架的 WiFi 和 HTTPClient 库，但实现逻辑与 reference 一致
 */

class WifiComm : public IComm {
private:
  bool connected = false;

public:
  const char *getName() override { return "WiFi_Bemfa"; }

  bool init() override {
    DEBUG_PRINTLN("[WiFi] 初始化...");
    WiFi.mode(WIFI_STA);
    // 关闭 WiFi 节能模式以提高稳定性
    esp_wifi_set_ps(WIFI_PS_NONE);
    return true;
  }

  bool connectNetwork() override {
    if (WiFi.status() == WL_CONNECTED) {
      connected = true;
      return true;
    }

    DEBUG_PRINTF("[WiFi] 连接中 SSID: %s ...\n", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    // 等待连接 (最长 10秒)
    int retry = 0;
    while (WiFi.status() != WL_CONNECTED && retry < 20) {
      delay(500);
      DEBUG_PRINT(".");
      retry++;
    }
    DEBUG_PRINTLN("");

    if (WiFi.status() == WL_CONNECTED) {
      DEBUG_PRINTF("[WiFi] ✓ 连接成功, IP: %s\n",
                   WiFi.localIP().toString().c_str());
      connected = true;
      return true;
    } else {
      DEBUG_PRINTLN("[WiFi] ❌ 连接失败");
      connected = false;
      return false;
    }
  }

  // 参考 project-name/main/bemfa_client.c:72 bemfa_send_message (HTTP GET)
  // 同时也适配了 IComm 接口的 sendAlarm 语义
  bool sendAlarm(const char *payload, char *outResponse = nullptr,
                 size_t maxResponseLen = 0) override {
    return sendRequest(BEMFA_API_MSG, payload, outResponse, maxResponseLen);
  }

  // 状态心跳使用相同的通道
  bool sendStatus(const char *payload, char *outResponse = nullptr,
                  size_t maxResponseLen = 0) override {
    return sendRequest(BEMFA_API_MSG, payload, outResponse, maxResponseLen);
  }

  // 参考 project-name/main/bemfa_client.c:35 bemfa_upload_photo (HTTP POST)
  bool uploadImage(const uint8_t *imageData, size_t imageSize,
                   const char *metadata = nullptr) override {
    if (WiFi.status() != WL_CONNECTED)
      return false;

    HTTPClient http;
    DEBUG_PRINTLN("[Bemfa] 上传图片...");

    http.begin(BEMFA_API_IMG);

    // 设置请求头 (参考 bemfa_client.c)
    http.addHeader("Authorization", BEMFA_USER_KEY);
    http.addHeader("Authtopic", BEMFA_TOPIC_IMG);
    http.addHeader("Content-Type", "image/jpeg");
    // http.addHeader("Expect", ""); // Arduino HTTPClient 默认可能处理此项

    // 如果有元数据，可以考虑通过 URL 参数或额外的 Header 传递，但 Bemfa
    // 图片接口主要看 Authtopic 这里暂时忽略 metadata 参数，除非 Bemfa
    // 支持自定义字段

    int httpCode = http.POST((uint8_t *)imageData, imageSize);

    if (httpCode > 0) {
      DEBUG_PRINTF("[Bemfa] 上传响应: %d\n", httpCode);
      String response = http.getString();
      DEBUG_PRINTLN(response);
      http.end();
      return (httpCode == 200);
    } else {
      DEBUG_PRINTF("[Bemfa] ❌ 上传失败: %s\n",
                   http.errorToString(httpCode).c_str());
      http.end();
      return false;
    }
  }

  void sleep() override {
    if (connected) {
      DEBUG_PRINTLN("[WiFi] 关闭 WiFi");
      WiFi.disconnect(true);
      WiFi.mode(WIFI_OFF);
      connected = false;
    }
  }

private:
  // 简单的 URL 编码实现
  String urlEncode(String str) {
    String encodedString = "";
    char c;
    char code0;
    char code1;
    for (int i = 0; i < str.length(); i++) {
      c = str.charAt(i);
      if (isalnum(c)) {
        encodedString += c;
      } else {
        code1 = (c & 0xf) + '0';
        if ((c & 0xf) > 9) {
          code1 = (c & 0xf) - 10 + 'A';
        }
        c = (c >> 4) & 0xf;
        code0 = (c) + '0';
        if (c > 9) {
          code0 = c - 10 + 'A';
        }
        encodedString += '%';
        encodedString += code0;
        encodedString += code1;
      }
    }
    return encodedString;
  }

  // 通用的请求发送函数
  bool sendRequest(const char *apiUrl, const char *message, char *outResponse,
                   size_t maxResponseLen) {
    if (WiFi.status() != WL_CONNECTED)
      return false;

    HTTPClient http;

    // 对 message 进行 URL 编码，确保 JSON 等特殊字符正确传输
    String encodedMsg = urlEncode(String(message));

    // 构造 URL:
    // http://apis.bemfa.com/va/sendMessage?uid=...&topic=...&type=1&msg=...
    String url = String(apiUrl) + "?uid=" + BEMFA_USER_KEY +
                 "&topic=" + BEMFA_TOPIC_MSG + "&type=1&msg=" + encodedMsg;

    DEBUG_PRINTF("[Bemfa] 发送请求: %s\n", url.c_str());

    http.begin(url);
    int httpCode = http.GET(); // Bemfa 消息接口使用 GET

    bool success = false;
    if (httpCode > 0) {
      DEBUG_PRINTF("[Bemfa] 响应代码: %d\n", httpCode);
      String response = http.getString();
      DEBUG_PRINTLN(response);

      // 如果需要回传响应
      if (outResponse != nullptr && maxResponseLen > 0) {
        strncpy(outResponse, response.c_str(), maxResponseLen - 1);
        outResponse[maxResponseLen - 1] = '\0';
      }
      success = (httpCode == 200);
    } else {
      DEBUG_PRINTF("[Bemfa] ❌ 请求失败: %s\n",
                   http.errorToString(httpCode).c_str());
    }

    http.end();
    return success;
  }
};
