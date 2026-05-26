# HAP demo 适配点与源码归属清单

日期：2026-05-17

结论：`harmony/app` 只保留验证 HAP 的职责。长期应交付到源码的 OHOS
RDP 语义，放到 `harmony/third_party/FreeRDP/client/OHOS` 或对应 FreeRDP
后端目录；HAP 只保留 UI、权限、`XComponent`/`NativeWindow` 句柄、N-API
转发、用户配置和诊断展示。

## 已收口到 FreeRDP OHOS 源码

| 适配点 | 源码位置 | HAP 现在只做 |
| --- | --- | --- |
| 键盘映射、modifier 合成、repeat、release-all | `client/OHOS/ohos_keyboard.*` | 传 OHOS key event，调用 native relay |
| IME committed text 到 Unicode key event | `client/OHOS/ohos_ime.*` | 传 committed text |
| Pointer flags、wheel delta、viewport 到 desktop 坐标映射 | `client/OHOS/ohos_pointer.*` | 传 `RemotePointerEvent` 语义事件和本地 surface 坐标 |
| 证书策略解析和 TOFU/strict/ignore callback 决策 | `client/OHOS/ohos_certificate.*` | 传用户选择、app-local 存储目录，展示日志 |
| display-control resize layout | `client/OHOS/ohos_display.*` | 转发 XComponent 宽高 |
| graphics mode、RDPGFX capability、H.264 fallback policy | `client/OHOS/ohos_graphics.*`, `client/OHOS/ohos_rdpgfx.*` | 传 `graphicsMode` 和 surface 能力 |
| 连接默认 settings 和标准 channel 参数 | `client/OHOS/ohos_session_config.*` | 传 host/user/password/resolution 等用户输入 |
| location 地理位置重定向 | `client/OHOS/ohos_location.*` | 做定位授权回调 |
| rdpsnd 播放后端 | `channels/rdpsnd/client/ohos/` | 展示播放诊断 |
| audin 采集后端 | `channels/audin/client/ohos/` | 做麦克风授权回调 |
| OHOS AVCodec H.264 解码入口 | `libfreerdp/codec/h264_ohos_avcodec.c` | 提供 `NativeWindow`/surface target |

## HAP 里允许保留

- ArkUI 连接表单、会话页、日志页和临时验证控件。
- `INTERNET`、`READ_PASTEBOARD`、`MICROPHONE`、`APPROXIMATELY_LOCATION`、`LOCATION` 权限声明和授权弹窗。
- `XComponent` 创建、销毁、尺寸变化、焦点管理和 `NativeWindow` 句柄转交。
- N-API 方法：`probe/connect/disconnect/sendPointerEvent/sendPlatformKey/sendText/releaseAllKeys` 等薄转发。
- 本地 app sandbox 目录、证书 known-hosts 存储目录的传入。
- 诊断展示：surface 状态、runtime symbol 状态、输入队列、音频和图形统计。

## 仍需注意的边界

| HAP 边界 | 处理状态 |
| --- | --- |
| `RemotePointerInput.ets` 拼 RDP pointer flags/wheel delta | 已改为语义事件；flags/wheel 由 `ohos_pointer.*` 生成 |
| `PointerMapper.ets` 重算 desktop 坐标 | 已删除；native 根据真实 surface viewport 映射 |
| Debug host/user/password/ignore cert 默认值 | 已清空账号信息，默认 `certPolicy=tofu`、`graphicsMode=rdpgfx-h264`，并默认启用 AVC444 GPU compositor |
| `certificate_policy.*` 证书 callback 行为 | 策略解析和 callback 决策已委托 `ohos_certificate.*` |
| `freerdp_session_runner.cpp` 连接 settings 默认值 | 已委托 `freerdp_ohos_session_apply_connection_settings` |
| channel 默认参数 | 已由 `ohos_session_config.*` 负责 |
| location 采样和 RDP PDU 语义 | 已委托 `ohos_location.*`；HAP 只处理权限，不调用位置采样 API |
| RDPGFX surface route 诊断 | route/caps/surface-command 统计在 `ohos_rdpgfx.*`；HAP 保留 NativeWindow 绑定和日志转发 |

## 后续建议

1. 真机验证 pointer、wheel、drag、touch scroll、changed certificate 和 GDI/rdpgfx 两种模式。
2. 如果要把 HAP 变成产品壳，再补本地 profile 或账号管理，不要恢复硬编码连接信息。
3. RDPGFX 的 NativeWindow 绑定仍在 HAP，这是合理边界；若后续有独立 OHOS client frontend，再整体迁到 `client/OHOS`。
