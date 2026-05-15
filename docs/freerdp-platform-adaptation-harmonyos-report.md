# FreeRDP Windows/Linux 适配点与 HarmonyOS 移植报告

日期：2026-05-14；2026-05-15 补充窗口、输入、音视频和硬件编解码专项

## 结论摘要

FreeRDP 的跨平台能力主要分两层：

1. **WinPR 系统抽象层**：把 Windows API 语义适配到 POSIX/Linux/Android/macOS 等平台，包括线程、事件、等待、socket、文件、路径、动态库、日志、时区、系统信息、加密、SSPI/NTLM 等。这一层在 HarmonyOS 上必须继续使用，但要按 OHOS NDK/musl 的实际能力做少量兼容补丁和验证。
2. **客户端前端层**：`client/Windows`、`client/X11`、`client/Wayland`、`client/SDL`、`client/Android` 分别负责窗口、渲染、输入、剪贴板、音频等平台 UI/设备能力。这一层不能直接搬到 HarmonyOS。HarmonyOS 应该保留 FreeRDP core + WinPR，把窗口和输入重写为 ArkUI `XComponent` + C++ N-API + `NativeWindow`。

当前仓库已经走的是正确方向：`harmony/scripts/wsl/build-freerdp-ohos.sh` 交叉编译 FreeRDP/WinPR/OpenSSL/zlib/cJSON，关闭桌面客户端、X11/Wayland、音频、设备重定向和大部分通道；`libentry.so` 通过 N-API 暴露 `probe/connect/disconnect/onState/onLog/onError`，并在运行期 `dlopen` FreeRDP runtime。下一步的关键不是移植 `wfreerdp` 或 `xfreerdp`，而是补 HarmonyOS 专用的渲染、输入、证书和生命周期层。

## 本次分析范围

本报告基于本地仓库：

- FreeRDP 源码：`harmony/third_party/FreeRDP`
- FreeRDP commit：`bc58aed6635c`（`ohos-port`，基于 upstream `1ab2572f17f5056f967727707d75f88e2e12270b`）
- FreeRDP OHOS 分支：`harmony/third_party/FreeRDP` 子模块内 `ohos-port`
- Harmony 构建脚本：`harmony/scripts/wsl/build-freerdp-ohos.sh`
- Harmony App native bridge：`harmony/app/entry/src/main/cpp/napi_init.cpp`
- Harmony App 页面：`harmony/app/entry/src/main/ets/pages/Index.ets`
- 本地 arm64 runtime 输出：`harmony/out/ohos-arm64/manifest.txt`、`harmony/out/ohos-arm64/elf-report.txt`

本报告只新增本文档，不修改 FreeRDP 或 Harmony App 代码文件。

## FreeRDP 已有 Windows/Linux 适配点

### 1. 构建系统按平台开关功能

FreeRDP 的 CMake 顶层通过 `WIN32`、`UNIX`、`ANDROID`、`APPLE`、`IOS` 等变量决定默认功能：

- `CMakeLists.txt` 中 Windows 会设置 `UNICODE/_UNICODE`、`WIN32_LEAN_AND_MEAN`、目标 Windows 版本和资源版本信息。
- 非 Windows/Android 平台默认启用 `WITH_X11`，桌面 Linux 会尝试构建 X11 客户端。
- `cmake/ConfigOptions.cmake` 默认启用 `WITH_CLIENT_COMMON`、`WITH_CLIENT`、`WITH_CLIENT_SDL`、`WITH_SERVER`、`WITH_CHANNELS`、`WITH_CLIENT_CHANNELS`，并按平台查找 ALSA/PulseAudio/OSS 等音频后端。
- `cmake/ConfigOptionsAndroid.cmake` 单独提供 Android 的 `WITH_OPENSLES`、`WITH_MEDIACODEC`。

对 HarmonyOS 的影响：不能沿用默认配置。HarmonyOS App 不需要任何桌面 client binary，也不需要 server binary。当前脚本仍关闭 `WITH_CLIENT`、`WITH_SERVER`、`WITH_X11`、`WITH_WAYLAND`、`WITH_ALSA`、`WITH_PULSE` 等桌面前端/桌面音频后端，但已经切到增强 profile：编译 FreeRDP client channels、RDPGFX、FFmpeg、OpenH264、uriparser，并在 OHOS NDK 提供 OpenSLES 时编译音频通道后端。

### 2. WinPR：Windows API 到 POSIX 的系统抽象

WinPR 是 FreeRDP 移植的基础。它提供类似 Windows API 的接口，并在非 Windows 上用 POSIX 实现：

- 线程：`winpr/libwinpr/thread/thread.c` 使用 `pthread` 实现 `CreateThread`、`ResumeThread`、`Wait` 相关行为；Linux 下用 `syscall(SYS_gettid)` 获取线程 ID；非 Linux 走 `pthread_self()` fallback。
- 等待和事件：`winpr/libwinpr/synch/wait.c`、`event.c` 用 `pthread_mutex_timedlock`、`eventfd` 或 `pipe` 模拟 `WaitForSingleObject/WaitForMultipleObjects` 和 Windows Event。
- socket：`winpr/libwinpr/winsock/winsock.c` 和 `libfreerdp/core/tcp.c` 把 Windows Winsock 语义映射到 POSIX socket、`fcntl`、`poll/select`、`setsockopt`。
- 文件和路径：`winpr/libwinpr/file/file.c`、`path/shell.c` 在 Windows 上用宽字符/Win32 API，在非 Windows 上用 `fopen`、`mkdir`、`rename`、`unlink`、XDG/HOME 路径。
- 动态库：`winpr/libwinpr/library/library.c` 在 Windows 上走 `LoadLibraryW`，非 Windows 走 `dlopen/dlsym`，Linux 下通过 `/proc/self/exe` 获取模块路径。
- 日志：`winpr/libwinpr/utils/wlog` 在 Windows 可输出到 debugger，在 Android 可走 `__android_log_print`，其他平台默认 stdout/stderr/file/syslog 等。
- 时区/系统信息：`timezone.c`、`sysinfo.c` 在 Linux/Android/macOS/Windows 上分别用系统 API 或文件推断 CPU、时区、tick count 等。
- 加密/认证：WinPR crypto/SSPI/NTLM 封装 OpenSSL/mbedTLS/internal MD4/MD5/RC4。FreeRDP 的 NLA/NTLM/RDP licensing 会依赖 MD4、RC4、HMAC、TLS 等能力。

对 HarmonyOS 的影响：HarmonyOS native 层更接近 POSIX/musl，WinPR 的非 Windows 路径可以作为主体复用。但要重点验证 `pthread_cancel`、`eventfd`、`poll`、`dladdr/dlopen`、`/proc`、时区文件、OpenSSL provider 路径和 app 沙箱路径。

### 3. Windows 客户端前端

`client/Windows` 构建 `wfreerdp-client`：

- 使用 Win32 窗口、GDI、Windows 事件循环、资源文件、浮动工具栏。
- 可选 Windows 证书存储校验：`WITH_WINDOWS_CERT_STORE`。
- 链接 `msimg32.lib`、`credui.lib`，MinGW 下还需要 `ntdll.lib`。
- 通过 Windows 控制台或 GUI 子系统处理参数、凭据、证书弹窗。

对 HarmonyOS 的影响：不需要，也不能移植。HarmonyOS 没有 Win32 窗口、GDI、Windows 证书存储、`wfreerdp.exe` 进程模式。Windows 客户端的价值只在于理解 FreeRDP 的 callback 和渲染流程，不作为移植代码来源。

### 4. Linux/X11、Wayland、SDL 客户端前端

`client/X11` 构建 `xfreerdp`：

- 依赖 X11，并可选 XShm、Xinerama、Xext、Xcursor、Xv、Xi、Xrender、XRandR、Xfixes。
- 自己处理 X11 window、keyboard layout、clipboard、multi-monitor、resize、pointer。

`client/Wayland` 构建 `wlfreerdp`：

- 依赖 Wayland/UWAC。
- 自己处理 Wayland pointer、input、clipboard、display。

`client/SDL` 构建 `sdl-freerdp`：

- 依赖 SDL2/SDL3。
- 适合桌面跨平台客户端，但仍然是独立窗口/事件循环模型。

对 HarmonyOS 的影响：X11/Wayland/SDL 客户端都不应作为 HAP 内的最终 UI 前端。HarmonyOS 应通过 ArkUI 页面承载会话，用 `XComponent` 暴露 native surface，C++ 侧写 `NativeWindow` buffer。

### 5. Android 客户端前端是最有参考价值的移动端实现

`client/Android/Studio/freeRDPCore/src/main/cpp` 通过 JNI 做移动端 bridge：

- `android_freerdp.c` 注册 FreeRDP callbacks：`PreConnect`、`PostConnect`、`PostDisconnect`、`AuthenticateEx`、`VerifyCertificateEx`、`VerifyChangedCertificateEx`。
- `PostConnect` 中调用 `gdi_init(instance, PIXEL_FORMAT_RGBX32)`，注册 `BeginPaint/EndPaint/DesktopResize`，再通知 Java 层更新画面。
- `android_event.c` 建立 event queue，把 Java 层键盘、Unicode 键、鼠标、剪贴板事件排队，RDP worker loop 中取出后调用 `freerdp_input_send_keyboard_event`、`freerdp_input_send_unicode_keyboard_event`、`freerdp_input_send_mouse_event`。
- Android 渲染使用 `AndroidBitmap_lockPixels` 拿 Java bitmap 像素，再用 `freerdp_image_copy` 从 FreeRDP GDI buffer 拷贝。

对 HarmonyOS 的影响：Android 代码不能直接用，因为 JNI、AndroidBitmap、Android log、OpenSLES、MediaCodec 都不是 HarmonyOS App 的主接口。但它的架构值得复用：native worker loop、输入事件队列、GDI buffer、证书/凭据 callback、连接状态回调。

### 6. 通道和设备重定向

FreeRDP 的 `channels/` 默认包含大量虚拟通道：

- 基础体验：`cliprdr` 剪贴板、`disp` 动态分辨率、`rdpgfx` 图形管线、`rdpei` 触摸输入。
- 音频视频：`rdpsnd`、`audin`、`tsmf`、`video`，会关联 ALSA/Pulse/OSS/OpenSLES/FFmpeg/OpenH264/MediaCodec 等平台依赖。
- 设备重定向：`drive`、`printer`、`smartcard`、`serial`、`parallel`、`urbdrc`、`rdpdr`。
- 其他：`rail`、`remdesk`、`sshagent`、`location`、`rdp2tcp` 等。

对 HarmonyOS 的影响：第一版先用最小通道闭环是合理的；当前增强构建已把 cliprdr、disp、rdpgfx、rdpdr/drive、rdpsnd/audin、printer、smartcard 和 TSMF 编译进 FreeRDP，但产品可用性仍取决于 HarmonyOS 侧的剪贴板、音频、文件选择/沙箱、打印和智能卡后端。

## 当前 HarmonyOS 适配现状

### 已完成或已具备基础

1. `harmony/scripts/wsl/build-freerdp-ohos.sh` 使用 Linux 版 OHOS NDK toolchain 交叉编译：
   - OpenSSL 3.3.2
   - zlib 1.3.1
   - cJSON 1.7.18
   - FreeRDP/WinPR arm64-v8a
2. FreeRDP 源码适配由子模块分支管理：
   - 复制 FreeRDP 源码到 build workdir。
   - OHOS 改动直接提交在 FreeRDP 子模块的 `ohos-port` 分支上，主仓库通过 submodule 指针固定对应 commit。
   - 当前子模块改动对 `winpr/libwinpr/thread/thread.c` 增加 `__OHOS__` 分支，避免 OHOS 上走 `pthread_cancel`。
3. FreeRDP CMake 配置已经最小化：
   - 保留：`WITH_OPENSSL=ON`、`WITH_CLIENT_COMMON=ON`、`WITH_UNICODE_BUILTIN=ON`。
   - 当前增强构建仍关闭桌面前端/服务端：`WITH_CLIENT`、`WITH_SERVER`、`WITH_X11`、`WITH_WAYLAND`、`WITH_ALSA`、`WITH_PULSE` 等；已打开 `WITH_CHANNELS`、`WITH_CLIENT_CHANNELS`、`WITH_FFMPEG`、`WITH_SWSCALE`、`WITH_OPENH264`、`WITH_URIPARSER`、`WITH_SMARTCARD_PCSC` 和 OHOS NDK 可用时的 `WITH_OPENSLES`。
   - `WITH_CUPS`、`WITH_FUSE` 仍默认不启用，因为当前 OHOS sysroot 没有 CUPS/fuse3；`WITH_PCSC` 的外部库发现仍关闭，但 WinPR smartcard PCSC backend 已编译，运行时动态加载 `libpcsclite`。
4. Runtime 打包链路已存在：
   - `harmony/out/ohos-arm64/runtime-libs/` 产出 `libfreerdp3.so`、`libfreerdp-client3.so`、`libwinpr3.so`、`libssl.so.3`、`libcrypto.so.3`、`libz.so.1`、`libcjson.so.1`，以及 uriparser、OpenH264、FFmpeg、OpenSLES、`libc++_shared.so` 等增强依赖。
   - `harmony/scripts/windows/sync-freerdp-runtime.ps1` 同步到 `harmony/app/entry/libs/arm64-v8a/`。
   - `elf-report.txt` 显示 arm64 产物为 ELF64 AArch64，依赖关系基本闭合。
5. Harmony App 已具备 native bridge：
   - `module.json5` 仅声明 `ohos.permission.INTERNET`。
   - `Index.ets` 调用 `libentry.so` 的 `probe/connect/disconnect/resize/paintTestPattern/sendPointer/sendKey/sendUnicode/onState/onLog/onError`。
   - `Index.ets` 已使用 `XComponent(id: 'rdpSurface', type: SURFACE, libraryname: 'entry')` 承载远端桌面区域，并在 ArkTS 侧处理 touch、mouse、axis、key 事件。
   - `napi_init.cpp` 已经做 TCP reachability check、运行期 `dlopen`、FreeRDP settings 映射、worker thread、FreeRDP event loop、线程安全回调。
   - `napi_init.cpp` 已注册 `OH_NativeXComponent_Callback`，维护 surface created/changed/destroyed 计数，保存 `NativeWindow`，并提供 `PaintTestPattern` 与 FreeRDP GDI frame 到 `NativeWindow` 的 CPU copy 渲染路径。
   - `napi_init.cpp` 已注册 FreeRDP `PostConnect/PostDisconnect/BeginPaint/EndPaint/DesktopResize`，`PostConnect` 中调用 `gdi_init(..., PIXEL_FORMAT_RGBA32)`，`EndPaint` 中把 `gdi->primary_buffer` 渲染到 XComponent surface。
   - native 侧已经有输入事件队列，ArkTS 侧可通过 `sendPointer/sendKey/sendUnicode` 入队，RDP worker loop 消费后发送 FreeRDP input PDU。
   - `napi_init.cpp` 会设置 `OPENSSL_MODULES` 到打包的 `ossl-modules`，这是 OpenSSL 3 provider 场景下 NLA/NTLM 的关键点。

### 仍未完成的关键闭环

1. **窗口/渲染已有骨架，但需要实机压测和性能优化**：当前已能注册 XComponent surface、写 `NativeWindow` buffer、按比例 letterbox/pillarbox 缩放 FreeRDP GDI frame；仍需验证真实设备上的 buffer format、stride、fence、后台/前台、窗口尺寸变化和高分辨率帧率。
2. **输入已有骨架，但需要补齐映射和焦点策略**：ArkTS 已有 touch/mouse/axis/key 入口，native 已有输入队列；仍需专项验证中文输入、组合键、软键盘、右键/滚轮、窗口失焦时释放按键、resize 后坐标映射。
3. **动态远端分辨率未完成**：当前 `resize()` 会明确返回 display-control channel disabled。窗口变化只能本地缩放，不能通知远端桌面真实变更。后续要打开 `disp` channel 或接受固定远端分辨率。
4. **证书策略已改成 TOFU/Strict/Ignore，但还要做产品化存储与提示**：TOFU 已通过 FreeRDP 证书回调接入，后续需要做证书详情展示、替换确认和严格模式默认策略。
5. **FreeRDP 源码层 OHOS 后端仍未完整**：当前只有 WinPR 线程兼容分支改动；后续剪贴板、音频、硬件解码、日志、文件/打印/智能卡等能力需要继续在 FreeRDP 或 bridge 层增加 OHOS 专用后端。
6. **增强通道已编译，但还没有全部业务闭环**：剪贴板文本、动态分辨率、音频、文件、打印、智能卡、RD Gateway 都需要继续接 HarmonyOS UI、权限、系统 API 或专用后端。
7. **生命周期需要压测**：断开、页面销毁、surface 销毁、App 后台、重复连接、网络抖动都需要验证 worker、FreeRDP context、dlopen runtime、NativeWindow 引用和输入队列的释放策略。

## HarmonyOS 是否需要这些适配，以及怎么适配

| FreeRDP 适配点 | Windows/Linux/Android 当前做法 | HarmonyOS 是否需要 | 建议适配方式 |
| --- | --- | --- | --- |
| WinPR 线程/事件/等待 | Windows 用原生 HANDLE；Linux/Android 用 pthread、eventfd/pipe、poll | 需要 | 复用 POSIX 路径；通过 FreeRDP 子模块 `ohos-port` 分支保留 `pthread_cancel` 禁用改动；优先用 `abortConnectContext` 和自有 `running` 标志停止线程 |
| WinPR socket/TCP | Windows Winsock；Linux POSIX socket | 需要 | 复用 POSIX socket；保留 App `INTERNET` 权限；继续用 TCP probe 区分网络失败和 RDP 失败 |
| WinPR 文件/路径/HOME | Windows AppData；Linux XDG/HOME；Android app files dir | 需要，但要收口 | App 启动时设置 HOME/XDG 到应用沙箱目录；证书、known_hosts、日志、缓存都放应用私有目录 |
| 动态库加载 | Windows `LoadLibrary`；Linux/Android `dlopen` | 需要 | 当前 `dlopen` runtime 可继续；注意库加载顺序、`RTLD_GLOBAL`、OpenSSL provider 路径；runtime 不要中途 `dlclose` |
| Windows client `wfreerdp` | Win32/GDI/Credui/证书存储 | 不需要 | 不移植；Windows demo 仅保留为目标环境诊断工具 |
| X11/Wayland client | X11/Wayland 窗口、输入、剪贴板 | 不需要 | 继续关闭 `WITH_X11/WITH_WAYLAND`；Harmony UI 用 ArkUI |
| SDL client | SDL 窗口和事件循环 | 首版不需要 | HAP 内不建议引 SDL；除非后续要做独立跨平台 native UI，当前路线不需要 |
| Android JNI client | JNI、AndroidBitmap、Android event queue | 需要参考，不直接复用 | 重写为 N-API；保留“native worker + event queue + GDI buffer + UI 回调”的架构思想 |
| 渲染 | Windows GDI/X11 image/AndroidBitmap | 需要重写 | `XComponent` 获取 surface，C++ 注册 `OH_NativeXComponent_Callback`，用 `NativeWindow` request/flush buffer，FreeRDP frame 转 `RGBA8888` |
| 输入 | Windows/X11/Android 各自把本地键鼠转 FreeRDP input | 需要重写 | ArkUI touch/key -> N-API -> native event queue -> `freerdp_input_send_mouse_event/keyboard_event/unicode_keyboard_event` |
| 剪贴板 | `cliprdr` + 平台 clipboard | 首版可不需要，后续需要 | `CHANNEL_CLIPRDR_CLIENT` 已编译；下一步接 Harmony clipboard API，先做文本，再评估文件 |
| 音频 | ALSA/Pulse/OSS/OpenSLES | 首个可交互版本可不做，产品化通常需要 | 继续关闭到 M6；后续打开 `rdpsnd/audin`，用 Harmony AudioRenderer/AudioCapturer 写 OHOS backend，不能直接用 ALSA/Pulse/OpenSLES |
| 图形/H.264/视频 | Bitmap/RFX/NSCodec/RDPGFX；H.264 可走 OpenH264/FFmpeg/MediaFoundation/MediaCodec | 基础画面必须做；RDPGFX/H.264 是性能增强；独立视频通道按需 | 首版先用基础 GDI/bitmap 到 `NativeWindow`；性能不足再打开 `rdpgfx` 和软件 H.264；硬件解码需新增 OHOS AVCodec backend，不要直接套 Android MediaCodec |
| 文件/磁盘重定向 | `drive`、FUSE、POSIX 文件系统 | 首版不需要 | `drive`/`rdpdr` 已编译；后续需配合 Harmony 文件选择器和沙箱权限，不能直接暴露任意路径。FUSE 文件复制仍是可选专项 |
| 打印/智能卡/USB | CUPS/PCSC/系统设备 API | 首版不需要 | printer/smartcard channel 已编译；WinPR smartcard PCSC backend 已编译但需要运行时 `libpcsclite` 或 OHOS 专用后端；真实打印仍需 CUPS 端口或 Harmony Print 后端 |
| 证书校验 | Windows 可用系统证书；Linux/Android 用 OpenSSL/known_hosts/回调 | 需要 | 实现 `VerifyCertificateEx/VerifyChangedCertificateEx` 回调到 ArkTS；做 TOFU 指纹存储；生产默认不使用 ignore |
| 日志 | Console/file/syslog/Android log | 需要 | N-API 回调继续保留；建议补 `hilog` 输出，App UI 只显示脱敏摘要 |
| 生命周期 | 各 client 自己管理事件循环 | 需要 | 页面销毁/后台/断网时统一进入 `Disconnecting`，调用 `freerdp_abort_connect_context`，等待 worker join，释放 surface |

## 建议的 HarmonyOS 具体适配方案

### M4.4：稳定连接和认证闭环

目标：不渲染，但真实 FreeRDP 会话连接、断开、失败分类稳定。

建议：

- 后续 FreeRDP 源码改动都继续提交到 FreeRDP 子模块的 OHOS 适配分支，主仓库只更新 submodule 指针，不再用构建脚本维护 patch 队列。
- 为 FreeRDP instance 注册必要 callbacks：
  - `AuthenticateEx`：后续可避免把密码长期留在 settings 中。
  - `VerifyCertificateEx` / `VerifyChangedCertificateEx`：用于 TOFU 和严格证书策略。
  - `PostConnect` / `PostDisconnect`：为 M5 渲染预留。
- 明确 `certPolicy`：
  - `strict`：证书必须可信或指纹已匹配。
  - `tofu`：首次保存指纹，后续变更必须提示/拒绝。
  - `ignore`：仅调试开关，不作为默认值。
- 连接失败要映射到用户可读错误：
  - TCP 不通
  - TLS/security negotiation 失败
  - NLA/NTLM 认证失败
  - 证书失败
  - 会话被服务端断开
- 保留 `OPENSSL_MODULES` 设置和 `legacy.so` 打包验证。OpenSSL 3 下 NTLM/旧算法能力容易在 provider 路径错误时失效。

验收：

- 正确账号能进入 `Connected` 并保持事件循环。
- 错误密码返回认证类错误。
- 错误 IP/端口返回 TCP 类错误。
- 断开后 worker 可 join，重复连接不崩溃。

### M5：接入 XComponent/NativeWindow 渲染

目标：App 内看到远端 Windows 桌面画面。

建议实现：

1. ArkTS 侧 `XComponent` 增加明确 `libraryname` 或按目标 SDK 规范让 native 模块拿到 `OH_NATIVE_XCOMPONENT_OBJ`。
2. C++ 侧从 N-API exports 中解析 `OH_NativeXComponent`，注册：
   - `OnSurfaceCreated`
   - `OnSurfaceChanged`
   - `OnSurfaceDestroyed`
   - `DispatchTouchEvent` 或 ArkTS 自行转发 touch event
3. `OnSurfaceCreated` 中保存 `NativeWindow*`，设置：
   - buffer geometry = XComponent 宽高
   - pixel format = `PIXEL_FMT_RGBA_8888`
   - CPU write usage
4. FreeRDP `PostConnect` 中：
   - 调用 `gdi_init(instance, PIXEL_FORMAT_RGBA32 或 RGBX32)`。
   - 注册 `BeginPaint/EndPaint/DesktopResize`。
5. `EndPaint` 中从 FreeRDP GDI invalid region 取 dirty rect：
   - 首版可以合并 dirty rect，做全帧或局部 CPU copy。
   - 写入 `NativeWindowBuffer` 后 flush。
6. `OnSurfaceDestroyed` 时 native renderer 断开 window 引用，避免后台后继续写无效 buffer。

注意：

- OpenHarmony 文档显示 `NativeWindow` 适合 C++ 绘制并显示到屏幕，核心流程是从 surface 创建 `NativeWindow`、request buffer、写入内容、flush buffer。
- `XComponent`/`OH_NativeXComponent_Callback` 正是用于注册 surface 生命周期和触摸回调。
- 当前页面上 `Text('Remote desktop surface')` 会覆盖画面，M5 需要按状态隐藏占位文案。

验收：

- 连接成功后不是黑屏/白屏/花屏。
- 断开后停止刷新并释放 window 引用。
- 横竖屏或窗口尺寸变化不崩溃。
- 颜色通道正确，鼠标光标/桌面背景能分辨。

### M5.5：窗口对接方案

这里的“窗口”建议拆成两类处理：

1. **本地 Harmony App 窗口**：HAP 的 `WindowStage`、页面布局、`XComponent`、`NativeWindow`、焦点、尺寸变化和生命周期。这是首版必须完成的窗口对接。
2. **远端 Windows 独立窗口/RemoteApp/RAIL**：把远端单个应用窗口映射成本地多窗口。这需要 RAIL channel、窗口管理、z-order、最小化/最大化、焦点同步、图标和任务栏语义，不建议首版做。

当前仓库更适合走“一个 Harmony App 窗口承载一个完整远端桌面”的路线。

#### 当前代码基础

- `module.json5` 当前配置：
  - `supportWindowMode`: `fullscreen`、`floating`
  - `minWindowWidth`: `1280`
  - `minWindowHeight`: `760`
- `EntryAbility.ets` 在 `onWindowStageCreate` 中 `loadContent('pages/Index')`，但还没有订阅 `windowStageEvent`，也没有在 `onForeground/onBackground/onWindowStageDestroy` 中通知 native 暂停或断开。
- `Index.ets` 使用 `Stack + XComponent` 作为远端桌面区域，`XComponent` 已指定 `libraryname: 'entry'`，并通过 `onAreaChange/onLoad/onDestroy` 触发 probe。
- `napi_init.cpp` 已经注册 `OH_NativeXComponent_Callback`，保存 surface/window 指针，并通过 `NativeWindow` request/map/flush buffer 绘制测试图和 FreeRDP GDI frame。
- 当前 `SurfaceBridge::RenderRgbaFrameLocked` 会按比例把远端桌面缩放到本地 surface，中间区域渲染，周边填黑；这适合 fixed remote resolution + local scale 的首版模型。

#### 推荐窗口模型

首版采用单窗口、单 surface、固定远端桌面分辨率：

```text
WindowStage
  -> Index.ets root layout
    -> Stack/session area
      -> XComponent(id='rdpSurface', type=SURFACE, libraryname='entry')
        -> OH_NativeXComponent surface callbacks
          -> NativeWindow buffer
            -> FreeRDP GDI frame copy
```

这个模型有几个优点：

- 不需要 X11/Wayland/SDL，也不需要本地多窗口管理。
- 不需要第一阶段打开 `disp` 动态分辨率 channel。
- App 窗口尺寸变化时只改本地 viewport，输入坐标按 viewport 映射回远端桌面。
- surface 销毁时 native 只需要停止写 window，不一定马上断开 RDP 会话；是否断开由产品策略决定。

#### 尺寸与缩放策略

建议把尺寸分三层保存，不要混在一起：

- `appWindowSize`：Harmony WindowStage/页面可用区域尺寸。
- `surfaceSize`：XComponent 实际像素尺寸，由 `OH_NativeXComponent_GetXComponentSize` 得到。
- `remoteDesktopSize`：FreeRDP 协商的远端桌面尺寸，即 `DesktopWidth/DesktopHeight` 或 `DesktopResize` 回调后的尺寸。

首版使用本地缩放：

- `remoteDesktopSize` 固定为连接参数里的 `resolution`。
- `surfaceSize` 改变时重新计算 viewport，不重连、不通知服务端。
- 输入事件先判断是否落在 viewport 内，再按比例映射到 `remoteDesktopSize`。
- viewport 外的 letterbox/pillarbox 区域不发送点击。

后续如果要真实改变远端分辨率，再打开 `disp` dynamic channel：

- CMake 打开 `WITH_CHANNELS=ON`、`WITH_CLIENT_CHANNELS=ON`、`CHANNEL_DISP_CLIENT=ON`。
- 在 XComponent size/window size 变化后，向服务端发送 display-control monitor layout。
- 远端回调 `DesktopResize` 后再更新 `remoteDesktopSize` 和 GDI buffer。
- 需要 debounce，避免拖动窗口大小时连续发送大量 resize。

#### WindowStage 生命周期建议

建议在 `EntryAbility.ets` 里补 `windowStage.on('windowStageEvent', ...)`，把窗口状态转给页面或 native：

- `ACTIVE`：允许输入，必要时恢复刷新。
- `INACTIVE`：释放 Ctrl/Alt/Win 等本地保持的 modifier，避免远端卡键。
- `HIDDEN` 或 `onBackground`：暂停主动刷新/丢弃输入。产品策略可以选择保持连接或自动断开；首版建议后台超过超时再断开。
- `onWindowStageDestroy` / `onDestroy`：调用 native `disconnect()`，等待 worker join，并清空 surface/window 引用。

当前 native 渲染是在 RDP worker 的 `EndPaint` 路径调用 `RenderSurfaceRgbaFrame`，`SurfaceBridge` 用 mutex 保护 window 指针。这个模型可用，但要注意：

- 不要在持有 surface mutex 时做过长阻塞；高分辨率全帧 copy 会影响 surface destroy 等待。
- surface 销毁后渲染要快速返回，不继续 request buffer。
- 连续窗口 resize 时应丢弃旧帧，只保留最后一帧。
- `NativeWindow` buffer format/stride 不稳定时要记录日志并 fallback。

#### `module.json5` 窗口模式建议

当前只支持 `fullscreen` 和 `floating`。建议按目标设备调整：

- 如果目标主要是 2in1/平板，`floating` + `fullscreen` 合理。
- 如果要支持分屏，多窗口办公场景，应评估加入 `split`，同时降低 `minWindowWidth/minWindowHeight`，否则分屏下可能无法进入合理布局。
- `minWindowWidth=1280`、`minWindowHeight=760` 对小屏或分屏偏高。建议把 UI 做成响应式后，再按真实设备调整到更低门槛。
- RDP surface 区域必须始终有稳定尺寸约束，避免 toolbar、诊断面板、软键盘出现时挤压到 0 高度。

#### RemoteApp/RAIL 边界

不要把 Windows/X11 的多窗口 client 逻辑直接搬到 Harmony：

- 普通远程桌面模式只需要一个本地 surface。
- RAIL/RemoteApp 需要 `rail` channel、远端窗口创建/销毁/移动/最小化/最大化、菜单、图标、z-order、焦点和本地窗口映射。
- Harmony HAP 的窗口能力、任务管理和权限模型与桌面 Windows/X11 不同，RAIL 应作为单独产品方向评估。

首版建议明确不支持 RemoteApp，只支持 full desktop session。

### M6：输入事件适配

目标：能基础操作远端桌面。

建议：

- ArkTS 增加 N-API：
  - `sendPointer({type, x, y, button, down, delta})`
  - `sendKey({key, down})`
  - `sendUnicode({codePoint, down})`
  - `resize(width, height)`
- Native 侧建立类似 Android 的输入 event queue：
  - UI 线程只 push event，不直接调用 FreeRDP。
  - RDP worker loop 中处理队列，调用 FreeRDP input API。
- 坐标映射：
  - surface 坐标按远端桌面宽高缩放。
  - 如果保持等比显示，需要扣除 letterbox/pillarbox 偏移。
- 触摸映射首版：
  - 单指点按：左键 down/up。
  - 单指移动：mouse move；拖拽保持左键 down。
  - 双指滑动：滚轮。
  - 长按：右键。
- 键盘映射：
  - 普通文本优先走 Unicode keyboard event。
  - 功能键和组合键走 scancode。
  - Ctrl/Alt/Win/Esc/Tab/方向键放工具栏。

验收：

- 点击、拖动、滚动、英文输入、Esc/Tab/方向键可用。
- 断开后输入事件被丢弃，不崩溃。
- 窗口 resize 后坐标仍正确。

### M6.5：键鼠、音视频和硬件编解码专项判断

这几项都属于平台适配，但优先级和实现路径不同：

| 能力 | 是否必须 | 当前状态 | HarmonyOS 适配建议 |
| --- | --- | --- | --- |
| 键盘 | 必须 | ArkTS `onKeyEvent`、工具栏快捷键、N-API `sendKey/sendUnicode` 和 native 输入队列已有骨架 | 继续完善 keycode/scancode 映射、中文输入、软键盘、组合键和窗口失焦释放 modifier |
| 鼠标/触摸 | 必须 | ArkTS `onTouch/onMouse/onAxisEvent`、坐标映射和 N-API `sendPointer` 已有骨架 | 继续验证触摸手势、右键、滚轮、viewport 外点击丢弃、窗口 resize 后坐标正确 |
| 音频播放 | 产品化通常需要，首个闭环可延后 | `rdpsnd` 已编译，OHOS NDK 有 OpenSLES 时启用 FreeRDP OpenSLES backend，并用兼容 shim 处理 Android simple buffer queue 头文件差异 | 先实机验证 OpenSLES 播放；如果稳定性不足，再新增 OHOS AudioRenderer backend；处理采样率、通道数、缓冲、静音和后台生命周期 |
| 麦克风 | 可选，取决于会议/语音场景 | `audin` 已编译，但还没有 OHOS AudioCapturer 采集后端和权限/UI 闭环 | 接 `audin-client-ohos` backend，用 Harmony AudioCapturer 采集 PCM，再按 FreeRDP audin 协议送给服务端；需要麦克风权限和隐私提示 |
| 桌面图形 | 必须 | `PostConnect/gdi_init/EndPaint` 到 `NativeWindow` CPU copy 已有骨架 | 继续做真机验证、dirty rect 优化、掉帧策略、surface destroy/resize 稳定性和高分辨率性能测试 |
| RDPGFX/H.264 | 性能增强 | `rdpgfx`、FFmpeg、OpenH264 已编译并打包 | 先用软件解码验证协议和稳定性，再考虑 OHOS AVCodec 硬件解码 |
| 硬件视频解码 | 非首版，性能专项 | FreeRDP 现有硬件路径是 MediaFoundation/MediaCodec/VAAPI/VideoToolbox 等，不覆盖 OHOS | 需要新增 OHOS 专用 `H264_CONTEXT_SUBSYSTEM`，用 AVCodec `OH_VideoDecoder`；输出到 YUV/RGBA buffer 再合成到桌面 surface，或谨慎走 decoder surface。不能直接启用 Android `WITH_MEDIACODEC` |
| 硬件视频编码 | RDP 客户端显示远端桌面通常不需要 | 当前无需求 | 只有做 RDP server、摄像头重定向、视频上行等场景才评估；普通 client 主要是解码，不是编码 |

键鼠输入不依赖 FreeRDP channel，属于 core input，必须和渲染一起做，否则只能“连上但不能操作”。音频和视频增强依赖 channel/codec，应该等 M5/M6 稳定后独立开里程碑。

#### 键鼠适配细节

- FreeRDP core 已有输入 API：`freerdp_input_send_keyboard_event_ex`、`freerdp_input_send_unicode_keyboard_event`、`freerdp_input_send_mouse_event`、`freerdp_input_send_extended_mouse_event`。
- Harmony 侧可选择两条路径：
  - ArkTS 收到 key/touch/mouse/wheel 后通过 N-API 转发。
  - Native `OH_NativeXComponent` 注册 key、mouse、touch/focus 回调，直接进入 C++ 队列。
- 推荐统一进入 native 输入队列，由 RDP worker 线程消费。这样能避免 UI 线程直接触碰 FreeRDP context，也便于断开时丢弃残留事件。
- 中文输入和普通文本优先走 Unicode event；Esc、Tab、方向键、Ctrl/Alt/Win 组合键走 RDP scancode。
- 触摸屏首版按鼠标模型映射，不要一开始启用 `rdpei` 触摸通道；`rdpei` 后续适合多点触控和更完整手势。

#### 音频适配细节

FreeRDP 音频不是“系统自动播放”，需要启用并接平台后端：

- 远端播放到本机：`rdpsnd` channel。FreeRDP 已有 `alsa`、`pulse`、`oss`、`winmm`、`opensles`、`mac`、`ios`、`fake` 后端，HarmonyOS 需要新增 `ohos` 后端。
- 本机麦克风到远端：`audin` dynamic channel。HarmonyOS 需要新增 `audin-client-ohos`，接 AudioCapturer。
- 首版建议只做 `rdpsnd`，而且先支持 PCM/S16LE/48k 或服务端常见格式；压缩格式、重采样、回声消除、音频焦点和蓝牙路由后置。
- 麦克风涉及权限、隐私状态、后台采集和设备切换，建议独立于播放能力验收。

#### 视频与硬件编解码适配细节

这里要区分三类“视频”：

1. **远端桌面图形**：这是必须能力。首版用 bitmap/GDI 更新就能看到桌面，不需要 H.264。
2. **RDPGFX + H.264/AVC444**：这是现代 RDP 的图形性能路径。FreeRDP 里 H.264 可由 OpenH264、FFmpeg、MediaFoundation、MediaCodec、VAAPI、VideoToolbox 等 subsystem 支撑。OHOS 目前没有现成 subsystem，需要新增。
3. **独立视频重定向通道**：`video`、`tsmf` 等用于视频优化或旧式多媒体重定向。`tsmf` 在 FreeRDP 里默认关闭且标注 deprecated，不建议作为 Harmony 首选路线。

硬件解码推荐路线：

1. 先不启用硬解，完成 `NativeWindow` 渲染和输入。
2. 打开 `rdpgfx`，用软件 H.264 路径验证服务端协商、AVC420/AVC444 解码和桌面合成正确。
3. 新增 `libfreerdp/codec/h264_ohos.c` 或同等模块，实现 FreeRDP `H264_CONTEXT_SUBSYSTEM`：
   - 初始化 `OH_VideoDecoder`，配置 H.264/AVC 格式。
   - 输入 RDPGFX 提供的 H.264 bitstream。
   - 输出 YUV/RGBA buffer，交给 FreeRDP 图形合成或自有 compositor。
   - 分辨率变化时 reset decoder。
   - 没有硬件能力或初始化失败时 fallback 到 OpenH264/FFmpeg。
4. 若使用 decoder surface 直出到 `NativeWindow`，要先证明它能和 RDP dirty rect、鼠标光标、多 surface、缩放策略正确合成；否则优先用 buffer 输出，虽然多一次拷贝，但行为更可控。

不建议直接复用 Android `WITH_MEDIACODEC`：它依赖 Android NDK `AMediaCodec`，不是 HarmonyOS AVCodec API。FreeRDP 的 CMake 里也标注 MediaCodec 是 experimental，且没有无设备支持时的自动 fallback。

### M7：产品化能力

建议优先级：

1. TOFU 证书存储和证书变更拦截。
2. 会话配置保存，但不保存明文密码。
3. 自动重连和网络抖动处理。
4. 剪贴板文本同步。
5. dirty rect、双缓冲、减少全帧 copy。
6. 高分辨率性能测试后再评估 RDPGFX、GPU texture 或平台硬件视频解码。
7. 音频播放、麦克风、文件、打印、智能卡、RD Gateway 等作为单独里程碑。

## 当前脚本配置建议

当前 WSL 增强 profile 保留“无桌面前端、无服务端”的边界，但已经打开通道和软件编解码：

```text
WITH_CLIENT_COMMON=ON
WITH_CLIENT=OFF
WITH_SERVER=OFF
WITH_CHANNELS=ON
WITH_CLIENT_CHANNELS=ON
WITH_X11=OFF
WITH_WAYLAND=OFF
WITH_OPENSSL=ON
WITH_UNICODE_BUILTIN=ON
WITH_FFMPEG=ON
WITH_DSP_FFMPEG=ON
WITH_VIDEO_FFMPEG=ON
WITH_OPENH264=ON
WITH_URIPARSER=ON
WITH_SMARTCARD_PCSC=ON
WITH_ALSA=OFF
WITH_PULSE=OFF
WITH_OPENSLES=ON when OHOS NDK provides it
WITH_CUPS=OFF
WITH_FUSE=OFF
WITH_PCSC=OFF
```

当前已显式启用这些目标 client channels：

```text
CHANNEL_CLIPRDR_CLIENT=ON
CHANNEL_DRDYNVC_CLIENT=ON
CHANNEL_DISP_CLIENT=ON
CHANNEL_RDPGFX_CLIENT=ON
CHANNEL_RDPSND_CLIENT=ON
CHANNEL_AUDIN_CLIENT=ON
CHANNEL_RDPDR_CLIENT=ON
CHANNEL_DRIVE_CLIENT=ON
CHANNEL_PRINTER_CLIENT=ON
CHANNEL_SMARTCARD_CLIENT=ON
CHANNEL_TSMF_CLIENT=ON
```

仍然不要一次性打开全部默认通道。`urbdrc`、摄像头、串口、并口、RAIL、remdesk、sshagent、telemetry 等非目标通道会引入 libusb、设备 API、桌面 shell 或额外权限风险，当前继续关闭。

音频当前只做到“编译进包并可加载”的阶段：

```text
CHANNEL_RDPSND_CLIENT=ON
CHANNEL_AUDIN_CLIENT=ON
WITH_ALSA=OFF
WITH_PULSE=OFF
WITH_OPENSLES=ON when available
```

FreeRDP 现有 OpenSLES backend 是 Android 风格实现，当前脚本通过 `OpenSLES_Android.h` 兼容 shim 让它在 OHOS NDK 下编译。它可以作为短期验证路径；产品化仍建议新增 `rdpsnd/client/ohos` 和 `audin/client/ohos`，直接链接 Harmony AudioRenderer/AudioCapturer。

后续做 H.264/硬件解码时建议分两步：

```text
# Step 1: software validation
WITH_CHANNELS=ON
WITH_CLIENT_CHANNELS=ON
CHANNEL_RDPGFX_CLIENT=ON
WITH_OPENH264=ON 或 WITH_FFMPEG=ON

# Step 2: OHOS hardware decoder after protocol validation
WITH_OHOS_AVCODEC=ON  # 需要新增，不是 FreeRDP 现有开关
WITH_MEDIACODEC=OFF
```

`WITH_MEDIACODEC` 只对应 Android MediaCodec；HarmonyOS 应新增自己的 CMake option 和 codec subsystem。

## 主要风险

1. **OpenSSL 3 provider 风险**：NTLM/NLA 可能依赖 legacy 算法；`OPENSSL_MODULES` 和 `legacy.so` 路径错误会导致认证阶段失败。
2. **WinPR POSIX 假设风险**：OHOS 不是完整桌面 Linux，`pthread_cancel`、`/proc`、时区文件、eventfd、syscall 行为都要实机验证。
3. **渲染性能风险**：首版 CPU copy 能快速闭环，但高分辨率下可能卡顿、发热、耗电，需要 dirty rect/双缓冲/GPU 路线优化。
4. **生命周期风险**：App 后台、页面销毁、surface 重建、重复连接容易触发野指针或 worker 卡住。
5. **证书安全风险**：当前 demo 式自动接受证书不能作为产品默认策略。
6. **输入体验风险**：中文输入、组合键、远端快捷键、横竖屏缩放都需要专项测试。
7. **通道扩展风险**：剪贴板、文件、音频、打印、智能卡每一个都不是“打开 CMake 开关”就结束，必须接 HarmonyOS 对应系统能力和权限。
8. **硬件解码风险**：RDPGFX/AVC444 不是普通播放器场景，硬解输出还要参与桌面合成、dirty rect、光标和缩放处理；直接 surface 直出可能破坏合成模型。

## 推荐任务拆分

1. **连接稳定化**
   - 维护 FreeRDP 子模块 OHOS 分支。
   - 增加 FreeRDP callbacks。
   - 做错误分类和 TOFU 设计。
2. **Native surface**
   - ArkTS `XComponent` 与 C++ `OH_NativeXComponent` 绑定。
   - 实现 `NativeWindow` renderer。
3. **FreeRDP frame**
   - `PostConnect` 初始化 GDI。
   - `EndPaint` 拷贝 frame 到 `NativeWindow`。
4. **输入**
   - N-API 增加 pointer/key/unicode/resize。
   - native event queue 接入 FreeRDP input。
5. **安全和配置**
   - 证书 TOFU。
   - 不保存明文密码。
   - 日志脱敏。
6. **扩展能力**
   - 剪贴板文本。
   - 动态分辨率。
   - 重连。
   - 性能优化。
7. **音视频专项**
   - `rdpsnd` + Harmony AudioRenderer。
   - `audin` + Harmony AudioCapturer。
   - `rdpgfx` + OpenH264/FFmpeg 软件验证。
   - OHOS AVCodec 硬解 subsystem 和 fallback。

## 参考资料

- 本地 FreeRDP 源码：`harmony/third_party/FreeRDP`
- 本地 FreeRDP OHOS 分支：`harmony/third_party/FreeRDP` 子模块内 `ohos-port`
- 本地 OHOS 构建脚本：`harmony/scripts/wsl/build-freerdp-ohos.sh`
- 本地 Harmony native bridge：`harmony/app/entry/src/main/cpp/napi_init.cpp`
- 本地 Harmony 页面：`harmony/app/entry/src/main/ets/pages/Index.ets`
- OpenHarmony NativeWindow 文档：https://gitee.com/openharmony/docs/blob/673f3471596850245bdcce9e61d8589da49ddee9/en/application-dev/napi/native_window-guidelines.md
- OpenHarmony Native XComponent API 文档：https://gitee.com/openharmony/docs/blob/43726785b4033887cd1a838aaaca5e255897a71e/en/application-dev/reference/apis-arkui/native__interface__xcomponent_8h.md
- OpenHarmony XComponent/NAPI 指南：https://gitee.com/openharmony/docs/blob/54a84aefd5b06fd937a20063d39ee73444b41344/zh-cn/application-dev/ui/napi-xcomponent-guidelines.md
- OpenHarmony UIAbility/WindowStage 生命周期文档：https://gitee.com/openharmony/docs/blob/115c3238e4c0cd4534bf2543c0b722819e889ba4/en/application-dev/application-models/uiability-lifecycle.md
- OpenHarmony AudioRenderer 文档：https://gitee.com/openharmony/docs/blob/990f9a43dac9610fdecfd9e70f503329b64ac7d7/zh-cn/application-dev/media/audio/using-audiorenderer-for-playback.md
- OpenHarmony AudioCapturer 文档：https://gitee.com/openharmony/docs/blob/dc58ead6628a294cbc0e7b17faca3f5dca74103b/zh-cn/application-dev/media/audio/using-audiocapturer-for-recording.md
- OpenHarmony 视频解码/AVCodec 文档：https://gitee.com/openharmony/docs/blob/master/zh-cn/application-dev/media/avcodec/video-decoding.md
