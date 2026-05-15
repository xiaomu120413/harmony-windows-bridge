# FreeRDP OHOS Feature Matrix

更新时间：2026-05-15

## 结论

当前 HarmonyOS 增强 profile 已经能完整交叉编译并打包：

- 基础 RDP、TLS/NLA、WinPR、OpenSSL、zlib、cJSON。
- client channels：`cliprdr`、`drdynvc`、`disp`、`rdpgfx`、`rdpsnd`、`audin`、`rdpdr`、`drive`、`printer`、`smartcard`、`tsmf`。
- 软件编解码：FFmpeg、OpenH264、SWSCALE。
- 音频短期验证后端：FreeRDP OpenSLES backend + OHOS NDK OpenSLES 兼容 shim。
- 智能卡短期验证后端：WinPR `smartcard_pcsc.c` 已编译；它运行时动态加载 `libpcsclite.so.1`/`libpcsclite.so`，不是链接期依赖。

仍不能直接在当前 OHOS NDK sysroot 下打开的后端：

- CUPS printer backend：缺 `CUPS_LIBRARIES` 和 `CUPS_INCLUDE_DIR`。
- FUSE clipboard file-copy backend：缺 `fuse3` pkg-config 包。
- CUPS + PCSC + FUSE 全开：因 CUPS 先失败，整体配置失败。

## 为什么没调用 HarmonyOS API 也能编译

FreeRDP 的 channel 大多是协议层 C 代码，编译时只需要 C/C++ 编译器、POSIX/WinPR 抽象和第三方库。它们是否真正可用，取决于运行时有没有接平台后端：

- `cliprdr` 能编译，不代表已经接了 Harmony clipboard API。
- `drive` 能编译，不代表已经接了 Harmony 文件选择器、沙箱授权和路径映射。
- `printer` channel 能编译，不代表已经有 CUPS 或 Harmony 打印后端。
- `smartcard_pcsc.c` 能编译，不代表真机上有 PC/SC 服务或 `libpcsclite.so`。
- `rdpsnd/audin` 能编译，不代表音频焦点、路由、采集权限、缓冲生命周期已经产品化。

所以当前状态是“协议和可编译后端已经进包”，下一步是“接 HarmonyOS 运行时 API 并真机验证”。

## 编译矩阵

| 能力 | 编译状态 | 当前打包状态 | 运行时遗留 |
| --- | --- | --- | --- |
| 剪贴板文本 `cliprdr` | 通过 | 已进 HAP | 需要接 Harmony clipboard API，处理文本方向、编码、焦点和权限提示 |
| 剪贴板文件/FUSE | 失败 | 未进 HAP | `WITH_FUSE=ON` 当前缺 `fuse3`；普通应用沙箱下也不建议直接暴露任意路径 |
| 音频播放 `rdpsnd` | 通过 | 已进 HAP | OpenSLES 路径需真机验证；产品化建议新增 AudioRenderer 后端 |
| 麦克风 `audin` | 通过 | 已进 HAP | 需要 AudioCapturer 后端、麦克风权限、隐私提示和后台策略 |
| 文件重定向 `rdpdr/drive` | 通过 | 已进 HAP | 需要 UI 选择共享目录、沙箱权限、只读/读写策略和路径脱敏 |
| 打印 channel `printer` | 通过 | 已进 HAP | channel 已有；CUPS backend 当前不能配置，后续接 Harmony Print 或移植 CUPS |
| CUPS printer backend | 失败 | 未进 HAP | 缺 CUPS headers/libs；即使移植也要评估普通应用权限和打印服务模型 |
| 智能卡 channel `smartcard` | 通过 | 已进 HAP | channel 已有 |
| WinPR smartcard PCSC backend | 通过 | 已编进 `libwinpr3.so` | 运行时会 `dlopen` PCSC 库；真机需确认系统是否有 `libpcsclite`，否则要随包移植或写 OHOS 后端 |
| RD Gateway core | 通过 | 依赖已进 HAP | 需要 UI 参数、settings 映射、证书/代理错误提示和服务器验证 |
| RDPGFX/H.264 | 通过 | FFmpeg/OpenH264 已进 HAP | 先软件验证稳定性；硬解要新增 OHOS AVCodec subsystem |
| TSMF | 通过 | 已进 HAP | FreeRDP 标注 deprecated，不建议作为首选视频路线 |

## 本轮验证命令

```bash
harmony/scripts/wsl/check-freerdp-ohos-feature-matrix.sh
harmony/scripts/wsl/build-freerdp-ohos.sh
```

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\harmony\scripts\windows\sync-freerdp-runtime.ps1
```

HAP 构建使用 HarmonyOS MCP clean build，目标产物：

```text
harmony/app/entry/build/default/outputs/default/entry-default-signed.hap
```

## 真机验证清单

下一次连接真机后按这个顺序测：

1. 启动 App，确认 `probe()` 能看到 FreeRDP runtime、client channel loader、FFmpeg/OpenH264/OpenSLES。
2. 连接 Windows，确认基础画面、鼠标、键盘仍正常。
3. 观察 RDPGFX/H.264 是否被协商；如果没有，记录服务端能力和 FreeRDP 日志。
4. 远端复制文本，本机读取；本机复制文本，远端粘贴，验证 `cliprdr` 接线缺口。
5. 播放 Windows 系统声音，观察 `rdpsnd` 日志、延迟、断连和后台行为。
6. 如果设备允许麦克风权限，验证 `audin` 是否需要新增采集后端。
7. 启用一个只读共享目录，验证 `drive` runtime settings、路径选择和权限策略。
8. 验证 smartcard 打开后是否能加载 `libpcsclite.so`；如果失败，记录 dlopen 错误。
9. 填 RD Gateway 参数，验证 settings 映射和失败提示。

## 影响

- HAP 体积会上升，当前 signed HAP 约 74 MB。
- `libwinpr3.so` 编入 PCSC 代码后略增大，但不会新增链接期 PCSC 依赖。
- CUPS/FUSE 不进入包，避免把当前无法闭合的 Linux 服务模型带进普通 HarmonyOS 应用。
