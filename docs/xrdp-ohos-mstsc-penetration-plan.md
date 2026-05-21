# xrdp OHOS MSTSC 穿刺方案

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

## 当前状态

- 本地 base 已更新到 `origin/main`。
- xrdp OHOS 交叉构建通过。
- `libxrdpserver.so` 已导出嵌入入口和 stop 入口。
- `xrdp.hnp` 已能打包。
- `build_hap.bat` 已能产出带 HNP、xrdp native libs 和 xrdp runtime config/share 的签名 HAP：
  `harmony/app/entry/build/default/outputs/default/entry-default-signed.hap`。
- 下一步是设备侧点 `Probe xrdp` / `Start xrdp`，确认实际展开路径、监听端口和 mstsc 连接结果。
