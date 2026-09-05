# FreeRDP OHOS 手写笔与多显示器设计

状态：`Implemented`

设计版本：`PEN-MON-D1`
适用范围：common HSP 客户端、Native XComponent、FreeRDP OHOS client port；两个 Entry 共享此实现。源码与构建已复核，外接屏和 Windows Ink 动作级验收仍未在本轮执行。

## 1. 目标与边界

本轮补齐两项通用能力：

1. 手写笔输入使用 HarmonyOS Native XComponent 原始笔事件和 FreeRDP RDPEI pen PDU，保留压感、倾角和橡皮语义。
2. FreeRDP OHOS client 接受一组本地显示器布局，在首次连接和 `disp` 动态更新时向远端发送标准 monitor layout。

不增加 ArkTS 手势分支、手写笔开关或产品私有协议。多显示器仍由一个 RDP 会话和一个组合桌面坐标系承载；本轮不创建“一块本地屏幕一个窗口/XComponent”的多窗口展示层。

## 2. 手写笔数据流

```text
Native XComponent TouchEvent
  -> toolType/sourceTool 分类
  -> App Native PenEvent（surface 坐标、deviceId、pressure、tilt、eraser）
  -> 复用 RemoteContentGeometry 映射到远端组合桌面坐标
  -> FreeRDP OHOS session pen ABI
  -> RDPEI PenBegin/PenUpdate/PenEnd/PenCancel
  -> Windows Ink/远端应用
```

规则：

- `PEN/PENCIL/BRUSH/AIRBRUSH` 走 pen；`RUBBER` 额外携带 eraser/inverted 语义；finger 继续走系统 Tap/Pan 手势。
- `DOWN/MOVE/UP/CANCEL` 分别映射 begin/update/end/cancel。仅在系统提供 hover 事件时发送 hover，本轮不伪造 hover。
- `force` 限制到 `[0, 1]` 后按 FreeRDP pen 注册的最大压力归一化；tilt 限制到协议范围 `[-90, 90]`。
- 坐标必须复用现有 contain viewport 和组合远端桌面尺寸；黑边 down 拒绝，已按下笔的 up/cancel 允许 clamp，防止远端残留按下状态。
- RDPEI 未协商时只回退为基础左键移动/按下/抬起，不伪造压感或倾角；诊断明确记录 fallback。
- XComponent blur、Surface destroyed、disconnect 和 `releaseAllInput` 必须 cancel 活跃 pen，并清理设备状态。

HarmonyOS API 依据为 API 22 SDK 的 `OH_NativeXComponent_GetTouchPointToolType`、`OH_NativeXComponent_GetTouchPointTiltX/Y` 以及 `OH_NativeXComponent_TouchPoint.force`。协议依据为 FreeRDP `freerdp/client.h` 的 pen event 语义和 `freerdp/client/rdpei.h`。

## 3. 多显示器数据流

```text
OH_NativeDisplayManager_CreateAllDisplays
  + GetDisplayPosition / CreatePrimaryDisplay
  -> App Native MonitorLayoutSnapshot
  -> FreeRDP OHOS versioned monitor-layout ABI
  -> initial rdpSettings MonitorDefArray / UseMultimon
  -> disp SendMonitorLayout（连接后 add/remove/change）
  -> 远端组合桌面
  -> 现有 XComponent contain viewport + 输入逆变换
```

布局规则：

- 只采纳 alive、宽高有效的显示器；上限遵循服务端 `DisplayControlCaps.maxNumMonitors` 和 FreeRDP/RDPEDISP 限制。
- 使用系统 primary display；将其左上角平移为 `(0,0)`，其余显示器保留相对位置。每个布局仅允许一个 primary。
- width/height、物理毫米、orientation、desktop/device scale 均逐显示器传递并严格校验；H.264 尺寸按现有 alignment 策略逐屏规范化。
- desktop scale 按本地 `densityDPI` 映射到 RDP 支持值，device scale 保持 100；物理毫米优先使用显示器 DPI 推导，并按 Surface 与显示器像素比例缩放。
- 首次连接前无论单屏或多屏都保存完整 desired monitor layout；多屏同时设置 `UseMultimon`、`SupportMonitorLayoutPdu` 和 `MonitorDefArray`，单屏使用同一份布局初始化 DesktopWidth/Height、方向、物理尺寸和 scale。
- 外接屏移除并回到单屏时，用当前 XComponent Surface 和主显示器属性替换 desired layout，并立即通过既有 Display Control 状态机应用；不发送零屏布局。
- 连接后监听 display add/remove/change。布局变化经 `disp` 发送；caps 未就绪时缓存最后一版，caps 到达后发送。
- 服务端拒绝、数量/面积超限或系统枚举失败时保留最近一次已确认布局；首次连接无有效多屏快照时回退单屏，不中断会话。
- 远端帧和输入继续使用显示器联合矩形形成的组合桌面尺寸，禁止为各 codec 或各输入方式复制坐标算法。

## 4. 所有权与公共 ABI

| 层 | 所有权 |
| --- | --- |
| App Native `input/` | 读取 XComponent pen 字段、生命周期清理、基础 pointer fallback |
| App Native `session/` | 枚举/监听本地显示器，构造无 UI 依赖的 monitor snapshot，调用动态加载的 OHOS session ABI |
| FreeRDP `client/OHOS` | pen 设备状态、RDPEI attach/send/cancel；monitor layout 校验、settings 应用、caps 缓存和发送 |
| N-API / ArkTS | 不新增接口；页面、布局和 XComponent 所有权不变 |

新增 ABI 必须使用带 `structSize/version` 的请求结构，并追加到 `FreerdpRuntimeApi` 可选 symbol 表。旧 runtime 缺少 symbol 时，笔回退基础 pointer、多屏回退单屏；N-API 不感知版本差异。

## 5. 文件级修改计划

- FreeRDP：`client/OHOS/ohos_pen.*`、`ohos_display.*`、`ohos_session*.{c,h}`、`CMakeLists.txt` 和对应 tests。
- App Native：`input/xcomponent_pen.*`、`input/xcomponent_touch_gesture.cpp`、`session/rdp_display_layout_monitor.*`、`session/rdp_session_channels.*`、`freerdp/freerdp_runtime.*`、CMake 和 Native tests。
- 文档：平板架构、Native 模块规范、FreeRDP feature matrix、验证基线和仓库变更台账。

## 6. 验收

| ID | 条件 |
| --- | --- |
| `AC-PEN-01` | pen/pencil/brush 事件不进入 finger Tap/Pan；down/move/up 顺序正确，压感与倾角边界正确 |
| `AC-PEN-02` | rubber 映射 eraser；cancel/blur/surface destroy/disconnect 后无活跃 pen |
| `AC-PEN-03` | RDPEI 可用时发送 pen PDU；不可用时基础点击/拖动可用且只回退一次，不双发 |
| `AC-MON-01` | 1/2/3 屏布局 primary、负坐标、旋转、物理尺寸和 scale 规范化测试通过 |
| `AC-MON-02` | 首次连接多屏 settings 正确；`disp` caps 延迟、add/remove/change 和 unchanged 去重正确 |
| `AC-MON-03` | 多屏联合桌面的画面 contain、黑边拒绝、各屏四角输入坐标一致 |
| `AC-REG-01` | Native tests、FreeRDP OHOS tests、ArkTS tests、Debug HAP 构建通过；单屏、鼠标、触控、IME、旋转无回归 |

真机 Verified 还要求：HarmonyOS 手写笔在 Windows Ink/画图中完成细线、重压、倾斜和橡皮测试；连接/断开外接屏后远端显示设置中的显示器数量与拓扑同步，两个显示器四角均可点击。

## 7. 实施台账

| Change ID | 状态 | 代码范围 | 验收 ID | 说明 |
| --- | --- | --- | --- | --- |
| `PEN-MON-D1` | `Implemented` | FreeRDP OHOS pen/multimon ABI；App Native XComponent pen 和 display layout monitor | `AC-PEN-01..03`、`AC-MON-01..03`、`AC-REG-01` | FreeRDP OHOS 交叉编译、Native/ArkTS 测试和 Debug HAP 已通过；无手写笔与外接屏真机证据，因此未标 Verified |

## 8. 2026-08-05 实施证据

- FreeRDP：`CHANNEL_RDPEI/CHANNEL_RDPEI_CLIENT=ON`；OHOS client 编译并链接 `ohos_pen.c`、monitor-layout settings 与 `disp` 动态更新。
- App Native：XComponent pen tool/pressure/tilt/eraser 分流、finger 系统手势排除 pen、blur/surface/disconnect cancel；DisplayManager 首次枚举和 add/remove/change 监听接入。
- 验证：FreeRDP OHOS arm64 交叉编译通过；`tools/run_tablet_native_tests.ps1`、`tools/run_tablet_arkts_tests.ps1` 退出码 0；`harmony/app/build_hap.bat debug` 完成 Native 编译、打包与 `SignHap`，产物 35753953 字节。
- 未覆盖：Windows Ink 压感/倾角/橡皮动作级真机测试；外接屏热插拔后的 Windows 显示器数量、拓扑和各屏四角点击。因此状态保持 `Implemented`。

## 9. 单屏期望布局状态命名优化

Change ID：`MON-DESIRED-LAYOUT-001`

设计状态：`DesignReady`

实现状态：`Implemented`

- App Native 持有的是最新“期望布局”，不是一份永远延迟发送的 cache。会话未激活时，`RdpSessionChannels::SetMonitorLayout` 只保存 desired layout，供创建 OHOS session 时作为 initial monitor layout；会话已激活时，同一入口立即更新 Display Control 状态。
- 单屏辅助函数命名为 `UpdateDesiredSingleMonitorLayout`，明确它既可连接前缓存，也可连接后应用。函数接收调用方已生成的同一份 `DisplayResizeRequest`，不重复读取 Surface/Display 快照，并统一负责校验和失败日志；日志使用 `desired single monitor layout updated`，携带 `session_connected` 和底层 detail，不通过字符串反推协议状态。
- Surface 稳定变化继续走 200ms trailing coalescer；只有连接前初始化和多屏回单屏的拓扑切换直接更新 desired layout，不新增 resize 旁路或第二套状态机。
- 不修改公共 ABI、Monitor Layout 字段、对齐算法、fallback、generation 或渲染行为。

计划代码文件：`harmony/app/entry/src/main/cpp/napi/native_bridge_context.cpp`。

验收：`AC-MON-02`、`AC-REG-01`；代码中无旧 cache 辅助函数引用，Native tests、App Native 构建和 Debug HAP 通过。

实施证据（2026-08-06）：

- 实际代码文件与计划一致；`CacheCurrentSingleMonitorLayout` 已替换为接收预计算 request 的 `UpdateDesiredSingleMonitorLayout`，调用点不再丢弃 bool 返回值或重复读取快照，日志包含 `session_connected` 和底层 detail。
- `tools/run_tablet_native_tests.ps1` 退出码 0；App Native arm64 CMake 编译、链接通过；`harmony/app/build_hap.bat` 完成打包和签名，HAP 为 35,854,621 字节。
- 文档记录的 `tools/check_tablet_architecture.ps1` 在当前仓库不存在，因此未宣称该项通过；本次仅重命名和日志语义收敛，不改变 ABI 或运行行为。
