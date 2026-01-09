#pragma once

/**
 * @file ATGM336H_Driver.h
 * @brief ATGM336H-5N GPS/北斗双模模块驱动
 * @note 基于 NMEA-0183 协议 + TinyGPS++ 库
 */

#include "../../../include/AppConfig.h"
#include "../../../include/PinMap.h"
#include "../../interfaces/IGPS.h"
#include <HardwareSerial.h>
#include <TinyGPS++.h>

class ATGM336H_Driver : public IGPS {
private:
  HardwareSerial gpsSerial;
  TinyGPSPlus gpsParser;
  bool isPowered;

  /**
   * @brief 转换 NMEA 格式坐标为十进制度数
   * @param nmeaCoord NMEA 格式坐标（DDMM.MMMM 或 DDDMM.MMMM）
   * @param isLatitude true=纬度, false=经度
   * @return 十进制度数
   */
  double convertNmeaToDecimal(double nmeaCoord, bool isLatitude) {
    int degrees = (int)(nmeaCoord / 100);
    double minutes = nmeaCoord - (degrees * 100);
    return degrees + (minutes / 60.0);
  }

public:
  ATGM336H_Driver() : gpsSerial(1), isPowered(false) {}  // 使用 UART1

  bool init() override {
    if (isPowered) return true;

    pinMode(PIN_GPS_PWR, OUTPUT);
    digitalWrite(PIN_GPS_PWR, LOW);
    isPowered = true;
    delay(500);

    gpsSerial.begin(9600, SERIAL_8N1, PIN_GPS_RX, PIN_GPS_TX);
    delay(2000);

    DEBUG_PRINTLN("[GPS] ✓ 模块就绪");
    return true;
  }

  bool getLocation(GpsData &data, unsigned long timeoutMs = 30000) override {
    if (!isPowered) {
      DEBUG_PRINTLN("[GPS] ❌ 未上电");
      return false;
    }

    DEBUG_PRINTF("[GPS] 搜星中（超时 %lus）...\n", timeoutMs / 1000);

    unsigned long startTime = millis();
    bool receivedData = false;
    uint32_t lastReportTime = 0;

    while (gpsSerial.available()) {
      gpsSerial.read();
    }

    while (millis() - startTime < timeoutMs) {
      while (gpsSerial.available() > 0) {
        char c = gpsSerial.read();
        receivedData = true;

#ifdef GPS_DEBUG_RAW
        DEBUG_PRINT(c);
#endif

        gpsParser.encode(c);
      }

      // 每 10 秒报告一次
      uint32_t now = millis();
      if (now - lastReportTime > 10000) {
        lastReportTime = now;
        DEBUG_PRINTF("[GPS] 卫星: %u\n", gpsParser.satellites.value());
      }

      // 检查是否定位成功
      if (gpsParser.location.isUpdated() && gpsParser.location.isValid()) {
        // 额外质量检查：至少 4 颗卫星
        if (gpsParser.satellites.value() >= 4) {
          // 填充数据
          data.latitude = gpsParser.location.lat();
          data.longitude = gpsParser.location.lng();
          data.altitude = gpsParser.altitude.meters();
          data.speed = gpsParser.speed.kmph();
          data.course = gpsParser.course.deg();
          data.satellites = gpsParser.satellites.value();
          data.hdop = gpsParser.hdop.hdop();
          data.isValid = true;
          data.timestamp = millis();

          DEBUG_PRINTF("[GPS] ✓ 定位成功: %.6f, %.6f (卫星: %u)\n", 
                       data.latitude, data.longitude, data.satellites);

          return true;
        }
      }

      delay(10);
    }

    if (!receivedData) {
      DEBUG_PRINTLN("[GPS] ❌ 无数据");
    } else {
      DEBUG_PRINTF("[GPS] ❌ 超时（卫星: %u）\n", gpsParser.satellites.value());
    }

    data.isValid = false;
    return false;
  }

  void sleep() override {
    // 关闭电源（P-MOS：拉高截止）
    digitalWrite(PIN_GPS_PWR, HIGH);
    isPowered = false;
  }

  const char *getName() override { return "ATGM336H-5N"; }
};
