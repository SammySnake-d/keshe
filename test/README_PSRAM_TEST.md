# ESP32-S3 PSRAM 验证测试

## 测试目的

验证 ESP32-S3-WROOM-1-N8R2 的 2MB PSRAM 是否正确配置和工作。

## 测试文件

1. **test_psram_simple.cpp** - 快速验证版本（推荐先用这个）
2. **test_psram.cpp** - 完整测试版本（包含4个子测试）

## 如何运行测试

### 方法 1: 使用 test_psram_simple.cpp（推荐）

```bash
# 1. 备份当前 main.cpp
mv src/main.cpp src/main.cpp.bak

# 2. 复制测试文件为 main.cpp
cp test/test_psram_simple.cpp src/main.cpp

# 3. 编译上传
pio run --target upload

# 4. 打开串口监视器
pio device monitor
```

### 方法 2: 使用 test_psram.cpp（完整测试）

```bash
# 1. 备份当前 main.cpp
mv src/main.cpp src/main.cpp.bak

# 2. 复制测试文件为 main.cpp
cp test/test_psram.cpp src/main.cpp

# 3. 编译上传
pio run --target upload

# 4. 打开串口监视器
pio device monitor
```

### 方法 3: 修改 platformio.ini 切换测试

在 `platformio.ini` 中添加新的测试环境：

```ini
[env:test-psram]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
monitor_speed = 115200
board_build.arduino.memory_type = qio_qspi
build_flags = 
    -DBOARD_HAS_PSRAM
    -DCORE_DEBUG_LEVEL=3
build_src_filter = 
    +<../test/test_psram.cpp>
```

然后运行：
```bash
pio run --environment test-psram --target upload
pio device monitor --environment test-psram
```

## 预期结果

### 成功输出示例：

```
--- Testing PSRAM ---
Total PSRAM: 2097152 bytes (2.00 MB)
Free PSRAM: 2071552 bytes (1.98 MB)
SUCCESS: PSRAM is enabled and working!
Memory allocation test: OK
Writing test pattern...
Verifying data...
Data verification: PASSED ✅
Memory released

--- Test Complete ---
Free PSRAM: 2023 KB | Free Heap: 320 KB
```

### 失败情况排查：

#### 情况 1: Total PSRAM: 0 bytes

**原因：** PSRAM 未启用

**解决方法：**
1. 检查 `platformio.ini` 是否包含：
   ```ini
   board_build.arduino.memory_type = qio_qspi
   build_flags = -DBOARD_HAS_PSRAM
   ```

2. 清理重新编译：
   ```bash
   pio run --target clean
   pio run --target upload
   ```

#### 情况 2: Memory allocation test: FAILED

**原因：** PSRAM 启用但无法分配内存

**解决方法：**
1. 检查是否有其他代码占用了大量 PSRAM
2. 尝试分配更小的内存块测试
3. 检查编译参数是否包含 `-mfix-esp32-psram-cache-issue`

#### 情况 3: Data verification: FAILED

**原因：** PSRAM 硬件问题或缓存问题

**解决方法：**
1. 确保 `build_flags` 包含 `-mfix-esp32-psram-cache-issue`
2. 检查板子型号是否真的是 N8R2（带 2MB PSRAM）
3. 尝试降低 PSRAM 时钟频率

## 完整测试报告说明

如果运行 `test_psram.cpp`，会看到详细的测试报告：

```
╔══════════════════════════════════════╗
║   ESP32-S3 PSRAM Validation Test    ║
║   Target: ESP32-S3-WROOM-1-N8R2     ║
║   Expected PSRAM: 2MB (2097152 bytes)║
╚══════════════════════════════════════╝

========================================
TEST 1: PSRAM Detection
========================================
Total PSRAM: 2097152 bytes (2.00 MB)
Free PSRAM:  2071552 bytes (1.98 MB)
✅ psramFound() = true
SPIRAM (heap_caps): 2097152 bytes total, 2071552 bytes free
✅ RESULT: PSRAM Detected Successfully!

========================================
TEST 2: Basic ps_malloc() (100 KB)
========================================
Allocating 102400 bytes using ps_malloc()...
✅ Allocation SUCCESS
Writing test pattern...
Verifying data...
✅ Data verification PASSED
Memory released

========================================
TEST 3: Large heap_caps_malloc() (1 MB)
========================================
Allocating 1048576 bytes using heap_caps_malloc(MALLOC_CAP_SPIRAM)...
✅ Allocation SUCCESS
Writing pattern to first/last 1KB...
✅ Data verification PASSED
Memory released

========================================
TEST 4: Camera Buffer Simulation (150 KB)
========================================
Simulating camera JPEG buffer allocation (153600 bytes)...
✅ Camera buffer allocation SUCCESS
✅ JPEG marker validation PASSED
Buffer released

========================================
FINAL TEST REPORT
========================================
[✅] PSRAM Detection
[✅] Basic Allocation (ps_malloc)
[✅] Large Allocation (1MB)
[✅] Camera Buffer Simulation
========================================
Tests Passed: 4 / 4

🎉 ALL TESTS PASSED!
PSRAM is working correctly and ready for camera use.
```

## 恢复原始代码

测试完成后，恢复原始 main.cpp：

```bash
mv src/main.cpp.bak src/main.cpp
pio run --target upload
```

## PSRAM 使用示例

测试通过后，可以在实际代码中这样使用 PSRAM：

```cpp
// 方法 1: 使用 ps_malloc (推荐用于简单场景)
uint8_t* buffer = (uint8_t*)ps_malloc(100 * 1024);  // 100KB
if (buffer != nullptr) {
    // 使用缓冲区
    free(buffer);
}

// 方法 2: 使用 heap_caps_malloc (更灵活)
#include "esp_heap_caps.h"

uint8_t* buffer = (uint8_t*)heap_caps_malloc(100 * 1024, MALLOC_CAP_SPIRAM);
if (buffer != nullptr) {
    // 使用缓冲区
    heap_caps_free(buffer);
}

// 方法 3: 检查 PSRAM 可用性
if (psramFound()) {
    size_t free = ESP.getFreePsram();
    Serial.printf("Free PSRAM: %d bytes\n", free);
}
```

## 注意事项

1. **PSRAM 访问速度比 SRAM 慢** - 不适合高频实时操作
2. **相机缓冲区非常适合放在 PSRAM** - 因为是一次性读取
3. **记得释放内存** - 避免内存泄漏
4. **2MB PSRAM 可以存储约 10-20 张 UXGA JPEG 图片**

## 技术细节

### PSRAM 配置参数

| 参数 | 值 | 说明 |
|------|-----|------|
| 类型 | QSPI | 四线 SPI 接口 |
| 大小 | 2MB | ESP32-S3-WROOM-1-N8R2 |
| 电压 | 1.8V | 低功耗模式 |
| 访问速度 | ~40MHz | 比 SRAM 慢 |
| 用途 | 大数据缓冲 | 图片、音频等 |

### API 对比

| API | 分配位置 | 灵活性 | 推荐场景 |
|-----|---------|--------|----------|
| `malloc()` | 自动 | 低 | 小内存 |
| `ps_malloc()` | PSRAM | 中 | 大块内存 |
| `heap_caps_malloc()` | 指定 | 高 | 精确控制 |
