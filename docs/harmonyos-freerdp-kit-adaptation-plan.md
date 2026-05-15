# HarmonyOS FreeRDP Kit Adaptation Plan

更新日期：2026-05-15

本文档记录当前 HarmonyOS FreeRDP App 的四个核心问题、原因分析、代码归属建议、可执行实施计划、每一步修改点、验收标准、遗留风险和影响。

当前结论：

- 音频、剪贴板、图形加速和整体性能不能只靠把 FreeRDP channel 编译进 HAP。它们需要对接 HarmonyOS 运行时 Kit。
- 协议核心和平台设备 backend 应尽量放在 FreeRDP 适配层中，不应继续堆在 `napi_init.cpp`。
- ArkTS / N-API 层只负责 UI、生命周期、配置、权限、状态和日志，不负责实现 RDP 协议细节。
- 性能问题需要分阶段处理。先做 GDI 路径减负，再接 `rdpgfx` / H.264；不能直接打开增强管线，因为此前打开后出现过黑屏回退。

## 当前代码状态

相关文件：

- `harmony/app/entry/src/main/ets/pages/Index.ets`
- `harmony/app/entry/src/main/cpp/napi_init.cpp`
- `harmony/scripts/wsl/build-freerdp-ohos.sh`
- `harmony/third_party/FreeRDP/`
- `harmony/README.md`
- `docs/freerdp-ohos-feature-matrix.md`

当前实现状态：

- RDP 连接、认证、GDI 画面渲染、鼠标键盘输入已经打通。
- FreeRDP 增强依赖已经编译并打包，包括 `cliprdr`、`rdpsnd`、`audin`、`rdpgfx`、FFmpeg、OpenH264、OpenSLES 等。
- 运行时为避免黑屏，`rdpgfx/H.264/AVC444` 仍关闭，使用软件 GDI framebuffer。
- 音频目前只是请求静态 `rdpsnd` + `sys:opensles`，没有真正接 HarmonyOS `OH_AudioRenderer`。
- 剪贴板通道已编译，但没有对接 HarmonyOS Pasteboard，也没有注册完整 `cliprdr` callback。
- 当前渲染路径是 CPU 拷贝整帧到 `NativeWindow`，未做 dirty rect、帧队列、限帧、GPU texture 或硬解。

## 问题 1：音频没声音

### 根因

当前代码在 `napi_init.cpp` 中请求：

```cpp
{"rdpsnd", "sys:opensles"}
```

这只表示 FreeRDP 会尝试加载 `rdpsnd` 的 OpenSLES 后端。当前构建脚本中使用了 `OpenSLES_Android.h` shim，使 Android 风格的 FreeRDP OpenSLES 后端能在 OHOS NDK 下编译，但这不是完整 HarmonyOS 音频播放适配。

实际缺口：

- 没有实现 FreeRDP `rdpsndDevicePlugin` 的 OHOS backend。
- 没有调用 `OH_AudioStreamBuilder` / `OH_AudioRenderer`。
- 没有管理音频设备生命周期、PCM 格式变化、缓冲延迟、音量、后台暂停。
- 没有确认 `Play` / `PlayEx` 是否真正收到 PCM 数据并写入系统音频输出。

### 正确架构

```text
Windows RDP audio
  -> RDP rdpsnd channel
  -> FreeRDP rdpsnd decoder
  -> rdpsnd sys:ohos backend
  -> OH_AudioRenderer
  -> HarmonyOS speaker output
```

### 代码归属

建议放在 FreeRDP 平台适配层：

```text
harmony/third_party/FreeRDP/channels/rdpsnd/client/ohos/
  rdpsnd_ohos.c
  CMakeLists.txt
```

App bridge 层只负责设置：

```cpp
{"rdpsnd", "sys:ohos"}
```

不建议把 `OH_AudioRenderer` 直接写进 `napi_init.cpp`，否则后续音频 backend 无法复用，也会让 N-API bridge 继续膨胀。

### 伪码

```c
typedef struct
{
    rdpsndDevicePlugin iface;
    OH_AudioRenderer* renderer;
    AUDIO_FORMAT currentFormat;
    uint32_t sampleRate;
    uint32_t channels;
    uint32_t bitsPerSample;
    bool started;
} OhosRdpsndDevice;

static BOOL ohos_rdpsnd_format_supported(rdpsndDevicePlugin* device, const AUDIO_FORMAT* format)
{
    return format->wFormatTag == WAVE_FORMAT_PCM &&
           (format->wBitsPerSample == 16 || format->wBitsPerSample == 32) &&
           (format->nChannels == 1 || format->nChannels == 2);
}

static BOOL ohos_rdpsnd_open(rdpsndDevicePlugin* device, const AUDIO_FORMAT* format, UINT32 latency)
{
    OhosRdpsndDevice* ohos = (OhosRdpsndDevice*)device;

    OH_AudioStreamBuilder_Create(&builder, AUDIOSTREAM_TYPE_RENDERER);
    OH_AudioStreamBuilder_SetSamplingRate(builder, format->nSamplesPerSec);
    OH_AudioStreamBuilder_SetChannelCount(builder, format->nChannels);
    OH_AudioStreamBuilder_SetSampleFormat(builder, convert_sample_format(format));
    OH_AudioStreamBuilder_SetEncodingType(builder, AUDIOSTREAM_ENCODING_TYPE_RAW);
    OH_AudioStreamBuilder_SetRendererInfo(builder, AUDIOSTREAM_USAGE_MOVIE, AUDIOSTREAM_CONTENT_TYPE_SPEECH);
    OH_AudioStreamBuilder_GenerateRenderer(builder, &ohos->renderer);
    OH_AudioRenderer_Start(ohos->renderer);

    ohos->currentFormat = *format;
    ohos->started = true;
    return TRUE;
}

static UINT ohos_rdpsnd_play(rdpsndDevicePlugin* device, const BYTE* data, size_t size)
{
    OhosRdpsndDevice* ohos = (OhosRdpsndDevice*)device;
    if (!ohos->renderer || !ohos->started) {
        return CHANNEL_RC_BAD_INIT_HANDLE;
    }

    int32_t written = OH_AudioRenderer_Write(ohos->renderer, (void*)data, size);
    return written >= 0 ? CHANNEL_RC_OK : CHANNEL_RC_IO_ERROR;
}

static void ohos_rdpsnd_close(rdpsndDevicePlugin* device)
{
    OhosRdpsndDevice* ohos = (OhosRdpsndDevice*)device;
    if (ohos->renderer) {
        OH_AudioRenderer_Stop(ohos->renderer);
        OH_AudioRenderer_Release(ohos->renderer);
        ohos->renderer = NULL;
    }
    ohos->started = false;
}
```

### 执行计划

#### A1：新增 `rdpsnd sys:ohos` backend 骨架

修改范围：

- `harmony/third_party/FreeRDP/channels/rdpsnd/client/ohos/rdpsnd_ohos.c`
- `harmony/third_party/FreeRDP/channels/rdpsnd/client/ohos/CMakeLists.txt`
- `harmony/third_party/FreeRDP/channels/rdpsnd/client/CMakeLists.txt`
- `harmony/scripts/wsl/build-freerdp-ohos.sh`
- `harmony/app/entry/src/main/cpp/napi_init.cpp`
- `docs/freerdp-ohos-feature-matrix.md`

验收标准：

- WSL clean build 通过。
- HAP 中仍包含 `libfreerdp-client3.so` 和必要系统依赖。
- `probe()` 日志显示 `rdpsnd sys:ohos` 可用。
- 连接 Windows 后日志能看到 `rdpsnd ohos backend open`。

风险和影响：

- FreeRDP third_party 需要维护 OHOS 分支，后续升级有合并成本。
- 初始阶段可能只支持 PCM 16-bit stereo，部分远端格式需要降级或拒绝。

#### A2：实现 `OH_AudioRenderer` PCM 写入

修改范围：

- `rdpsnd_ohos.c`
- CMake 链接 OHOS audio native 库。
- native 日志增加音频 open/play/close 统计。

验收标准：

- 播放 Windows 系统测试音能从真机扬声器输出。
- 断开连接后音频 renderer 被释放。
- 重连后声音仍正常。
- 插拔/切换系统音量不会导致 RDP 连接崩溃。

风险和影响：

- 缓冲过小会破音，缓冲过大会延迟。
- 后台播放策略需要按 HarmonyOS 应用生命周期处理。
- 如果 Windows 端禁用了远程音频播放，客户端正常也不会有声音。

#### A3：音频生命周期和降级策略

修改范围：

- `rdpsnd_ohos.c`
- `EntryAbility.ets`
- `Index.ets`
- `napi_init.cpp` 中新增状态上报，不放音频核心逻辑。

验收标准：

- App 后台、前台恢复、断开、重连不泄露 renderer。
- 音频 backend 初始化失败时连接仍可继续，只提示音频不可用。
- Diagnostics 展示采样率、声道、播放包数、最后错误。

风险和影响：

- 后台是否继续播放需要产品策略明确。
- 音频权限和隐私提示需要按真机系统要求验证。

## 问题 2：视频很卡

### 根因

当前画面路径是：

```text
FreeRDP software GDI framebuffer
  -> CPU copy / scale full frame
  -> NativeBuffer map/unmap
  -> NativeWindow flush
```

视频卡顿的核心原因：

- `rdpgfx/H.264/AVC444` runtime 当前关闭。
- 视频画面退化成大量 GDI bitmap/order 更新。
- 当前每次 paint 接近整帧拷贝，没有 dirty rect。
- 每帧会做 CPU 缩放、填黑边、map/unmap、flush。
- 渲染在 RDP 回调路径上，渲染慢会拖累协议处理。

### 正确架构

短期稳定优化：

```text
FreeRDP GDI dirty region
  -> frame queue, keep latest
  -> render thread
  -> dirty rect copy
  -> NativeWindow flush
```

长期视频优化：

```text
FreeRDP rdpgfx
  -> AVC420/AVC444/H.264
  -> software decode first: FFmpeg/OpenH264
  -> later optional OHOS AVCodec hardware decode
  -> NativeWindow / GPU texture
```

### 代码归属

短期 GDI 渲染优化可以暂时在 App native bridge 层做，但应抽出独立模块：

```text
harmony/app/entry/src/main/cpp/rdp_renderer/
  native_window_renderer.cpp
  native_window_renderer.h
  frame_queue.cpp
  frame_queue.h
```

长期 `rdpgfx` backend 建议放 FreeRDP 平台适配层：

```text
harmony/third_party/FreeRDP/client/ohos/
  ohos_gfx.c
  ohos_gfx.h
  ohos_avcodec_decoder.c
```

### 执行计划

#### V1：增加性能指标，不改渲染策略

修改范围：

- `napi_init.cpp` 或新 `native_window_renderer.*`
- `Index.ets` Diagnostics 显示指标。

指标：

- RDP paint count
- render fps
- average copy ms
- average flush ms
- frame drop count
- last dirty rect count
- full-frame copy count

验收标准：

- 连接后 Diagnostics 能看到 fps 和 render 耗时。
- 指标不会导致 UI 日志暴涨。

风险和影响：

- 只观测，不解决卡顿。
- 指标采样必须低频，避免增加额外卡顿。

#### V2：渲染线程和最新帧队列

修改范围：

- 新增 `frame_queue.*`
- 新增 `native_window_renderer.*`
- `napi_init.cpp` 的 `HarmonyEndPaint` 只入队，不直接做重渲染。

伪码：

```cpp
void HarmonyEndPaint(rdpContext* context)
{
    RgbaFrameSnapshot snapshot = CaptureLatestGdiFrame(context);
    g_renderQueue.ReplaceLatest(snapshot);
    g_renderThread.Wakeup();
}

void RenderThreadLoop()
{
    while (running) {
        RgbaFrameSnapshot frame;
        if (!queue.WaitPopLatest(frame)) {
            continue;
        }
        renderer.Render(frame);
    }
}
```

验收标准：

- 视频播放时 RDP 连接不断。
- 输入延迟不随渲染阻塞明显放大。
- Diagnostics 中能看到丢帧计数，但画面仍保持最新。

风险和影响：

- 需要拷贝或锁定 GDI framebuffer，避免 RDP 更新线程覆盖正在渲染的数据。
- 内存占用会上升，一帧 1280x720 RGBA 约 3.5 MB。

#### V3：dirty rect 拷贝和限帧

修改范围：

- `HarmonyBeginPaint` / `HarmonyEndPaint`
- `native_window_renderer.*`
- `Index.ets` 增加可选帧率配置或先写死 30fps。

伪码：

```cpp
bool ShouldRenderNow()
{
    return now - lastRenderAt >= 33ms || dirtyAreaRatio > 0.35;
}

void RenderDirtyRects(const Frame& frame, const DirtyRegion& dirty)
{
    for (rect in dirty.rects) {
        CopyScaledRect(frame, nativeBuffer, rect);
    }
    FlushDirtyRegion(nativeWindow, dirty);
}
```

验收标准：

- 普通桌面操作 CPU 占用下降。
- 鼠标移动和文本输入不卡顿。
- 视频仍可能不流畅，但不应拖垮整个 App。

风险和影响：

- 如果缩放后 dirty rect 计算错误，会出现残影或局部未刷新。
- 需要全屏 fallback，用于 resize、格式变化、首帧。

#### V4：重新接 `rdpgfx` 软件解码

修改范围：

- `napi_init.cpp` 中恢复 `rdpgfx` settings，但必须受 runtime 开关控制。
- FreeRDP `client/ohos/ohos_gfx.*`
- `docs/freerdp-ohos-feature-matrix.md`

验收标准：

- 开启 `rdpgfx` 后不黑屏。
- Windows 视频播放比 GDI 路径更流畅。
- 关闭开关可回退 GDI。

风险和影响：

- 此前已经出现过打开增强图形管线黑屏，因此必须做独立分支和回退开关。
- AVC444/H.264 格式处理复杂，首版建议只开软件解码。

#### V5：评估 OHOS AVCodec 硬解

修改范围：

- `client/ohos/ohos_avcodec_decoder.*`
- FreeRDP codec dispatch 适配。

验收标准：

- H.264 bitstream 能进入 AVCodec。
- 解码输出能转成 NativeWindow 可用 buffer 或中间 RGBA。
- 视频 CPU 明显下降。

风险和影响：

- AVCodec 与 FreeRDP frame ack、surface lifetime、异步解码节奏耦合较强。
- 这是高风险优化，不应和剪贴板、音频一起混改。

## 问题 3：剪贴板粘贴失败

### 根因

当前只是编译了 `cliprdr`，但没有实现运行时闭环：

- 没有加载或配置 `cliprdr` runtime channel。
- 没有注册 `CliprdrClientContext` callbacks。
- 没有对接 HarmonyOS Pasteboard。
- 没有做 `CF_UNICODETEXT` 与 Harmony text/plain 的转换。
- 没有处理本地剪贴板变化通知、远端格式列表、数据请求和响应。

### 正确架构

远端复制到本机：

```text
Windows clipboard
  -> RDP cliprdr ServerFormatList / DataResponse
  -> FreeRDP cliprdr callback
  -> OHOS pasteboard setData
```

本机复制到远端：

```text
OHOS pasteboard getData
  -> FreeRDP cliprdr ClientFormatList / DataResponse
  -> Windows clipboard
```

### 代码归属

协议部分建议放 FreeRDP 平台适配层：

```text
harmony/third_party/FreeRDP/client/ohos/
  ohos_cliprdr.c
  ohos_cliprdr.h
```

鸿蒙系统剪贴板访问可以做薄适配：

```text
harmony/app/entry/src/main/cpp/platform/ohos_pasteboard_adapter.cpp
harmony/app/entry/src/main/cpp/platform/ohos_pasteboard_adapter.h
```

如果 native Pasteboard API 不满足当前 SDK 条件，可以先通过 ArkTS Pasteboard + N-API callback 做过渡。

### 首版范围

只做文本：

- `CF_UNICODETEXT`
- `text/plain`
- UTF-16LE <-> UTF-8 转换
- 暂缓 HTML、RTF、图片、文件列表。

### 执行计划

#### C1：加载 `cliprdr` 并注册回调

修改范围：

- `napi_init.cpp`
- `ohos_cliprdr.*`
- `build-freerdp-ohos.sh`

伪码：

```c
static UINT ohos_cliprdr_monitor_ready(CliprdrClientContext* cliprdr, const CLIPRDR_MONITOR_READY* ready)
{
    send_client_capabilities(cliprdr);
    send_client_format_list_text_only(cliprdr);
    return CHANNEL_RC_OK;
}

static UINT ohos_cliprdr_server_format_list(CliprdrClientContext* cliprdr, const CLIPRDR_FORMAT_LIST* list)
{
    cache_remote_formats(list);
    if (remote_has_unicode_text(list)) {
        request_remote_unicode_text(cliprdr);
    }
    return CHANNEL_RC_OK;
}
```

验收标准：

- Diagnostics 显示 `cliprdr loaded`。
- 连接后 Windows 端复制文本，客户端收到 `ServerFormatList`。
- 不影响基础连接和画面。

风险和影响：

- 一旦 `cliprdr` 加载方式导致动态通道变化，可能影响连接稳定性。需要保留关闭开关。

#### C2：远端复制到本机 Pasteboard

修改范围：

- `ohos_cliprdr.*`
- `ohos_pasteboard_adapter.*`
- `Index.ets` 可增加诊断显示。

伪码：

```c
static UINT ohos_cliprdr_server_format_data_response(
    CliprdrClientContext* cliprdr,
    const CLIPRDR_FORMAT_DATA_RESPONSE* response)
{
    if (!response->msgFlags & CB_RESPONSE_OK) {
        return CHANNEL_RC_OK;
    }

    char* utf8 = ConvertCfUnicodeTextToUtf8(response->requestedFormatData, response->dataLen);
    OhosPasteboardSetText(utf8);
    free(utf8);
    return CHANNEL_RC_OK;
}
```

验收标准：

- Windows 远端复制英文、中文文本，本机粘贴板能粘贴出一致内容。
- 大文本有大小上限和错误提示。
- 断开后不继续写 pasteboard。

风险和影响：

- 系统剪贴板读写权限或安全提示可能影响“普通应用少权限”目标。
- 中文、换行、末尾 NUL 处理容易出错。

#### C3：本机复制到远端

修改范围：

- `ohos_cliprdr.*`
- `ohos_pasteboard_adapter.*`
- ArkTS 可加“同步剪贴板”按钮作为过渡。

伪码：

```c
void NotifyLocalClipboardChanged()
{
    if (!cliprdrReady) {
        return;
    }
    send_client_format_list_text_only(cliprdr);
}

static UINT ohos_cliprdr_server_format_data_request(
    CliprdrClientContext* cliprdr,
    const CLIPRDR_FORMAT_DATA_REQUEST* request)
{
    if (request->requestedFormatId != CF_UNICODETEXT) {
        send_empty_response(cliprdr);
        return CHANNEL_RC_OK;
    }

    char* utf8 = OhosPasteboardGetText();
    BYTE* utf16le = ConvertUtf8ToCfUnicodeText(utf8);
    send_data_response(cliprdr, utf16le);
    return CHANNEL_RC_OK;
}
```

验收标准：

- 本机复制文本后，在远端 Windows 可以粘贴。
- 中文、换行、英文正常。
- 空剪贴板不会导致远端粘贴卡死。

风险和影响：

- 监听系统剪贴板变化可能受系统限制。首版可以用手动同步按钮降低权限和生命周期风险。

## 问题 4：整体卡顿

### 当前记录

2026-05-15 真机验证后，当前最严重问题已经从“无声音/不可连接”转为“会话整体卡顿”。表现为远程桌面视频和高频画面变化时画面刷新不连续，输入响应也可能被拖慢。

当前必须区分两条路线：

- 短期治理：继续使用 FreeRDP software GDI，但把渲染从 RDP 回调线程拆出去，做 latest-frame queue、限帧、指标、dirty rect，避免 NativeWindow map/copy/flush 拖慢协议处理。
- 长期治理：接 `rdpgfx/H.264 -> OHOS AVCodec -> NativeWindow/GPU texture`，这才是真正的 GPU 直连/少拷贝路线。当前 GDI framebuffer 本身是 CPU 内存，不能直接做到严格零拷贝。

影响：

- 如果先不做短期治理，直接打开 `rdpgfx/H.264`，会重新遇到此前增强图形管线黑屏、frame ack、surface lifetime 和回退策略不完整的问题。
- 如果只做 GDI 优化，视频仍可能不如硬解流畅，但能先降低协议线程阻塞、输入延迟和整体 App 卡顿。

本轮先落地 S4-1：

- `HarmonyEndPaint` 不再直接执行 NativeWindow map/copy/flush。
- 增加 latest-frame render thread，RDP 线程只投递最新 GDI frame。
- NativeWindow buffer 按远端帧尺寸申请，缩放交给系统合成器/GPU，避免每帧 CPU 放大。
- 入队使用 direct GDI pointer，去掉 RDP 线程里的整帧 snapshot copy。
- Diagnostics 增加 render queued/rendered/replaced/throttled/fps/copyMs/renderMs。

本轮继续落地 S4-2：

- 帧率控制从 `HarmonyEndPaint` 回调线程移动到 render thread。
- `HarmonyEndPaint` 每次只更新 latest-frame 指针并立即返回，不再因为帧间隔过短返回 `render throttled`。
- render thread 按 16ms 目标间隔取当时最新帧写入 NativeWindow，旧 pending frame 只统计为 replaced。
- Diagnostics 将原 `throttled` 含义调整为 `paced`，表示 render thread 因帧节奏等待的次数。

验收标准：

- 高频画面下 hilog 不再刷 `FreeRDP GDI frame queue throttled`。
- 日志显示 `FreeRDP GDI frame queued: 1280x720 latest-gdi`。
- render thread 继续稳定输出帧，单帧 renderMs 维持低毫秒级。
- 真机连接后远程视频页面可显示，输入和断开不回归。

遗留风险：

- 目标 16ms 会比 33ms 消耗更多 CPU/内存带宽；如果真机发热或耗电明显，需要改成自适应 16/33ms。
- 当前仍未做 dirty rect，NativeWindow 写入仍是整帧。

本轮继续落地 S4-3：

- 暂缓直接实现 dirty rect。原因是当前 NativeWindow 走 buffer queue，未确认 buffer age 前只拷贝脏区可能让非脏区残留旧 buffer 内容，造成残影。
- 先降低会话热路径日志开销：`FreeRDP audio diagnostics` 从连接后立即每 2 秒输出一次，改为连接 10 秒后每 10 秒输出一次。
- 目标是减少高频视频场景下 hilog 大字符串拼接和写日志对协议线程的干扰。

验收标准：

- 连接后前 10 秒不再持续输出大段 audio diagnostics。
- 远程视频页面仍可显示，render thread 日志继续正常。
- 音频链路不变，只降低诊断日志频率。

遗留风险：

- 音频问题排查时实时性下降；需要排查音频时可临时缩短诊断间隔或改成手动诊断入口。

本轮继续落地 S4-4：

- render thread 保存最近一次 latest frame 元数据。
- XComponent surface created/changed 后，如果会话中已有最新帧，主动排队一次 `surface repaint`。
- repaint 帧绕过 16ms pacing，优先恢复画面，避免窗口重建、尺寸变化后短暂黑屏。
- surface resize 过程中 repaint 请求会被 latest-frame 队列合并，日志只输出前几次和周期性计数，避免 resize 时刷屏。

验收标准：

- surface created/changed 时不崩溃。
- 已连接场景下重建 surface 后能复用最新帧补画。
- 不回退 S4-1 的远端尺寸 NativeWindow buffer 策略。

遗留风险：

- repaint 仍然读取 GDI direct pointer；disconnect/resize 前已停止 render thread，但极端并发下仍需生命周期压测。

遗留风险：

- direct GDI pointer 读取期间 RDP 线程可能继续写 framebuffer，极端情况下可能出现轻微撕裂；resize/disconnect 前已停止 render thread，避免释放后访问。
- 当前仍不是完整 GPU 零拷贝，render thread 仍需把 GDI framebuffer 写入 NativeWindow buffer。彻底解决视频高负载还需要 `rdpgfx/H.264 -> OHOS AVCodec -> NativeWindow/GPU texture`。

### 根因

整体卡顿不是单点问题，是渲染、协议、输入和 UI 日志叠加：

- RDP 线程直接触发重渲染。
- 每帧 CPU 整帧拷贝和缩放。
- 视频未走 `rdpgfx/H.264`。
- Diagnostics 日志持续追加，文本越来越大。
- 输入、渲染、日志和状态回调都在争抢资源。
- 没有背压策略，渲染慢时不丢旧帧。

### 执行计划

#### P1：日志限长和降频

修改范围：

- `napi_init.cpp`
- `Index.ets`

方案：

- native 侧 ring buffer，最多 300 条或 500 条。
- 高频日志只按采样输出，例如每 60 帧一次。
- ArkTS 日志显示只显示最后 N 条。

验收标准：

- 长时间连接 10 分钟后 Diagnostics 不明显卡顿。
- 日志仍能看到关键状态、错误和统计。

风险和影响：

- 过度限流可能隐藏问题，需要保留 debug 开关。

#### P2：输入与渲染隔离

修改范围：

- `napi_init.cpp`
- `frame_queue.*`

方案：

- 输入队列继续合并 pointer move。
- 渲染线程慢时只丢画面帧，不丢输入。
- 断开连接时先停止输入接收，再停止渲染，再释放 RDP context。

验收标准：

- 视频播放时鼠标点击仍有响应。
- 输入队列 drop 只发生在极端情况下，并有统计。

风险和影响：

- 多线程生命周期复杂度增加，必须补充断开、重连、页面销毁测试。

#### P3：NativeWindow 调用减负

修改范围：

- `native_window_renderer.*`

方案：

- surface size 不变时，不每帧重复设置 geometry/format/usage。
- 只在 `OnSurfaceCreated` / `OnSurfaceChanged` 或格式变化时设置。
- 复用 renderer 状态。

验收标准：

- render copy/flush 平均耗时下降。
- 连接、resize、断开不崩溃。

风险和影响：

- 某些设备 surface 重建后状态失效，需要检测并重新设置。

#### P4：分辨率策略

修改范围：

- `Index.ets`
- `napi_init.cpp`

方案：

- 默认远端分辨率改为接近 XComponent 实际大小，减少缩放。
- 提供低/中/高三个档位：
  - 960x540
  - 1280x720
  - 1600x900
- 后续接 `disp` 后再做动态分辨率。

验收标准：

- 低档位明显更流畅。
- 字体可读性和操作区域可接受。

风险和影响：

- 分辨率降低会影响清晰度。
- 动态分辨率仍需要 `disp` channel 后续支持。

## 推荐阶段路线

### S1：文档和设计确认

目标：

- 明确核心平台能力归属。
- 不再把音频、剪贴板、图形核心全部写进 `napi_init.cpp`。

修改点：

- 新增本文档。
- 后续可同步更新 `docs/freerdp-ohos-feature-matrix.md`。

验收：

- 文档说明四个问题根因。
- 每个问题都有代码位置、执行步骤、验收点、风险。

风险：

- 仅文档，不改变运行行为。

### S2：音频 OHOS backend

目标：

- `rdpsnd` 真正对接 HarmonyOS `OH_AudioRenderer`。

修改点：

- FreeRDP 新增 `channels/rdpsnd/client/ohos`。
- 构建脚本启用 `sys:ohos`。
- App 运行时从 `sys:opensles` 切到 `sys:ohos`。

验收：

- Windows 测试音在真机出声。
- 断开、重连、后台不会崩溃。

风险：

- 缓冲、格式和生命周期是主要风险。
- OpenSLES 路径可保留为 fallback，但不作为主线。

### S3：文本剪贴板

目标：

- 本机和远端双向文本复制粘贴可用。

修改点：

- FreeRDP 新增 `client/ohos/ohos_cliprdr.*`。
- 新增 OHOS Pasteboard 适配。
- 首版只支持文本。

验收：

- Windows -> HarmonyOS 文本复制成功。
- HarmonyOS -> Windows 文本粘贴成功。
- 中文、英文、换行正常。

风险：

- 系统剪贴板权限策略可能需要 UI 明示或手动同步按钮。

### S4：GDI 性能治理

目标：

- 在不打开 `rdpgfx` 的前提下降低整体卡顿。

修改点：

- 抽出 native renderer。
- 增加 frame queue。
- dirty rect。
- 日志限流。
- NativeWindow 状态复用。

验收：

- 普通桌面操作不卡。
- 长时间连接 Diagnostics 不拖慢 UI。
- 输入延迟比当前版本低。

风险：

- dirty rect 计算错误会造成残影。
- 多线程生命周期需要压测。

### S5：rdpgfx/H.264 软件解码

目标：

- 解决视频场景卡顿。

修改点：

- FreeRDP `client/ohos/ohos_gfx.*`。
- 开启 `rdpgfx` runtime 开关。
- 先走 FFmpeg/OpenH264 软件解码。
- 保留 GDI fallback。

验收：

- 开启增强图形后不黑屏。
- 视频播放帧率优于 GDI。
- 关闭开关可回退稳定 GDI。

风险：

- 这是最高风险阶段。
- 之前黑屏说明不能只开配置，必须完整接 gfx surface/update/ack。

### S6：AVCodec 硬解评估

目标：

- 用 HarmonyOS 媒体能力降低 H.264 视频 CPU。

修改点：

- 新增 OHOS AVCodec decoder adapter。
- 对接 FreeRDP codec pipeline。

验收：

- H.264 数据可被硬解。
- CPU 占用下降。
- 画面同步正常。

风险：

- 硬解异步模型、buffer ownership、surface lifetime 都复杂。
- 不建议在 S2/S3/S4 未完成前启动。

## 总体验收矩阵

| 阶段 | 验收点 | 必须真机验证 | 可回退策略 |
| --- | --- | --- | --- |
| S2 音频 | Windows 测试音出声 | 是 | 回退 `sys:opensles` 或关闭音频 |
| S3 剪贴板 | 双向文本复制粘贴 | 是 | 关闭 `cliprdr` |
| S4 GDI 性能 | 输入、桌面操作流畅度提升 | 是 | 回退直接渲染 |
| S5 rdpgfx | 视频流畅且不黑屏 | 是 | 回退 GDI |
| S6 硬解 | CPU 降低、画面同步 | 是 | 回退软件解码 |

## 关键风险清单

1. FreeRDP third_party 改动会增加后续升级成本，需要保持 OHOS patch 小而清晰。
2. 音频和剪贴板属于系统能力，对普通应用权限和后台策略敏感。
3. `rdpgfx/H.264` 不能直接打开，必须做完整 backend 和 fallback。
4. 性能优化需要指标先行，否则无法判断改动有效性。
5. 当前硬编码调试账号、密码、证书策略只能用于真机验证，不应进入生产配置。
6. 当前 HAP 体积已经随 FFmpeg/OpenH264 等依赖增大，后续需要评估裁剪。

## 后续提交建议

每一步单独提交：

1. `docs: add HarmonyOS FreeRDP kit adaptation plan`
2. `freerdp: add OHOS rdpsnd backend skeleton`
3. `freerdp: play rdpsnd audio through OH_AudioRenderer`
4. `freerdp: add OHOS text clipboard bridge`
5. `native: move rendering into queued NativeWindow renderer`
6. `native: add dirty rect rendering and metrics`
7. `freerdp: add gated OHOS rdpgfx software decode path`

每个提交必须带：

- 修改范围。
- 真机验证结果。
- 未解决问题。
- 回退方式。

## 参考

- OpenHarmony OHAudio native playback: https://docs.openharmony.cn/pages/v4.1/en/application-dev/media/audio/replace-opensles-by-ohaudio.md
- OpenHarmony Pasteboard API: https://github.com/openharmony/docs/blob/master/zh-cn/application-dev/reference/apis-basic-services-kit/js-apis-pasteboard.md
- FreeRDP rdpsnd client interface: `harmony/third_party/FreeRDP/include/freerdp/client/rdpsnd.h`
- FreeRDP cliprdr client interface: `harmony/third_party/FreeRDP/include/freerdp/client/cliprdr.h`
- FreeRDP rdpgfx client interface: `harmony/third_party/FreeRDP/include/freerdp/client/rdpgfx.h`
