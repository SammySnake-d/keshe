# 功能缺口与TODO清单

生成时间：2026-01-03

## 范围

本清单基于以下范围自动梳理：

- `src/`
- `include/`
- `docs/`

并结合需求（1）-（7）逐条对照，输出“已实现/部分实现/未实现”的结论与证据位置。

## 总体结论（摘要）

- **核心监测链路（倾斜/声音→报警→拍照→4G上报）已具备雏形**：`WorkflowManager.h` 已实现倾斜阈值判断、声音阈值判断、拍照与 HTTP 上传、报警/心跳上报。
- **低功耗“可运行”框架存在但离“5天恶劣天气不断联”的工程化要求还有明显差距**：默认配置处于 Wokwi/Mock 模式（`USE_MOCK_HARDWARE=1`、`ENABLE_DEEP_SLEEP=0`），缺少太阳能充电管理/能耗预算/低电量策略闭环。
- **“物联网平台/服务器端”在仓库内不存在实现**：仅有协议说明文档，缺少服务端 API（/api/status、/api/alarm、/api/upload/image 等）的落地代码与部署方式。
- **远程控制接口仅完成一部分**：`reboot` 已实现；`set_interval` 与 `capture` 仍为 TODO；没有独立的 `test` 接口、查询电量等接口闭环。

## 需求实现矩阵（1）-（7）

### （1）锂电池+太阳能供电，低功耗，5天恶劣天气不中断通讯

- **状态**：部分实现（偏“框架/示例级”）
- **已具备**：
  - **电池电压/百分比读取**：`src/core/SystemManager.h`（`readBatteryVoltage()`、`getBatteryPercentage()`、`isBatteryHealthy()`）
  - **模块按需上电/断电思路**：
    - GPS 通过 `PIN_GPS_PWR` 断电：`src/modules/real/ATGM336H_Driver.h`（`init()` 上电、`sleep()` 断电）
    - 4G 模块通过 `DTR` 休眠 + `QIDEACT`：`src/modules/real/EC800K_Driver.h`（`sleep()`）
- **缺口/未实现**：
  - **默认未启用真实深度睡眠**：`include/AppConfig.h`：`ENABLE_DEEP_SLEEP=0`。
  - ~~**心跳间隔未按 `HEARTBEAT_INTERVAL_SEC` 执行**~~ ✅ **已完成**（2026-01-04）：`Settings.h` 使用条件编译统一为 `HEARTBEAT_INTERVAL_SEC`（Mock=5s / Real=3600s），`WorkflowManager.h` 所有 `deepSleep()` 调用已更新。
  - **低电量保护未形成"策略闭环"**：存在 `isBatteryHealthy()`，但业务流程中未见在关键路径（上报/拍照/定位前）统一决策"降级/跳过耗电操作/延长休眠"。
  - **太阳能充电/充电状态检测未实现**：代码中未见太阳能充电 IC 状态检测、充电电流/电压采样、充放电策略等。
  - **5天续航的量化与验证未实现**：缺少能耗模型（休眠电流、4G峰值、电池容量、阴雨发电量），也缺少"极端情况下只发最小心跳/不上图"的降级策略。

### （2）倾斜检测：倾斜 > 5° 告警

- **状态**：已实现（软件轮询判定），中断唤醒未实现
- **证据**：
  - 阈值：`include/Settings.h`：`TILT_THRESHOLD = 5.0f`
  - 轮询检测与报警：`src/core/WorkflowManager.h`：`handleTimerWakeup()` 中 `relativeAngle > TILT_THRESHOLD` → `sendTiltAlarmWithPhoto()`
  - 倾斜角计算：`src/modules/real/LSM6DS3_Sensor.h`：`readData()`（基于加速度 `atan2` 计算 Pitch/Roll，输出相对偏移）
- **缺口/未实现**：
  - **倾斜中断唤醒（EXT1）未实现**：`src/main.cpp`：`ESP_SLEEP_WAKEUP_EXT1` 分支打印"未实现"。
  - **INT1 数据就绪中断未被利用**：
    - LSM6DS3 已配置 INT1 引脚输出 26Hz 数据就绪信号到 GPIO 10
    - 主程序采用轮询方式 (`readData()`)，完全忽略 INT1 信号
    - 缺少 ESP32 中断配置 (`attachInterrupt`)、ISR、深度睡眠唤醒配置
    - **结果**：设计意图"节省功耗"未达成，INT1 中断配置是半成品
  - `TILT_DEBOUNCE_COUNT`/`TILT_SAMPLE_INTERVAL_MS` 目前未在业务流程中形成明显的防抖采样逻辑（需要确认是否要按需求实现）。

### （3）电量监测 + 摄像头模块，具有拍照功能

- **状态**：已实现（但依赖硬件飞线/配置）
- **证据**：
  - 电量监测：`src/core/SystemManager.h`
  - 摄像头驱动与拍照：`src/modules/real/OV2640_Camera.h`：`init()`、`capturePhoto()`
  - 报警流程内拍照并上传：`src/core/WorkflowManager.h`：`sendTiltAlarmWithPhoto()` / `sendNoiseAlarmWithPhoto()`
- **注意/工程条件**：
  - **XCLK 需飞线**：`docs/Hardware_Fix_XCLK.md` 明确指出 PCB 未连 XCLK，需 GPIO2 飞线到 OV2640 XCLK。

### （4）北斗定位，定位信息发到物联网平台

- **状态**：部分实现
- **已实现**：
  - 北斗/GPS 双模驱动：`src/modules/real/ATGM336H_Driver.h`（基于 TinyGPS++ 解析 NMEA）
  - 在倾斜/噪音报警与心跳中获取定位：`src/core/WorkflowManager.h`：`getGpsLocation()`、`sendTiltAlarmWithPhoto()`、`sendNoiseAlarmWithPhoto()`、`sendStatusHeartbeat()`
- **缺口/未实现**：
  - **上报 JSON 中缺少 `device_id` 字段**：`src/utils/DataPayload.h` 的 `StatusPayload/TiltAlarmPayload/NoiseAlarmPayload/FullAlarmPayload` 只包含 type/angle/voltage/location 等，不含设备标识；平台侧难以区分多设备。
  - 定位失败降级虽有（不带 GPS 也能上报），但缺少“定位耗时/耗电”的策略（例如低电量时跳过定位）。

### （5）环境音监测：超过阈值报警，并拍照上传

- **状态**：已实现（轮询检测）；中断唤醒为可选扩展
- **证据**：
  - 音频 ADC 采样与峰峰值：`src/modules/real/AudioSensor_ADC.h`：`readPeakToPeak()`、`isNoiseDetected()`
  - 阈值：`include/Settings.h`：`NOISE_THRESHOLD_HIGH`
  - 报警流程：`src/core/WorkflowManager.h`：`handleTimerWakeup()` 检测到噪音后调用 `sendNoiseAlarmWithPhoto()`（含拍照+上传+报警上报）
- **缺口/未实现**：
  - `handleAudioWakeup()` 注释说明：模拟信号无法直接触发中断，需外接比较器；当前主要依赖定时轮询。
  - 阈值/策略远程配置未实现。

### （6）无线通信：移远 EC800K；客户端接口：test、主动上报、设置上报间隔、重启、查询电量

- **状态**：部分实现
- **已实现**：
  - EC800K AT 指令 + HTTP POST：`src/modules/real/EC800K_Driver.h`
  - 主动上报：
    - 状态：`IComm::sendStatus()`（`/api/status`）
    - 报警：`IComm::sendAlarm()`（`/api/alarm`）
    - 图片：`IComm::uploadImage()`（`/api/upload/image`）
  - **重启接口**：服务端响应包含 `"command": "reboot"` 时执行 `ESP.restart()`：`src/core/WorkflowManager.h`
- **缺口/未实现**：
  - **test 接口缺失**：未发现 `/api/test` / `ping` / `health` 等实现或配置。
  - **设置上报间隔（set_interval）未实现**：`src/core/WorkflowManager.h` 存在 TODO（“解析 value 并修改定时器”），且目前睡眠时长仍是测试值。
  - **立即拍照（capture）未实现**：`src/core/WorkflowManager.h` 存在 TODO（“触发拍照流程”）。
  - **查询电量接口缺失**：虽有电压/百分比读取，但没有形成“平台查询→设备响应”的接口（HTTP 被动响应模型本身也需要设计：例如下行命令 `query_battery` 并在下一次心跳/报警中携带结果）。
  - **下行指令 JSON 字段不一致风险**：代码查找的是 `"command"`，而 `MockComm` 示例返回的是 `{"cmd":"set_interval"...}`（字段名不同），会导致 Mock 与 Real 行为不一致。

### （7）编写或利用现有物联网平台，完成软件功能

- **状态**：未实现（仓库内未包含平台/服务端代码）
- **证据**：
  - `docs/Protocol_Refactor_MQTT_to_HTTP.md` 明确写了“下一步：服务器端开发”，并在检查清单中勾选了“服务器端 API 开发”为未完成项。
- **缺口/未实现**：
  - 缺少服务端实现（接收 /api/status、/api/alarm、/api/upload/image；下发 set_interval/reboot/capture 等指令；存储图片与数据；设备管理与可视化）。

## 代码中的 TODO / 未实现（精确汇总）

- **`src/core/WorkflowManager.h`**
  - `set_interval`：`// TODO: 解析 value 并修改定时器`
  - `capture`：`// TODO: 触发拍照流程`
- **`src/main.cpp`**
  - `ESP_SLEEP_WAKEUP_EXT1` 分支：输出"倾斜中断唤醒（未实现）"
- **`src/modules/real/LSM6DS3_Sensor.h`**
  - **INT1 中断未被利用**：硬件已配置 INT1 数据就绪中断(26Hz)，但主程序采用轮询方式读取，未实现：
    - ESP32 GPIO 中断配置 (`attachInterrupt`)
    - 中断服务程序 (ISR)
    - ESP32 深度睡眠唤醒源配置 (`esp_sleep_enable_ext0_wakeup`)
  - 设计意图是"节省功耗"，实际未达成

## docs/ 文档中的未勾选清单项（- [ ]）

- **`docs/Protocol_Refactor_MQTT_to_HTTP.md`**
  - `- [ ] WorkflowManager.h 业务流程适配（下一步）`
  - `- [ ] 测试编译`
  - `- [ ] 服务器端 API 开发`
- **`docs/Hardware_Fix_XCLK.md`**
  - 摄像头初始化失败检查项：飞线/占用/I2C/供电
  - 完成检查项：初始化成功/拍照/JPEG有效/图片大小

## 建议下一步（按优先级）

1. **补齐“平台/服务端”最小可用版本（MVP）**
   - 提供 `/api/status`、`/api/alarm`、`/api/upload/image` 三个端点
   - 设备管理：至少按 `device_id` 区分数据
   - 图片存储：落盘或对象存储，返回可访问 URL

2. **统一并实现“下行指令协议”**
   - 字段统一使用 `command`（或 `cmd`，二选一），并在设备端解析实现：
     - `set_interval`（持久化到 NVS/RTC，并影响下一次休眠时长）
     - `capture`（立即拍照上传，并带上原因/时间戳）
     - `query_battery`（下一次状态上报包含电量详情）

3. **把"低功耗"从示例变为可验证方案**
   - 切换为真实硬件配置（`USE_MOCK_HARDWARE=0`、`ENABLE_DEEP_SLEEP=1`）
   - ~~用 `HEARTBEAT_INTERVAL_SEC` 驱动休眠周期~~ ✅ **已完成**（2026-01-04）
   - 增加电量策略：低电量时禁用拍照/定位/降低上报频率
   - 明确太阳能充电检测/充电状态输入（硬件引脚/ADC）并实现

4. **完善倾斜中断唤醒（可选）**
   - 若要满足“实时性/更低功耗”，实现 `EXT1` 倾斜唤醒链路，并与轮询逻辑协同。
