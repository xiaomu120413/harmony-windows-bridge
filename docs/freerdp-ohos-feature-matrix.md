# FreeRDP OHOS Feature Matrix

更新时间：2026-05-25

## 结论

当前 HarmonyOS 交付 profile 已经能完整交叉编译并打包：

- 基础 RDP、TLS/NLA、WinPR、OpenSSL、zlib、cJSON。
- client channels：`cliprdr`、`drdynvc`、`disp`、`rdpgfx`、`rdpsnd`、`audin`、`rdpdr`、`drive`、`printer`。运行时默认只打开基础桌面、display-control、RDPGFX 和音频播放；剪贴板、麦克风、drive、printer 默认关闭。
- 软件编解码与硬解合成：FFmpeg、OpenH264、SWSCALE、OHOS AVCodec-backed AVC444 GPU compositor。
- 音频短期验证后端：FreeRDP OpenSLES backend + OHOS NDK OpenSLES 兼容 shim。
- 首版交付不编译 FreeRDP smartcard source/channel、WinPR smartcard PCSC backend、TSMF，避免把未闭环的平台服务和 deprecated 视频路径带进包。

仍不能直接在当前 OHOS NDK sysroot 下打开的后端：

- CUPS printer backend：缺 `CUPS_LIBRARIES` 和 `CUPS_INCLUDE_DIR`。
- FUSE clipboard file-copy backend：缺 `fuse3` pkg-config 包。
- smartcard source/channel/PCSC：从交付 profile 裁剪，不再作为默认矩阵编译项。
- TSMF：从交付 profile 裁剪；后续视频路线优先走 RDPGFX/H.264 或 OHOS AVCodec。

## 为什么没调用 HarmonyOS API 也能编译

FreeRDP 的 channel 大多是协议层 C 代码，编译时只需要 C/C++ 编译器、POSIX/WinPR 抽象和第三方库。它们是否真正可用，取决于运行时有没有接平台后端：

- `cliprdr` 能编译，不代表已经接了 Harmony clipboard API。
- `drive` 能编译，不代表已经接了 Harmony 文件选择器、沙箱授权和路径映射。
- `printer` channel 能编译，不代表已经有 CUPS 或 Harmony 打印后端。
- `smartcard` 被裁剪，不代表协议层永远不支持；只是首版没有 PC/SC 服务、权限、读卡器交互和验收闭环。
- `rdpsnd/audin` 能编译，不代表音频焦点、路由、采集权限、缓冲生命周期已经产品化。

所以当前状态是“首版交付需要的协议和可编译后端已经进包”，下一步是“接 HarmonyOS 运行时 API 并真机验证”。smartcard source/channel/PCSC 和 TSMF 不进入首版包。

## 编译矩阵

| 能力 | 编译状态 | 当前打包状态 | 运行时遗留 |
| --- | --- | --- | --- |
| 剪贴板文本 `cliprdr` | 通过 | 已进 HAP，默认关闭 | 主界面开关启用后才注册 cliprdr bridge 和触发 Pasteboard 读取权限 |
| 剪贴板文件/FUSE | 失败 | 未进 HAP | `WITH_FUSE=ON` 当前缺 `fuse3`；普通应用沙箱下也不建议直接暴露任意路径 |
| 音频播放 `rdpsnd` | 通过 | 已进 HAP，默认开启 | 通过主界面声音开关显式控制；OHAudio 后端需持续真机回归 |
| 麦克风 `audin` | 通过 | 已进 HAP，默认关闭 | 主界面麦克风开关启用后才注册 audin 和触发麦克风权限 |
| 文件重定向 `rdpdr/drive` | 通过 | 已进 HAP，默认关闭 | 需要 UI 选择共享目录、沙箱权限、只读/读写策略和路径脱敏 |
| 打印 channel `printer` | 通过 | 已进 HAP，默认关闭 | channel 已有；CUPS backend 当前不能配置，后续接 Harmony Print 或移植 CUPS |
| CUPS printer backend | 失败 | 未进 HAP | 缺 CUPS headers/libs；即使移植也要评估普通应用权限和打印服务模型 |
| 智能卡 source/channel `smartcard` | 关闭 | 未进 HAP | 首版不交付；恢复时需要独立开关、PC/SC 服务/权限模型和真机读卡验收 |
| WinPR smartcard PCSC backend | 关闭 | 未进 HAP | 产品构建硬关 OFF，避免运行时 `dlopen` PCSC 和合规能力不闭环 |
| RD Gateway core | 通过 | 依赖已进 HAP | 需要 UI 参数、settings 映射、证书/代理错误提示和服务器验证 |
| RDPGFX/H.264 + AVC444 GPU compositor | 通过 | FFmpeg/OpenH264/OHOS AVCodec 已进 HAP | 商用默认 `rdpgfx-h264` 并默认启用 AVC444 GPU compositor；单条 command 失败时保留 FreeRDP native GDI fallback |
| TSMF | 关闭 | 未进 HAP | FreeRDP 标注 deprecated，首版裁剪；视频路线优先 RDPGFX/H.264 或 OHOS AVCodec |

## 验收基线

T00 已把后续任务的可重复验收口径整理到 `docs/freerdp-ohos-validation-baseline.md`。本矩阵继续记录 feature 状态；构建命令、runtime 同步命令、HAP 产物路径和真机最小回归清单以基线文档为准。

当前基线：

- 主仓库提交：`b449ff223262c7605dc183bbb78cf48ac1a2b113`
- FreeRDP 子模块提交：`d00af99d5d6abddc9e6daf46a738a18ee656e949`
- FreeRDP 标识：`3.26.0-135-gd00af99d5`

## 本轮验证命令

```bash
harmony/scripts/wsl/check-freerdp-ohos-feature-matrix.sh
harmony/scripts/wsl/build-freerdp-ohos.sh
```

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\harmony\scripts\windows\sync-freerdp-runtime.ps1
```

HAP 构建使用 `harmony/app/build_hap.bat`，目标产物：

```text
harmony/app/entry/build/default/outputs/default/entry-default-signed.hap
```

每个 T01-T18 任务完成后，应在任务说明中明确是否已覆盖 FreeRDP build、runtime sync、HAP build 和真机检查。未覆盖的检查不能默认为通过。

## 真机验证清单

下一次连接真机后按这个顺序测：

1. 启动 App，确认 `probe()` 能看到 FreeRDP runtime、client channel loader、FFmpeg/OpenH264/OpenSLES，并显示 smartcard/TSMF excluded。
2. 连接 Windows，确认基础画面、鼠标、键盘仍正常。
3. 观察 RDPGFX/H.264 是否被协商，并确认 AVC444 GPU compositor 日志为默认开启；如果没有，记录服务端能力和 FreeRDP 日志。
4. 默认关闭剪贴板连接一次，确认不会加载 `cliprdr`、不会申请 Pasteboard 权限；再打开剪贴板开关验证文本同步。
5. 播放 Windows 系统声音，观察 `rdpsnd` 日志、延迟、断连和后台行为；关闭声音开关后确认不加载 `rdpsnd`。
6. 默认关闭麦克风连接一次，确认不会加载 `audin`、不会申请麦克风权限；打开麦克风开关后再验证权限和采集路径。
7. 启用一个只读共享目录，验证 `drive` runtime settings、路径选择和权限策略。
8. 检查构建 manifest：`with_smartcard=OFF`、`with_smartcard_pcsc=OFF`，运行包内不应出现 smartcard/TSMF addin。
9. 填 RD Gateway 参数，验证 settings 映射和失败提示。

## 影响

- 当前 signed HAP 为 33,320,512 bytes，约 31.78 MiB；相对裁剪前工作区基线 88.27 MiB 减少约 56.49 MiB。
- smartcard source/channel/PCSC 和 TSMF 不进入包，减少未闭环平台服务和 deprecated 通道带来的商业验收风险。
- CUPS/FUSE 不进入包，避免把当前无法闭合的 Linux 服务模型带进普通 HarmonyOS 应用。
