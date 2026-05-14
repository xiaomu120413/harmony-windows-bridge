# FreeRDP Windows/Linux 适配点与 HarmonyOS 移植报告

日期：2026-05-14

## 结论摘要

FreeRDP 的跨平台能力主要分两层：

1. **WinPR 系统抽象层**：把 Windows API 语义适配到 POSIX/Linux/Android/macOS 等平台，包括线程、事件、等待、socket、文件、路径、动态库、日志、时区、系统信息、加密、SSPI/NTLM 等。这一层在 HarmonyOS 上必须继续使用，但要按 OHOS NDK/musl 的实际能力做少量兼容补丁和验证。
2. **客户端前端层**：`client/Windows`、`client/X11`、`client/Wayland`、`client/SDL`、`client/Android` 分别负责窗口、渲染、输入、剪贴板、音频等平台 UI/设备能力。这一层不能直接搬到 HarmonyOS。HarmonyOS 应该保留 FreeRDP core + WinPR，把窗口和输入重写为 ArkUI `XComponent` + C++ N-API + `NativeWindow`。

当前仓库已经走的是正确方向：`harmony/scripts/wsl/build-freerdp-ohos.sh` 交叉编译 FreeRDP/WinPR/OpenSSL/zlib/cJSON，关闭桌面客户端、X11/Wayland、音频、设备重定向和大部分通道；`libentry.so` 通过 N-API 暴露 `probe/connect/disconnect/onState/onLog/onError`，并在运行期 `dlopen` FreeRDP runtime。下一步的关键不是移植 `wfreerdp` 或 `xfreerdp`，而是补 HarmonyOS 专用的渲染、输入、证书和生命周期层。

## 本次分析范围

本报告基于本地仓库：

- FreeRDP 源码：`harmony/third_party/FreeRDP`
- FreeRDP commit：`1ab2572f17f5056f967727707d75f88e2e12270b`
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

对 HarmonyOS 的影响：不能沿用默认配置。HarmonyOS App 不需要任何桌面 client binary，也不需要 server binary。当前脚本关闭 `WITH_CLIENT`、`WITH_SERVER`、`WITH_X11`、`WITH_WAYLAND`、`WITH_ALSA`、`WITH_PULSE`、`WITH_FFMPEG`、`WITH_OPENH264` 等，是合理的最小化构建策略。

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

对 HarmonyOS 的影响：第一版应关闭大多数通道。剪贴板、动态分辨率、rdpgfx 可以在基础可用后再逐步打开；音频、文件、打印、智能卡、USB 重定向都需要 HarmonyOS 专用权限和系统服务适配，不应混入首个闭环版本。

## 当前 HarmonyOS 适配现状

### 已完成或已具备基础

1. `harmony/scripts/wsl/build-freerdp-ohos.sh` 使用 Linux 版 OHOS NDK toolchain 交叉编译：
   - OpenSSL 3.3.2
   - zlib 1.3.1
   - cJSON 1.7.18
   - FreeRDP/WinPR arm64-v8a
2. 构建脚本阶段性补丁：
   - 复制 FreeRDP 源码到 build workdir。
   - 对 `winpr/libwinpr/thread/thread.c` 应用 `__OHOS__` 兼容补丁，避免 OHOS 上走 `pthread_cancel`。
3. FreeRDP CMake 配置已经最小化：
   - 保留：`WITH_OPENSSL=ON`、`WITH_CLIENT_COMMON=ON`、`WITH_UNICODE_BUILTIN=ON`。
   - 关闭：`WITH_CLIENT`、`WITH_SERVER`、`WITH_CHANNELS`、`WITH_CLIENT_CHANNELS`、`WITH_X11`、`WITH_WAYLAND`、`WITH_FFMPEG`、`WITH_SWSCALE`、`WITH_OPENH264`、`WITH_ALSA`、`WITH_PULSE`、`WITH_CUPS`、`WITH_FUSE`、`WITH_PCSC` 等。
4. Runtime 打包链路已存在：
   - `harmony/out/ohos-arm64/runtime-libs/` 产出 `libfreerdp3.so`、`libwinpr3.so`、`libssl.so.3`、`libcrypto.so.3`、`libz.so.1`、`libcjson.so.1`。
   - `harmony/scripts/windows/sync-freerdp-runtime.ps1` 同步到 `harmony/app/entry/libs/arm64-v8a/`。
   - `elf-report.txt` 显示 arm64 产物为 ELF64 AArch64，依赖关系基本闭合。
5. Harmony App 已具备 native bridge：
   - `module.json5` 仅声明 `ohos.permission.INTERNET`。
   - `Index.ets` 调用 `libentry.so` 的 `probe/connect/disconnect/onState/onLog/onError`。
   - `napi_init.cpp` 已经做 TCP reachability check、运行期 `dlopen`、FreeRDP settings 映射、worker thread、FreeRDP event loop、线程安全回调。
   - `napi_init.cpp` 会设置 `OPENSSL_MODULES` 到打包的 `ossl-modules`，这是 OpenSSL 3 provider 场景下 NLA/NTLM 的关键点。

### 仍未完成的关键闭环

1. **渲染没有接入**：`Index.ets` 有 `XComponent(id: 'rdpSurface')` 占位，但 native 侧尚未注册 `OH_NativeXComponent` surface 生命周期，也没有把 FreeRDP frame 写入 `NativeWindow`。
2. **输入没有接入**：页面上的 Ctrl/Alt/Win/Esc/Tab 只是按钮，还没有 N-API `sendPointer/sendKey`，native 侧也没有输入事件队列。
3. **证书策略仍偏 demo**：当前 `certPolicy != "deny"` 时设置 `IgnoreCertificate/AutoAcceptCertificate`，第一版产品不能这样默认放行，应改成 TOFU 或严格校验。
4. **通道关闭导致能力有限**：没有剪贴板、动态分辨率、音频、文件、打印、智能卡等。
5. **生命周期需要压测**：断开、页面销毁、App 后台、重复连接、网络抖动都需要验证 worker、FreeRDP context、dlopen runtime 的释放策略。

## HarmonyOS 是否需要这些适配，以及怎么适配

| FreeRDP 适配点 | Windows/Linux/Android 当前做法 | HarmonyOS 是否需要 | 建议适配方式 |
| --- | --- | --- | --- |
| WinPR 线程/事件/等待 | Windows 用原生 HANDLE；Linux/Android 用 pthread、eventfd/pipe、poll | 需要 | 复用 POSIX 路径；保留 `pthread_cancel` 禁用补丁；优先用 `abortConnectContext` 和自有 `running` 标志停止线程 |
| WinPR socket/TCP | Windows Winsock；Linux POSIX socket | 需要 | 复用 POSIX socket；保留 App `INTERNET` 权限；继续用 TCP probe 区分网络失败和 RDP 失败 |
| WinPR 文件/路径/HOME | Windows AppData；Linux XDG/HOME；Android app files dir | 需要，但要收口 | App 启动时设置 HOME/XDG 到应用沙箱目录；证书、known_hosts、日志、缓存都放应用私有目录 |
| 动态库加载 | Windows `LoadLibrary`；Linux/Android `dlopen` | 需要 | 当前 `dlopen` runtime 可继续；注意库加载顺序、`RTLD_GLOBAL`、OpenSSL provider 路径；runtime 不要中途 `dlclose` |
| Windows client `wfreerdp` | Win32/GDI/Credui/证书存储 | 不需要 | 不移植；Windows demo 仅保留为目标环境诊断工具 |
| X11/Wayland client | X11/Wayland 窗口、输入、剪贴板 | 不需要 | 继续关闭 `WITH_X11/WITH_WAYLAND`；Harmony UI 用 ArkUI |
| SDL client | SDL 窗口和事件循环 | 首版不需要 | HAP 内不建议引 SDL；除非后续要做独立跨平台 native UI，当前路线不需要 |
| Android JNI client | JNI、AndroidBitmap、Android event queue | 需要参考，不直接复用 | 重写为 N-API；保留“native worker + event queue + GDI buffer + UI 回调”的架构思想 |
| 渲染 | Windows GDI/X11 image/AndroidBitmap | 需要重写 | `XComponent` 获取 surface，C++ 注册 `OH_NativeXComponent_Callback`，用 `NativeWindow` request/flush buffer，FreeRDP frame 转 `RGBA8888` |
| 输入 | Windows/X11/Android 各自把本地键鼠转 FreeRDP input | 需要重写 | ArkUI touch/key -> N-API -> native event queue -> `freerdp_input_send_mouse_event/keyboard_event/unicode_keyboard_event` |
| 剪贴板 | `cliprdr` + 平台 clipboard | 首版可不需要，后续需要 | 先关闭；后续打开 `CHANNEL_CLIPRDR_CLIENT`，接 Harmony clipboard API，只做文本，再扩展文件 |
| 音频 | ALSA/Pulse/OSS/OpenSLES | 首个可交互版本可不做，产品化通常需要 | 继续关闭到 M6；后续打开 `rdpsnd/audin`，用 Harmony AudioRenderer/AudioCapturer 写 OHOS backend，不能直接用 ALSA/Pulse/OpenSLES |
| 图形/H.264/视频 | Bitmap/RFX/NSCodec/RDPGFX；H.264 可走 OpenH264/FFmpeg/MediaFoundation/MediaCodec | 基础画面必须做；RDPGFX/H.264 是性能增强；独立视频通道按需 | 首版先用基础 GDI/bitmap 到 `NativeWindow`；性能不足再打开 `rdpgfx` 和软件 H.264；硬件解码需新增 OHOS AVCodec backend，不要直接套 Android MediaCodec |
| 文件/磁盘重定向 | `drive`、FUSE、POSIX 文件系统 | 首版不需要 | 后续需配合 Harmony 文件选择器和沙箱权限，不能直接暴露任意路径 |
| 打印/智能卡/USB | CUPS/PCSC/系统设备 API | 首版不需要 | 默认关闭；只有明确产品需求时再按 Harmony 能力专项适配 |
| 证书校验 | Windows 可用系统证书；Linux/Android 用 OpenSSL/known_hosts/回调 | 需要 | 实现 `VerifyCertificateEx/VerifyChangedCertificateEx` 回调到 ArkTS；做 TOFU 指纹存储；生产默认不使用 ignore |
| 日志 | Console/file/syslog/Android log | 需要 | N-API 回调继续保留；建议补 `hilog` 输出，App UI 只显示脱敏摘要 |
| 生命周期 | 各 client 自己管理事件循环 | 需要 | 页面销毁/后台/断网时统一进入 `Disconnecting`，调用 `freerdp_abort_connect_context`，等待 worker join，释放 surface |

## 建议的 HarmonyOS 具体适配方案

### M4.4：稳定连接和认证闭环

目标：不渲染，但真实 FreeRDP 会话连接、断开、失败分类稳定。

建议：

- 把 `pthread_cancel` 兼容补丁固化成可维护 patch 文件，而不是只在脚本里 `perl` 替换。至少在报告/脚本中记录 upstream 文件、匹配片段和原因。
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
| 键盘 | 必须 | UI 有快捷键按钮，但没有 native 输入通道 | `XComponent`/ArkTS key event -> N-API `sendKey/sendUnicode` -> native event queue -> `freerdp_input_send_keyboard_event_ex` 或 `freerdp_input_send_unicode_keyboard_event` |
| 鼠标/触摸 | 必须 | `XComponent` 还没有触摸、鼠标、滚轮转发 | touch/mouse event -> 远端坐标换算 -> `freerdp_input_send_mouse_event`、`freerdp_input_send_extended_mouse_event`；首版支持点击、拖动、滚轮、右键 |
| 音频播放 | 产品化通常需要，首个闭环可延后 | `WITH_CHANNELS=OFF`，`rdpsnd` 未构建 | 打开 `rdpsnd` 后新增 `rdpsnd-client-ohos` backend，把远端 PCM/解码后音频写入 Harmony AudioRenderer；处理采样率、通道数、缓冲、静音和后台生命周期 |
| 麦克风 | 可选，取决于会议/语音场景 | `audin` 未构建 | 打开 `audin` 后新增 `audin-client-ohos` backend，用 Harmony AudioCapturer 采集 PCM，再按 FreeRDP audin 协议送给服务端；需要麦克风权限和隐私提示 |
| 桌面图形 | 必须 | 连接逻辑已有，渲染未接入 | M5 先做 FreeRDP GDI buffer -> `NativeWindow` CPU copy，保证可见、稳定、可交互 |
| RDPGFX/H.264 | 性能增强 | 当前 `WITH_CHANNELS=OFF`、`WITH_OPENH264=OFF`、`WITH_FFMPEG=OFF` | 高分辨率或视频播放卡顿时再打开 `rdpgfx` 和 H.264；先用 OpenH264/FFmpeg 软件解码验证协议，再考虑硬件解码 |
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

短期继续保留：

```text
WITH_CLIENT_COMMON=ON
WITH_CLIENT=OFF
WITH_SERVER=OFF
WITH_CHANNELS=OFF
WITH_CLIENT_CHANNELS=OFF
WITH_X11=OFF
WITH_WAYLAND=OFF
WITH_OPENSSL=ON
WITH_UNICODE_BUILTIN=ON
WITH_FFMPEG=OFF
WITH_OPENH264=OFF
WITH_ALSA=OFF
WITH_PULSE=OFF
WITH_CUPS=OFF
WITH_FUSE=OFF
WITH_PCSC=OFF
```

M5/M6 如果只用基础 GDI 和输入，仍可保持 `WITH_CHANNELS=OFF`。

后续要做剪贴板/动态分辨率/rdpgfx 时再谨慎打开：

```text
WITH_CHANNELS=ON
WITH_CLIENT_CHANNELS=ON
CHANNEL_CLIPRDR_CLIENT=ON
CHANNEL_DISP_CLIENT=ON
CHANNEL_RDPGFX_CLIENT=ON
```

但不要一次性打开全部默认通道，否则会引入音频、打印、智能卡、USB、文件重定向等平台依赖和权限风险。

后续做音频时建议单独建配置 profile，而不是和图形增强混在一起：

```text
WITH_CHANNELS=ON
WITH_CLIENT_CHANNELS=ON
CHANNEL_RDPSND_CLIENT=ON
CHANNEL_AUDIN_CLIENT=ON
WITH_ALSA=OFF
WITH_PULSE=OFF
WITH_OPENSLES=OFF
```

同时在 FreeRDP channel 构建中新增 OHOS audio subsystem，例如 `rdpsnd/client/ohos` 和 `audin/client/ohos`，链接 Harmony 音频 native 库。OpenSLES 是 Android 路线，不是 HarmonyOS 路线。

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
   - 固化 OHOS patch。
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
- 本地 OHOS 构建脚本：`harmony/scripts/wsl/build-freerdp-ohos.sh`
- 本地 Harmony native bridge：`harmony/app/entry/src/main/cpp/napi_init.cpp`
- 本地 Harmony 页面：`harmony/app/entry/src/main/ets/pages/Index.ets`
- OpenHarmony NativeWindow 文档：https://gitee.com/openharmony/docs/blob/673f3471596850245bdcce9e61d8589da49ddee9/en/application-dev/napi/native_window-guidelines.md
- OpenHarmony Native XComponent API 文档：https://gitee.com/openharmony/docs/blob/43726785b4033887cd1a838aaaca5e255897a71e/en/application-dev/reference/apis-arkui/native__interface__xcomponent_8h.md
- OpenHarmony XComponent/NAPI 指南：https://gitee.com/openharmony/docs/blob/54a84aefd5b06fd937a20063d39ee73444b41344/zh-cn/application-dev/ui/napi-xcomponent-guidelines.md
- OpenHarmony AudioRenderer 文档：https://gitee.com/openharmony/docs/blob/990f9a43dac9610fdecfd9e70f503329b64ac7d7/zh-cn/application-dev/media/audio/using-audiorenderer-for-playback.md
- OpenHarmony AudioCapturer 文档：https://gitee.com/openharmony/docs/blob/dc58ead6628a294cbc0e7b17faca3f5dca74103b/zh-cn/application-dev/media/audio/using-audiocapturer-for-recording.md
- OpenHarmony 视频解码/AVCodec 文档：https://gitee.com/openharmony/docs/blob/master/zh-cn/application-dev/media/avcodec/video-decoding.md
