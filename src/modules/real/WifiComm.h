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
    WiFi.mode(WIFI_STA);
    esp_wifi_set_ps(WIFI_PS_NONE);
    return true;
  }

  bool connectNetwork() override {
    if (WiFi.status() == WL_CONNECTED) {
      connected = true;
      return true;
    }

    DEBUG_PRINTF("[通信] WiFi 连接中: %s\n", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    int retry = 0;
    while (WiFi.status() != WL_CONNECTED && retry < 20) {
      delay(500);
      retry++;
    }

    if (WiFi.status() == WL_CONNECTED) {
      DEBUG_PRINTF("[通信] ✓ WiFi 已连接: %s\n", WiFi.localIP().toString().c_str());
      connected = true;
      return true;
    } else {
      DEBUG_PRINTLN("[通信] ❌ WiFi 连接失败");
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
    http.begin(BEMFA_API_IMG);
    http.addHeader("Authorization", BEMFA_USER_KEY);
    http.addHeader("Authtopic", BEMFA_TOPIC_IMG);
    http.addHeader("Content-Type", "image/jpeg");

    int httpCode = http.POST((uint8_t *)imageData, imageSize);

    if (httpCode > 0) {
      String response = http.getString();
      http.end();
      return (httpCode == 200);
    } else {
      DEBUG_PRINTF("[通信] ❌ 图片上传失败: %s\n", http.errorToString(httpCode).c_str());
      http.end();
      return false;
    }
  }

  void sleep() override {
#if !WIFI_KEEP_ALIVE
    if (connected) {
      WiFi.disconnect(true);
      WiFi.mode(WIFI_OFF);
      connected = false;
    }
#endif
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
    String encodedMsg = urlEncode(String(message));
    String url = String(apiUrl) + "?uid=" + BEMFA_USER_KEY +
                 "&topic=" + BEMFA_TOPIC_MSG + "&type=1&msg=" + encodedMsg;

    http.begin(url);
    int httpCode = http.GET();

    bool success = false;
    if (httpCode > 0) {
      String response = http.getString();
      if (outResponse != nullptr && maxResponseLen > 0) {
        strncpy(outResponse, response.c_str(), maxResponseLen - 1);
        outResponse[maxResponseLen - 1] = '\0';
      }
      success = (httpCode == 200);
    } else {
      DEBUG_PRINTF("[通信] ❌ 请求失败: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();
    return success;
  }
};
