/**
 * @file test_lsm6ds3.cpp
 * @brief LSM6DS3 倾斜传感器模块测试
 *
 * 测试目标：
 *   1. I2C 通信正常
 *   2. 加速度计数据读取
 *   3. 角度计算准确
 *   4. 零点校准功能
 *   5. 数据就绪中断（可选）
 *
 * 硬件连接：
 *   SDA → GPIO 17
 *   SCL → GPIO 18
 *   INT1 → GPIO 10 (可选)
 *   VCC → 3.3V
 *   GND → GND
 *
 * I2C 地址：0x6A (SDO=0) 或 0x6B (SDO=1)
 */

#include "PinMap.h"
#include <Arduino.h>
#include <SparkFunLSM6DS3.h>
#include <Wire.h>
#include <unity.h>

// 引入 Mock 和 Real 测试函数
#include "lsm6ds3_mock.h"
#include "lsm6ds3_real.h"

// 测试对象
LSM6DS3 imu;

// ==================== 测试主程序 ====================

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("\n");
  Serial.println("╔═══════════════════════════════════════╗");
  Serial.println("║   LSM6DS3 倾斜传感器 - 单元测试      ║");
  Serial.println("╚═══════════════════════════════════════╝");

  UNITY_BEGIN();

#if USE_MOCK_HARDWARE == 1
  // Mock 模式测试
  Serial.println("\n========== Mock 测试模式 ==========");
  RUN_TEST(test_mock_angle_calculation);
  RUN_TEST(test_mock_calibration);
  RUN_TEST(test_mock_threshold_detection);
#else
  // Real 硬件测试
  Serial.println("\n========== Real 硬件测试模式 ==========");
  Serial.println("提示: 如果 Real 测试失败，请检查：");
  Serial.println("  1. I2C 连线: SDA→GPIO17, SCL→GPIO18");
  Serial.println("  2. 电源: VCC→3.3V, GND→GND");
  Serial.println("  3. I2C 地址: 0x6A (SDO接地)\n");

  RUN_TEST(test_real_i2c_communication);
  RUN_TEST(test_real_sensor_initialization);
  RUN_TEST(test_real_accel_reading);
  RUN_TEST(test_real_angle_calculation);
  RUN_TEST(test_real_sampling_stability);

#endif

  UNITY_END();

  Serial.println("\n========== 测试完成 ==========");
}

void loop() {
  delay(2000);

  // 持续监测倾斜角度
  float ax = imu.readFloatAccelX();
  float ay = imu.readFloatAccelY();
  float az = imu.readFloatAccelZ();

  float pitch = calculatePitch(ax, ay, az);
  float roll = calculateRoll(ay, ax, az);

  Serial.printf(
      "\n[MONITOR] Pitch=%.2f°, Roll=%.2f° | Accel: %.2f, %.2f, %.2f\n", pitch,
      roll, ax, ay, az);
}
