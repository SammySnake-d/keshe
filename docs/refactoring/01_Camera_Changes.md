# 摄像头模块重构文档

## 改动概述

为了与 `project-name` 保持一致并简化内存管理，对 `OV2640_Camera` 模块进行了重构。

---

## 详细改动

### 1. 内存管理简化

**修改文件**: [OV2640_Camera.h](file:///Users/snakesammy/Desktop/iot-keshe/keshe/src/modules/real/OV2640_Camera.h)

**修改前**:
使用了手动分配的 PSRAM 缓冲区 (`buffer`, `bufferSize`) 和复杂的内存拷贝逻辑。

**修改后**:
直接使用 `esp_camera` 驱动提供的帧缓冲区管理机制：
- `capturePhoto`: 调用 `esp_camera_fb_get()` 获取帧缓冲指针。
- `releasePhoto`: 调用 `esp_camera_fb_return()` 释放帧缓冲。
- **优势**: 减少代码量，避免双重内存分配，提高效率。

### 2. 引脚配置同步

**修改文件**: [PinMap.h](file:///Users/snakesammy/Desktop/iot-keshe/keshe/include/PinMap.h)

**修改内容**:
- `PIN_CAM_XCLK` 更新为 -1 (假设使用外部晶振或不需要引脚控制，参考 `project-name` 配置)。
- 确认了 `PIN_CAM_PWDN` 与 GPS 电源引脚 (IO46) 的共用关系，在逻辑层需要注意互斥。

### 3. 配置参数调整

**修改文件**: [Settings.h](file:///Users/snakesammy/Desktop/iot-keshe/keshe/include/Settings.h)

**修改内容**:
- 设置分辨率为 `FRAMESIZE_QVGA` (320x240)。
- 设置 JPEG 质量为 `12`。
- 仅使用 1 个帧缓冲区 (`CAM_FB_COUNT 1`) 以节省内存。

---

## 验证结果

- 编译器检查通过。
- 逻辑上符合 ESP-IDF 官方驱动用法。
