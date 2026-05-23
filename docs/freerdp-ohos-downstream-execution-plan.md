# HarmonyOS FreeRDP 下沉与可接入化任务清单

状态：待执行  
目标分支：`codex/prelaunch-main`  
目标交付：可商用、可被其他 HarmonyOS 应用快速接入的 FreeRDP OHOS 版本

## 执行原则

每个任务必须同时满足三件事：代码职责更清楚、HAP 层更薄、外部应用更容易接入。

执行时按任务编号推进。不要一次性重构多个方向；每个任务完成后都要能单独构建、单独回归、单独说明风险。

## 职责边界

### 下沉到 FreeRDP

- RDP 协议语义：settings、channels、证书策略、rdpgfx、codec、input packet、clipboard format。
- 会话策略：连接参数校验、domain/user 拆分、默认通道开关、graphics fallback、display-control resize。
- 平台后端：OHAudio、OHOS AVCodec、Pasteboard backend、FreeRDP OHOS helper API。

### 留在 HAP

- ArkTS UI、权限弹窗、账号表单、上架文案。
- N-API 导出和事件桥。
- XComponent 注册、触摸手势、NativeWindow/EGL/GLES 生命周期。
- 应用签名、bundleName、隐私协议、商用配置。

## 任务总览

| ID | 任务 | 优先级 | 依赖 | 主要产出 |
| --- | --- | --- | --- | --- |
| T00 | 建立基线和验收脚本 | P0 | 无 | 可重复验证当前行为 |
| T01 | 去掉商用构建中的 mock/header fallback | P0 | T00 | 缺 FreeRDP 时 fail fast |
| T02 | 新增 FreeRDP OHOS Session 公共 API | P0 | T00 | `freerdp_ohos_session_*` |
| T03 | HAP 改成调用 Session API | P0 | T02 | HAP 不再拼接核心 FreeRDP 流程 |
| T04 | 会话参数和存储路径下沉 | P1 | T02 | `ohos_session_options/config` |
| T05 | 证书策略彻底下沉 | P1 | T04 | HAP 只负责展示/确认 |
| T06 | 通道默认开关改成显式配置 | P1 | T04 | clipboard/mic/audio 不默认全开 |
| T07 | graphicsMode 和 fallback 单一来源 | P1 | T03 | HAP 不再解析图形语义 |
| T08 | 输入队列和 worker dispatch 下沉 | P1 | T02 | `ohos_input_queue` |
| T09 | HAP XComponent 输入文件拆分 | P2 | T08 | HAP 手势代码可维护 |
| T10 | display-control resize 管理下沉 | P2 | T02 | resize 逻辑可复用 |
| T11 | RDPGFX bridge 拆分 | P1 | T07 | caps/surface/diagnostics 分离 |
| T12 | AVC444 policy 下沉并默认关闭 GPU path | P1 | T11 | FreeRDP 统一决定 GDI suppress |
| T13 | AVC420 surface fallback 收敛 | P2 | T11 | fallback 走向可解释 |
| T14 | 剪贴板按职责拆分 | P1 | T06 | 单文件不超过 600 行 |
| T15 | H.264 OHOS AVCodec 拆分 | P2 | T13 | decoder/surface/fallback 分离 |
| T16 | OHAudio backend 拆分 | P2 | T06 | rdpsnd/audin 更易测 |
| T17 | SDK quickstart 和 public headers 安装 | P0 | T02/T03 | 第三方应用可照文档接入 |
| T18 | 商用 feature matrix 和 fallback 清单 | P0 | T06/T07/T12 | 上架前功能边界清楚 |

## T00：建立基线和验收脚本

修改点：

- 新增或整理本地验证命令文档，记录 FreeRDP 子模块 commit、构建命令、runtime sync 命令、HAP 构建命令。
- 在 `docs/freerdp-ohos-feature-matrix.md` 中补当前默认 feature、已知风险、需要真机验证项。
- 不改业务逻辑。

验收点：

- 能明确记录当前 submodule commit 和 runtime libs 来源。
- 能列出每次任务完成后至少要跑哪些检查：FreeRDP build、runtime sync、HAP build、真机连接、输入、剪贴板、音频、resize。
- `git status` 只出现文档变更。

验收风险：

- 本机缺 `ohpm/hvigor` 时无法完成 HAP build，只能把构建缺口写进验收结果。
- runtime libs 如果不是从当前 submodule commit 构建，后续问题可能被误判为代码问题。

## T01：去掉商用构建中的 mock/header fallback

修改点：

- 修改 `harmony/app/entry/src/main/cpp/CMakeLists.txt`，商用 profile 下 FreeRDP headers 和 `client/OHOS` headers 必须存在，否则 CMake 直接失败。
- 保留 demo/mock profile 时，把 `HARMONY_HAS_FREERDP_HEADERS` 缺失路径移动到明确的 demo build 开关，不参与商用构建。
- 清理 HAP C++ 中大量 `FreeRDP headers not found at build time` 的商用路径。

验收点：

- 缺少 `harmony/out/ohos-arm64/sysroot/include/freerdp3` 时，商用构建失败且错误信息明确。
- 正常 runtime sync 后，商用构建不再走 mock 或 unavailable 文案。
- `probe()` 不再把缺 FreeRDP 当成可运行状态。

验收风险：

- 当前开发环境如果还依赖 mock build，会影响本地调试，需要保留显式 demo profile。
- 过早删除 fallback 会暴露 CI/DevEco 环境路径问题，需要先固化构建路径。

## T02：新增 FreeRDP OHOS Session 公共 API

修改点：

- 新增 `harmony/third_party/FreeRDP/client/OHOS/ohos_session.h`。
- 新增 `harmony/third_party/FreeRDP/client/OHOS/ohos_session.c`。
- 在 `harmony/third_party/FreeRDP/client/common/CMakeLists.txt` 中编译该文件。
- 暴露最小 API：
  - `freerdp_ohos_session_new/free`
  - `freerdp_ohos_session_connect/disconnect`
  - `freerdp_ohos_session_send_pointer/send_key/send_text`
  - `freerdp_ohos_session_resize`
  - `freerdp_ohos_session_get_diagnostics`
- 定义 `FREERDP_OHOS_SESSION_OPTIONS` 和 `FREERDP_OHOS_SESSION_CALLBACKS`。

验收点：

- FreeRDP 能编出带 `freerdp_ohos_session_*` 符号的 `libfreerdp-client3.so`。
- HAP 还不改调用路径时，原功能不受影响。
- 新 API 头文件不包含 HAP 私有头，不依赖 N-API，不依赖 ArkTS。

验收风险：

- API 一旦给第三方使用，后续破坏性修改成本高。第一版要少暴露结构体内部细节。
- 如果把 NativeWindow/EGL 细节放进 API，会重新耦合 HAP，必须只用 callback 传 surface target。

## T03：HAP 改成调用 Session API

修改点：

- 修改 `harmony/app/entry/src/main/cpp/session/freerdp_session_runner.cpp`，让它成为 `freerdp_ohos_session_*` 的薄适配。
- 修改 `harmony/app/entry/src/main/cpp/freerdp/freerdp_runtime.h/.cpp`，动态加载新增 session API 符号。
- HAP 保留 N-API、日志、状态、surface callback，但不再直接控制 FreeRDP connect/event loop/channel wiring。

验收点：

- HAP 可以连接、断开、收状态、收日志。
- `freerdp_session_runner.cpp` 中直接操作 `freerdp_new/contextNew/connect/getEventHandles` 的代码明显减少。
- 第三方应用理论上可以跳过 HAP，直接复用同一套 FreeRDP session API。

验收风险：

- event loop 从 HAP 移到 FreeRDP 后，线程退出和取消语义容易回归。
- 旧 HAP 的 `running`、`connected` 状态和新 session 状态可能短期重复，需要明确唯一来源。

## T04：会话参数和存储路径下沉

修改点：

- 将 `SplitDomainUsername` 从 `freerdp_session_runner.cpp` 移到 `client/OHOS/ohos_session_config.c` 或新文件 `ohos_session_options.c`。
- 将 `ConfigureFreerdpStoragePaths` 下沉，HAP 只传 `appDataDir`。
- 在 FreeRDP OHOS 层校验 host、port、username、password、desktop size、certificatePolicy、graphicsMode。
- `ConnectParams` 保留为 HAP/N-API 入参，不再作为 FreeRDP 语义结构。

验收点：

- username 支持 `DOMAIN\\user`，解析结果由 FreeRDP OHOS 层产生日志。
- `appDataDir` 为空时连接失败，错误来自 FreeRDP OHOS 层。
- HAP 不直接设置 `FreeRDP_HomePath`、`FreeRDP_ConfigPath`。

验收风险：

- 证书 TOFU 存储路径改变会影响已保存证书，需要迁移或清晰提示。
- 用户名解析如果改变，域账号登录可能回归，要用域账号和本地账号分别验收。

## T05：证书策略彻底下沉

修改点：

- 扩展 `client/OHOS/ohos_certificate.h/.c`，覆盖当前 HAP 里 `HarmonyVerifyCertificateEx` 和 `HarmonyVerifyChangedCertificateEx` 的返回码语义。
- HAP 只提供证书确认/展示 callback，不决定 strict/tofu/ignore 的底层返回码。
- 商用默认策略从 `ignore` 改为 `tofu` 或 `strict`，`ignore` 只允许显式高级选项。

验收点：

- 首次证书、证书变更、strict 拒绝、ignore 接受都有明确日志。
- HAP 不再保存 per-instance certificate policy map，或只保存 UI 状态。
- 默认连接不会使用 `FreeRDP_IgnoreCertificate=TRUE`。

验收风险：

- TOFU 返回码和 FreeRDP 证书存储行为必须实测，否则可能出现每次都提示证书。
- 改默认策略会影响现有测试机连接，需要同步更新测试配置。

## T06：通道默认开关改成显式配置

修改点：

- 修改 `FREERDP_OHOS_SESSION_CONFIG`，新增或明确 `clipboard/audioPlayback/audioCapture/displayControl/graphicsPipeline` 均由调用方传入。
- 修改 `freerdp_ohos_session_config_default`，商用默认关闭 clipboard 和 audioCapture。
- 修改 HAP UI/参数传递，让用户启用剪贴板、麦克风时才打开对应通道。
- smartcard source/channel/PCSC 和 TSMF 从交付 runtime 构建中裁剪；drive/printer 保留编译但默认关闭，等权限、UI 和后端验收闭环后再对外开放。

验收点：

- 默认连接不申请剪贴板权限，不申请麦克风权限。
- 关闭 clipboard 时不会加载 cliprdr。
- 关闭 audioCapture 时 audin 不请求权限。
- feature matrix 标明每个通道默认状态。

验收风险：

- 服务端策略可能依赖某些通道，裁剪后需要确认不会影响基本桌面连接。
- 剪贴板从默认开改为显式开，原测试用例要更新。

## T07：graphicsMode 和 fallback 单一来源

修改点：

- 删除或收缩 `harmony/app/entry/src/main/cpp/freerdp/graphics_config.cpp` 的本地解析逻辑。
- 所有图形模式解析、fallback ladder、retry 判断统一调用 `client/OHOS/ohos_graphics.c`。
- 商用默认 `gdi`，`rdpgfx` 和 `rdpgfx-h264` 通过显式配置开启。
- fallback 只对图形失败触发，不对认证、网络、证书失败触发。

验收点：

- 日志显示 `graphics fallback ladder` 来自 FreeRDP OHOS 层。
- `rdpgfx-h264` 失败能按策略回到 `gdi`。
- 密码错误、证书拒绝、TCP 失败不会触发图形 fallback。

验收风险：

- 如果 fallback 判断过窄，部分真实图形失败不会重试。
- 如果判断过宽，会掩盖认证/网络错误，导致用户看到错误的失败原因。

## T08：输入队列和 worker dispatch 下沉

修改点：

- 新增 `client/OHOS/ohos_input_queue.h/.c`。
- 从 `rdp_session_input.cpp` 移入：
  - pointer coalescing
  - max queue/backpressure
  - key repeat collection
  - release-all
  - worker-thread dispatch
  - diagnostics counters
- HAP 只把 XComponent 事件转为 `FREERDP_OHOS_POINTER_EVENT`、`FREERDP_OHOS_KEY_EVENT`、UTF-16 text。

验收点：

- `rdp_session_input.cpp` 缩为薄适配或删除。
- 连接后 pointer/key/text 输入正常。
- 快速拖动鼠标时队列不会无限增长，diagnostics 能看到 coalesce/drop 计数。
- blur/disconnect 会释放按下的键。

验收风险：

- 输入线程和 FreeRDP event loop 同步不当会导致丢键或卡死。
- key repeat 从 HAP 移动后，要验证长按、Ctrl/Alt/Shift 组合键和释放顺序。

## T09：HAP XComponent 输入文件拆分

修改点：

- 拆 `harmony/app/entry/src/main/cpp/input/xcomponent_input_bridge.cpp`：
  - `xcomponent_touch_gesture.cpp`
  - `xcomponent_mouse.cpp`
  - `xcomponent_axis.cpp`
  - `xcomponent_key.cpp`
  - `xcomponent_input_registration.cpp`
- 手势识别留在 HAP，不进入 FreeRDP。

验收点：

- 每个新文件不超过 500-600 行。
- tap、right click、drag、wheel、axis、soft keyboard、focus/blur 行为保持一致。
- 拆分前后输入日志语义一致。

验收风险：

- 拆分时全局状态容易重复或初始化顺序改变。
- focus/blur 释放键如果漏掉，会造成远端持续按键。

## T10：display-control resize 管理下沉

修改点：

- 扩展 `client/OHOS/ohos_display.h/.c`，保存 caps ready、last sent size、alignment。
- HAP 的 `RdpSessionChannels::RequestDynamicDesktopResize` 只调用 FreeRDP OHOS session API。
- HAP 只提供当前 surface size，不判断 display-control channel 是否 ready。

验收点：

- XComponent 尺寸变化后能发送 monitor layout。
- `rdpgfx-h264` 模式下尺寸按 16 对齐。
- display-control caps 未就绪时不会报错刷屏，ready 后能补发。

验收风险：

- resize 过早发送可能被服务端忽略。
- 对齐策略改变可能导致鼠标坐标和显示区域不匹配。

## T11：RDPGFX bridge 拆分

修改点：

- 拆 `client/OHOS/ohos_rdpgfx.c`：
  - `ohos_rdpgfx_bridge.c`
  - `ohos_rdpgfx_caps.c`
  - `ohos_rdpgfx_surface.c`
  - `ohos_rdpgfx_avc420.c`
  - `ohos_rdpgfx_avc444_policy.c`
  - `ohos_rdpgfx_diagnostics.c`
- 公共头 `ohos_rdpgfx.h` 保持兼容。

验收点：

- 拆分后导出的符号不变。
- RDPGFX connect/disconnect、caps advertise/confirm、surface command 日志保持。
- 单个 rdpgfx 源文件不超过 600 行。

验收风险：

- 回调注册顺序改动会导致 RDPGFX 不工作。
- registry/context 查找如果拆错，会出现多 session 混淆。

## T12：AVC444 policy 下沉并默认关闭 GPU path

修改点：

- 将 `avc444_gpu_compositor_internal.cpp` 中的 LC、dirty rect、pending present、EndFrame match、GDI suppress、authoritative owner 逻辑移动到 `ohos_rdpgfx_avc444_policy.c`。
- HAP 的 GPU compositor 只实现：
  - decode stream
  - upload texture
  - compose
  - present
- 商用默认 `avc444GpuExperimental=false`。

验收点：

- FreeRDP OHOS 层是唯一决定是否 suppress GDI 的地方。
- HAP compositor 不再引用 frame policy 或决定 authoritative。
- 默认商用包不走 AVC444 GPU path。
- 显式打开实验路径时，失败能回到 FreeRDP native GDI。

验收风险：

- AVC444 policy 移动是高风险项，容易出现黑屏、旧帧、GDI/GPU 双写。
- EndFrame mismatch 处理如果错，会导致画面不刷新或持续抑制 GDI。

## T13：AVC420 surface fallback 收敛

修改点：

- 把 `rdpgfx_pipeline.cpp` 中 AVC420 surface active/configured/fallback 状态移动到 FreeRDP OHOS route 或 session API。
- HAP 只提供 `GetSurfaceTarget`、`PrepareNativeWindowForAvcDecoder`、`RestoreNativeWindowToRgba` callback。
- `freerdp_ohos_avcodec_set_fallback_callback` 的处理在 FreeRDP OHOS session 内完成。

验收点：

- AVCodec 初始化失败时能回到软件/GDI。
- surface 销毁、resize、重建后不会卡在 NV12 或无输出状态。
- fallback 日志说明失败原因和恢复路径。

验收风险：

- NativeWindow format 从 NV12/RGBA 切换不当会影响 GDI 渲染。
- fallback 后如果没有重新启动 render pipeline，会出现黑屏。

## T14：剪贴板按职责拆分

修改点：

- 将 `client/OHOS/ohos_clipboard.c` 拆成：
  - `ohos_clipboard_core.c`
  - `ohos_clipboard_pasteboard.c`
  - `ohos_clipboard_formats.c`
  - `ohos_clipboard_text.c`
  - `ohos_clipboard_image.c`
  - `ohos_clipboard_file.c`
  - `ohos_clipboard_cache.c`
  - `ohos_clipboard_worker.c`
  - `ohos_clipboard_cliprdr.c`
  - `ohos_clipboard_internal.h`
- 修改 `client/common/CMakeLists.txt` 加入新文件。
- 保持 `ohos_clipboard.h` 公共 API 不变。

验收点：

- 每个 clipboard 源文件不超过 600 行。
- 文本双向复制正常。
- HTML 双向复制正常。
- URI/local image 复制正常。
- 远端文件列表和文件内容请求正常。
- 剪贴板权限缺失时不会导致 cliprdr 崩溃。

验收风险：

- worker/cache/registry 拆分后生命周期最容易出错，重点验收 disconnect 和 app 退出。
- 图片和文件路径涉及 UDMF/Pasteboard 权限，不同设备 API 行为可能不一致。

## T15：H.264 OHOS AVCodec 拆分

修改点：

- 拆 `libfreerdp/codec/h264_ohos_avcodec.c`：
  - `h264_ohos_surface_state.c`
  - `h264_ohos_decoder.c`
  - `h264_ohos_callbacks.c`
  - `h264_ohos_diagnostics.c`
- 保持当前 FreeRDP codec 入口函数不变。

验收点：

- `WITH_OHOS_AVCODEC=ON` 能编译。
- AVC420 direct surface 正常。
- AVCodec 不可用时 fallback 到软件/GDI。
- diagnostics 仍能输出 decoder attempts、surface active、fallback reason。

验收风险：

- codec 文件在 `libfreerdp` 下，符号可见性和 CMake 链接要谨慎。
- callback 生命周期处理不当会出现异步回调访问已释放对象。

## T16：OHAudio backend 拆分

修改点：

- 拆 `channels/rdpsnd/client/ohos/rdpsnd_ohos.c`：
  - format negotiation
  - queue/ring buffer
  - renderer lifecycle
  - diagnostics
- 拆或整理 `channels/audin/client/ohos/audin_ohos.c`：
  - permission callback
  - format
  - capturer lifecycle
  - diagnostics

验收点：

- rdpsnd 播放正常，underrun 计数可见。
- audin 只在用户授权后启动采集。
- 拒绝麦克风权限时连接不崩溃，audin 明确失败。

验收风险：

- 音频是实时路径，拆 queue 容易引入卡顿或 underrun。
- 权限 callback 阻塞时间过长会影响 audin open。

## T17：SDK quickstart 和 public headers 安装

修改点：

- 新增 `docs/freerdp-ohos-sdk-quickstart.md`。
- 更新 `harmony/third_party/FreeRDP/client/OHOS/README.md`，标出 public/internal API。
- 修改 CMake install，把 public OHOS headers 安装到 `include/freerdp/client/ohos/`。
- 提供 20-30 行 C/C++ 示例，展示 create session、set callbacks、connect、send input、resize、disconnect。

验收点：

- 第三方应用只看 quickstart 就能知道需要链接哪些 `.so` 和 include 哪些头。
- public headers 不暴露 HAP 私有类型。
- quickstart 包含权限回调和 surface callback 的最小示例。

验收风险：

- 文档如果仍依赖 Demo HAP 类名，就没有达到 SDK 化目标。
- headers 安装路径一旦确定，后续迁移成本高。

## T18：商用 feature matrix 和 fallback 清单

修改点：

- 更新 `docs/freerdp-ohos-feature-matrix.md`，增加：
  - 默认启用
  - 可选启用
  - 实验
  - 首版不交付
  - 需要权限
  - 真机验证状态
- 新增 fallback 清单，列出每个 fallback 是否可达、触发条件、恢复路径、是否保留。
- 对 drive/printer 做首版默认关闭和接入策略确认；smartcard source/channel/PCSC 和 TSMF 已按首版不交付从交付构建裁剪。

验收点：

- 每个功能都有明确商用状态。
- 不可达 fallback 被删除或标记为待删。
- 首版 HAP 不声明未实际启用的权限。

验收风险：

- 裁剪通道可能影响少数企业 RDP 环境，需要确认首版范围。
- feature matrix 如果没有真机记录，后续上架风险仍然不可控。

## 每个任务的通用验收模板

执行每个任务后，必须记录：

1. 修改了哪些文件。
2. 哪些逻辑从 HAP 下沉到了 FreeRDP。
3. 哪些逻辑明确保留在 HAP。
4. 跑了哪些验证命令。
5. 真机是否验证。
6. 是否改变默认功能开关。
7. 是否改变 public API。
8. 是否新增或删除 fallback。
9. 剩余风险。

## 完成标准

- HAP 不直接持有 FreeRDP settings、channels、RDPGFX policy、input queue 的核心逻辑。
- 第三方应用只依赖 FreeRDP OHOS public headers 和 runtime libs 即可接入。
- 新增或拆分后的适配文件单文件不超过 600 行。
- 商用默认不打开实验图形路径、剪贴板、麦克风。
- 所有 fallback 都有明确触发条件和恢复路径；不可达或无价值的 fallback 删除。
- 文档记录每个 feature 的默认状态、权限、包体影响和真机验证结论。
