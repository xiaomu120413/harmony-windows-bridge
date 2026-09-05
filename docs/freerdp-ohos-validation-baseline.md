# FreeRDP OHOS Validation Baseline

当前操作说明核对：2026-09-05；历史验收记录保留原日期。

本文档是当前 FreeRDP OHOS 构建和真机回归基线。后续每个下沉或可接入化任务完成后，都应基于这里的命令和真机清单做最小回归，并在任务说明中写明未覆盖项。

## 基线标识

| 项目 | 当前值 |
| --- | --- |
| 主仓库分支 | `main` |
| 主仓库基线提交 | 验收时记录 `git rev-parse HEAD` 与工作区差异，避免将不同构建混为同一基线 |
| FreeRDP 子模块路径 | `harmony/third_party/FreeRDP` |
| FreeRDP 子模块提交 | `0c242c6e2e989e4d0a2cbb183585b7167ff557fd` |
| FreeRDP 标识 | `3.26.0-171-g0c242c6e2` |
| 运行库输出目录 | `harmony/out/ohos-arm64/runtime-libs` |
| HAP 运行库目录 | `harmony/app/common/libs/arm64-v8a` |
| 2in1 Entry 目标产物 | `harmony/app/entry/build/default/outputs/default/entry-default-signed.hap` |

子模块标识为 2026-09-05 源码快照，不代表当前安装包使用该版本；每次验收必须重新记录主仓库及两个子模块 SHA。

基线命令：

```powershell
git rev-parse HEAD
git submodule status
git -C harmony/third_party/FreeRDP rev-parse HEAD
```

最新交付包（CHG-20260905-003）：2026-09-05 已完成 Release 干净构建及正式签名，最终 `harmony/app/build/outputs/default/app-default-signed.app` 为20,010,801 bytes，SHA-256 `d1fb89ed242bf85b07aac32b174dbd7b0e0d952c21888343e272342e6ccc69cc`。三个模块 `debug=false`，签名/profile 和公共 ABC 门禁通过；此前15:41的 Debug 包已被替换。Release 新包启动待用户测试。操作及输出清理记录见多设备打包方案12.6节。

最新图标修复包（CHG-20260905-004）：同日18:45后重新完成Release构建及正式签名，替换上述交付路径；20,011,454 bytes，SHA-256 `331586f0a3c5357e54eda374dd7f857673249073b595de3b2a6ad7ac031802b5`。ArkTS、模块debug=false、签名/profile一致性和ABC门禁通过；设置概览图标的原质量工具评分待复测。

当前交付更新（CHG-20260905-005）：按用户反馈将概览替换为实心四宫格后，再次完成Release正式签名构建及ABC检查，三个模块debug=false、签名/profile检查通过。最终路径不变，20,010,872 bytes，SHA-256 `8a7aebf3a4850cf1590f6a88d0edf0740a10c63f29ab2748ab4a78a45c35084d`，替换004测试包；图标评分待同工具复测。

## 构建前检查

从仓库根目录执行：

```powershell
git status --short --branch
git submodule status
```

期望：

- 主仓库只包含当前任务的预期改动。
- FreeRDP 子模块提交和任务说明一致。
- 若有未跟踪的本地工具配置文件，应在任务说明中明确它们不属于本轮交付。

## FreeRDP 构建检查

在 WSL/Linux 环境执行：

```bash
harmony/scripts/wsl/check-freerdp-ohos-feature-matrix.sh
harmony/scripts/wsl/build-freerdp-ohos.sh
```

环境前提：

- `OHOS_NDK_HOME` 指向 Linux 侧 HarmonyOS native SDK。
- `OHOS_LLVM_HOME` 未设置时由脚本默认推导为 `$OHOS_NDK_HOME/llvm`。
- 可选能力默认来自 `build-freerdp-ohos.sh`：`ENABLE_OHAUDIO=1`、`ENABLE_OHOS_AVCODEC=1`、`ENABLE_OHOS_PASTEBOARD=1`、`ENABLE_OHOS_PRINT=1`。
- geometry channel 默认编译并注册为动态虚拟通道；当前只接入协议层 addin 和 session 请求，不消费 region 数据，也不改变 HAP 渲染或布局策略。
- location channel 默认编译但不注册，OHOS 后端通过 native LocationKit 采样；HAP 只负责定位权限申请。启用 channel 后，若远端在连接后主动发起 `LocationStart`，定位权限弹窗属于远端业务触发，不是打印链路依赖。
- drive channel 默认只映射固定 Download 子目录。HAP 在启动后通过下载控件授权并准备 `Download/com.muhub.desktop`；FreeRDP 连接时把该目录注册为 `\\tsclient\Downloads`，不接收 ETS 传入的任意路径。
- printer channel 默认暴露一个虚拟打印机；OHOS PrintKit 初始化、打印机查询/连接和 `StartPrintJob` 只在 Windows 提交打印作业时发生。
- 首版交付 profile 继续保持 smartcard source/channel/PCSC 和 TSMF 关闭。

输出期望：

- `harmony/out/ohos-arm64/sysroot` 包含 FreeRDP/WinPR 头文件和库。
- `harmony/out/ohos-arm64/runtime-libs` 包含 HAP 运行所需 `.so`。
- probe 属于开发验证产物，不进入交付包；包校验会拒绝 probe 文件。

## Runtime 同步检查

在 Windows PowerShell 从仓库根目录执行：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\harmony\scripts\windows\sync-freerdp-runtime.ps1
```

输出期望：

- `harmony/app/common/libs/arm64-v8a` 中至少包含：
  - `libfreerdp-client3.so`
  - `libfreerdp3.so`
  - `libwinpr3.so`
  - OpenSSL、zlib、cJSON、uriparser、OpenH264、FFmpeg 运行库
- `ossl-modules/legacy.so` 在源目录存在时被同步。
- 同步失败必须阻断后续 HAP 构建，不能用旧运行库继续验收。

## HAP 构建检查

本地兜底命令：

```powershell
.\harmony\app\build_hap.bat app
```

单设备构建可显式使用 `build_hap.bat tablet` 或 `build_hap.bat 2in1`；完整交付以 App Pack 及包校验结果为准。

输出期望：

- 产物包括 `app-default-signed.app`、`common-default-signed.hsp` 与两个设备 Entry HAP；完整路径见根 README。
- 构建日志中没有回退到 mock FreeRDP runtime 的提示。
- `common.hsp` 包含 `librdpclient.so` 和 FreeRDP runtime；2in1 Entry 独占 `libxrdpcontrol.so` 和 xrdp HNP；tablet 不携带被控端。

### release ArkTS 跨模块 ABI 检查

`common.hsp` 独立编译后，其 `Index.ets` 公共导出名属于 Entry/HSP 运行时 ABI。两个 Entry 不得启用
`-enable-export-obfuscation`。最终 App Pack 必须执行：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\verify_multidevice_app.ps1 `
  -AppPath harmony/app/build/outputs/default/app-default-signed.app
```

通过标准：

- 输出包含 `arktsCommonAbi=passed`；
- 门禁使用 SDK `ark_disasm` 反汇编包内 `ets/modules.abc`，两个 Entry 从 `common/Index` 请求的每个
  运行时导入名都存在于 `common.hsp` 导出集合；
- `entry` 或 `entry_tablet` 混淆规则中一旦出现有效的 `-enable-export-obfuscation`，门禁立即失败；
- 2in1 真机全新安装和上一正式版本覆盖安装后均能启动，HiviewDFX/HiLog 不出现
  `common/Index` 缺少导出名的 `SyntaxError`。

如果本机缺少 `ohpm`、`hvigor` 或 DevEco/HarmonyOS SDK，任务说明必须明确 HAP build 未覆盖，并保留 FreeRDP build 与 runtime sync 的结果。

### 自动签名材料检查

当前 `harmony/app/build-profile.json5` 使用 `tools/app/muhub.p12`、`muhub_debug.cer`、`muhub_debugDebug.p7b` 和 `muhub` alias。配置含本机绝对路径，换机器后需核对路径，不能照搬旧工作目录。

打包与 HNP 重签脚本通过 `signing-passwords.ps1` 解析密码：独立的 store/key 参数或对应环境变量优先，然后使用通用签名密码；缺项才从 build-profile 中的加密值及 `tools/app/material` 读取。具体优先级以该脚本为准。

本地自动解密依赖 `material/ac`、`material/ce` 和 `material/fd/*` 的完整配套文件，以及 DevEco Node；不要打印明文密码。旧 `tools/hapsigner` 属于历史 OpenHarmony 签名方案，不是当前打包脚本默认目录。

自动签名验收要求：

1. `SignHap` 不再出现 `DecipherUtil.getKey`、`decryptPwd` 或 `material` 路径缺失错误。
2. `harmony/app/entry/build/default/outputs/default/entry-default-signed.hap` 成功生成。
3. 对携带 xrdp HNP 的交付包继续执行 HNP 重打包和最终签名，不能用普通 unsigned HAP 直接签名替代 HNP 注入流程。
4. 2in1 私有 HNP 在应用进程沙箱内应通过 `/data/app` 访问：业务代码只固定清单生成的无版本号链接
   `/data/app/bin/xrdp`，通过 `realpath()` 获得当前版本目录并派生 `lib/config/share`，不得硬编码
   `xrdp_0.1.0` 等版本号。`/data/service/hnp` 属于公共 HNP 挂载点；也不得硬编码宿主机用户号和
   bundle 物理安装目录。

`SIGN-A-01` 历史验证记录（2026-08-04，旧 tools/hapsigner 签名方案，不代表当前材料已复验）：

- 恢复当时 tools/hapsigner/material 的5个解密材料后，`hvigorw --no-daemon assembleHap --mode module -p product=default -p module=entry@default` 完成 `SignHap`，结果为 `BUILD SUCCESSFUL`。
- 使用 `tools/hapsigner` 下的 OpenHarmony P12、证书、debug profile 和 `openharmony application release` alias 对注入 xrdp HNP 后的 HAP 重签成功；最终产物为 `harmony/app/entry/build/default/outputs/default/entry-default-signed.hap`，大小 `41782896` 字节。
- 设备 `3QC0124C11000711` 执行 `hdc install -r` 返回 `install bundle successfully`，系统验签及 HNP 解包通过。

## 真机最小回归清单

每个任务完成后至少按影响范围选择以下检查。P0/P1 任务默认应覆盖 1-8。

1. 启动 App，确认 `probe()` 能识别 FreeRDP runtime、channel loader、OpenSSL、FFmpeg/OpenH264、OHOS AVCodec/Pasteboard/OHAudio 能力状态；构建 manifest/profile 包含 geometry，且 smartcard/TSMF excluded；系统下载目录下准备好了 `com.muhub.desktop`。
2. 连接 Windows RDP，确认认证、连接状态、断开状态和错误提示正常。
3. 验证画面：首帧、持续更新、窗口/桌面刷新、GDI fallback、`rdpgfx-h264` 与 AVC444 GPU compositor 日志。
4. 验证指针：点击、拖拽、右键长按、双指纵向/横向滚轮，断开或页面销毁后无残留按下状态。
5. 验证键盘：普通字符、数字、Delete、Backspace、Tab、Esc、Ctrl/Alt/Win 组合键。
6. 验证 IME：中文提交、英文软键盘、软键盘 Backspace，硬件 Delete/Backspace 不依赖 TextInput 兜底。
7. 验证剪贴板文本：Windows 到 HarmonyOS、HarmonyOS 到 Windows，关注权限提示、编码和 change echo。
8. 验证音频：Windows 测试音、断开释放、重连恢复、后台/前台切换日志。
9. 验证地理位置：启用 `location` channel 后，服务端发起 `LocationStart` 才申请定位权限，授权后发送样本，拒绝或定位服务关闭时会话继续。
10. 验证文件重定向：Windows 中打开 `\\tsclient\Downloads`，确认对应 HarmonyOS `Download/com.muhub.desktop`，并完成一个小文件读写回归。
11. 验证打印：连接开始不初始化/连接 PrintKit；Windows 打印时生成临时 spool 文件并进入 OHOS printer backend，Harmony Print/虚拟 PDF 打印成功或失败都不影响 RDP 会话。
12. 验证 display resize：连接前完整单屏布局，连接后窗口/方向变化携带像素尺寸、物理毫米、desktop/device scale，重复请求去重，服务端不支持时错误原因；不把 Windows 主机物理显示器变化误判为 `disp` 能力。
13. 验证 geometry：动态通道协商和日志正常；当前不要求 region 数据影响画面布局。
14. 验证生命周期：快速连接/断开、页面切换、App 后台、网络失败、凭据错误、证书变化。
15. 验证手写笔：轻压、重压、X/Y 倾斜、橡皮、抬笔、取消、失焦和断连；笔输入不得同时触发 finger Tap/Pan，RDPEI 不可用时只回退一次基础指针。
16. 验证多显示器：外接屏连接/移除、主屏和负坐标拓扑、不同方向；Windows 显示数量与拓扑同步，组合桌面各屏四角均可点击，回到单屏后 surface resize 恢复。

## 历史 T00 判定（初始基线任务）

T00 只建立验收基线，不改变业务逻辑。完成判定：

- 本文档存在并记录主仓库和 FreeRDP 子模块基线。
- `docs/freerdp-ohos-feature-matrix.md` 引用本基线，并保留当前 feature 状态。
- 后续 T01-T18 可以直接引用本文档中的命令和真机清单。

## 实施台账

当前应用使用 API 26 compile/target SDK，并保持 API 22 compatible SDK。发布前除设备 HAP/HSP、
签名和设备矩阵外，还需验证 `CONTROL_DEVICE` 未授权、设置页授权、授权后重启保持，以及
旧 injection dialog fallback；详见 `TAB-E-02`。本地调试 profile 已同步该受限权限 ACL，
否则真机安装会返回 9568289；商店包必须使用 AGC 审核通过的正式 profile。

| Change ID | 状态 | 修改范围 | 验收 ID | 验收条件 |
| --- | --- | --- | --- | --- |
| `SIGN-D-01` | 历史 Verified（旧签名方案） | 已恢复 `tools/hapsigner/material/**` 中5个已跟踪的自动签名解密材料；未修改业务代码、证书、P12、profile 或包名 | `SIGN-A-01` | `SignHap`、HNP 重签及真机覆盖安装均通过；日志不再出现 `DecipherUtil` 读取 `material` 失败 |


### 2026-09-05 构建范围收敛（SCOPE-20260905-001）

移除临时 CPU-only 录屏编译选项和强制 GDI 分支，构建恢复既有 GPU 渲染及正常 GDI fallback。桌面 Web/Node Demo 与独立文件传输设计已删除，HarmonyOS RDP 下载目录重定向保留。Native 回归及 diff 检查通过；本次未重建 HAP 或重新验证真机图形协商。详细记录见 project-scope-and-session-controls.md。


### DOC-AUDIT-20260905-001 最新复核

2026-09-05：Native与xrdp进程策略检查通过；统一8个资源文件纯换行差异后ArkTS测试通过（无Git内容差异）。build_hap.bat app 完整构建、签名及包校验通过，App Pack 39,890,794 bytes，SHA-256 c0f7fbdae23d8cea28afa09c53217f59efaa5be6217782270fb875e0806cf72d。本轮自动复核未安装新包；用户随后确认真机远程会话整体验收通过（CHG-20260905-002），未指定对应包哈希。市场验收仍待执行。外链/截图/技术声明的逐项状态见 [复核报告](audit/2026-09-05-document-verification.md)。
