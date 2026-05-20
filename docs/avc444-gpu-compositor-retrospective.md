# AVC444 GPU compositor 复盘

日期：2026-05-20

## 结论

这次问题反复没有改对，核心原因不是单个 shader 或单个 stride bug，而是 AVC444 的更新语义没有严格贴住 FreeRDP 原生路径。早期实现把“硬解成功”“纹理更新成功”“可以 suppress GDI”“可以 present”混在 `SurfaceCommand` 里处理，导致 GDI surface、GPU texture、dirty rect、EndFrame 四套状态不同步。

最终正确方向是：

- AVC420 surface 硬解路径继续独立保留，不和 AVC444 GPU compositor 绑定。
- AVC444 / AVC444v2 默认仍有 FreeRDP native GDI fallback。
- GPU compositor 只在当前 `SurfaceCommand` 已完整处理成功后，才 suppress 这一条 FreeRDP native GDI 更新。
- present 不在 `SurfaceCommand` 里直接 swap，而是排队到匹配的 `EndFrame`。
- 一旦 GPU 已成为输出 owner，GDI 渲染线程不能再把旧的 CPU surface 盖回 XComponent。

## 为什么之前几次没改对

### 1. 过早 suppress GDI

早期按“协商到 AVC444 + 开关开启”就倾向于跳过 FreeRDP 原生 GDI。问题是 GPU path 当时还可能没有拿到完整 luma/chroma decode 输出，也可能还没有完成 base chroma 状态初始化。

这样一来，FreeRDP 的 `avc444_decompress()` 没跑，GDI surface 没更新；GPU 又没有可 present 的完整状态，显示端就会停在旧帧、白屏、闪屏或局部脏区。

正确做法是 per-command suppress：只有当前 AVC444 command 在 GPU 侧完成 decode、layout、texture update、pending present 入队后，才返回 consumed，阻止原生 GDI 路径处理这一条 command。

### 2. validate 顺序不符合原生路径

FreeRDP 原生 `gdi_SurfaceCommand_AVC444()` 的顺序是：

1. `GetSurfaceData()`
2. `is_within_surface()`
3. 创建或复用单个 `surface->h264`
4. `avc444_decompress()`
5. stream1 rects 更新 `invalidRegion` 并调用 `UpdateSurfaceArea`
6. stream2 rects 更新 `invalidRegion` 并调用 `UpdateSurfaceArea`
7. `gdi_interFrameUpdate()`

之前 OHOS GPU callback 先进入 HAP compositor，可能已经解码、更新纹理、claim owner，然后 FreeRDP OHOS bridge 才做 surface/rect validate。这违反了原生调用顺序。无效 surface 或越界 rect 本来应该直接返回错误，不能污染 GPU 状态。

这次把 `ohos_rdpgfx_validate_avc444_gpu_surface_update()` 移到 GPU callback 之前：校验失败时直接返回对应错误，不进入 GPU，不 claim owner。

### 3. 两个 H.264 decoder 不等价于 FreeRDP 原生 H264_CONTEXT

社区原生 AVC444 使用一个 `H264_CONTEXT`，`avc444_decompress()` 在同一个上下文里按 LC 和 stream 顺序处理 luma/chroma。之前 GPU 版用 luma/chroma 两个 OH_AVCodec decoder，各自维护 SPS/PPS、DPB、参考帧和输出节奏。

这不是 bit-for-bit 的解码状态模型。即使 SPS/PPS 补包能缓解启动问题，两个 decoder 仍然可能在参考帧、输出时机、NoOutput、重置语义上和原生不一致。

这次改成单个 `Avc444HardwareDecoder`，luma 和 chroma 按 command 语义顺序喂给同一个硬解上下文。它仍不是 FreeRDP 的 CPU `H264_CONTEXT`，但至少和“一个 AVC444 解码状态机”的模型对齐。

### 4. present 时机错误

之前在 `SurfaceCommand` 里处理完纹理后直接 present，和 FreeRDP 的 frame boundary 不一致。RDPGFX 的一帧可能包含多条 surface command，`EndFrame` 才是客户端应该稳定输出的边界。

这会造成：

- 当前 command 的 luma/chroma 和本帧其他更新不同步；
- dirty rect 对不上 frame id；
- 鼠标移动或局部刷新后只画出对应区域，旧区域不能恢复；
- GDI EndPaint 或 UpdateSurfaces 可能在 GPU present 之后又覆盖旧 CPU surface。

这次 GPU path 只在 `SurfaceCommand` 中更新内部 YUV state 并记录 `pendingPresent`，实际 swap 放到匹配的 `EndFrame` callback。

### 5. 把 FreeRDP dirty state 和 GPU state 混用

原生 AVC444 解码后会写 `surface->data`，然后更新 `surface->invalidRegion` 并调用两次 `UpdateSurfaceArea`。GPU suppress GDI 后，`surface->data` 没有被 `avc444_decompress()` 更新。

如果此时仍然更新 FreeRDP 的 invalidRegion，后续 `gdi_EndFrame()` / `UpdateSurfaces()` 就会拿 stale CPU surface 去 present，等于把旧画面盖到 GPU 输出上。

这次 suppress GDI 的 GPU command 不再触碰 FreeRDP dirty state。GPU 自己维护 YUV state 和 pending present；GDI surface 只在 GPU 没有消费 command 时由原生路径更新。

### 6. shader 合成曾经偏离 primitive 语义

曾经的 present shader 对 even/even 像素做过类似 `4*U - neighbor` 的二次修正，但 FreeRDP `general_ChromaV2ToYUV444` 合成后的路径是直接 YUV444 -> RGB，不在最终 present 再做这类修正。

这类“看起来补偿 chroma”的逻辑会放大旧 chroma 或邻域污染，表现就是颜色叠加、鼠标经过区域变了但恢复不了。后续改成按照 luma/chroma update 阶段维护 YUV state，present 阶段直接采样当前 Y/U/V 转 RGB。

### 7. zero-copy 方向过早引入了额外变量

真正 zero-copy 应该是两路 decoder surface/texture 直接由 shader 采样合成。但之前尝试把 decoder output surface、XComponent target、NativeWindow format/stride 放到一条链里，导致需要同时处理：

- `NativeWindow` format 可能沿用 RGBA；
- OHOS buffer stride 可能大于 `width * 4`；
- decoder surface 和显示 surface 的 ownership；
- CPU GDI renderer 与 GPU compositor 谁占 XComponent；
- resize 和 surface destroy 的异常恢复。

这些变量掩盖了 AVC444 语义本身的问题。当前正确版本删除了 zero-copy 尾巴，使用硬解输出的 mappable planes 上传到 GL texture，再由 shader 合成。它还不是最终性能最优，但状态边界清楚。

## 这次正确的调用链

### FreeRDP 原生 AVC444 fallback

当 GPU 没有消费当前 command 时，流程保持 FreeRDP 原生语义：

```text
RDPGFX SurfaceCommand(AVC444/AVC444v2)
  -> ohos_rdpgfx_surface_command()
  -> ohos_rdpgfx_record_avc444_gpu_candidate()
       GPU disabled / not ready / current command failed
       return FALSE
  -> original RdpgfxClientContext::SurfaceCommand
  -> gdi_SurfaceCommand_AVC444()
  -> avc444_decompress()
  -> UpdateSurfaceArea(stream1 rects)
  -> UpdateSurfaceArea(stream2 rects)
  -> gdi_interFrameUpdate()
  -> OHOS GDI/RGBA render backend
```

这条路径是正确性兜底。只要 GPU 没有明确消费当前 command，就不破坏 FreeRDP 已验证的 AVC444 语义。

### GPU AVC444 command path

GPU 消费成功时，调用链是：

```text
RDPGFX SurfaceCommand(AVC444/AVC444v2)
  -> ohos_rdpgfx_surface_command()
  -> ohos_rdpgfx_record_avc444_gpu_candidate()
  -> validate surface and command rect first
  -> build FREERDP_OHOS_RDPGFX_AVC444_COMMAND_INFO
  -> OhosRdpgfxAvc444SurfaceCommandCallback()
  -> SharedAvc444GpuCompositor().OnSurfaceCommand()
  -> Avc444GpuCompositorImpl::ProcessCommand()
       validate LC and rects
       feed luma/chroma into one OH_AVCodec decoder
       wait bounded synchronous output
       OH_NativeBuffer_MapPlanes / AVBuffer fallback
       upload decoded planes to GL textures
       ApplyLuma / ApplyChromaV1 / ApplyChromaV2
       queue pending EndFrame present
       claim render output owner when GPU becomes authoritative
  -> return TRUE
  -> ohos_rdpgfx_surface_command() returns without original GDI SurfaceCommand
```

这里 `TRUE` 的含义很严格：当前 command 已经在 GPU state 中完整落地，且已经排队等待 frame boundary present。否则必须返回 `FALSE`，让 FreeRDP 原生 GDI 路径处理。

### EndFrame present path

GPU 不在 SurfaceCommand 里直接 swap。正确 present 链路是：

```text
RDPGFX EndFrame
  -> ohos_rdpgfx_end_frame()
  -> original RdpgfxClientContext::EndFrame
  -> OhosRdpgfxAvc444EndFrameCallback()
  -> SharedAvc444GpuCompositor().OnEndFrame()
  -> Avc444GpuCompositorImpl::PresentEndFrame()
       require pendingPresent
       require endFrame.frameId == pendingFrameId
       attach current XComponent NativeWindow if needed
       draw YUV state to window surface
       eglSwapBuffers()
       clear pending present
```

如果 command 不在 StartFrame/EndFrame 包围内，bridge 会模拟一次 frame callback，让 GPU 仍然按同一套 pending present 逻辑输出，而不是直接在 command 中随手 swap。

### GDI render ownership

GPU 一旦成为 authoritative output owner：

```text
Avc444GpuCompositor claims RenderOutputOwner::Avc444Gpu
  -> stop GDI render pipeline
  -> release GDI render target before GPU takeover
  -> HarmonyEndPaint sees Avc444Gpu owner
  -> clear stale GDI invalid region
  -> skip GDI frame queue
```

恢复到 GDI 的入口包括：

- compositor `Configure(false)` / `Reset()`;
- XComponent surface destroy;
- rdpgfx diagnostics hook restore / detach;
- 首次 present 前失败时释放 warm-up ownership。

这样可以避免“GPU 刚画完，GDI 又把旧 CPU surface 盖回去”的闪屏。

## 关键源码位置

- FreeRDP 原生 AVC444：`harmony/third_party/FreeRDP/libfreerdp/gdi/gfx.c`
  - `gdi_SurfaceCommand_AVC444()`
  - `avc444_decompress()`
  - `UpdateSurfaceArea()` 两次调用
  - `gdi_interFrameUpdate()`
- OHOS RDPGFX bridge：`harmony/third_party/FreeRDP/client/OHOS/ohos_rdpgfx.c`
  - `ohos_rdpgfx_surface_command()`
  - `ohos_rdpgfx_record_avc444_gpu_candidate()`
  - `ohos_rdpgfx_validate_avc444_gpu_surface_update()`
  - `ohos_rdpgfx_end_frame()`
- HAP channel glue：`harmony/app/entry/src/main/cpp/channels/rdpgfx_pipeline.cpp`
  - `ConfigureGraphicsPipelineChannel()`
  - `InstallRdpgfxDiagnosticsHooks()`
  - `OhosRdpgfxAvc444SurfaceCommandCallback()`
  - `OhosRdpgfxAvc444EndFrameCallback()`
  - `RestoreRdpgfxDiagnosticsHooks()`
- GPU compositor facade：`harmony/app/entry/src/main/cpp/surface/avc444_gpu_compositor.cpp`
  - `OnSurfaceCommand()`
  - `OnEndFrame()`
- GPU compositor implementation：`harmony/app/entry/src/main/cpp/surface/avc444_gpu_compositor_internal.cpp`
  - `Avc444HardwareDecoder`
  - `Avc444GpuRenderer`
  - `Avc444GpuCompositorImpl::ProcessCommand()`
  - `Avc444GpuCompositorImpl::PresentEndFrame()`
- GDI ownership guard：`harmony/app/entry/src/main/cpp/freerdp/freerdp_gdi_bridge.cpp`
  - `HarmonyEndPaint()`
  - `StartGdiRenderPipeline()`

## 用到的 Harmony / OHOS 接口

当前不是 zero-copy，主要使用这些 OHOS 能力：

- `OH_AVCodec` / `OH_VideoDecoder_*`
  - 创建硬解 decoder；
  - 配置 H.264、同步模式、低延迟；
  - input buffer 入队；
  - output buffer 同步查询；
  - 获取 output description、stride、slice height、pixel format。
- `OH_AVBuffer`
  - 访问 input/output buffer；
  - 读写 buffer attr；
  - 在没有 native buffer planes 时作为 fallback 读取连续内存。
- `OH_NativeBuffer`
  - `OH_AVBuffer_GetNativeBuffer()`;
  - `OH_NativeBuffer_GetConfig()`;
  - `OH_NativeBuffer_MapPlanes()`;
  - `OH_NativeBuffer_Unmap()` / `OH_NativeBuffer_Unreference()`。
- EGL / OpenGL ES
  - pbuffer 作为离屏 YUV state 更新目标；
  - window surface 绑定 XComponent NativeWindow；
  - shader 做 luma/chroma update 和最终 YUV -> RGB present；
  - `eglSwapBuffers()` 在 EndFrame 边界输出。
- XComponent NativeWindow
  - 由 HAP surface bridge 提供当前显示目标；
  - GPU takeover 前释放 GDI render target，避免双写同一个窗口。

## 当前仍需注意

1. 这不是最终 zero-copy compositor。现在仍然是 `OH_NativeBuffer_MapPlanes -> GL texture upload -> shader compose/present`。
2. GPU suppress 后没有调用 FreeRDP `UpdateSurfaceArea()`，这是有意差异：因为 CPU `surface->data` 没有更新，调用它反而会让 GDI present 旧数据。
3. 单个 OH_AVCodec decoder 比两个 decoder 更接近 FreeRDP AVC444 状态模型，但仍不是 CPU primitive 的 bit-exact 实现。
4. AVC444v1 / AVC444v2 都已接入 GPU path，但 v1/v2 chroma layout 后续仍建议用 FreeRDP primitive 输出做离线对照测试。
5. 如果 GPU 已经 authoritative，再随意回退 GDI 是危险的，因为 FreeRDP native H264 context 已经错过被 suppress 的 bitstream。活动状态下失败应丢弃本次 GPU update、保留上一帧或重置 GPU decoder 等下一次可恢复 command，不能中途把同一条 stream 交回 GDI。

## 后续排查日志应该看什么

优先看这些日志关键字：

- `AVC444 GPU compositor candidate`
- `rejected command before GPU callback`
- `handled command; suppressing FreeRDP native GDI`
- `queued EndFrame present`
- `EndFrame callback skipped`
- `dropped pending present at EndFrame mismatch`
- `presented at EndFrame`
- `FreeRDP GDI EndPaint skipped: outputOwner=avc444-gpu`
- `OHOS AVC444 route=freerdp-native-gdi avc444_decompress success`

正常 GPU 路径应该能看到：

```text
candidate -> update detail -> queued EndFrame present -> handled command; suppressing GDI -> EndFrame present attempt -> presented at EndFrame
```

正常 fallback 路径应该能看到：

```text
candidate -> did not request GDI suppression -> OHOS AVC444 route=freerdp-native-gdi avc444_decompress success
```

如果看到 GPU 已经 `suppress=this-command`，但随后 GDI `EndPaint` 仍然入队真实 frame，就说明 render output owner 又被错误释放或 GDI guard 漏了。
