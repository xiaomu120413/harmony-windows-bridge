# 仓库修改记录

这里是仓库级统一修改台账，用来快速回答“哪一笔改了什么、影响哪里、到哪里看证据”。

Git 历史仍是提交内容和文件差异的最终事实来源；设计目标、状态机和详细验收继续由对应专项文档维护。本台账只保留便于回看的一层摘要，不复制专项文档全文。

## 维护规则

- 一笔 Git 提交对应一条记录，文档提交也必须登记；同一提交不要拆成多条，也不要把多笔提交合并成一条。
- 新记录加在对应日期表格最上方，Change ID 使用 `CHG-YYYYMMDD-NNN`，同一天序号只增不减。
- “提交”优先填写短哈希。记录与实现同提交、尚无哈希时，先填写唯一的提交主题；提交完成后可在后续整理时补短哈希。
- “改了什么”写行为变化和主要范围，不罗列所有文件；精确文件差异使用 `git show <提交>` 查看。
- “验证/关联”写实际执行的测试或证据入口。未执行的验证必须明确写“未执行”，不能写成已通过。
- 未提交的试验性修改不进入历史表；使用 `git diff` 查看。准备形成提交的完整修改，应在提交前登记。
- 设计先行、实施状态和证据回写仍按 [平板适配架构与验收方案](harmonyos-tablet-adaptation-architecture-and-acceptance.md) 等专项文档执行。
- 不记录密码、令牌、证书私钥、用户输入正文或其他敏感数据。

初始台账回填范围是 2026-08-04 当前分支相对上游的 32 笔提交（`6000284` 至 `5937f8e`）。更早的历史不反向补写摘要，仍通过 `git log` / `git show` 查询；从本台账建立后产生的每笔提交都必须持续登记。

## 新记录模板

| Change ID | 提交 | 类型/范围 | 改了什么 | 验证/关联 |
| --- | --- | --- | --- | --- |
| `CHG-YYYYMMDD-NNN` | `type(scope): subject` | 功能/模块 | 用一两句话说明行为变化、兼容性和主要影响范围 | 测试命令、结果或专项文档/证据链接；未执行要明示 |

## 2026-09-05

| Change ID | 提交 | 类型/范围 | 改了什么 | 验证/关联 |
| --- | --- | --- | --- | --- |
| CHG-20260905-006 | fix(ui): align overview icon with sidebar style | UI/资源 | 按用户反馈替换实心图案，改为与其他侧栏图标同线宽和圆角的简洁面板，同步三模块。 | 四图资源预览、XML、一致性、Release正式签名及ABC门禁通过；20,010,822 bytes，哈希见验证基线。风格确认及原工具评分待反馈。 |
| CHG-20260905-005 | fix(ui): replace overview icon with solid grid | UI/资源 | 按用户反馈将概览图案换为圆角实心四宫格，同步三个模块，保持导航尺寸和主题着色。 | SVG解析、一致性、预览、Release正式签名及ABC门禁通过；新包20,010,872 bytes，哈希见验证基线。原工具评分待复测。 |
| CHG-20260905-004 | fix(ui): simplify settings overview icon contours | UI/资源 | 将设置概览SVG描边改为较宽的单色填充轮廓，同步三个模块资源，保留原导航尺寸和主题着色。 | SVG解析、一致性、ArkTS、Release正式签名及ABC门禁通过；新包20,011,454 bytes，哈希见验证基线。原报告95.98/90锯齿评分待同工具复测。 |
| CHG-20260905-003 | docs: record release signing and clean delivery | 构建交付/记录 | 干净重建 Release，所有签名阶段使用正式材料；输出目录仅保留最终 App，中间文件因删除策略限制移至本机 tmp。临时配置已恢复。 | 三个模块 debug=false；App 和源模块签名、release profile 一致性、ABC 门禁通过；20,010,801 bytes，哈希及操作见多设备方案12.6。新包真机启动待用户测试。 |
| CHG-20260905-002 | docs: record user-confirmed remote session acceptance | 文档/真机验收 | 根据用户确认，将真机远程会话整体状态更新为用户验收通过；同步复核报告和验证基线。 | 用户原话“真机远程会话，整个ok了”；diff检查通过，本次仅更新文档，未重复执行真机测试。 |
| DOC-AUDIT-20260905-001 | docs: verify references and technical evidence | 文档/证据/构建 | 核对20个原始外链、40处图片引用；替换5个外部来源，补全7处图片路径，标注2张临时图缺失；修正xrdp尺寸上限、SDK权限和多设备范围。 | Native/进程策略/ArkTS（本地换行统一后）及完整App构建、签名和包ABI门禁通过；产物哈希与端到端缺口见docs/audit复核报告。 |
| DOC-CLEAN-20260905-001 | 本地文档整理 | 文档/现行基线 | 修正签名与运行库路径、证书入口、独立 HNP 进程说明和旧页面路径；重新整理索引并标记历史方案。 | 补充扫描31份仓库Markdown，83个本地链接目标存在；补齐打包状态、平板架构、旧签名及商店草稿说明；diff检查通过，未新增构建或真机验收。 |
| SCOPE-20260905-001 | 本地清理 | 产品范围/构建 | 删除桌面 Web/Node Demo、独立 FTP/SFTP 设计和 CPU-only 录屏模式；保留 HarmonyOS RDP、xrdp 与正常图形 fallback。 | Native 回归、diff 检查通过；未重建 HAP；工具栏缩放另行设计，尚未实现。 |
| `CHG-20260905-001` | `fix(ohos): restore packaging and recording changes` | 打包、签名、Native 录屏配置及设计归档 | 恢复原 stash 全部内容：App Pack 文件名、签名材料与密码解析、跨模块导出校验、CPU-only 录屏构建和独立文件传输客户端设计。 | Native 测试、PowerShell/Node 语法检查及 diff 检查通过；ArkTS 门禁因现有资源 CRLF/LF 差异失败；未重新构建或真机验收。详见多设备打包设计的本次归档记录，关联 MDP-04A、CPU-RECORD-001、FT-ARCH-001。 |

## 2026-08-07

| Change ID | 提交 | 类型/范围 | 改了什么 | 验证/关联 |
| --- | --- | --- | --- | --- |
| `CHG-20260807-007` | `fix(ux): align tablet controller settings` | 修复/平板远控设置、项目帮助与版本 | 将远控设置的设备隔离粒度收窄到被动控制区：tablet 保留远控入口、鸿蒙共享目录和主动连接验证码，仅隐藏被控服务及录屏/注入权限；项目帮助统一为与 2in1 相同的方向卡片和可折叠步骤结构，tablet 默认展开主控方向并使用主控专属文案。应用版本升级为 `1.0.2`（`1000002`）。 | `tools/run_tablet_arkts_tests.ps1`、完整 App Pack 构建及多设备包校验通过；App Pack 39,887,994 字节，SHA-256 `813726aac48d587f539d534fdac67e081865108646d8a52cc5d35b7a3afe35fc`，模块为 `common,entry,entry_tablet`；tablet `5JB0223804000371` 通过同目录 HSP/HAP 原子升级，`bm dump` 确认 `1.0.2 / 1000002`，远控设置与项目帮助真机元素及折叠交互验证通过。截图见 `artifacts/design-audit/2026-08-07-tablet-project-help/`。 |
| `CHG-20260807-006` | `df81834` | 修复/首页设备操作 | 为“设备操作”使用与右侧操作按钮相同的 `36vp` 居中行，消除复用 `48vp` 表单标签造成的文字基线下沉，并增加回归门禁。 | `tools/run_tablet_arkts_tests.ps1`、完整 App Pack 构建通过；2in1 真机覆盖安装并核对标签与按钮中心线。截图见 `artifacts/design-audit/2026-08-07-device-actions-alignment/`。 |
| `CHG-20260807-005` | `fix(ux): refine remote settings hierarchy` | 修复/设置、帮助与首页操作控件 | 重新梳理远控设置的被动/主动控制层级：连接地址改为只读展示，主动连接验证码移到鸿蒙共享目录之后；项目帮助移除重复地址并补齐卡片动效。设置文案按业务域拆分并消除重复静态值，修复连接卡异常撑高，同时将界面中带文字的操作按钮统一为 `36vp`。 | `tools/run_tablet_arkts_tests.ps1`、完整 App Pack 构建及多设备包校验通过；App Pack 39,880,838 字节，SHA-256 `02b870f415e0e7af6ab5f3acd2d8dece5702037d74ddd7eb2084781bfbe0e221`，模块为 `common,entry,entry_tablet`；2in1 `3QC0124C11000711` 覆盖安装并完成首页、远控设置和项目帮助真机截图。设计核对见 `design-qa.md` 与 `artifacts/design-audit/2026-08-07-button-height/`。 |
| `CHG-20260807-004` | `fix(ux): clarify controlled-device connection flow` | 修复/设置、帮助与多尺寸适配 | 按第二版重构设置概览、远控设置和项目帮助：本机网络信息从基础设置移到被控连接场景，连接地址与复制动作前置；被控端不再展示或生成验证码，并明确验证码由主控端设置；2in1 展示被控指引，tablet 保持纯主控帮助，Compact/Expanded 分别采用纵向与分栏布局。 | `tools/run_tablet_arkts_tests.ps1`、完整 App Pack 构建及多设备包校验通过；App Pack 39,860,437 字节，SHA-256 `06d7ed3674197bcb2bbc03304a62a34f893f577f729e45ea0ce74b49b222ab2f`，模块为 `common,entry,entry_tablet`；2in1 `3QC0124C11000711` 覆盖安装并完成宽屏、紧凑窗口及复制成功态截图。设计对照见 `design-qa.md`，真实 tablet 截图因本轮仅连接 2in1 设备未补拍。 |
| `CHG-20260807-003` | `fix(ohos): resolve private HNP runtime path` | 修复/2in1 私有 HNP 进程 | XRDP 控制桥改用私有 HNP 的无版本沙箱链接 `/data/app/bin/xrdp`，通过 `realpath()` 动态解析当前版本根目录并派生库、配置和资源路径；门禁拒绝公共 HNP、宿主机物理目录和固定版本号，tablet 包边界不变。 | 进程策略门禁、`git diff --check`、2in1 与 tablet HAP 构建通过；PC `3QC0124C11000711` 覆盖安装后独立 xrdp PID 监听 3390，应用强停清理并重启恢复通过；tablet HAP 确认无 HNP 和 `libxrdpcontrol.so`。MSTSC 会话、显式 UI 停止、异常退出和卸载清理待验。关联 MDP-03A。 |
| `CHG-20260807-002` | `fix(ux): refine adaptive home header and window bounds` | 修复/共享首页头部与窗口下限 | PC 与 tablet 共用的首页头部改为 Flex 剩余空间居中，移除左右固定 `260vp` 占位，保留应用名“木枢”和完整宣传语；两个 Entry 的最小窗口统一调整为 `660 × 540vp`，PC 自动旋转与分屏声明保持不变。 | 完整 App Pack 构建和多设备校验通过；App Pack 39,799,360 字节，SHA-256 `1c5a2d542fb794557819fea7672a28077973e89df32ca475725bd7c9fc200bb2`。tablet `5JB0223804000371` 与 2in1/PC `3QC0124C11000711` 均完成覆盖安装、启动和居中标题截图；PC `bm dump` 确认 `common + entry`、私有 `xrdp.hnp`、`660 × 540vp`、自动旋转及三种窗口模式。 |
| `CHG-20260807-001` | `fix(ohos): align tablet production resources and printing` | 修复/多设备资源、打印与交付清理 | tablet Entry 改用与 2in1 Entry 完全一致的 35 项正式资源并注册 `MuHubPrintExtension`；虚拟打印实现下沉到共享 HSP，两个 Entry 保持同一能力。移除打包探针、探针图标及未被产品使用的诊断 Native 探针库，并增加资源、打印扩展和无探针门禁。 | tablet ArkTS 测试、完整 App Pack 构建及多设备校验通过；App Pack 39,807,209 字节，SHA-256 `0b5755a6152094502c77699507a76fe686896e72dfe8d50f8bd2f2fb8e47d996`。tablet `5JB0223804000371` 安装启动成功，`bm dump` 确认一个打印扩展且无 HNP，桌面截图确认正式产品图标。打印任务端到端仍需从系统打印入口补验。关联 MDP-04。 |

## 2026-08-06

| Change ID | 提交 | 类型/范围 | 改了什么 | 验证/关联 |
| --- | --- | --- | --- | --- |
| `CHG-20260806-010` | `feat(ohos): isolate tablet client and HNP process` | 功能/多设备权限、Native 与 HNP 进程 | 将共享 Native 收窄为 `librdpclient.so`，2in1 Entry 独占 `libxrdpcontrol.so` 并以 `fork/execve` 启动 HNP xrdp 独立进程；tablet 获得完整客户端权限但不含 PC 权限、HNP 或服务端库。新增三模式构建、App Pack/HNP ELF/进程策略门禁。 | 干净 Hvigor 构建、`app/tablet/2in1` 三模式、ArkTS/Native、HNP 交叉构建及两项新增门禁通过；App Pack 39,794,635 字节，SHA-256 `2f7e42928cf771390a27004a394e75acbd9dc15ed94b12909ba4435424356690`。tablet `5JB0223804000371` 安装启动及模块/权限/无 xrdp 进程通过；2in1 独立 PID 与真实商店分发待补，旧 Entry 迁移 P0 仍阻断发布。关联 MDP-02/03/04。 |
| `CHG-20260806-009` | `chore(ohos): remove generated common outputs` | 清理/Hvigor 与 CMake 生成物 | 从版本库移除误纳入的 `common/.cxx`、`common/.test` 编译缓存和测试报告，并增加递归忽略规则，保留正式源码、测试源码和可复现构建入口。 | `git ls-files` 生成目录检查与 `git diff --check` 通过；删除内容均可由 Hvigor/CMake 重新生成。 |
| `CHG-20260806-008` | `feat(ohos): add multi-device HNP packaging` | 功能/多设备 App Pack、共享 UI 与薄 Entry | 新增共享 HSP 和 tablet-only Entry，将正式 UI、RDP 客户端、资源与 Native bridge 迁入 HSP；2in1/tablet Entry 仅保留设备专属能力和共享根组件挂载。2in1 HAP 封装 HNP，tablet HAP 无 HNP/Native 库及 PC 权限；构建入口生成并检查互斥 Entry、共享 HSP 和 App Pack。 | 完整构建、签名、包门禁及 ArkTS/Native 测试通过；2in1 `3QC0124C11000711` 与 tablet `5JB0223804000371` 安装、启动和正式 UI 截图通过。tablet 仅有 `common,entry_tablet`、无 HNP、仅两项网络权限；旧 `entry`→`entry_tablet` 覆盖升级仍返回 `9568267`，应用市场分发和独立 XRDP PID 待后续工作包验证。关联 `MDP-00/01`。 |
| `CHG-20260806-006` | `fix(rdp): preserve complete dynamic monitor layouts` | 修复/FreeRDP OHOS 单屏动态分辨率 | 单屏首次连接和连接后 resize 统一携带像素尺寸、物理毫米、方向及 desktop/device scale；连接前保存 desired layout，连接后经 Display Control 发送，移除连接阶段重复 resize，并补齐结果/GDI 日志。明确该能力由客户端窗口驱动，不读取 Windows 主机物理显示器设置。 | Native resize/display tests、FreeRDP OHOS arm64 交叉编译、App Native 链接和 Debug HAP 打包签名通过；API 22 2in1 真机覆盖安装后全屏 `3120×1872`、浮窗 `2080×1312` 请求均为 Sent，无新增 crash。关联 `MON-DESIRED-LAYOUT-001`。 |
| `CHG-20260806-005` | `docs: design multi-device HNP packaging` | 文档/多设备 App Pack 与 HNP 架构 | 定义 tablet/2in1 互斥 Entry、共享 HSP、2in1 私有 HNP、权限物理隔离、XRDP 独立进程、升级回滚和应用市场分发门禁；旧单 HAP 约束保留为历史事实，MDP-00 仅标记 DesignReady，不误报实现。 | UTF-8、Markdown 差异及文档索引链接检查通过；本笔仅为设计基线，未执行目标模块构建、真机升级或应用市场分发验证。关联 `docs/harmonyos-multidevice-hnp-packaging-plan.md`。 |
| `CHG-20260806-004` | `fix(xrdp): serialize raw capture lifecycle` | 修复/xrdp OHOS 屏幕采集生命周期 | 更新 xrdp 子模块，使 raw AVScreenCapture 的 Start/Stop 由独立生命周期锁串行化，并在启动失败时安全摘除 worker 与 capture，避免动态 resize、暂停恢复或断连并发造成竞态；帧和协议行为不变。 | `harmony/scripts/wsl/build-xrdp-ohos.sh` 完整 OHOS arm64 交叉构建、安装和产物检查通过；连续 resize/suppress/disconnect 真机压力测试待补。关联 `XRDP-OHOS-CAPTURE-LIFECYCLE-001`。 |
| `CHG-20260806-003` | `fix(ux): stabilize saved credential switching` | 修复/首页连接配置与账户提示 | 保存密码的配置改为凭据读取完成后原子切换，读取期间禁止密码编辑和连接，并用 generation 门禁阻止旧异步结果覆盖新选择；用户名提示明确为远程主机名与用户名格式，设备卡保留完整账户名。记住密码开关和 Connect 按压动效不变。 | `tools/run_tablet_arkts_tests.ps1` 通过；`harmony/app/build_hap.bat debug` 完整构建、打包和签名通过；signed HAP 已覆盖安装到设备 `3QC0124C11000711` 并成功启动，双保存密码配置连续切换动作待设备操作确认。关联 `HOME-CONNECTION-CREDENTIAL-001`。 |
| `CHG-20260806-002` | `feat(xrdp): add OHOS rdpecam camera redirection` | 功能/xrdp OHOS 摄像头重定向 | 在 xrdp OHOS backend 接入标准 MS-RDPECAM v1/v2 动态虚拟通道，完成设备枚举、媒体类型协商和样本请求循环，并通过 N-API/ArkTS 暴露低频诊断状态；默认配置启用摄像头重定向。 | xrdp OHOS arm64 交叉编译、Debug HAP 构建签名及真机安装启动通过；Windows 11 MSTSC v2 将 `Integrated Webcam_FHD` 以 H.264 1920×1080@30 连续传入，实测至少 1500 帧/27,525,668 字节且错误为 0；无摄像头主机 3 次连接/断开错误为 0。权限拒绝、占用和热拔插待补。关联 `docs/xrdp-ohos-mstsc-penetration-plan.md`。 |
| `CHG-20260806-001` | `fix(ux): constrain minimum app window size` | 修复/应用窗口与 Compact 布局 | 为主 UIAbility 恢复适度的 `720 × 560vp` 窗口下限，保留 Compact 小窗能力，同时阻止标题截断、设备条目消失和底部状态区挤占主体的过小窗口；补回连接详情已引用但缺失的 Windows 用户名标签常量，保证完整构建可复现。 | ArkTS 策略测试和 Debug HAP 完整构建签名通过；真机覆盖安装后拖拽停在 `1368 × 1064px`（密度 1.9），标题、设备条目和四张状态卡完整显示。关联 `docs/settings-desktop-current-interactions.md`。 |

## 2026-08-05

| Change ID | 提交 | 类型/范围 | 改了什么 | 验证/关联 |
| --- | --- | --- | --- | --- |
| `CHG-20260805-007` | `fix(ux): clarify Windows connection guidance` | 修复/首页与项目帮助 | 首页被控服务状态不再显示端口；连接详情用标签和占位示例明确 Windows 用户名；项目帮助按操作顺序补充用户名识别、账号密码说明和 Windows RDP 视频硬件加速策略。 | ArkTS 策略测试与 Debug HAP 构建签名通过；真机覆盖安装后确认首页无端口、用户名示例及项目帮助 1～4 步完整渲染。关联 `docs/settings-desktop-current-interactions.md`。 |
| `CHG-20260805-006` | `feat(ohos): add API 26 persistent input permission` | 功能/HAP 权限与 xrdp OHOS 输入 | 目标 SDK 升级 API 26 并申请 `CONTROL_DEVICE`，同步调试 profile ACL；首页“验证码”状态改为“注入权限”，远控设置保留验证码门禁并增加长期注入授权；Native 优先长期权限、未授权时回退旧注入弹窗。 | ArkTS/Native 测试、xrdp OHOS 交叉编译及 Debug HAP 编译签名通过；补 ACL 后真机覆盖安装、冷启动、首页/远控设置布局和系统权限页跳转通过。实际授权、重启保持和旧弹窗回退待补。关联 `TAB-E-02`。 |
| `CHG-20260805-005` | `feat(xrdp): add OHOS audin playback backend` | 功能/xrdp OHOS 音频输入重定向 | 在 xrdp core/module ABI 中增加通用动态虚拟通道桥，并为 OHOS backend 接入标准 MS-RDPEAI `audin` 流程，将 Windows 客户端重定向的 PCM 通过 OHAudio Renderer 播放；默认配置启用 `audin`，不引入产品私有协议。 | `build-xrdp-ohos.sh` OHOS arm64 干净交叉编译及符号检查；MSTSC + 真机动作级验收待补。关联 `docs/xrdp-ohos-mstsc-penetration-plan.md` 第 9 节。 |
| `CHG-20260805-004` | `feat(freerdp): add native pen and multimon support` | 功能/FreeRDP OHOS、XComponent 输入与显示拓扑 | 接入 Native XComponent 手写笔压力/倾角/橡皮到 FreeRDP RDPEI；枚举并监听本地多显示器，在首次连接和 `disp` 动态更新中同步组合桌面布局，回到单屏时恢复现有 surface resize。ArkTS/N-API 不新增开关或页面分支。 | FreeRDP OHOS arm64 交叉编译、Native/ArkTS 测试、Debug HAP 编译与签名通过；手写笔和外接屏动作级真机验收待补。关联 PEN-MON-D1。 |
| `CHG-20260805-003` | `feat(tablet): finalize adaptive session architecture` | 功能/平板适配与会话架构 | 完成单 HAP 的平板自动旋转、首次方向同步和权限桥接收口；将首页连接校验、配置、权限、XRDP、远程会话及 XComponent 宿主职责拆分到独立协调器，并同步系统关闭行为和验收状态。忽略本地验收产物与签名辅助脚本。 | ArkTS 14/14、Native 测试和 Debug HAP 构建通过；平板覆盖安装、冷启动成功。关联 TAB-A-05/A-06、TAB-B-03、TAB-C-05、TAB-F-07。 |
| `CHG-20260805-002` | `feat(input): use native xcomponent system gestures` | 功能/Native XComponent 与触控 | 改由 Native Node API 直接创建 XComponent，并用系统 Tap、LongPress、1指 Pan、2指 Pan 识别单/双击、右键、拖动和滚动；ArkTS 只保留 NodeContent 宿主，删除手写 Touch 手势 reducer 和声明式 XComponent 所有权。 | Native/ArkTS 测试及 Debug HAP 构建通过，平板覆盖安装并冷启动成功；远端动作级手势矩阵待真机操作确认。关联 TAB-F-06。 |
| `CHG-20260805-001` | `fix(input): harden native remote gestures` | 修复/XComponent 输入 | 将触屏手势和 Axis 量化收敛为 API 22 Native 纯策略，补齐统一释放；新增 GDI/AVC 共用的远端内容几何与输入逆变换，修复 AVC 画面可见但 viewport 未由 CPU 回写时全部指针事件被拒绝的问题，同时保留黑边拒绝。 | Native/ArkTS/通用几何测试和 Debug HAP 构建通过；平板覆盖安装后普通输入恢复。双击真机仍不生效，未误报完成，后续独立修正；关联 TAB-F-04、TAB-F-05。 |

## 2026-08-04

| Change ID | 提交 | 类型/范围 | 改了什么 | 验证/关联 |
| --- | --- | --- | --- | --- |
| `CHG-20260804-035` | `fix(input): make remote double tap timing explicit` | 修复/触屏双击 | 在第二次触摸按下时立即发送第二个远端 left-down、抬起时发送 left-up，缩短两击间隔并保留真实按压时长；同步自动归因钩子重写后的 xrdp 子模块指针。 | Native 触控策略测试与 Debug HAP 构建通过，最终包覆盖安装成功；远端主机 TCP 超时，动作级双击复测待补。关联 TAB-F-03。 |
| `CHG-20260804-034` | `feat(ohos): add adaptive remote input and printing` | 功能/Native 输入、IME、打印 | 将显示方向和远程 IME 核心所有权收口到 Native，按远端文本光标与直接触摸显示/隐藏键盘，补充触屏双击判定与拖动容差；接入 HarmonyOS 打印扩展及 xrdp rdpdr 打印后端，并同步签名、模块和构建配置。 | `tools/run_tablet_native_tests.ps1`、`tools/run_tablet_arkts_tests.ps1`、`harmony/app/build_hap.bat debug` 均通过；平板完成 IME 显示/“完成”隐藏真机验证；双击最终动作因远端主机连接超时待补。关联 TAB-B-02、TAB-F-02、TAB-F-03。 |
| `CHG-20260804-033` | `docs: add repository change ledger` | 文档/流程 | 新增本统一修改台账，在文档索引增加入口，并要求后续每笔提交同步登记。 | 文档链接、提交映射、UTF-8 和 Markdown 表格检查通过；本条随该主题提交。 |
| `CHG-20260804-032` | `5937f8e` | 文档/旋转与 IME 架构 | 将显示方向和 IME 的核心所有权移至 API 22 Native：ArkTS 只传宿主窗口身份并展示键盘控制，Native 负责方向监听、IME client 和远端输入提交。 | [平板专项台账 TAB-B-02/TAB-F-01](harmonyos-tablet-adaptation-architecture-and-acceptance.md)；`git show 5937f8e`。 |
| `CHG-20260804-031` | `86996fa` | 文档/IME 架构 | 明确键盘环境由 `EntryAbility` 唯一启停，键盘高度只经 AppStorage 镜像给会话页，不进入 Native、Surface 尺寸或远端 resize。 | [平板专项台账 TAB-F-01](harmonyos-tablet-adaptation-architecture-and-acceptance.md)；`git show 86996fa`。 |
| `CHG-20260804-030` | `ec9440e` | 修复/RDP 旋转 | 将 ArkTS 侧真实显示方向传入 Native 会话，由会话统一持有方向，并在远端 resize 时发送正确的 RDP orientation；补充方向策略测试。 | [平板专项台账 TAB-B-02](harmonyos-tablet-adaptation-architecture-and-acceptance.md)；`git show ec9440e`。 |
| `CHG-20260804-029` | `f41cdb6` | 文档/RDP 旋转 | 明确显示方向由会话统一持有，补齐 ArkTS、N-API、Native 的接口边界和验收口径。 | [平板适配架构与验收方案](harmonyos-tablet-adaptation-architecture-and-acceptance.md)；`git show f41cdb6`。 |
| `CHG-20260804-028` | `5759423` | 文档/旋转与 IME | 定义旋转、远端 resize 和虚拟键盘/中文输入的后续适配方案，登记对应实施项。 | [平板专项台账 TAB-B-02/TAB-F-01](harmonyos-tablet-adaptation-architecture-and-acceptance.md)；`git show 5759423`。 |
| `CHG-20260804-027` | `3159f47` | 修复/ArkUI 兼容 | 将三个状态圆点从 `Circle` 改为带圆角的 `Column`，消除 API 22 构建兼容问题，同时保持原尺寸和颜色语义。 | [平板专项台账 TAB-D-03](harmonyos-tablet-adaptation-architecture-and-acceptance.md)；`git show 3159f47`。 |
| `CHG-20260804-026` | `a1a3b06` | 维测/RDP 会话 | 为登录、会话阶段、非敏感输入活动、resize 与断开结果增加低频结构化关键日志；统一字段并避免打印密码和高频逐事件日志。 | [平板专项台账 TAB-A-04](harmonyos-tablet-adaptation-architecture-and-acceptance.md)；`git show a1a3b06`。 |
| `CHG-20260804-025` | `c41b3fd` | 修复/RDP resize | 新增 resize 协调器和结构化结果，限制远端桌面 resize 等待时间，并在超时后执行有界 fallback，避免窗口尺寸变化长期卡住。 | [平板专项台账 TAB-B-01](harmonyos-tablet-adaptation-architecture-and-acceptance.md)、[Native 模块规范](ohos-native-cpp-module-guidelines.md)；`git show c41b3fd`。 |
| `CHG-20260804-024` | `0326453` | 文档/RDP resize | 定义平板 resize 的结构化结果、两秒等待上限和 fallback 边界，明确 Native 模块拆分要求。 | [平板专项台账 TAB-B-01](harmonyos-tablet-adaptation-architecture-and-acceptance.md)、[Native 模块规范](ohos-native-cpp-module-guidelines.md)；`git show 0326453`。 |
| `CHG-20260804-023` | `0a64531` | 修复/首页设备列表 | 取消设备列表文本的固定高度约束，使平板大字体下列表项能够随文本自然增长。 | [平板专项台账 TAB-D-07](harmonyos-tablet-adaptation-architecture-and-acceptance.md)；`git show 0a64531`。 |
| `CHG-20260804-022` | `9548d70` | 重构/RDP 会话 UI | 将远程会话 Surface/XComponent UI 从首页协调器拆入独立 `RdpSessionPage`，保持连接状态与生命周期由首页协调。 | [平板专项台账 TAB-A-03](harmonyos-tablet-adaptation-architecture-and-acceptance.md)；`git show 9548d70`。 |
| `CHG-20260804-021` | `b329f75` | 修复/Expanded 首页 | 让 Expanded 模式下的连接表单在矮窗口中可滚动，避免下方操作区域不可达。 | [平板专项台账 TAB-D-06](harmonyos-tablet-adaptation-architecture-and-acceptance.md)；`git show b329f75`。 |
| `CHG-20260804-020` | `f052817` | 功能/平板能力隔离 | 增加设备能力策略并从初始化、服务、路由和展示四层隔离 tablet 上不支持的 XRDP 服务端能力，同时保留 RDP 客户端路径。 | [平板专项台账 TAB-E-01](harmonyos-tablet-adaptation-architecture-and-acceptance.md)；`git show f052817`。 |
| `CHG-20260804-019` | `e22adac` | 功能/单 HAP 安装 | 调整产品配置、构建脚本和模块声明，使 tablet 与 2in1 复用同一 HAP，并解除 tablet 安装对产品级 HNP 的依赖。 | [平板专项台账 TAB-C-04](harmonyos-tablet-adaptation-architecture-and-acceptance.md)、[验证基线](freerdp-ohos-validation-baseline.md)；`git show e22adac`。 |
| `CHG-20260804-018` | `173626f` | 功能/设备声明 | 在 Entry HAP 中声明 tablet 设备支持，使同一应用包可被平板识别和安装。 | [平板专项台账 TAB-C-02](harmonyos-tablet-adaptation-architecture-and-acceptance.md)；`git show 173626f`。 |
| `CHG-20260804-017` | `3e75a8d` | 功能/Compact 验收 | 放开应用最小窗口限制，并调整设置页组件的触控热区和文本高度，使 Compact 窗口能进入验收。 | [平板专项台账 TAB-C-01/TAB-D-05](harmonyos-tablet-adaptation-architecture-and-acceptance.md)、[平板验收结果](../artifacts/tablet-acceptance/2026-08-04/result.md)；`git show 3e75a8d`。 |
| `CHG-20260804-016` | `8594b5a` | 功能/连接表单 | 连接表单按组件实际宽度重排，并允许文本自然增高，改善窄窗和大字体下的可读性与可达性。 | [平板专项台账 TAB-D-04](harmonyos-tablet-adaptation-architecture-and-acceptance.md)、[平板验收结果](../artifacts/tablet-acceptance/2026-08-04/result.md)；`git show 8594b5a`。 |
| `CHG-20260804-015` | `a25f564` | 修复/2in1 验收 | 修正 Expanded 首页布局以完成 2in1 自适应 UI 冒烟验证，并回写布局矩阵和截图证据。 | [平板验收结果](../artifacts/tablet-acceptance/2026-08-04/result.md)；`git show a25f564`。 |
| `CHG-20260804-014` | `2384b74` | 文档/签名 | 补充自动签名材料的生成、检查和使用说明，更新仓库入口与 HAP 验证基线。 | [验证基线：自动签名材料检查](freerdp-ohos-validation-baseline.md)；`git show 2384b74`。 |
| `CHG-20260804-013` | `d38abd6` | 修复/ArkUI 兼容 | 将状态圆点的着色方式由 `fill` 改为 API 22 可用的背景色，覆盖首页状态和设置状态组件。 | [平板专项台账 TAB-D-03](harmonyos-tablet-adaptation-architecture-and-acceptance.md)；`git show d38abd6`。 |
| `CHG-20260804-012` | `db66966` | 文档/ArkUI 兼容 | 登记 API 22 状态圆点兼容修复的目标、范围和验收项。 | [平板专项台账 TAB-D-03](harmonyos-tablet-adaptation-architecture-and-acceptance.md)；`git show db66966`。 |
| `CHG-20260804-011` | `52bb6b2` | 功能/自适应首页 | 拆分首页头部、设备列表、连接详情和状态区，并增加 Compact/Expanded 两套外层布局。 | [平板专项台账 TAB-D-02](harmonyos-tablet-adaptation-architecture-and-acceptance.md)；`git show 52bb6b2`。 |
| `CHG-20260804-010` | `1daee02` | 文档/首页架构 | 设计自适应首页的组件拓扑、共享状态和 Compact/Expanded 布局边界。 | [平板专项台账 TAB-D-02](harmonyos-tablet-adaptation-architecture-and-acceptance.md)；`git show 1daee02`。 |
| `CHG-20260804-009` | `add8bd7` | 功能/自适应设置页 | 拆分设置页共享原语和布局，隔离 Compact/Expanded 导航拓扑，并保持业务状态共享。 | [平板专项台账 TAB-D-01](harmonyos-tablet-adaptation-architecture-and-acceptance.md)；`git show add8bd7`。 |
| `CHG-20260804-008` | `5f4266a` | 文档/可访问性 | 为设置页返回按钮补充最小 48vp 触控目标要求和验收标准。 | [平板专项台账 TAB-D-01](harmonyos-tablet-adaptation-architecture-and-acceptance.md)；`git show 5f4266a`。 |
| `CHG-20260804-007` | `083f9db` | 文档/设置页架构 | 设计设置页 Compact/Expanded 拓扑隔离、共享内容和路由边界。 | [平板专项台账 TAB-D-01](harmonyos-tablet-adaptation-architecture-and-acceptance.md)；`git show 083f9db`。 |
| `CHG-20260804-006` | `b2b58dd` | 功能/窗口断点 | 在运行时监听窗口尺寸变化，并用统一策略驱动页面 Compact/Medium/Expanded 布局状态。 | [平板专项台账 TAB-A-02](harmonyos-tablet-adaptation-architecture-and-acceptance.md)；`git show b2b58dd`。 |
| `CHG-20260804-005` | `e26d8d1` | 文档/窗口断点 | 登记窗口断点运行时接入的文件范围、行为不变量和验收项。 | [平板专项台账 TAB-A-02](harmonyos-tablet-adaptation-architecture-and-acceptance.md)；`git show e26d8d1`。 |
| `CHG-20260804-004` | `de8b60f` | 功能/窗口策略 | 新增平板窗口断点纯策略、单元测试和本地测试脚本，为自适应页面提供统一布局模式。 | [平板专项台账 TAB-A-01](harmonyos-tablet-adaptation-architecture-and-acceptance.md)；`git show de8b60f`。 |
| `CHG-20260804-003` | `2b063f4` | 文档/窗口策略 | 登记平板断点策略任务，明确输入、输出、边界值和验收命令。 | [平板专项台账 TAB-A-01](harmonyos-tablet-adaptation-architecture-and-acceptance.md)；`git show 2b063f4`。 |
| `CHG-20260804-002` | `86aa844` | 文档/研发流程 | 在仓库规则中强制设计先行、实施台账和验收证据回写，并在平板方案中定义同步流程。 | [仓库协作规则](../agent.md)、[平板适配架构与验收方案](harmonyos-tablet-adaptation-architecture-and-acceptance.md)；`git show 86aa844`。 |
| `CHG-20260804-001` | `6000284` | 文档/平板基线 | 新增 HarmonyOS 单包平板/2in1 适配架构、文件清单和验收方案，并注册到文档索引。 | [平板适配架构与验收方案](harmonyos-tablet-adaptation-architecture-and-acceptance.md)；`git show 6000284`。 |
