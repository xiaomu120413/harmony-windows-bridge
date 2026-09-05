# xrdp OHOS MSTSC 穿刺方案

> 历史阶段方案：早期内嵌 xrdp、dummy backend、目录与文件行数描述不代表当前实现。当前 2in1 使用 libxrdpcontrol 启动独立 HNP 子进程，tablet 无被控端；现行包模型和验收状态见 [多设备打包方案](harmonyos-multidevice-hnp-packaging-plan.md)。后续带日期的记录仅代表对应阶段验证。

本文档只描述 xrdp 线路。当前目标是让 Windows 自带 `mstsc` 能连进 HarmonyOS demo，Phase 1 先证明 TCP/RDP/xrdp/HAP/HNP/native 调用链打通；显示采集、输入注入、剪贴板、音频等能力放到 Phase 2。

## 当前结论

- 选择 xrdp 的前提成立：如果必须让 Windows 自带 `mstsc` 连接，就需要服务端 RDP 协议栈，xrdp 比从零实现 RDP 服务端更合适。
- OHOS 上不能依赖 Linux 桌面栈，`libX11-dev` / `libX11-devel` 这类依赖要剔除，后端改成 OHOS 自定义 backend。
- Phase 1 不做真实桌面远控，只做 dummy backend，验证 mstsc 可以连到 xrdp 并看到测试画面或明确日志。
- xrdp runtime 已按 HNP 方式进入 HAP，同时为了 HAP native `dlopen` 稳定性，最终 HAP 也同步包含 `libs/arm64-v8a/libxrdpserver.so`、`libxrdpohos.so` 和 `libs/arm64-v8a/xrdp/config|share`。

## 权限和能力

Phase 1 必需：

- `ohos.permission.INTERNET`：监听 TCP 3390，并接受 Windows mstsc 连接。
- HNP private package：`module.json5` 中声明 `hnpPackages`，打包 `xrdp.hnp`。
- 应用沙箱文件能力：使用 `/data/storage/el2/base/files/xrdp` 存放 run/log 等可写目录。
- Native 动态库加载能力：HAP native 层通过 `dlopen` 加载 `libxrdpserver.so`。

Phase 2 可能需要：

- 屏幕/窗口采集相关能力，用于把真实 HarmonyOS 画面送给 xrdp backend。
- 输入注入/自动化/企业管控能力，用于把 RDP 鼠标键盘映射回系统。
- 剪贴板读取写入能力，用于 `cliprdr`。
- 麦克风/音频播放能力，用于 `rdpsnd` / `audin`。
- TLS 证书、访问控制、端口配置和前后台运行策略。

## Phase 1 Stories

### Story 1: xrdp OHOS 交叉构建

修改点：

- 新增 `harmony/scripts/wsl/build-xrdp-ohos.sh`。
- xrdp `configure.ac` 增加 `--enable-ohos`。
- OHOS 模式跳过 X11、Xfixes、Xrandr、XKB、PAM、FUSE、Linux 音频等桌面依赖。
- 只构建 Phase 1 需要的最小目录：`common`、`libipm`、`libxrdp`、`painter`、`ohos`、`xrdp`、`pkgconfig` 等。

可能问题：

- OHOS SDK 环境变量缺失会导致工具链找不到。
- FreeRDP 线路复用的 OpenSSL/zlib sysroot 被清理后，需要先重建依赖。
- Autotools 文件变化后必须自动跑 `bootstrap`。

验证点：

- 构建日志不再要求 `libx11-dev` / `libX11-devel`。
- 输出目录为 `harmony/out/xrdp-ohos-arm64`。
- `sysroot/sbin/xrdp`、`sysroot/lib/libxrdpserver.so`、`sysroot/lib/xrdp/libxrdpohos.so` 存在。

### Story 2: OHOS dummy backend

修改点：

- 新增 `harmony/third_party/xrdp/ohos/ohos.c`。
- 实现 xrdp module ABI：`mod_init`、`mod_exit`、`mod_connect` 等。
- `mod_connect` 先绘制固定测试画面。
- 记录 key、mouse、resize、frame ack 日志，但不做真实系统输入注入。
- 新增 `xrdp/xrdp-ohos.ini.in`，默认 `port=3390`、`security_layer=rdp`、`autorun=OHOS`，并关闭非必要通道。

可能问题：

- `security_layer=rdp` 只适合 bring-up，后续需要 TLS/证书/认证策略。
- dummy backend 只能证明 xrdp 回调链路，不代表真实远控性能。

验证点：

- `llvm-nm -D libxrdpohos.so` 能看到 `mod_init` 和 `mod_exit`。
- `xrdp.ini` 的 `[OHOS]` 段指向 `libxrdpohos.so`。
- mstsc 连接后能看到测试画面或明确 backend 日志。

### Story 3: xrdp main loop 内嵌 native library

修改点：

- `xrdp/xrdp.c` 将原 `main()` 主逻辑拆成 `xrdp_server_main()`。
- OHOS 构建导出 `xrdp_ohos_server_main()` 和 `xrdp_ohos_server_stop()`。
- 早退路径从 `g_exit()` 改成返回状态，避免 HAP 进程被 xrdp 直接退出。
- `xrdp/Makefile.am` 在 `XRDP_OHOS` 下额外构建 `libxrdpserver.so`。
- xrdp 配置、share、module、pid、log 路径支持运行时环境变量覆盖：`XRDP_CFG_PATH`、`XRDP_SHARE_PATH`、`XRDP_MODULE_PATH`、`XRDP_PID_PATH`、`XRDP_LOG_PATH`。

可能问题：

- xrdp 全局状态仍是单实例设计，Phase 1 只允许一个 server 实例。
- stop 依赖 xrdp wait object 唤醒主循环，必须在启动后调用才有效。
- 设备侧仍要验证线程退出和二次启动。

验证点：

- `llvm-nm -D libxrdpserver.so` 能看到 `xrdp_ohos_server_main` 和 `xrdp_ohos_server_stop`。
- `libxrdpserver.so` 和 `libxrdpohos.so` 的 RUNPATH 包含 `$ORIGIN`，依赖库可从同目录加载。

### Story 4: HNP 打包和 HAP 集成

修改点：

- 新增 `harmony/scripts/windows/package-xrdp-hnp.ps1`，用 `hnpcli.exe` 打包 xrdp runtime。
- HNP staging 包含 `bin/xrdp`、`lib/*.so`、`config/*`、`share/*` 和 `hnp.json`。
- `module.json5` 增加：

```json
"hnpPackages": [
  {
    "package": "xrdp.hnp",
    "type": "private"
  }
]
```

- 新增 `harmony/scripts/windows/repack-hap-with-hnp.ps1`，用 `app_packing_tool.jar --hnp-path` 把 `entry/hnp/arm64-v8a/xrdp.hnp` 复打进最终 HAP 并重新签名。
- `harmony/app/build_hap.bat` 自动同步 xrdp native runtime、打 HNP、跑 hvigor、复打带 HNP 的签名 HAP。

可能问题：

- 当前复打签名使用本地 debug signing material，默认 keystore 密码为 `123456`，换签名材料时用 `HAP_SIGN_PASSWORD` 覆盖。
- HNP 安装后的真实物理路径仍需设备确认。
- 为了当前 HAP 侧可直接 `dlopen`，包内同时保留了一份 native libs/runtime；后续可以根据设备实测收敛成纯 HNP 路径。

验证点：

- `entry/hnp/arm64-v8a/xrdp.hnp` 能生成。
- 最终 `entry-default-signed.hap` 内存在 `hnp/arm64-v8a/xrdp.hnp`。
- 最终 HAP 内存在 `libs/arm64-v8a/libxrdpserver.so`、`libs/arm64-v8a/libxrdpohos.so`、`libs/arm64-v8a/xrdp/config/xrdp.ini`。

### Story 5: HAP N-API 启停入口

修改点：

- 新增 `harmony/app/entry/src/main/cpp/xrdp/xrdp_server_bridge.cpp`。
- N-API 增加 `probeXrdpServer`、`startXrdpServer`、`stopXrdpServer`。
- `startXrdpServer` 在 native thread 调用 `xrdp_ohos_server_main()`，避免阻塞 UI。
- 启动前创建 `/data/storage/el2/base/files/xrdp/run` 和 `/data/storage/el2/base/files/xrdp/log`。
- 运行时设置 xrdp 路径环境变量，优先从 HAP native lib 目录寻找库和配置，同时保留 `hnpRoot` 参数用于设备侧 HNP 路径实测。
- Demo 页面增加 `Probe xrdp`、`Start xrdp`、`Stop xrdp` 按钮。

可能问题：

- `dlopen` 是否允许从 HNP 私有安装路径加载，需要设备实测。
- 非 ELF 文件放在 `libs/arm64-v8a/xrdp` 下已能打包和签名，但安装后是否原样展开也需要设备确认。
- `startXrdpServer` 当前只返回“启动请求已发出”，如果 xrdp 很快退出，需要从日志看 `lastExitCode`。

验证点：

- ArkTS 编译通过。
- HAP 构建通过。
- 点 `Probe xrdp` 能返回库路径、configPath、modulePath。
- 点 `Start xrdp` 后 3390 端口可监听，Windows `mstsc` 可尝试连接。

## Phase 2 Stories

### Story 6: 显示采集和 RDP 图像输出

修改点：

- 将 OHOS 当前窗口、Surface 或屏幕帧接入 xrdp backend。
- 先走稳定 BGRA/RGBA 帧，再评估 RDP bitmap update、NSCodec、GFX。
- 增加帧率、脏区、超时和背压控制。

验证点：

- mstsc 持续看到实时画面。
- 分辨率变化后不黑屏。

### Story 7: 输入映射

修改点：

- 将 xrdp backend 的 key/mouse 事件映射到 OHOS 输入事件。
- 处理组合键、修饰键、滚轮、触控板滚动和输入法边界。

验证点：

- 鼠标移动、点击、滚轮和常用键盘输入可操作目标界面。

### Story 8: 剪贴板、文件和安全

修改点：

- 打开 `cliprdr` 后做 Windows RDP 格式和 OHOS Pasteboard/UDMF 的正向映射。
- 图片至少支持 `CF_DIB`、`CF_DIBV5`、PNG、JPEG、WebP 的缓存 worker 路径。
- 补 TLS、证书、认证、访问控制和端口配置。

验证点：

- 文本、图片、文件双向复制不阻塞主链路。
- 非授权连接被拒绝并记录日志。

## Phase 2 取舍和优先级

优先做：

- xrdp 运行诊断：补齐启动路径、配置路径、监听端口、active mstsc session、断连原因、frame/input counters 和 HiLog 关键日志。这些只涉及 xrdp/HAP bridge，不受 OHOS 系统接口限制。
- 屏幕采集链路：继续基于 `AVScreenCapture`，补帧率控制、分辨率同步、脏区/region 更新、队列背压、丢帧策略、cursor 策略和采集异常日志。`OH_NativeDisplayManager_CaptureScreenPixelmap` 只适合截图兜底，不适合作为高帧率远控主链路。
- 图像输出链路：先稳定 BGRA/RGBA + bitmap update + dirty rect，再评估 `rfxcodec`、OpenH264/x264 或 OHOS `AVCodec`。当前 build 关闭了 `rfxcodec/openh264/x264`，恢复这些属于编解码和包依赖工作，不是 OHOS 缺接口。
- 输入稳定性：继续完善 keysym/scancode、组合键、修饰键、滚轮、鼠标按键、移动合并、队列背压、失败重试和断连释放按键。输入注入的系统授权弹框不在这一项内解决。
- 剪贴板文本通道：xrdp 协议层能收发 channel data，OHOS 有 Pasteboard NDK；先做 `cliprdr` 文本双向同步，再扩展 HTML、图片、URI。
- 安全控制：补访问 token、一次性码、IP allowlist、会话确认、TLS cert/key 管理和调试/生产配置隔离。

可以做但暂缓：

- RDPGFX/H264：OHOS 有视频编码接口，但 xrdp server 侧要补 RDPGFX/H264 packetization 和编码器适配，投入高，等基础帧链路稳定后再做。
- 音频：`rdpsnd` 已由 OHOS backend 采集设备播放音频并发送给 Windows；`audin` 已按标准 MS-RDPEAI / `AUDIO_INPUT` 动态通道接入，将 Windows 客户端重定向的麦克风 PCM 交给 OHAudio Renderer 在 OHOS 端播放。两者都复用 xrdp 通用协议语义，只把平台采集/播放接口适配为 OHOS；`audin` 已通过 OHOS arm64 干净交叉编译、链接及 MSTSC + OHOS 真机数据闭环验收。
- 文件/磁盘重定向：xrdp 原生 devredir/FUSE 路线依赖 Linux FUSE，不适合 OHOS app 环境；如需要，只做 app sandbox 内的虚拟文件通道。
- 多显示器：DisplayManager 能查询显示信息，但采集、坐标注入和 RDP multimon 映射都要单独适配，先保持单显示器稳定。

当前不做：

- 不接原生 `chansrv`。`chansrv` 依赖 sesman、X11、PulseAudio/Unix socket、FUSE 等 Linux 桌面会话环境；OHOS 需要按 `cliprdr`、`rdpsnd/audin`、`rdpdr` 分别写 backend。
- 不承诺键鼠注入免远程协助弹框。`OH_Input_RequestInjection` 属于系统授权链路，当前按系统策略处理。
- 不绕过屏幕采集隐私策略，也不捕获隐私窗口内容。`AVScreenCapture` 相关 picker/strategy 只能按系统允许范围配置。
- 不复用 Xorg/Xvnc/PAM/systemd/PulseAudio/CUPS/FUSE 这类 Linux 桌面能力。

建议执行顺序：

1. 运行诊断、日志和配置一致性。
2. 屏幕采集帧链路和 dirty rect。
3. 输入映射稳定性，不处理免弹框。
4. `cliprdr` 文本双向剪贴板。
5. 视产品需要再评估 RDPGFX/H264、音频、文件重定向和多显示器。

## 当前状态

- 本地 base 已更新到 `origin/main`。
- xrdp OHOS 交叉构建通过。
- `libxrdpserver.so` 已导出嵌入入口和 stop 入口。
- `xrdp.hnp` 已能打包。
- `build_hap.bat` 已能产出带 HNP、xrdp native libs 和 xrdp runtime config/share 的签名 HAP：
  `harmony/app/entry/build/default/outputs/default/entry-default-signed.hap`。
- 下一步是设备侧点 `Probe xrdp` / `Start xrdp`，确认实际展开路径、监听端口和 mstsc 连接结果。

## 9. 2026-08-05 `audin` 适配记录

- xrdp core/module ABI 新增通用动态虚拟通道桥，core 继续负责 MS-RDPEDYC 分帧、通道 ID 和回调路由，OHOS backend 不实现产品私有协议。
- OHOS backend 按 xrdp `chansrv/audin.c` 的 MS-RDPEAI 流程实现 `VERSION -> FORMATS -> OPEN -> DATA`，支持 PCM16 单/双声道的 8 kHz、16 kHz、44.1 kHz、48 kHz 协商子集。
- 平台层仅使用 OHAudio Renderer、有限环形缓冲、静音补帧和溢出丢旧帧策略，并记录 open/packet/byte/drop/underrun/error 诊断。
- 默认配置为 `[OHOS] audin=true`，依赖 `[Channels] drdynvc=true`。
- `wsl bash harmony/scripts/wsl/build-xrdp-ohos.sh` 干净构建、安装和产物符号校验通过；状态为“代码、构建及 MSTSC + OHOS 真机数据闭环验收完成”。
- `rdpecam` 摄像头重定向已完成首版代码和构建验收；带摄像头 MSTSC 客户端的数据闭环尚未验收，不标记为 `Verified`。
- 2026-08-05 首轮真机验证：`HAD-W32` 2in1（`3QC0124C11000711`）完成 HAP 覆盖安装、启动和屏幕录制授权，但 `libxrdpohos.so` 首次加载失败；HiLog 记录 C++ renderer 将 C `log_message` 错误解析为 `_Z11log_message9logLevelsPKcz`，并同时提示 `libimage_ndk.z.so` namespace 加载警告。此轮尚未进入 `AUDIO_INPUT` 协商，状态保持“真机待验收”，修复动态链接后重测。
- 动态链接修复后，xrdp 可监听 3390 且 MSTSC 显示、输入、H.264、`rdpsnd` 会话正常；显式使用 `audiocapturemode:i:1` 重连仍无 `audin` open。时序证据显示 MS-RDPEDYC capability 在 backend module 加载前完成，原 ready 通知因 module 尚不存在而丢失。通用 DVC bridge 需保存 ready 状态，并在 module 晚加载后补发且每个 module 实例只通知一次。
- 通用 DVC bridge 保存并向晚加载 module 补发 ready 状态后，真机重测通过：MSTSC 打开 `AUDIO_INPUT` 通道，协商 `48 kHz / mono / PCM16`，OHOS OHAudio Renderer 成功启动；会话收到 683 包、1,523,090 字节，`dropped=0`、`errors=0`，renderer 记录 `pushed=1,523,090`。`audin` 状态更新为“真机数据闭环已验收”。

## 10. `rdpecam` 摄像头重定向设计与实施台账

### 10.1 设计状态

- Change ID：`XRDP-OHOS-RDPECAM-001`
- 状态：`Implemented / BuildVerified / DeviceDataVerified / FaultCasesPending`
- 目标：Windows `mstsc` 作为 RDP 客户端连接 OHOS xrdp 服务端时，将 Windows 侧摄像头通过标准 MS-RDPECAM 动态虚拟通道送入 OHOS 应用进程。
- 协议依据：微软 MS-RDPECAM；协议消息布局和服务端时序对照仓库内 FreeRDP `channels/rdpecam/server` 实现；DVC 创建、分片和回调继续使用 xrdp `server_drdynvc_*` module ABI。

### 10.2 能力边界

- 首版支持 `RDCamera_Device_Enumerator`、设备增删通知、单个活动摄像头设备通道、流/媒体类型协商、启动/停止和按请求拉取样本。
- 首版通过版本化 `xrdp_ohos` C ABI 把设备事件和样本同步回调给 HAP native 层。样本内存只在回调期间有效，HAP 如需异步处理必须自行复制。
- HAP 默认 sink 只记录设备、媒体格式、样本数和字节数，用于真机验收；后续业务预览、编码或 AI 消费复用同一 callback，不进入 xrdp 协议模块。
- 这不是 OHOS 系统级“虚拟摄像头”注册。OHOS 浏览器或普通应用直接调用 `getUserMedia` / CameraKit 时仍会取得 OHOS 本机摄像头，除非业务显式接入上述 callback。
- 首版不做多摄像头同时采集、摄像头属性控制、后台静默启用或绕过 Windows 的资源重定向选择。设备移除或会话断开必须立即停止请求并释放 DVC 状态。

### 10.3 状态机和安全限制

1. xrdp `drdynvc` ready 后，OHOS backend 打开 `RDCamera_Device_Enumerator`。
2. 收到 `SelectVersionRequest` 后，对当前 FreeRDP/MSTSC 使用的 MS-RDPECAM v1-v2 返回同版本 `SelectVersionResponse`；版本为 0 或高于 v2 时关闭通道并记录错误。
3. 收到 `DeviceAddedNotification` 后保存设备名和客户端给出的设备 DVC 名；若无活动设备则打开该 DVC。
4. 设备 DVC 打开后依次发送 `ActivateDeviceRequest`、`StreamListRequest`、`MediaTypeListRequest`，选择可接受媒体类型后发送 `StartStreamsRequest`。
5. 启动成功后仅保持一个在途 `SampleRequest`；每个 `SampleResponse` 经边界检查后同步投递 callback，再请求下一帧。
6. `DeviceRemovedNotification`、DVC close、session disconnect 或 module exit 进入停止态，清除分片、活动格式、在途请求和设备标识。

边界检查：DVC PDU 最大 32 MiB；设备名和 DVC 名必须以 NUL 结束并有长度上限；宽高不超过 8192；帧率分母不能为 0；单帧大小不得超过 32 MiB；未知消息、越界长度和不一致的 stream index 只终止摄像头子通道，不影响桌面会话。

媒体类型选择优先级为 `MJPG -> NV12 -> YUY2 -> I420 -> RGB32 -> RGB24 -> H264`，同格式优先不超过 1920x1080、30 fps 且分辨率最高的条目；没有满足软上限的条目时退到客户端提供的第一个合法条目。callback 接收协议原始样本及完整媒体元数据，不在协议层做私有转码。

### 10.4 文件级修改项

- `harmony/third_party/xrdp/ohos/ohos_rdpecam.[ch]`：MS-RDPECAM DVC 生命周期、设备选择和统计。
- `harmony/third_party/xrdp/ohos/ohos_rdpecam_protocol.c`、`ohos_rdpecam_internal.h`：协议 PDU 解析/生成、状态转换和媒体格式选择；拆分后单个实现文件保持在 1000 行硬限制以内。
- `harmony/third_party/xrdp/ohos/ohos.c`、`ohos_private.h`、`ohos_module_callbacks.c`：模块装配、配置、connect/disconnect 和 DVC ready。
- `harmony/third_party/xrdp/ohos/xrdp_ohos.h`、`ohos_events.c`：新增版本化摄像头事件/样本 callback 和 feature flag，不向 N-API 泄漏协议状态机。
- `harmony/third_party/xrdp/ohos/Makefile.am`、`harmony/third_party/xrdp/xrdp/xrdp-ohos.ini.in`：构建源和默认 `rdpecam=true`。
- `harmony/app/entry/src/main/cpp/xrdp/*`、`napi/napi_exports.cpp`、`napi/napi_utils.[ch]`、ArkTS 类型声明：动态加载可选 callback、记录 64 位统计并通过既有 xrdp diagnostics 返回验收数据。
- `harmony/scripts/wsl/build-xrdp-ohos.sh`：构建验收同时检查 rdpecam 回调 ABI 与默认配置。

### 10.5 验收项

- `XRDP-CAM-A01`：无摄像头或未勾选重定向时，enumerator DVC 可正常打开但无 `DeviceAdded`，也允许客户端直接拒绝该 DVC；两种结果均可诊断，桌面、声音、输入和剪贴板不回归。
- `XRDP-CAM-A02`：带摄像头 Windows 设备勾选“相机和其他视频捕获设备”后，日志出现版本协商、设备名、设备 DVC、stream 和选中媒体类型。
- `XRDP-CAM-A03`：远端实际请求开始后，diagnostics 的 sample/byte 计数持续增长，callback 获得非空数据且 `format/width/height/stream_index` 与协商结果一致。
- `XRDP-CAM-A04`：Windows 禁止权限、摄像头被占用、设备拔出时仅关闭摄像头子通道并给出错误；xrdp 主会话保持可用。
- `XRDP-CAM-A05`：连续连接/断开三次，无残留活动 DVC、分片或在途 sample；断连摘要中的 open/device/sample/byte/error 计数正确。
- `XRDP-CAM-A06`：OHOS arm64 干净交叉构建、HAP 构建和符号检查通过；真机证据未完成前状态不得改为 `Verified`。

### 10.6 实施证据

- 2026-08-06：10.4 所列代码已实现；`ohos_rdpecam.c` 为 945 行，低于单文件 1000 行硬限制。
- 2026-08-06 首次真机协商发现 Windows 11 MSTSC 使用协议版本 v2，而初版只接受 v1；仓库内 FreeRDP `ECAM_PROTO_VERSION` 同为 v2，且客户端允许服务端选择不高于 v2 的版本，因此实现调整为兼容 v1-v2 后继续重测。
- `wsl bash -lc 'cd /mnt/c/Users/mu/Desktop/code/demo && bash harmony/scripts/wsl/build-xrdp-ohos.sh'` 完整 OHOS arm64 交叉构建、安装和产物检查通过；`libxrdpohos.so` 导出 `xrdp_ohos_backend_set_rdpecam_callback`，安装配置包含 `rdpecam=true`。
- `cmd /c harmony\app\build_hap.bat` 通过，最终签名 HAP 为 `harmony/app/entry/build/default/outputs/default/entry-default-signed.hap`。
- 真机 `3QC0124C11000711` 覆盖安装成功；首次自动拉起曾被设备锁屏拒绝（`10106102`），解锁后已继续完成下述协议和负向路径验证。
- 设备解锁后 HAP 启动成功，xrdp 在 `0.0.0.0:3390` 监听；HDC `tcp:13390 -> tcp:3390` 与 TLS MSTSC 会话建立成功。日志依次出现 `enumerator open requested id=4`、`enumerator channel ready`、`protocol version selected=2`，证明 HAP 已加载新版 callback ABI 且 DVC 协议链路可用。
- Windows 主机 `DESKTOP-2RKE2O5` 的 `Camera` / `Image` PnP 设备枚举为空。使用 `camerastoredirect:s:*` 和显式空值分别连接时，MSTSC 都建立 enumerator v2，但不发送 `DeviceAdded`，符合无摄像头负向路径；桌面、H.264、输入、`audin` 等主链路保持工作。
- 连续 3 次 MSTSC 连接/断开均输出 `enumerator_opens=1 devices_added=0 devices_removed=0 device_opens=0 streams=0 samples=0 bytes=0 sample_errors=0 errors=0`，断开后 3390 继续监听；A01 和无设备条件下的 A05 通过，带摄像头数据结果见下。
- 带摄像头 Windows `192.168.43.36`（客户端名 `DESKTOP-B9D1U6R`）直连 OHOS `192.168.43.38:3390` 验收通过：枚举到 `Integrated Webcam_FHD` / `RDCamera_Device_0`，选择 MS-RDPECAM v2、stream 0 和 H.264 `1920x1080@30`（6 个候选媒体类型），随后进入 `stream started`。
- 真帧持续增长至至少 1500 帧、27,525,668 字节；协议日志在 sample 1/2/3/300/600/900/1200/1500 处持续输出，HAP `FreeRDPBridge` callback 日志在对应采样时间同步出现，证明样本已穿过 xrdp backend callback 到达 HAP native 层。A02、A03 通过；Windows 权限拒绝、摄像头占用和热拔出等 A04 故障场景仍待补测，因此总状态暂不标为完整 `Verified`。
