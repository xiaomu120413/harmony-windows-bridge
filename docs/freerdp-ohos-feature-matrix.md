# FreeRDP OHOS Feature Matrix

更新时间：2026-05-26

## 结论

当前 HarmonyOS 交付 profile 已经能完整交叉编译并打包：

- 基础 RDP、TLS/NLA、WinPR、OpenSSL、zlib、cJSON。
- client channels：`cliprdr`、`drdynvc`、`disp`、`location`、`rdpgfx`、`rdpsnd`、`audin`、`rdpdr`、`drive`、`printer`。首版交付包含剪贴板文本、麦克风采集、地理位置重定向和 OHOS PrintKit 打印后端；剪贴板/麦克风权限保持按需申请，不在连接开始时主动弹权限；地理位置默认注册 channel，服务端发起 `LocationStart` 时申请定位权限。`printer` 默认暴露一个虚拟打印机，但只在 Windows 提交打印作业时初始化/连接 PrintKit；`drive` 代码可编译但首版默认不请求。
- 软件编解码与硬解合成：FFmpeg、OpenH264、SWSCALE、OHOS AVCodec-backed AVC444 GPU compositor。
- 音频短期验证后端：FreeRDP OpenSLES backend + OHOS NDK OpenSLES 兼容 shim。
- RD Gateway core 已编译进 FreeRDP runtime，但当前 HAP 没有 UI 参数、N-API 参数和 settings 映射；在没有 RD Gateway 服务端环境前不计入已适配能力。
- 首版交付不编译 FreeRDP smartcard source/channel、WinPR smartcard PCSC backend、TSMF，避免把未闭环的平台服务和 deprecated 视频路径带进包。

仍不能直接在当前 OHOS NDK sysroot 下打开的后端：

- CUPS printer backend：缺 `CUPS_LIBRARIES` 和 `CUPS_INCLUDE_DIR`；交付路径改走 OHOS PrintKit backend。
- FUSE clipboard file-copy backend：缺 `fuse3` pkg-config 包。
- smartcard source/channel/PCSC：从交付 profile 裁剪，不再作为默认矩阵编译项。
- TSMF：从交付 profile 裁剪；后续视频路线优先走 RDPGFX/H.264 或 OHOS AVCodec。

## 为什么没调用 HarmonyOS API 也能编译

FreeRDP 的 channel 大多是协议层 C 代码，编译时只需要 C/C++ 编译器、POSIX/WinPR 抽象和第三方库。它们是否真正可用，取决于运行时有没有接平台后端：

- `cliprdr` 能编译，不代表已经接了 Harmony clipboard API。
- `drive` 能编译，不代表已经接了 Harmony 文件选择器、沙箱授权和路径映射。
- `printer` channel 能编译，不代表已经有运行时打印能力；当前交付通过 OHOS PrintKit backend 补齐，CUPS 路径仍不可用。
- `smartcard` 被裁剪，不代表协议层永远不支持；只是首版没有 PC/SC 服务、权限、读卡器交互和验收闭环。
- `rdpsnd/audin` 能编译，不代表音频焦点、路由、采集权限、缓冲生命周期已经产品化。
- `location` 已接 OHOS LocationKit native API；ETS/HAP 层只负责定位权限申请，不负责采样和 RDP PDU 语义。

所以当前状态是“首版交付需要的协议和可编译后端已经进包”，下一步是“接 HarmonyOS 运行时 API 并真机验证”。smartcard source/channel/PCSC 和 TSMF 不进入首版包。

## 编译矩阵

| 能力 | 编译状态 | 当前打包状态 | 运行时遗留 |
| --- | --- | --- | --- |
| 剪贴板文本 `cliprdr` | 通过 | 已进 HAP，首版交付 | 实际读取 Harmony Pasteboard 时才通过 callback 申请权限 |
| 剪贴板文件/FUSE | 失败 | 未进 HAP | `WITH_FUSE=ON` 当前缺 `fuse3`；普通应用沙箱下也不建议直接暴露任意路径 |
| 音频播放 `rdpsnd` | 通过 | 已进 HAP | OHAudio 后端需持续真机回归 |
| 麦克风 `audin` | 通过 | 已进 HAP，首版交付 | 远端实际请求采集时才通过 callback 申请麦克风权限 |
| 地理位置 `location` | 通过 | 已进 HAP，首版交付 | 默认注册 channel；服务端发起 `LocationStart` 时通过 callback 申请定位权限，FreeRDP OHOS 后端用 LocationKit 采样并发送 PDU |
| 文件重定向 `rdpdr/drive` | 通过 | 库内可用，首版默认不请求 | 需要 UI 选择共享目录、沙箱权限、只读/读写策略和路径脱敏 |
| 打印 channel `printer` | 通过 | 已进 HAP，按远端打印作业按需启动 PrintKit | 默认只向 Windows 暴露一个虚拟打印机；远端提交作业后才初始化 PrintKit、查询/连接实际打印机并提交作业；CUPS backend 仍不可用 |
| CUPS printer backend | 失败 | 未进 HAP | 缺 CUPS headers/libs；即使移植也要评估普通应用权限和打印服务模型 |
| 智能卡 source/channel `smartcard` | 关闭 | 未进 HAP | 首版不交付；恢复时需要独立开关、PC/SC 服务/权限模型和真机读卡验收 |
| WinPR smartcard PCSC backend | 关闭 | 未进 HAP | 产品构建硬关 OFF，避免运行时 `dlopen` PCSC 和合规能力不闭环 |
| RD Gateway core | 通过 | core 依赖已进 HAP，HAP 参数链路未接入 | 当前不作为可用功能；需 RD Gateway 服务端环境后再接 UI 参数、settings 映射、证书/代理错误提示并验收 |
| RDPGFX/H.264 + AVC444 GPU compositor | 通过 | FFmpeg/OpenH264/OHOS AVCodec 已进 HAP | 商用默认 `rdpgfx-h264` 并默认启用 AVC444 GPU compositor；单条 command 失败时保留 FreeRDP native GDI fallback |
| TSMF | 关闭 | 未进 HAP | FreeRDP 标注 deprecated，首版裁剪；视频路线优先 RDPGFX/H.264 或 OHOS AVCodec |

## 商用状态矩阵

| 能力 | 商用状态 | 默认策略 | 需要权限 | 真机验证状态 |
| --- | --- | --- | --- | --- |
| 基础 RDP/TLS/NLA | 首版交付 | 默认启用 | `INTERNET`、网络状态 | 待按 T00 基线复测连接、认证、证书 |
| RDPGFX/H.264 + AVC444 GPU compositor | 首版交付 | 默认 `rdpgfx-h264`，AVC444 GPU compositor 开启 | 无新增权限 | 待真机确认协商、首帧、resize、fallback |
| GDI/software render | 保留 fallback | 图形失败时可回退 | 无新增权限 | 待真机确认失败场景不黑屏 |
| 动态分辨率 `disp` | 首版交付 | 默认请求 | 无新增权限 | 待真机确认 resize 和服务端不支持提示 |
| 剪贴板文本 `cliprdr` + Pasteboard | 首版交付 | 默认接入，按需授权 | `READ_PASTEBOARD` | 待真机确认双向文本、拒绝权限和 change echo |
| 剪贴板文件/FUSE | 首版不交付 | 不编译 FUSE backend | 不声明额外文件权限 | 当前依赖缺失，后续专项 |
| 音频播放 `rdpsnd` | 首版交付 | 默认接入 OHAudio/OpenSLES backend | 无新增权限 | 待真机确认延迟、断连、前后台 |
| 麦克风 `audin` | 首版交付 | 默认接入，远端请求采集时按需授权 | `MICROPHONE` | 待真机确认授权、拒绝、采集路径 |
| 地理位置 `location` | 首版交付 | 默认接入，远端请求定位时按需授权 | `APPROXIMATELY_LOCATION`、`LOCATION` | 已完成本地构建和真机安装；仍需远端策略、授权/拒绝和服务端接收回归 |
| 文件重定向 `rdpdr/drive` | 可选，首版默认关闭 | 不主动请求共享目录 | 首版不声明额外文件权限 | 未产品化；后续需 UI 和沙箱授权闭环 |
| 打印 `printer` channel | 可选，已接入 OHOS 后端 | 默认暴露虚拟打印机；PrintKit 在远端打印作业到达时按需启动 | `PRINT` | 已真机触发 Harmony PDF Printer/CUPS job 成功；仍需真实打印机选择、失败提示和多设备回归 |
| RD Gateway core | 后续专项，需服务端环境 | 当前不启用；有 RD Gateway 服务器后再接 UI 参数和 settings 映射 | 复用网络权限 | 未验收；无 RD Gateway 服务器时不能判定可用 |
| Smartcard source/channel/PCSC | 首版不交付 | 交付构建裁剪 | 不声明智能卡相关权限 | 不进包 |
| TSMF | 首版不交付 | 交付构建裁剪 | 无 | 不进包 |

## Fallback 清单

| Fallback | 是否保留 | 触发条件 | 恢复路径 | 验收口径 |
| --- | --- | --- | --- | --- |
| 缺 FreeRDP headers/runtime 的 demo/mock fallback | 不保留在商用构建 | 构建或启动缺核心 headers/`.so` | fail-fast，提示缺失产物 | 商用 profile 不允许假装可运行 |
| `rdpgfx-h264` 到 `rdpgfx`/`gdi` | 保留 | 图形协商、codec、surface 或 compositor 路径失败 | 只在图形失败时重试下一档 | 密码、证书、TCP 失败不能触发图形 fallback |
| AVC444 GPU compositor 到 FreeRDP native GDI | 保留 | 单条 AVC444 command 不可消费或 Surface 不可用 | 该 command 交回 native GDI/RGB 输出 | 不允许 GPU/GDI 双写、旧帧或黑屏 |
| `disp` resize 失败到固定分辨率 | 保留 | 服务端不支持 display-control 或 caps 未就绪 | 保持当前桌面尺寸并记录原因 | 日志说明未发送、待重试或服务端不支持 |
| Pasteboard 权限拒绝到剪贴板操作失败 | 保留 | 用户拒绝或权限请求超时 | 本次剪贴板读写失败，会话继续 | 不崩溃，不在连接开始弹权限 |
| 麦克风权限拒绝到 `audin` open 失败 | 保留 | 用户拒绝或权限请求超时 | 采集通道失败，会话和播放继续 | 日志说明拒绝；不影响基础 RDP |
| 地理位置权限拒绝到 location sample 失败 | 保留 | 用户拒绝、定位服务关闭或权限请求超时 | 本次 location sample 不发送，会话继续 | 日志说明拒绝或采样失败；不影响基础 RDP |
| OHAudio 播放失败到无声会话 | 暂保留 | 播放后端初始化或写入失败 | 会话继续，记录 rdpsnd/OHAudio diagnostics | 不因播放失败断开桌面 |
| `drive` 首版默认关闭 | 保留为产品策略，不是运行时 fallback | 文件共享未产品化或未授权 | 不请求 drive channel | matrix 标明默认关闭，HAP 不声明额外文件权限 |
| 打印作业提交失败 | 保留 | PrintKit 初始化、查询、连接或 `StartPrintJob` 失败 | 当前打印作业失败，会话继续 | 日志说明失败阶段；不影响基础 RDP |
| FUSE/CUPS/smartcard/TSMF fallback | 不保留 | 依赖缺失、服务未闭环或 deprecated | 从交付构建裁剪或标为后续专项 | 包内不出现对应 addin/runtime 路径 |

## 验收基线

T00 已把后续任务的可重复验收口径整理到 `docs/freerdp-ohos-validation-baseline.md`。本矩阵继续记录 feature 状态；构建命令、runtime 同步命令、HAP 产物路径和真机最小回归清单以基线文档为准。

当前基线：

- 地理位置功能保持默认 channel 行为；若连接后立即弹定位权限，触发源是服务端发起 `LocationStart`，不是打印链路依赖。
- 打印功能新增 OHOS PrintKit backend；连接时只注册虚拟打印机，远端提交打印作业后才进入 PrintKit。
- 验证构建：`harmony/scripts/wsl/build-freerdp-ohos.sh`、`harmony/app/build_hap.bat`

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
4. 连接开始时不应立即弹 Pasteboard 权限；触发剪贴板读取时才申请权限，并验证文本同步。
5. 播放 Windows 系统声音，观察 `rdpsnd` 日志、延迟、断连和后台行为。
6. 连接开始时不应立即弹麦克风权限；远端实际请求音频采集时才申请权限，并验证 `audin` 采集路径。
7. 连接到会请求位置重定向的服务端，确认 `LocationStart` 后申请定位权限；授权后能发送样本，拒绝后会话继续。
8. 在 Windows 内打印，确认连接开始未初始化 PrintKit，提交作业后才生成 spool 文件并进入 OHOS printer backend；成功或失败都不影响 RDP 会话。
9. 启用一个只读共享目录，验证 `drive` runtime settings、路径选择和权限策略。
10. 检查构建 manifest：`with_smartcard=OFF`、`with_smartcard_pcsc=OFF`，运行包内不应出现 smartcard/TSMF addin。
11. 有 RD Gateway 服务端环境后再启动 RD Gateway 专项：补 UI 参数和 settings 映射，并验证网关认证、证书、错误提示和目标机透传。

## 影响

- 当前 signed HAP 为 33,320,512 bytes，约 31.78 MiB；相对裁剪前工作区基线 88.27 MiB 减少约 56.49 MiB。
- smartcard source/channel/PCSC 和 TSMF 不进入包，减少未闭环平台服务和 deprecated 通道带来的商业验收风险。
- CUPS/FUSE 不进入包，避免把当前无法闭合的 Linux 服务模型带进普通 HarmonyOS 应用；打印交付路径使用 OHOS PrintKit backend。
- 地理位置已进入默认 channel 集合；上架隐私材料和真机验收必须覆盖定位权限用途、拒绝授权行为和服务端触发时机。
