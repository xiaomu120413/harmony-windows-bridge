# MuHub RDP 产品可靠性、诊断与故障验收方案

> 状态：`Implemented`（本机构建与门禁通过，真机故障矩阵待验）
>
> 日期：2026-08-10
>
> 范围：HNP XRDP 生命周期、FreeRDP 连接前端点探测、脱敏诊断导出、标准通道故障验收

## 1. 目标与边界

本工作包收口已有 RDP 能力的可控性、可诊断性和失败路径，不新增私有协议，不恢复 Strict 证书策略入口，也不修改已经标记为 Verified 的 FreeRDP 手写笔与多显示器能力。

目标：

1. 2in1 用户可以显式停止私有 HNP 中的 xrdp，子进程异常退出后 UI 在有限时间内更新，重新启动不产生第二进程或残留端口。
2. FreeRDP 连接前异步探测用户填写的 host/port，区分端点不可达与后续 RDP 认证/协商失败；探测结果不能替代 FreeRDP 的最终错误。
3. 设置页可以一键导出版本化、脱敏的 JSON 诊断，覆盖会话、渲染、输入、通道、显示、权限和 xrdp 进程摘要。
4. 剪贴板、固定共享目录、声音、麦克风、摄像头、打印和位置通道形成可重复的拒绝、占用、热插拔、断线和恢复验收矩阵。
5. ArkTS 按业务域拆分，仓库内每个 `.ets` 文件物理行数不超过 600 行，并由统一测试入口强制检查。

非目标：

- 不实现 RD Gateway、xrdp 多显示器、智能卡、TSMF 或系统虚拟摄像头。
- 不导出密码、访问码、证书私钥、完整用户名、完整主机地址或私有沙箱绝对路径。
- 不把 TCP connect 成功解释为 RDP、TLS、NLA 或账号认证成功。
- 无对应硬件或服务端时不把人工动作项误写为通过。

## 2. HNP 生命周期收口 `MDP-03B`

### 2.1 状态和交互

- `RemoteControlCoordinator` 是 ArkTS 侧唯一服务生命周期所有者，新增 `stopFromSettings()`，调用现有 `RemoteControlPort.stop()` 并发布不可变快照。
- 2in1 远控设置的服务卡在运行时同时提供“刷新”和“停止服务”；停止期间禁用重复操作。tablet 继续物理隔离，不显示服务卡、不调用 xrdp Native。
- `MuHubApp` 在页面可见期间每 2 秒调用一次 diagnostics；页面销毁时取消定时器。轮询只做 `waitpid(WNOHANG)` 和状态同步，不自动重启异常退出的进程。
- 用户显式停止后，同一页面生命周期内不得被轮询或页面 show 自动启动；下一次完整应用启动仍沿用现有自动启动策略。
- Native 保留 SIGTERM、3 秒有限等待、SIGKILL 兜底和 `PR_SET_PDEATHSIG`，不引入按进程名批量 kill。

### 2.2 验收

- `AC-HNP-STOP`：UI 停止后 PID 消失、3390 不监听、状态为 Stopped，重复停止幂等。
- `AC-HNP-EXIT`：对子进程发送 SIGKILL 后 2 秒轮询窗口内状态变为 Exited 并保留非零退出码，可再次启动。
- `AC-HNP-OWNER`：应用强停、窗口销毁、卸载均无孤儿 xrdp 和残留端口。
- `AC-HNP-SESSION`：MSTSC 连接后画面、输入、剪贴板和已启用通道正常；停止服务会断开会话。
- `AC-HNP-UPGRADE`：上一正式版本覆盖升级、必要回滚和真实应用市场分发另行取证，仍属于发布阻断项。

## 3. FreeRDP 端点预检 `RDP-PREFLIGHT-01`

- 新增纯 ArkTS `RdpEndpointProbe`，使用 NetworkKit `TCPSocket.connect()` 和用户填写的实际端口，超时 3000ms，并在 finally 中关闭 socket。
- 连接提交生成单调递增 generation；配置切换、重复点击或页面销毁后，过期结果不得启动 Native 会话或覆盖新提示。
- 状态为 `Checking/Reachable/Unreachable`。失败提示保留平台错误码的非敏感摘要；日志不得输出密码。
- 预检失败时保持连接表单，提示检查网络、端口、防火墙和远程桌面服务；不进入黑色会话页。预检成功后才执行现有 Native connect。
- FreeRDP 的 Resolving、TCP connected、Negotiating、Authenticating 和最终错误仍是连接事实来源。

验收：有效端点进入 Native connect；拒绝、超时、不可解析返回明确提示；连续点击只允许最后一代结果生效；TCP 可达但账号错误仍显示 FreeRDP 认证错误。

## 4. 脱敏诊断导出 `RDP-DIAG-01`

### 4.1 Native 与 ArkTS 边界

- `librdpclient.so` 新增 `getDiagnostics()`，只返回当前进程内可安全读取的 FreeRDP/渲染/输入/显示摘要字符串，不执行文件 IO。
- Native 摘要包含：连接状态、诊断会话 ID、Surface、显示方向/多屏状态、输入队列、渲染帧统计、RDPGFX/AVC 路径、rdpsnd/audin、剪贴板及当前通道可用性。不可用项明确为 unavailable。
- ArkTS `RdpDiagnosticsExporter` 组装版本化 JSON，附加应用版本、设备能力、权限状态、FreeRDP Native 摘要和 xrdp PID/端口/退出码。
- 诊断写入用户已授权的鸿蒙共享目录；目录未准备时先走现有系统目录选择流程。文件名仅使用 UTC 时间。

### 4.2 脱敏与失败策略

- 不读取或导出密码、Asset alias 对应 secret、访问码、证书材料。
- host 与 username 只输出“是否填写”和长度，不输出正文；Native 原始摘要不得包含连接参数。
- 文件写入失败只影响导出操作，不影响正在运行的 RDP/xrdp 会话。

验收：未连接、FreeRDP 会话中、xrdp 运行中均可导出合法 JSON；敏感字段扫描为 0；目录拒绝和写失败有明确提示。

### 4.3 独立诊断卡与完整性增量 `RDP-DIAG-02`

- “导出诊断”从鸿蒙共享目录卡移出，在远控设置底部形成独立“诊断与排障”分区和卡片；共享目录卡只负责打开固定共享目录。tablet 与 2in1 都显示诊断卡。
- JSON 增加当前连接阶段、最近一次端点预检状态/耗时和脱敏错误类别；错误只允许 `none/network/credentials/certificate/session/unknown`，不导出原始错误正文。
- Native 摘要增加诊断会话 ID、resize 状态/目标/generation/跳过帧及剪贴板实时计数；已有渲染摘要继续提供 FPS、失败帧、替换帧、节流、完整/局部帧和耗时，图形摘要继续提供 RDPGFX/AVC 路径，音频摘要继续提供 rdpsnd/audin 包与字节统计。
- 摄像头 OHOS FreeRDP 当前没有通用 diagnostics 导出，只记录 rdpecam 能力符号是否可用；没有实际计数时必须明确为 unavailable，不以权限状态冒充帧数据。
- 文件内增加 `coverage`，逐项标记 `available/unavailable`，让验收人员能区分“数值为零”和“实现无法读取”；剪贴板能力与会话期计数分开，xrdp 录屏/注入计数及摄像头包计数未实现时明确标记 unavailable。
- 验收：独立卡片可导出；连接前、端点失败、认证失败、会话中各导出一次；字段覆盖门禁通过，敏感正文扫描为 0。

## 5. 标准通道故障验收 `RDP-FAULT-01`

| 能力 | 正常动作 | 失败/恢复动作 | 通过标准 |
|---|---|---|---|
| 剪贴板 | Windows↔OHOS 中英文、多行、重复内容 | 拒绝权限、断线重连 | 不死循环、不重复弹权、重连后恢复 |
| 固定共享目录 | `\\tsclient\Downloads` 列举、创建、覆盖、删除、中文名 | 取消目录授权、只读/空间不足、传输中断线 | 失败不崩溃且会话保持 |
| rdpsnd | 内置扬声器播放 | 耳机/蓝牙切换、静音、前后台、断线 | 路由恢复且无残留播放 |
| audin | 内置麦克风采集 | 拒绝、占用、设备切换、断线 | 释放采集资源，可再次授权连接 |
| rdpecam | 远端应用预览 | 拒绝、占用、热拔插、断线 | 错误可诊断，会话不中断，资源释放 |
| 打印 | 提交 PDF/作业 | 用户取消、打印机离线、连续作业 | PrintKit 失败不影响 RDP 会话 |
| 位置 | 服务端请求位置 | 拒绝、关闭定位、后台 | 权限按需且拒绝不影响其他通道 |

## 6. 文件范围

- 文档：本文、`docs/README.md`、`docs/harmonyos-multidevice-hnp-packaging-plan.md`、`docs/harmonyos-tablet-adaptation-architecture-and-acceptance.md`、`docs/freerdp-ohos-feature-matrix.md`、`docs/CHANGELOG.md`。
- ArkTS：`rdp/RemoteControlCoordinator.ets`、`rdp/RdpEndpointProbe.ets`、`rdp/RdpDiagnosticsExporter.ets`、`rdp/NativeRdpGateway.ets`、`pages/Index.ets`、设置/首页组件及对应测试。
- Native：`napi/native_bridge_context.*`、`napi/napi_exports.cpp`、`session/rdp_session_core.*`、`session/rdp_session_channels.*`、`types/librdpclient/Index.d.ts`。
- 门禁：`tools/run_tablet_arkts_tests.ps1`、`tools/run_tablet_native_tests.ps1`、`tools/verify_xrdp_process_control.ps1`，必要时新增仅做只读设备检查的验证脚本。

### 6.1 ArkTS 文件规模 `RDP-ARCH-ETS-600`

- 600 行是硬上限，不区分页面、组件、协调器或测试；新增代码不得通过压缩排版规避门禁。
- `Index.ets` 只保留页面生命周期、顶层 ArkUI 状态和页面装配；连接配置、会话回调、权限与远控设置动作下沉到各自协调器。
- `RemoteControlPageCoordinator` 必须延迟到页面生命周期中首次访问时创建；不得在组件字段初始化阶段捕获 `remoteControlPort`，因为 2in1 Entry 的 ArkUI 构造参数在该阶段尚未完成注入。tablet 的默认 unavailable 端口与 2in1 的实际端口必须分别保持。
- `SettingsPage.ets` 将设置概览展示拆成独立组件；`RemoteControlCards.ets` 将共享目录/诊断卡拆成独立文件，组件行为和路由不变。
- `tools/run_tablet_arkts_tests.ps1` 递归扫描产品源码与测试中的全部 `.ets` 文件，发现任一文件超过 600 行立即失败并输出相对路径与行数。
- 验收：全部 `.ets` 文件均不超过 600 行；ArkTS 策略测试、架构门禁与 Debug 构建通过；现有连接、设置、远控和诊断回调无语义变化。

## 7. 实施台账

| Change ID | 状态 | 验收 |
|---|---|---|
| `MDP-03B` | `Verified` | AC-HNP-STOP/EXIT/OWNER/SESSION/UPGRADE；用户 2026-08-12 真机确认 |
| `RDP-PREFLIGHT-01` | `Implemented` | TCP 成功、拒绝、超时、过期 generation、RDP 认证失败 |
| `RDP-DIAG-01` | `Implemented` | 三种运行状态导出、JSON解析、敏感字段扫描、失败路径 |
| `RDP-DIAG-02` | `Implemented` | 独立卡片、连接/resize/剪贴板补全、coverage 与脱敏错误类别；2in1 真机已导出 schema 2 JSON |
| `RDP-FAULT-01` | `DesignReady` | 第 5 节动作矩阵；无硬件项保持 Pending |
| `RDP-ARCH-ETS-600` | `Verified` | 全量 `.ets` 行数门禁、ArkTS 测试、Debug 构建与行为回归 |

`MDP-03B` 的 MSTSC 会话、显式停止、异常退出、强停、卸载、升级及无孤儿进程动作已由用户于 2026-08-12 确认真机通过，状态升级为 Verified。该结论只覆盖 2in1 HNP 生命周期，不覆盖 tablet Entry 模块迁移和应用市场分发。设备动作记录见 [RDP 通道与 HNP 真机故障验收清单](rdp-channel-fault-acceptance-checklist.md)。

## 8. 本机实施证据

- 2026-08-10：所有 `.ets` 均不超过 600 行，`Index.ets` 591 行；ArkTS 策略测试与模块编译通过。
- Native resize/display/stylus/input/geometry 测试和 xrdp 独立进程策略门禁通过；`git diff --check` 通过。
- 使用 DevEco 内置 Node 24.14.1 完整构建成功；修复后的 App Pack 39,972,465 bytes，SHA-256 `7f3b746acd848113a7687f5d06e83f23afacff18eb635d47a76db0b626bee992`，模块为 `common,entry,entry_tablet`。
- 首次构建被系统 Node 25.5.0 与 Hvigor 的递归 rmdir 兼容问题阻断；将 DevEco Node 目录置于 PATH 首位后通过，属于构建环境问题，不是产品代码失败。
- 2in1 安装检查发现协调器过早捕获默认 unavailable 端口，导致正确的 `entry` 包仍隐藏被控端能力；已将端口绑定改为属性注入后的延迟创建，并增加静态回归门禁。
- 修复包已原子覆盖安装到 2in1 `3QC0124C11000711`：远控设置恢复服务卡、录屏和输入授权，xrdp PID `24928` 独立于应用 PID `24780`，`0.0.0.0:3390` 监听。显式停止等完整动作仍按真机清单继续验收。
- 独立“诊断与排障”卡已覆盖安装并在同一 2in1 实机显示；实际导出的 schema 2 JSON 可解析，包含应用/连接阶段/端点预检/权限/FreeRDP Native 摘要/xrdp PID 与状态/coverage/共享目录状态，host 和 username 仅保留长度，密码与访问码未出现。空闲态 session ID、帧率和通道计数为 0 属正常；剪贴板会话计数、摄像头包计数、xrdp 录屏与注入计数按实际能力标为 unavailable。
- 本轮完整 App Pack 40,002,044 bytes，SHA-256 `b7a6ea7dad543c6f98b9cacd28abc46e92faac6211baad41064a5696e7db69e7`，模块为 `common,entry,entry_tablet`。2in1 使用 `common + entry`、平板 `5JB0223804000371` 使用 `common + entry_tablet` 分别原子覆盖安装；平板远控设置仅显示主动控制、共享目录、验证码和独立诊断卡，未安装 HNP 且无 xrdp 进程。
- 2026-08-12 用户确认输入/IME 真机收口及 xrdp/HNP 生命周期验收通过；输入专项 TAB-F-01～TAB-F-06 与 `MDP-03B` 升级为 Verified。权限通道 TAB-F-07、外设故障矩阵和 tablet Entry 分发迁移仍独立保持原状态。
