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
    DEBUG_PRINTLN("[ATGM336H] 初始化中...");

    // 1. 配置电源控制引脚
    pinMode(PIN_GPS_PWR, OUTPUT);
    DEBUG_PRINTF("[ATGM336H] 电源引脚: GPIO%d\n", PIN_GPS_PWR);

    // 2. 上电（P-MOS：拉低导通）
    digitalWrite(PIN_GPS_PWR, LOW);
    isPowered = true;
    DEBUG_PRINTLN("[ATGM336H] 电源已开启 (P-MOS LOW)");
    delay(500);  // 增加等待时间

    // 3. 初始化 UART1 (9600bps, 8N1)
    // Arduino HardwareSerial: begin(baud, config, RX_PIN, TX_PIN)
    DEBUG_PRINTF("[ATGM336H] 串口配置: RX=GPIO%d, TX=GPIO%d\n", PIN_GPS_RX, PIN_GPS_TX);
    gpsSerial.begin(9600, SERIAL_8N1, PIN_GPS_RX, PIN_GPS_TX);

    // 4. 等待模块启动稳定
    delay(2000);  // 增加到 2 秒，与测试代码一致

    DEBUG_PRINTLN("[ATGM336H] ✓ 初始化成功，波特率 9600");
    DEBUG_PRINTLN("[ATGM336H] ⚠️  请确保天线放置在室外空旷处");

    return true;
  }

  bool getLocation(GpsData &data, unsigned long timeoutMs = 30000) override {
    if (!isPowered) {
      DEBUG_PRINTLN("[ATGM336H] ❌ 模块未上电");
      return false;
    }

    DEBUG_PRINTF("[ATGM336H] 开始定位（超时 %lu 秒）...\n", timeoutMs / 1000);
    DEBUG_PRINTF("[ATGM336H] 串口配置: RX=GPIO%d, TX=GPIO%d, PWR=GPIO%d\n", 
                 PIN_GPS_RX, PIN_GPS_TX, PIN_GPS_PWR);

    unsigned long startTime = millis();
    bool receivedData = false;
    uint32_t lastCharCount = 0;
    uint32_t totalChars = 0;

    // 清空缓冲区
    while (gpsSerial.available()) {
      gpsSerial.read();
    }

    // 首次等待数据的调试
    DEBUG_PRINTLN("[ATGM336H] 等待串口数据...");

    while (millis() - startTime < timeoutMs) {
      // 读取并解析串口数据
      while (gpsSerial.available() > 0) {
        char c = gpsSerial.read();
        totalChars++;

        // 首次收到数据时打印
        if (!receivedData) {
          DEBUG_PRINTLN("[ATGM336H] ✓ 开始接收数据");
          receivedData = true;
        }

// 调试：打印原始数据（可选）
#ifdef GPS_DEBUG_RAW
        DEBUG_PRINT(c);
#endif

        gpsParser.encode(c);
      }

      // 定期报告进度
      uint32_t elapsed = millis() - startTime;
      if (elapsed > 0 && elapsed % 5000 < 20) {
        if (gpsParser.charsProcessed() != lastCharCount || totalChars > 0) {
          lastCharCount = gpsParser.charsProcessed();
          DEBUG_PRINTF("[ATGM336H] 已处理 %u 字符，卫星数 %u，总接收 %u\n", 
                       lastCharCount, gpsParser.satellites.value(), totalChars);
        }
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

          DEBUG_PRINTLN("\n[ATGM336H] ✓ 定位成功！");
          DEBUG_PRINTF("  纬度: %.6f°\n", data.latitude);
          DEBUG_PRINTF("  经度: %.6f°\n", data.longitude);
          DEBUG_PRINTF("  海拔: %.1fm\n", data.altitude);
          DEBUG_PRINTF("  卫星数: %u\n", data.satellites);
          DEBUG_PRINTF("  HDOP: %.2f\n", data.hdop);

          return true;
        }
      }

      delay(10);
    }

    // 超时
    if (!receivedData) {
      DEBUG_PRINTLN("[ATGM336H] ❌ 未收到任何数据，请检查：");
      DEBUG_PRINTLN("  1. 天线是否连接");
      DEBUG_PRINTLN("  2. 串口接线是否正确（TX<->RX 交叉）");
      DEBUG_PRINTLN("  3. 电源是否正常");
    } else {
      DEBUG_PRINTF(
          "[ATGM336H] ❌ 定位超时（卫星数 %u，可能室内或天线位置不佳）\n",
          gpsParser.satellites.value());
    }

    data.isValid = false;
    return false;
  }

  void sleep() override {
    DEBUG_PRINTLN("[ATGM336H] 进入休眠模式");

    // 关闭电源（P-MOS：拉高截止）
    digitalWrite(PIN_GPS_PWR, HIGH);
    isPowered = false;

    // 注意：ATGM336H 无软件休眠指令，直接断电最省电
  }

  const char *getName() override { return "ATGM336H-5N"; }
};
