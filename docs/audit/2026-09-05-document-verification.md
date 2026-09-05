# 文档、外链与历史证据复核

Change ID：DOC-AUDIT-20260905-001。核对日期：2026-09-05。
源码基线：`42c923f`（已推送的范围清理提交）；FreeRDP `0c242c6e2e989e4d0a2cbb183585b7167ff557fd`，xrdp `8d3facf32a7175ee1bd8b9ca35f42793c42cdca1`。

本次处理范围为主仓库31份Markdown，包含根文档、历史归档、商店草稿、Native说明和签名说明；不改第三方源码及skills。结论分为“来源可读”“源码支持”“本轮构建/测试通过”“历史证据存在”“历史证据缺失”“需要端到端验收”，不把这些状态互相替代。

逐文件覆盖范围见 [document-coverage.json](document-coverage.json)。

## 外部链接：20个原始地址逐项处理

逐条原地址、来源位置、HTTP结果、正文核对方式和替代地址见 [external-links.json](external-links.json)。行号对应复核起点的文档。

- 12个华为地址：HTTP均可达；设计入口通过网页读取核对，其余11个直接请求仅返回文档壳，已用真实浏览器加载并确认对应标题/主题。CONTROL_DEVICE、RequestInjection、QueryAuthorizedStatus锚点已核对。保留原链接。标题核对不等于文档内每条API规则均适用于API 22。
- 2个HarmonyOS示例：直接HTTP返回405，但网页读取可取得官方README。属于访问限制，不判为链接删除。
- 4个Gitee固定版本文档：3个替换为同提交的官方GitHub原文；旧shared-guide路径在镜像返回404，替换为现行 [in-app HSP说明](https://raw.githubusercontent.com/openharmony/docs/master/en/application-dev/quick-start/in-app-hsp.md)。已核对包、进程和共享库相关段落；后者是现行版本，不伪装成原提交内容。
- 原GitCode HNP页面显示路径不存在，改为 [OpenHarmony HNP开发指南](https://raw.githubusercontent.com/openharmony/startup_appspawn/master/service/hnp/README_zh.md)，其正文支持HAP分发、签名及fork/exec的说明。
- GitHub项目首页可达。

固定版本的包说明支持每设备Entry约束和App Pack分发规则，但不能证明本项目旧tablet升级或应用市场分发已验收。[HAP原文](https://raw.githubusercontent.com/openharmony/docs/43d836fe05a882d386c6c42e3827221cd2051256/en/application-dev/quick-start/hap-package.md)

## 历史图片：40处引用逐项处理

详见 [image-evidence.json](image-evidence.json)，记录原文位置、原路径、恢复路径、尺寸、SHA-256、解码结果及是否被Git跟踪。

- 34处栅格图片引用可解码，4处SVG引用可解析；其中7处原来只写了文件名，已补全为仓库相对路径。
- 2处临时截图缺失：`muhub-d07-home.jpeg`、`muhub-a03-home.jpeg`。已检查当前TEMP和仓库artifacts目录，未找到；在原记录旁明确标注缺失，不能重造历史截图冒充证据。
- 已查看现存图片缩略总览：可以确认其内容属于首页、设置、包探针和IME等记录场景；这不证明截图对应的二进制SHA、性能、20次操作循环或所有字体/输入验收结果。
- Git未跟踪的本地图片仍是本机证据，不保证新克隆可取得。本次不把包含设备桌面/网络信息的历史图片额外上传到仓库；清单保留哈希和可用性。

## 技术事实核对与处理

| 断言/范围 | 证据与本轮结论 | 处理 |
| --- | --- | --- |
| 产品范围 | 根目录桌面Demo、FTP/SFTP设计已删除；CPU编译开关无源码匹配 | 不再作为能力入口 |
| HSP/Entry/Native归属 | 模块配置、CMake及最终包校验：common拥有rdpclient，2in1拥有xrdpcontrol/HNP，tablet无被控端 | 当前说明保持；旧布局列为历史 |
| bundle/version/SDK | AppScope与build-profile；设备只读bm dump为1.0.2、common+entry、含xrdp.hnp，兼容22/目标26 | 只证明已装版本元数据；没有把新构建安装到设备 |
| xrdp独立进程 | xrdp_server_bridge.cpp的fork/execve/waitpid/kill/PDEATHSIG；进程策略测试通过 | 内嵌描述只作历史 |
| xrdp默认尺寸上限 | ohos_private.h与xrdp_ini_builder.cpp均为0；ohos_desktop_size.c仅对正数上限裁剪 | 修正旧1920×1280默认断言；该值仅保留为显式示例 |
| 显示内容比例 | ohos_desktop_size.c分别维护桌面尺寸与居中内容矩形 | 不再混同桌面、采集内容与物理显示尺寸 |
| TOFU/Strict | Index.ets固定tofu，Native具备策略支持 | 保留“Strict无页面入口” |
| 连接凭据 | WindowsConnectionStore.ets使用AssetStore SECRET，preferences保存元数据 | 源码支持；本轮未进行密码存取真机测试 |
| 默认图形路径 | 录屏强制分支已删除，FreeRDP ohos_graphics.c保留rdpgfx-h264，surface保留正常fallback | 当前构建通过；未确认新包真实会话协商/首帧 |
| location默认关闭 | FreeRDP ohos_session_config.c设置location=FALSE | 源码支持；后端存在不代表默认启用 |
| smartcard/TSMF/FUSE/CUPS | build-freerdp-ohos.sh硬关smartcard/TSMF，FUSE/CUPS默认0 | 源码支持默认构建说明；未重编第三方全部依赖 |
| 音频、摄像头、剪贴板、定位、打印、drive | FreeRDP OHOS后端及通道配置存在，包内runtime通过校验 | 仅确认源码接入/构建；端到端和拒绝权限仍引用各自验收项 |
| 权限分界 | Entry manifest、SDK oh_input_manager.h及官方动态文档；纯客户端无需录屏/输入控制权限 | SDK Quickstart补齐“仅2in1被控端”限定 |
| XComponent | Native xcomponent_native_host.cpp创建节点，ArkTS通过NodeContent/ContentSlot挂载 | 废弃旧ArkTS Controller门禁说明 |
| 手写笔/多显示器 | FreeRDP ohos_pen.c具备PenBegin/Update；Native monitor及相关测试、构建通过 | 修正“单HAP”范围；Windows Ink与外接屏端到端未重测 |
| SDK示例 | session/options/clipboard/rdpgfx头文件有对应入口；app_*为调用方占位 | 明确片段不是独立可执行程序；未单独构建第三方示例应用 |
| 签名/打包 | 当前build_hap.bat app完成重签和App Pack校验 | 新包事实见下一节；不推断AGC审核状态 |
| Native与ArkTS测试 | 本轮运行通过；ArkTS初次因CRLF/LF不一致失败，8个资源仅统一换行后通过 | 无Git内容差异；新克隆仍需关注换行环境，未声称永久修复门禁 |
| UI布局/动效/热区 | 源码路径存在，历史图可查看；当前测试通过 | 不把历史像素图当作当前多尺寸/大字体验收 |
| NV12/NV21/PCM概念 | 本地libavutil/pixfmt.h和samplefmt.h支持格式描述 | 修正UV“分辨率一半”的含糊说法；补充stride和概念示例边界 |
| AVC444/颜色故障概念 | FreeRDP primitives和应用compositor为实际实现依据 | 文档示意不是位精确算法/故障诊断；无新的图形质量结论 |
| 许可证/商店文案 | NOTICE为组件清单，store-listing为草稿 | 不将构建通过等同法律履约、设备能力全覆盖或发布批准 |
| 历史提交/截图/构建成功记录 | 日期台账保留，现存图片有哈希，临时图缺失已标明 | 历史Verified不升级为本轮Verified |

## 本轮执行结果

| 检查 | 结果 |
| --- | --- |
| tools/run_tablet_native_tests.ps1 | 退出码0 |
| tools/verify_xrdp_process_control.ps1 | 退出码0 |
| tools/run_tablet_arkts_tests.ps1 | 本地8个纯换行差异统一后退出码0；无源码内容变化 |
| harmony/app/build_hap.bat app | 退出码0，Native/ArkTS/签名/App Pack完成 |
| 包内模块、HNP、签名及ABC导入导出校验 | 通过，arktsCommonAbi=passed |
| App Pack | 39,890,794 bytes；SHA-256 c0f7fbdae23d8cea28afa09c53217f59efaa5be6217782270fb875e0806cf72d |
| 2in1 Entry HAP | 5,390,547 bytes |
| 设备核对 | 只读查询已有安装信息；未安装新包、未改授权、未建立远程会话 |

本轮构建沿用已有第三方runtime输入，不等同FreeRDP/xrdp及全部依赖从零构建。原始构建/ArkTS日志在本机tmp目录；日志未直接发布，避免包含本地签名与环境信息。上表保留可复核的命令、退出状态和产物哈希。

## 仍需实际环境才能完成的验证

1. 两张已丢失的临时截图只能从原备份恢复；新截图只能作为新的验收，不能替代历史证据。
2. 新包的RDP认证、首帧、音视频/打印/文件往返、拒绝权限、断连恢复，需要对应Windows服务、外设和实际操作。
3. 外接屏、Windows Ink、不同字体/窗口和tablet升级矩阵需要对应设备；本轮只读确认一台2in1在线。
4. 商店分发、旧tablet无损升级、正式签名ACL/审核必须单独完成。链接、源码和本地包验证不能证明这些结论。

这些条目已经从笼统的“未逐项复验”改为具体缺口；它们没有被标记为通过。
