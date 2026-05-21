# xrdp OHOS 商用分支整体整改文档

日期：2026-05-21

本文基于近期对 xrdp OHOS 适配代码、运行日志、目录边界、app native 与
`third_party/xrdp` 归属关系的扫描结果，整理后续商用分支的整体整改方案。

目标不是继续把功能堆在 HAP native bridge 里，而是把真正属于 xrdp OHOS
port 的能力逐步下沉到 `harmony/third_party/xrdp`，让后续对外交付的是一条
边界清晰、可维护、可验证的 xrdp 鸿蒙版源码分支。

## 1. 整改目标

- `harmony/app` 只保留 HAP、N-API、权限弹框、UI、加载器和诊断展示职责。
- `harmony/third_party/xrdp` 承担 xrdp OHOS backend 的协议、采集、输入、
  音频、剪贴板、队列、背压和日志统计职责。
- 控制单文件大小和职责边界，避免继续在大文件里追加能力。
- 建立生产日志策略，避免 INFO 日志刷屏或暴露输入、路径、剪贴板元数据。
- 建立稳定 ABI 策略，避免 app native 和 `libxrdpohos.so` 版本错配。
- 明确哪些系统弹框和权限限制目前不能通过代码绕过，避免产品预期失真。
- 建立商用验收门槛：构建、安装、连接、重连、音频、剪贴板、输入、采集、
  压力测试都要有明确验证项。

## 2. 当前边界问题

当前实现里，app native 的 `xrdp/` 目录已经不只是 bridge，而是包含大量
xrdp OHOS backend 实际逻辑。

app native 当前承担：

- `libxrdpserver.so` / `libxrdpohos.so` 的 `dlopen`、符号加载和路径解析。
- N-API 导出、ArkTS callback 和诊断结果对象构造。
- 收到 xrdp backend event 后启动/停止 OHOS 屏幕采集。
- `OH_AVScreenCapture` raw 采集。
- `OH_AVScreenCapture + OH_AVCodec` surface H264 采集编码。
- playback audio capture，然后转交 xrdp rdpsnd。
- xrdp 输入事件到 OHOS 输入注入的映射。
- 显示器 geometry 查询和鼠标坐标换算。

xrdp 源码当前承担：

- 嵌入式 `xrdp_ohos_server_main()` / `xrdp_ohos_server_stop()`。
- `libxrdpohos.so` backend module ABI。
- frame/audio/encoded frame submit API。
- `cliprdr` 协议和 OHOS Pasteboard / UDMF 适配。
- `rdpsnd` 协议和音频队列。
- AVC420 / RDPGFX 发送。
- xrdp/common 日志对接 HiLog。

商用分支应让第二部分继续扩大，第一部分收缩为薄桥。

## 3. 已发现的主要问题

### 3.1 目录职责混杂

以下 app native 文件包含了 xrdp OHOS port 的核心逻辑，后续不适合留在 HAP
bridge 里：

- `harmony/app/entry/src/main/cpp/xrdp/xrdp_screen_capture_bridge.cpp`
- `harmony/app/entry/src/main/cpp/xrdp/xrdp_surface_h264_capture.cpp`
- `harmony/app/entry/src/main/cpp/xrdp/xrdp_audio_capture_bridge.cpp`
- `harmony/app/entry/src/main/cpp/xrdp/xrdp_input_injector.cpp`
- `harmony/app/entry/src/main/cpp/xrdp/xrdp_display_geometry.cpp`
- `harmony/app/entry/src/main/cpp/xrdp/xrdp_server_bridge.cpp` 里的 capture
  lifecycle 和 backend event 驱动逻辑。

这些应逐步迁移或重写到 `harmony/third_party/xrdp/ohos`。

### 3.2 大文件风险

当前超过或接近维护警戒线的文件：

| 文件 | 当前情况 | 风险 |
| --- | --- | --- |
| `xrdp_server_bridge.cpp` | 约 1397 行 | loader、callback、capture、diagnostics 混在一起 |
| `ohos.c` | 约 1391 行 | module 生命周期、frame queue、input、event、API 混在一起 |
| `xrdp_input_injector.cpp` | 约 1056 行 | 授权、映射、队列、注入、坐标换算混在一起 |
| `xrdp_surface_h264_capture.cpp` | 约 714 行 | capture、encoder、audio pump、submit 混在一起 |
| `ohos_gfx_avc420.c` | 约 547 行 | 可暂缓，但后续新增能力前应拆分 |
| `ohos_cliprdr_protocol.c` | 约 530 行 | 可暂缓，后续新增格式前应拆分 |

建议规则：

- 新文件目标控制在 500 行左右。
- 超过 800 行要开始规划拆分。
- 超过 1000 行原则上不再追加新功能。
- 拆文件时优先保持行为不变，避免“拆分 + 功能改动”混在一笔提交里。

### 3.3 日志不适合生产

运行日志已经证明视频、音频、输入、剪贴板链路有数据，但日志策略还不适合
商用默认配置。

问题：

- `ohos.c` 里 key down/up/sync 当前以 INFO 打印，运行时会刷大量按键日志。
- 按键 code/flags 属于输入元数据，默认 INFO 不应该长期保留。
- per-frame、per-audio-buffer 日志虽然有采样，但缺少统一分类策略。
- 正常 mstsc 断连时，xrdp 原生日志可能出现 `xrdp_sec_recv failed`、
  `xrdp_rdp_recv failed`，需要在产品日志里区分“正常断开”和“异常失败”。
- 剪贴板 URI、本地路径、文件名等不应默认出现在 INFO 日志。

整改方向：

- 生产 INFO 只保留 session 生命周期、通道协商、采集启停、权限摘要、错误摘要。
- key/mouse/frame/audio/PDU 细节降为 DEBUG 或 runtime debug flag。
- 断连时输出一条 session summary，替代大量过程日志。

### 3.4 ABI 稳定性不足

`xrdp_ohos.h` 当前既是源码头文件，也是 app/backend ABI 边界，但版本策略不够。

风险：

- `xrdp_ohos_encoded_frame` 已经新增过 `flags`。
- status enum 已新增 `XRDP_OHOS_BACKEND_STATUS_BACKPRESSURE`。
- event 有 version，但 frame/audio/encoded frame 结构体没有 `size`。
- 如果 app native 和 `libxrdpohos.so` 不是同一版本，可能出现字段错读。

整改方向：

- 增加 `XRDP_OHOS_API_VERSION`。
- 增加 ABI 查询接口。
- 公共结构体增加 `size`，并采用 append-only 策略。
- app native 加载 backend 后先做 ABI/feature 检查。

### 3.5 生命周期和背压分裂

当前链路：

1. xrdp backend 产生 `SESSION_CONNECT` / `SUPPRESS_OUTPUT` / `MONITOR_RESIZE`
   等 event。
2. app native callback 收到 event 后启动或停止 OHOS capture。
3. app native 再把 capture frame/audio 通过 exported API 提交回 xrdp backend。

这条链路能跑，但职责割裂：

- xrdp backend 知道 RDP session 和客户端背压。
- app native 却控制 capture 生命周期。
- backpressure 在 app native 里只是普通 submit 失败，缺少专门统计和恢复策略。
- fps、码率、I 帧间隔、队列长度、重试间隔等配置散落在实现里。

商用方向应是：xrdp backend 内部根据 session event 自己控制 capture/audio/input，
app native 只负责启动 server 和查看 diagnostics。

### 3.6 已确认的具体缺陷

需要优先修复：

- `ohos.c`：encoded frame submit 中 `ohos_lock_frame_state()` 失败时，只释放了
  payload，没有释放 queued frame 对象，存在错误路径泄漏。
- `xrdp_audio_capture_bridge.cpp`：audio callback 日志读取 `label_` 在锁外，低
  概率数据竞争。
- `xrdp_server_bridge.cpp`：`XRDP_OHOS_BACKEND_STATUS_BACKPRESSURE` 没有作为
  受控状态单独处理和计数。

## 4. 目标目录结构

### 4.1 app native 目标结构

`harmony/app/entry/src/main/cpp` 保留：

```text
napi/                         N-API exports, callback sink, permission bridge
surface/                      XComponent / NativeWindow / app rendering
freerdp/                      FreeRDP client-side loader and adapters
session/                      FreeRDP client session only
channels/                     FreeRDP client channel adapters only
xrdp/
  xrdp_runtime_loader.*       xrdp library loading, HAP/HNP paths, symbol binding
  xrdp_server_bridge.*        thin start/stop/diagnostics wrapper
```

app native 后续不应再拥有 xrdp server 的 capture、input、audio、frame queue 逻辑。

### 4.2 xrdp OHOS 目标结构

`harmony/third_party/xrdp/ohos` 建议演进为：

```text
ohos_module.c                 xrdp mod lifecycle and callback table
ohos_backend_api.c            exported app/backend ABI entry points
ohos_backend_api.h
ohos_events.c                 backend events and session summary
ohos_frame_queue.c            BGRA/NV12/H264 queue and backpressure
ohos_frame_queue.h
ohos_capture.c                capture controller and session event policy
ohos_capture.h
ohos_capture_raw.c            AVScreenCapture raw fallback
ohos_capture_h264.c           AVScreenCapture + AVCodec surface H264 path
ohos_audio_capture.c          playback audio capture pump
ohos_audio_capture.h
ohos_display_geometry.c       NativeDisplayManager helpers
ohos_display_geometry.h
ohos_input.c                  xrdp input to OHOS input injection
ohos_input.h
ohos_gfx_avc420.c             existing AVC420/RDPGFX sender
ohos_rdpsnd*.c                existing rdpsnd protocol and queue
ohos_cliprdr*.c               existing clipboard backend
xrdp_ohos.h                   public ABI and feature flags
```

`ohos.c` 最终应缩小成 module lifecycle 和组合入口，不再承载全部逻辑。

## 5. 下沉计划

### Phase 0：先稳住现有链路

先修小问题，不改大架构：

- 修 encoded frame 错误路径泄漏。
- key event INFO 日志降级或采样。
- backpressure 单独计数和日志摘要。
- 抽出 fps、码率、队列长度、I 帧间隔等常量。
- 增加断连 session summary。
- 为 ABI version / struct size 做准备。

验收：

- 行为基本不变。
- 日志明显降噪。
- `git diff --check` 无新增问题。
- mstsc 连接、视频、输入、音频、剪贴板基本链路不回退。

### Phase 1：下沉 input 和 display geometry

迁移：

- `xrdp_input_injector.cpp`
- `xrdp_display_geometry.cpp`

目标文件：

- `ohos_input.c`
- `ohos_display_geometry.c`

设计：

- `ohos.c` 收到输入事件后直接调用 `ohos_input_inject()`。
- app native 不再注册 input callback 来回绕。
- 输入授权结果可以保留 app permission bridge，但注入状态机应在 xrdp 源码侧。
- 远程协助/输入注入系统弹框目前不能通过代码绕掉，作为产品限制记录。

验收：

- 鼠标移动、点击、拖拽、滚轮正常。
- 键盘字母、数字、Enter、Backspace、方向键、功能键正常。
- modifier 按下/释放正常。
- 断连时 release-all 生效。
- 授权拒绝和授权恢复路径不崩溃。

### Phase 2：下沉 audio capture

迁移：

- `xrdp_audio_capture_bridge.cpp`

目标文件：

- `ohos_audio_capture.c`

设计：

- `ohos_rdpsnd` 统一拥有 RDP sound protocol、音频队列和 OHOS playback capture。
- 先保持当前 PCM 44100 / 2ch / 16bit。
- 后续再评估 48000 或格式协商。

验收：

- Windows 端持续播放声音清晰。
- 30 分钟播放无明显延迟累积。
- reconnect while audio active 不崩溃、不泄漏。
- audio submitted/dropped/queued bytes 有摘要统计。

### Phase 3：下沉 screen capture 和 H264 capture

迁移：

- `xrdp_screen_capture_bridge.cpp`
- `xrdp_surface_h264_capture.cpp`
- `xrdp_server_bridge.cpp` 中的 capture lifecycle event 逻辑。

目标文件：

- `ohos_capture.c`
- `ohos_capture_raw.c`
- `ohos_capture_h264.c`

设计：

- `SESSION_CONNECT` 由 xrdp backend 内部启动 capture。
- `SESSION_DISCONNECT` 由 xrdp backend 内部停止 capture。
- `SUPPRESS_OUTPUT` 由 xrdp backend 内部暂停/恢复 capture。
- `MONITOR_RESIZE` 由 xrdp backend 内部 retarget 或 restart capture。
- H264 sync frame、queue limit、backpressure 都归 xrdp backend 管。
- app native 只调用 `startXrdpServer()` 和 `getXrdpDiagnostics()`。

验收：

- mstsc 100 次连接/断开。
- resize storm 不黑屏、不崩溃。
- suppress/resume output 正常。
- H264 路径可用，raw fallback 明确可观测。
- backpressure 后可以恢复。

### Phase 4：瘦身 app native bridge

完成前三阶段后：

- 从 app native `CMakeLists.txt` 移除 xrdp capture/input/audio 实现文件。
- `xrdp_server_bridge.cpp` 缩减为 loader、start、stop、diagnostics。
- 废弃或兼容保留旧 callback symbols。
- 更新文档和构建脚本。

验收：

- HAP build 通过。
- xrdp build 通过。
- HAP 安装和 mstsc smoke test 通过。
- `git diff --stat` 能看到 app native bridge 明显瘦身。

## 6. ABI 整改

建议在 `xrdp_ohos.h` 增加：

```c
#define XRDP_OHOS_API_VERSION 1

struct xrdp_ohos_abi_info
{
    uint32_t size;
    uint32_t api_version;
    uint32_t feature_flags;
    uint32_t status_flags;
};
```

后续公共结构体策略：

- 新公共结构体第一个字段加 `uint32_t size`。
- 旧结构按 legacy 兼容处理。
- 字段只追加，不在中间插入。
- app native 加载 `libxrdpohos.so` 后先检查 ABI version 和 feature flags。
- 新能力通过 feature flag 判断，不通过猜测符号是否存在判断完整能力。

建议 feature flags：

- raw frame submit
- encoded H264 submit
- audio submit
- direct OHOS input injection
- internal OHOS screen capture
- internal OHOS audio capture
- diagnostics snapshot

## 7. 日志整改

### 7.1 日志分类

建议统一分类：

- `SESSION`
- `VIDEO`
- `AUDIO`
- `INPUT`
- `CLIPBOARD`
- `PERMISSION`
- `ABI`
- `PERF`

### 7.2 生产 INFO 允许内容

默认 INFO 只打：

- server start/stop。
- client connect/disconnect。
- channel connected/disconnected。
- capture start/stop/restart。
- permission requested/granted/denied 摘要。
- sampled performance summary。
- disconnect session summary。

### 7.3 DEBUG 或诊断开关内容

以下内容不应默认 INFO：

- key code / key flags。
- mouse 坐标逐条日志。
- 每帧日志。
- 每个 audio buffer 日志。
- 剪贴板 payload 细节。
- 文件 URI、本地路径、文件名。
- 原始 PDU trace。

### 7.4 断连摘要

断连时输出一条结构化摘要：

```text
xrdp session summary:
  duration_ms=
  desktop=
  frames_submitted=
  frames_dropped=
  h264_queue_dropped=
  audio_submitted=
  audio_dropped=
  clipboard_reads=
  clipboard_writes=
  input_queued=
  input_injected=
  input_dropped=
  last_error=
```

这样生产环境可以默认保留关键诊断，同时避免过程日志刷屏。

## 8. 配置整改

需要从实现细节里抽出来的配置：

- xrdp listen port。
- capture fps。
- capture max dimension。
- H264 bitrate。
- H264 I-frame interval。
- H264 queue limit。
- raw frame queue limit。
- audio sample rate / channels / bits。
- audio queue bytes。
- reconnect / retry interval。
- log sampling interval。

商用默认值应保守、可解释、可在文档中查到。调试覆盖项可以后续再加。

## 9. 权限和安全说明

必须明确写入产品说明：

- 输入注入和远程协助弹框受 OHOS 系统策略控制，当前 xrdp 代码不能保证消除。
- `ohos.permission.INJECT_INPUT_EVENT`、ACL、签名权限只能解决资格问题，不等价于
  所有系统确认框都消失。
- 屏幕录制、全盘访问、Pasteboard 读写都受系统权限策略控制。
- full-disk access 只能用于用户授权后的文件类剪贴板场景，不应作为绕权限手段。
- 剪贴板文件 URI 和本地路径不要默认进入 INFO 日志。
- 商用 UI 和默认配置必须移除 demo host、用户名、密码、ignore cert 这类调试值。

## 10. 验收门槛

### 10.1 构建门槛

- xrdp OHOS clean build 通过。
- HAP clean build 通过。
- `git diff --check` 无新增空白问题。
- autotools 生成文件和 line ending 变更可解释。
- HAP 使用正式签名材料可安装。

### 10.2 运行门槛

- Windows mstsc 可连接到 xrdp 端口。
- 100 次 connect/disconnect 不崩溃。
- resize storm 不黑屏、不崩溃。
- suppress/resume output 正常。
- app 前后台切换后 session 行为可控。
- xrdp stop/start 不需要重启 HAP 进程。

### 10.3 功能门槛

- 屏幕能持续显示当前 OHOS 内容。
- H264 可用或 raw fallback 可用，且日志能说明当前路径。
- 鼠标移动、点击、拖拽、滚轮正常。
- 键盘和 modifier release 正常。
- 文本剪贴板双向正常。
- HTML、图片、URI、文件类剪贴板单独验证。
- 音频可听、清晰、无持续延迟累积。

### 10.4 失败路径门槛

- 用户拒绝权限不崩溃。
- Pasteboard 不可用不崩溃。
- 音频采集不可用不崩溃。
- H264 encoder 不可用时 fallback 或错误清晰。
- 客户端断连时 capture/audio/input 能释放。
- backpressure 后可以恢复。

## 11. 商用分支 checklist

- [ ] app native xrdp 逻辑缩减为 loader/start/stop/diagnostics。
- [ ] OHOS input injection 下沉到 xrdp 源码。
- [ ] OHOS display geometry 下沉到 xrdp 源码。
- [ ] OHOS playback audio capture 下沉到 xrdp 源码。
- [ ] OHOS screen/H264 capture 下沉到 xrdp 源码。
- [ ] `ohos.c` 按职责拆分。
- [ ] `xrdp_ohos.h` ABI version / struct size / feature flags 落地。
- [ ] 生产日志策略落地。
- [ ] disconnect session summary 落地。
- [ ] demo credentials 和调试默认值清理。
- [ ] 权限和系统弹框限制写入产品说明。
- [ ] 设备压力测试完成。

## 12. 建议提交拆分

建议按以下顺序拆提交：

1. 修已知缺陷和生产日志噪声。
2. 增加 ABI metadata 和 diagnostics counters。
3. 拆分 `ohos.c`，不改变行为。
4. 下沉 input 和 display geometry。
5. 下沉 audio capture。
6. 下沉 screen/H264 capture。
7. 瘦身 app native bridge 和更新 build files。
8. 更新验证记录、权限限制和商用说明。

每笔提交都应能独立构建、独立 review，避免一笔提交同时做目录迁移、行为改动和
日志策略调整。

## 13. 2026-05-21 Phase 1 执行记录

已完成 display geometry 和输入注入下沉的第一步：

- `ohos_display_geometry.c` 已进入 `third_party/xrdp/ohos`，app 侧 `xrdp_display_geometry.cpp` 只保留薄封装。
- 新增 `ohos_input_auth.c`、`ohos_input_keymap.c`、`ohos_input_mouse.c`、`ohos_input.c`、`ohos_input.h`，分别承载注入授权、键盘映射、鼠标坐标/事件映射、注入状态和对外入口。
- `ohos.c` 在收到 xrdp 输入事件后直接调用 native input 模块，并在 session summary 中输出 native input 摘要。
- `xrdp_ohos.h` 新增 `XRDP_OHOS_FEATURE_DIRECT_INPUT`，app bridge 发现该 feature 后跳过旧的 input callback 注册；旧 `xrdp_input_injector.cpp` 先保留为旧 backend fallback，Phase 4 再移除。
- xrdp backend 新增 `multimodalinput/oh_input_manager.h` / `oh_key_code.h` configure 检查，并链接 `-lohinput`。

本轮验证：

- `wsl bash harmony/scripts/wsl/build-xrdp-ohos.sh` 通过。
- `cmd /c build_hap.bat` 在 `harmony/app` 下通过，输出 `entry-default-signed.hap`。
- 已安装到设备 `3QC0124C11000711` 并启动 `com.huawei.freerdp/EntryAbility`。
- `Test-NetConnection 127.0.0.1 -Port 13390` 通过，设备侧 `3390` 处于 LISTEN。
- Windows `mstsc /v:127.0.0.1:13390` 已连接；日志中出现 `xrdp.ohos.input: mouse inject ... rc=0`，说明输入已经走 xrdp native 侧注入路径。

## 14. 2026-05-21 Phase 2 执行记录

已完成 audio pump 核心实现下沉的第一步：

- 新增 `ohos_audio_capture_bridge.cpp` / `.h` 到 `third_party/xrdp/ohos`，承载 playback audio capture 配置、audio-ready worker、buffer acquire/release、采样日志和统计。
- app 侧 `xrdp_audio_capture_bridge.cpp` 缩减为薄包装，只负责把 `xrdp_ohos_audio_frame` 提交到现有 backend ABI。
- 当前 audio capture 的生命周期仍由 app 侧 screen/H264 capture 对象触发；等 screen/H264 capture 下沉后，再把 start/stop policy 完整移入 xrdp backend。

本轮验证：

- `cmd /c build_hap.bat` 在 `harmony/app` 下通过。
- 已安装到设备 `3QC0124C11000711` 并启动 `com.huawei.freerdp/EntryAbility`。
- Windows `mstsc /v:127.0.0.1:13390` 已连接；日志中出现 `xrdp audio capture pump started label=surface-h264`、`xrdp audio capture queued ...`、`xrdp.ohos.rdpsnd: sent wave chunk ...`、`wave confirm ...`。
- 连接初期在 rdpsnd channel ready 之前有少量 `status=-4` not queued 日志，随后 client format/training 完成并正常发送音频；该行为符合当前生命周期边界，后续 screen/H264 下沉时可进一步延后 pump start。

## 15. 2026-05-21 Phase 3 子阶段执行记录

已完成 screen/H264 capture 核心实现下沉的第一步，保持现有 session event 策略不变：

- `third_party/xrdp/ohos` 新增 `ohos_capture_types.h`、`ohos_capture_common.*`、`ohos_capture_raw.*`、`ohos_capture_h264.*`、`ohos_capture_h264_encoder.*`、`ohos_h264_payload.*`。
- raw screen capture、surface H264 capture、H264 encoder 配置/I 帧请求、H264 payload Annex-B 规范化、capture 日志与 diagnostics 类型已经放到 xrdp 源码侧。
- app 侧 `xrdp_screen_capture_bridge.cpp` 与 `xrdp_surface_h264_capture.cpp` 缩减为薄包装，只负责类型转换和调用 `QueueXrdpVideoFrame` / `QueueXrdpEncodedVideoFrame` / `QueueXrdpAudioFrame`。
- 当前 `SESSION_CONNECT` / `SESSION_DISCONNECT` / `SUPPRESS_OUTPUT` / `MONITOR_RESIZE` lifecycle 仍由 app 侧 `xrdp_server_bridge.cpp` 驱动；下一阶段再把 capture controller 和 session policy 继续下沉，避免本笔同时改变运行策略。

文件大小结果：

- `ohos_capture_raw.cpp` 约 383 行。
- `ohos_capture_h264.cpp` 约 424 行。
- `ohos_capture_h264_encoder.cpp` 约 164 行。
- `ohos_h264_payload.cpp` 约 121 行。
- app 侧 `xrdp_screen_capture_bridge.cpp` 约 84 行，`xrdp_surface_h264_capture.cpp` 约 64 行。

本轮验证：

- `cmd /c build_hap.bat` 在 `harmony/app` 下通过。
- 已安装到设备并启动 `com.huawei.freerdp/EntryAbility`。
- `Test-NetConnection 127.0.0.1 -Port 13390` 通过，设备侧 `3390` 处于 LISTEN。
- Windows `mstsc /v:127.0.0.1:13390` 已连接；日志中出现 `xrdp surface H264 encoder ready`、`xrdp surface H264 capture started 2560x1440@15fps`、`xrdp surface H264 frame queued: seq=1`、`xrdp.ohos.frame: queued AVC420 frame`。
- 音频链路仍正常，日志中出现 `xrdp.ohos.rdpsnd: client formats`、`training round_trip_ms`、`audio queued`、`sent wave chunk`、`wave confirm`。
