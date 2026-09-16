# GPU Compositor 拆分设计

将 `avc420_gpu_compositor_internal.cpp`（2684 行）与 `avc444_gpu_compositor_internal.cpp`（2771 行）拆到每个文件 <1000 行（满足 `ohos-native-cpp-module-guidelines.md` 硬上限），并抽出逐字重复的工具到公共头。台账：TAB-A-08。

## 策略

保守拆分 + 抽公共工具，**不抽类基类**。理由：Decoder/Renderer/State 差异大（平面映射 vs EGLImage、双流 LC vs 单流+GDI 背景、mapped-plane 多纹理 vs OES 单纹理），强抽继承关系在无真机验证下会让 AVC420/AVC444 硬解渲染路径崩，风险远大于收益。M11 的结构重复留后续单独立项（需真机 + 抽基类）。

## 公共工具头 `avc_gpu_common.h/.cpp`（阶段 1）

收**逐字相同**（命名空间提升到 `rdp_bridge::avc_gpu`）：

| 类别 | 符号 |
|---|---|
| 常量 | `kAvcMime`、`kInputTimeoutUs`、`kTimingSampleInterval`（值相同者；数值不同的 kOutputTimeoutUs 等留各自） |
| 工具函数 | `ShouldLogFrequent`、`ShouldSampleTiming`、`NowMicros`、`FormatRectText`、`RectsText`、`RectsValid`、`NativeBufferFormatName`（合并 RGB 系列） |
| 类型 | `TimingBucket`、`ScopedTiming`、`PreparedH264Packet`、`DecodeResult` |
| H264 解析 | `ReadBe32`、`FindAnnexBStartCode`、`ExtractH264ParameterSets`（`PrepareH264Packet` 留各自——avc444 双缓存 role 与 avc420 单缓存签名不同） |
| GL 辅助 | `CompileShader`、`LinkProgram`（日志前缀参数化） |

## avc420_internal 拆分映射（阶段 2，2684 → 5 文件）

| 新文件 | 来源行号 | 约行数 | 内容 |
|---|---|---|---|
| `avc420_gpu_compositor_utils.cpp` | 32–512（扣除公共） | ~280 | avc420 独有：`ActiveAvc420UpdatePolicy`、`FitAvc420PresentViewport`、`FormatFixed`、`FormatMs`、`MakeDecoderPts`、`NativeDecodedFrame`、`NativeFrameText` + 独有常量 |
| `avc420_hardware_decoder.cpp` | 514–816 | ~302 | `Avc420HardwareDecoder` |
| `avc420_native_buffer_renderer.cpp` | 818–1684 | ~867 | `Avc420NativeBufferRenderer` + 3 段 shader |
| `avc420_gpu_compositor_state.cpp` | 1688–2615 | ~928 | `Avc420GpuCompositorImpl::State` |
| `avc420_gpu_compositor_impl.cpp` | 2617–2683 | ~67 | `Avc420GpuCompositorImpl` 转发 |

新建 `avc420_gpu_compositor_internal_types.h` 暴露 `NativeDecodedFrame`/`Avc420HardwareDecoder`/`Avc420NativeBufferRenderer`/`State` 完整定义。

## avc444_internal 拆分映射（阶段 3，2771 → 4 文件）

| 新文件 | 来源行号 | 约行数 | 内容 |
|---|---|---|---|
| `avc444_gpu_compositor_utils.cpp` | 30–552（扣除公共） | ~300 | avc444 独有：`IsGpuReadbackEnabled`、`CodecRoleLogPrefix`、`AlignUp`、`PlaneView`、`DecodedFrame`、`FramePlaneText`、`IsValidLcForCommand`、`RectsCoverFullSurface`、`RequiredChromaV1SourceYHeight`、`StreamText` + 独有常量 |
| `avc444_hardware_decoder.cpp` | 554–1011 | ~460 | `Avc444HardwareDecoder` |
| `avc444_gpu_renderer.cpp` | 1013–2064 | ~980 | `Avc444GpuRenderer` + 7 段 shader（临界；若超抽 `SampleFramebuffer` 到 `avc444_gpu_readback.cpp`） |
| `avc444_gpu_compositor_state.cpp` | 2069–2771 | ~700 | `State` + 外层成员定义 |

新建 `avc444_gpu_compositor_internal_types.h`。

## CMakeLists

`common/src/main/cpp/CMakeLists.txt:114-124` surface 源文件清单：移除两个 `_internal.cpp`，新增所有新 `.cpp`。

## 不变项

- 渲染逻辑不变：shader 源、纹理模型、EGL/GL 调用顺序、解码器生命周期均不改动，纯文件拆分 + 工具提取。
- 公共 ABI、回调结构、`SharedAvc*GpuCompositor()` 单例、`render_output_owner` 切换均不变。

## 验收

- AC-ARCH：每文件 <1000 行；avc420/avc444 各 internal 拆分后文件职责清晰；公共工具无重复定义。
- AC-RESIZE：codec/fallback 真机渲染不变（AVC420 硬解 + AVC444 硬解 + GDI fallback + 旋转/分屏）。
- AC-XC：Surface 生命周期与拆分前一致。
- 构建：每阶段 `build_app` debug 0 error + signed。
- 真机：AVC420/AVC444 硬解画面 + GDI fallback（留真机验证）。

## 风险与回退

- 阶段 1 最低风险（工具逐字相同）。阶段 2/3 中风险（include + 内部头 + CMakeLists）。错误表现为编译/链接失败，可即时修。
- 渲染逻辑不变，真机验证是确认拆分未破坏 EGL/GL 调用顺序的保证。
- 每阶段独立 git 提交，可回退。
