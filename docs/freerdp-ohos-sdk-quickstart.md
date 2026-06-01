# FreeRDP OHOS SDK Quickstart

更新时间：2026-05-26

本文面向希望直接复用 FreeRDP OHOS client/backend 的第三方 HarmonyOS
Native 调用方。Demo HAP 只是一种接入方式；SDK 接入方应只依赖 FreeRDP
public headers、runtime `.so`、自己的 ArkUI/N-API 权限和 Surface 生命周期。

## 头文件和库

安装后的 public headers 位于：

```text
include/freerdp3/freerdp/client/ohos/
```

调用方使用 FreeRDP include root 后按下面方式 include：

```c
#include <freerdp/client/ohos/ohos_session.h>
#include <freerdp/client/ohos/ohos_clipboard.h>
#include <freerdp/client/ohos/ohos_audio.h>
#include <freerdp/client/ohos/ohos_rdpgfx.h>
#include <winpr/collections.h>
```

首版运行时至少需要随 HAP 打包：

- `libfreerdp-client3.so`
- `libfreerdp3.so`
- `libwinpr3.so`
- `libcrypto.so.3`
- `libssl.so.3`
- `libz.so.1`
- `libcjson.so.1`
- `liburiparser.so.1`
- `libc++_shared.so`

启用商用默认 `rdpgfx-h264`、AVC444 GPU compositor 和媒体能力时，还需要：

- `libopenh264.so.7`
- `libavcodec.so.60`
- `libavformat.so.60`
- `libavutil.so.58`
- `libswresample.so.4`
- `libswscale.so.7`
- `libavfilter.so.9`
- `libavdevice.so.60`

## 最小会话流程

`freerdp_ohos_session_connect` 会运行 FreeRDP event loop，通常放在工作线程。
下面示例省略 UI 线程投递、NativeWindow 生命周期和错误展示，只保留 SDK
接入顺序：准备参数、设置回调、启动连接、发送输入、resize、断开。

```c
static BOOL OnConfigure(freerdp* instance, rdpContext* context,
                        const FREERDP_OHOS_SESSION_OPTIONS* options,
                        char* message, size_t messageSize, void* userData);

void start_session(struct app_state* app)
{
    char message[512] = { 0 };
    FREERDP_OHOS_SESSION_INPUT input = {
        .serverHostname = "10.0.0.5",
        .serverPort = "3389",
        .username = "user",
        .password = "password",
        .desktopSize = "1920x1080",
        .graphicsMode = "rdpgfx-h264",
        .appDataDir = "/data/storage/el2/base/files",
        .certificatePolicy = "tofu",
        .colorDepth = 32,
        .tcpConnectTimeoutMs = 5000
    };
    FREERDP_OHOS_SESSION_PREPARED_OPTIONS prepared = { 0 };
    freerdp_ohos_session_prepare_options(&input, &prepared, message, sizeof(message));

    FREERDP_OHOS_SESSION_CALLBACKS callbacks = { 0 };
    callbacks.Configure = OnConfigure;
    callbacks.ShouldContinue = app_should_continue;
    callbacks.Log = app_log;
    callbacks.Error = app_error;
    callbacks.userData = app;

    app->session = freerdp_ohos_session_new();
    app->prepared = prepared;
    app->callbacks = callbacks;
    app_start_worker(app, app_connect_worker);
}

void app_connect_worker(struct app_state* app)
{
    char message[512] = { 0 };
    freerdp_ohos_session_connect(app->session, &app->prepared.options,
                                 &app->callbacks, message, sizeof(message));
}

void send_sample_input(struct app_state* app)
{
    char message[512] = { 0 };
    freerdp_ohos_session_send_pointer(app->session, &viewport, &pointerEvent, message, sizeof(message));
    freerdp_ohos_session_send_key(app->session, &keyEvent, message, sizeof(message));
    freerdp_ohos_session_resize(app->session, 1600, 900, message, sizeof(message));
}

void stop_session(struct app_state* app)
{
    freerdp_ohos_session_disconnect(app->session);
    app_join_worker(app);
    freerdp_ohos_session_free(app->session);
}
```

## 权限和 Surface 回调

HAP 必须在 `module.json5` 声明首版实际启用的权限：

- `ohos.permission.PRINT`
- `ohos.permission.CUSTOM_SCREEN_RECORDING`
- `ohos.permission.READ_PASTEBOARD`
- `ohos.permission.MICROPHONE`
- `ohos.permission.CAMERA`
- `ohos.permission.APPROXIMATELY_LOCATION`
- `ohos.permission.LOCATION`

声明权限不等于连接开始时弹窗。接入方应把权限弹窗放到 callback 内：
读取 Pasteboard、远端实际打开 `audin` 采集、远端实际请求 `rdpecam` 摄像头或远端发起 `LocationStart` 时才请求用户授权。HAP 侧可以用统一 permission-request bridge 承接 native 请求，再映射到具体系统权限；对外仍可保留独立 callback 名以兼容既有集成。`CUSTOM_SCREEN_RECORDING` 用于启动本机 xrdp 被控桌面流前授权，`PRINT` 用于远端打印作业到达后的 PrintKit 提交流程，连接开始时不应初始化或连接 PrintKit。

```c
static BOOL RequestPasteboard(void* userData, UINT32 timeoutMs)
{
    return app_request_pasteboard_permission(userData, timeoutMs);
}

static BOOL RequestMicrophone(void* userData, UINT32 timeoutMs)
{
    return app_request_microphone_permission(userData, timeoutMs);
}

static BOOL DrawAvc444(const FREERDP_OHOS_RDPGFX_AVC444_COMMAND_INFO* command,
                       void* userData)
{
    return app_render_avc444_to_surface(userData, command);
}

static BOOL OnConfigure(freerdp* instance, rdpContext* context,
                        const FREERDP_OHOS_SESSION_OPTIONS* options,
                        char* message, size_t messageSize, void* userData)
{
    struct app_state* app = (struct app_state*)userData;
    FREERDP_OHOS_CLIPBOARD_CONFIG clip = { 0 };
    FREERDP_OHOS_RDPGFX_BRIDGE_CONFIG gfx = { 0 };

    freerdp_audin_ohos_set_permission_callback(RequestMicrophone, app);

    clip.PubSubSubscribe = PubSub_Subscribe;
    clip.PubSubUnsubscribe = PubSub_Unsubscribe;
    clip.RequestReadPermission = RequestPasteboard;
    clip.permissionUserData = app;
    freerdp_ohos_clipboard_register(app->clipboard, context, &clip, message, messageSize);

    gfx.avc444GpuCompositor = TRUE;
    gfx.surfaceTargetWidth = app->surfaceWidth;
    gfx.surfaceTargetHeight = app->surfaceHeight;
    gfx.avc444SurfaceCommand = DrawAvc444;
    gfx.userData = app;
    return app_attach_rdpgfx_bridge(app, context, &gfx, message, messageSize);
}
```

`app_attach_rdpgfx_bridge` 是接入方自己的 Surface 绑定函数：它应在
RDPGFX context 可用时调用 `freerdp_ohos_rdpgfx_bridge_attach`，并在
Surface 销毁、会话断开或页面退出时 detach/free。不要把 ArkUI、N-API 或
Demo HAP 私有类型塞进 FreeRDP public API。

## 默认通道策略

首版商用默认接入：

- RDP/TLS/NLA
- `cliprdr` 文本剪贴板，按需申请 Pasteboard 权限
- `disp` 动态分辨率
- `geometry` 动态虚拟通道，默认注册；当前不消费 region 数据
- `rdpgfx-h264` 和 AVC444 GPU compositor
- `rdpsnd` 播放
- `audin` 麦克风采集，远端请求时按需申请麦克风权限
- `rdpecam` 摄像头重定向，远端请求时按需申请摄像头权限
- `location` 地理位置重定向后端已构建；当前默认 session config 关闭 channel，启用后远端请求时按需申请定位权限
- `drive` 文件重定向，固定映射下载控件授权的 `Download/com.muhub.desktop` 为 `\\tsclient\Downloads`
- `printer` 打印重定向，连接时只向 Windows 暴露虚拟打印机；Windows 提交打印作业时才初始化/查询/连接 PrintKit 并提交作业

首版默认关闭或不交付：

- smartcard source/channel、WinPR smartcard PCSC backend、TSMF：交付构建裁剪。
- FUSE clipboard file-copy、CUPS printer backend：当前 OHOS sysroot 不满足依赖；打印交付路径使用 OHOS PrintKit backend。

## 验收

SDK 接入方至少应验证：

1. 缺少 FreeRDP headers 或 runtime `.so` 时构建/启动 fail-fast。
2. 连接开始不弹 Pasteboard 或麦克风权限。
3. 触发剪贴板读取时才申请 Pasteboard 权限，拒绝后会话不崩溃。
4. 远端请求音频采集时才申请麦克风权限，拒绝后 `audin` 明确失败且会话继续。
5. 启用 `location` channel 后，远端请求位置重定向时才申请定位权限，拒绝后 location sample 失败但会话继续。
6. App 启动后能准备 `Download/com.muhub.desktop`；连接后 Windows 侧 `\\tsclient\Downloads` 能完成小文件读写。
7. 连接开始不初始化 PrintKit；远端提交打印作业时才进入 OHOS printer backend，提交失败只影响本次打印作业。
8. Surface resize 后能发送 `disp` monitor layout；服务端不支持时有明确日志。
9. `geometry` 通道协商不会改变现有画面布局；服务端不支持时会话继续。
10. `rdpgfx-h264` 失败只在图形路径内 fallback，不掩盖认证、证书或网络错误。
