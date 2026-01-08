/**
 * @file lsm6ds3_real.h
 * @brief LSM6DS3 Real 硬件测试函数
 *
 * 功能：
 *   - I2C 通信测试
 *   - 传感器初始化
 *   - 实时数据读取
 *   - 采样稳定性验证
 */

#ifndef LSM6DS3_REAL_H
#define LSM6DS3_REAL_H

#include "PinMap.h"
#include "lsm6ds3_mock.h" // 使用 Mock 中的角度计算函数
#include <Arduino.h>
#include <SparkFunLSM6DS3.h>
#include <Wire.h>
#include <unity.h>

// 测试配置
#define LSM6DS3_I2C_ADDR 0x6A

// 全局传感器对象
extern LSM6DS3 imu;

// ==================== Real 测试用例 ====================

/**
 * @brief Real测试：I2C 通信
 */
void test_real_i2c_communication() {
  Serial.println("\n[TEST] Real: I2C 通信");

  Wire.begin(PIN_LSM_SDA, PIN_LSM_SCL);
  Wire.setClock(400000);

  // 扫描 I2C 设备
  Wire.beginTransmission(LSM6DS3_I2C_ADDR);
  uint8_t error = Wire.endTransmission();

  Serial.printf("  I2C 地址 0x%02X: ", LSM6DS3_I2C_ADDR);
  if (error == 0) {
    Serial.println("✓ 找到设备");
    TEST_ASSERT_EQUAL_MESSAGE(0, error, "LSM6DS3 响应正常");
  } else {
    Serial.printf("❌ 未找到 (错误码: %d)\n", error);
    TEST_FAIL_MESSAGE("LSM6DS3 未响应，请检查连线");
  }
}

/**
 * @brief Real测试：传感器初始化
 */
void test_real_sensor_initialization() {
  Serial.println("\n[TEST] Real: 传感器初始化");

  if (imu.begin() != 0) {
    Serial.println("  ❌ 初始化失败");
    TEST_FAIL_MESSAGE("LSM6DS3 初始化失败");
  }

  Serial.println("  ✓ LSM6DS3 初始化成功");
}

/**
 * @brief Real测试：加速度计读取
 */
void test_real_accel_reading() {
  Serial.println("\n[TEST] Real: 加速度计读取");

  float ax = imu.readFloatAccelX();
  float ay = imu.readFloatAccelY();
  float az = imu.readFloatAccelZ();

  Serial.printf("  加速度: X=%.2f, Y=%.2f, Z=%.2f (g)\n", ax, ay, az);

  // 合理性检查（静止时总加速度应接近 1g）
  float magnitude = sqrt(ax * ax + ay * ay + az * az);
  Serial.printf("  总加速度: %.2fg (期望: ~1.0g)\n", magnitude);

  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.3f, 1.0f, magnitude,
                                   "总加速度接近 1g（地球重力）");

  Serial.println("✓ 加速度计数据正常");
}

/**
 * @brief Real测试：角度计算
 */
void test_real_angle_calculation() {
  Serial.println("\n[TEST] Real: 角度计算");

  float ax = imu.readFloatAccelX();
  float ay = imu.readFloatAccelY();
  float az = imu.readFloatAccelZ();

  float pitch = calculatePitch(ax, ay, az);
  float roll = calculateRoll(ay, ax, az);

  Serial.printf("  当前角度: Pitch=%.2f°, Roll=%.2f°\n", pitch, roll);

  // 合理性检查（静止时不应该超过 ±45°）
  TEST_ASSERT_TRUE_MESSAGE(abs(pitch) < 45.0f, "Pitch 角度合理");
  TEST_ASSERT_TRUE_MESSAGE(abs(roll) < 45.0f, "Roll 角度合理");

  Serial.println("✓ 角度计算正常");
}

/**
 * @brief Real测试：采样稳定性
 */
void test_real_sampling_stability() {
  Serial.println("\n[TEST] Real: 采样稳定性");

  // 连续读取 10 次
  float pitches[10];
  for (int i = 0; i < 10; i++) {
    float ax = imu.readFloatAccelX();
    float ay = imu.readFloatAccelY();
    float az = imu.readFloatAccelZ();
    pitches[i] = calculatePitch(ax, ay, az);
    delay(50);
  }

  // 计算标准差
  float avg = 0;
  for (int i = 0; i < 10; i++)
    avg += pitches[i];
  avg /= 10;

  float variance = 0;
  for (int i = 0; i < 10; i++) {
    float diff = pitches[i] - avg;
    variance += diff * diff;
  }
  float stddev = sqrt(variance / 10);

  Serial.printf("  平均值: %.2f°, 标准差: %.2f°\n", avg, stddev);
  TEST_ASSERT_TRUE_MESSAGE(stddev < 2.0f, "采样标准差 < 2°（稳定）");

  Serial.println("✓ 采样稳定");
}

#endif // LSM6DS3_REAL_H
