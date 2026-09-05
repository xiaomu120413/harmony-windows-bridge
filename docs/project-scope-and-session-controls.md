# 项目范围与远控工具栏

## SCOPE-20260905-001 — Verified（源码清理）

用户要求聚焦 HarmonyOS 远程桌面：删除独立 FTP/FTPS/SFTP 客户端设计、根目录桌面 Web/Node Demo（包括桌面 native 骨架、package.json 和示例配置），移除 CPU-only 录屏编译分支。保留 HarmonyOS 的 FreeRDP 客户端、xrdp 被控端、RDP 下载目录重定向、正常 GDI fallback 和签名打包工具。

文件范围：根 README、docs 索引与 feature matrix、app/ 下受版本管理的文件、根 package.json/config.example.json、common/build-profile.json5、Native CMakeLists.txt、napi_exports.cpp、surface_bridge.cpp。历史 changelog 保留，并记录后续删除决定。

验收：桌面 Demo 受版本管理文件清空；现行文档无已删除入口；Native 源码不再含录屏编译开关；正常 GPU/fallback 路径保留；Native 回归通过。

验证记录：2026-09-05，Native 回归脚本通过，git diff --check 通过；已移除 10 个受版本管理的文件，现行 README/索引同步清理。未重新构建 HAP，未做真机验证；ArkTS 仍有此前确认的资源换行差异门禁问题。忽略规则保留，用于隔离本机可能残留的旧构建产物。

## SESSION-ZOOM-001 — Planned

远控会话需要易于触达的缩放入口。初步方案为可收起悬浮工具栏，提供放大、缩小、适应窗口与重置；只改变本地显示，避免按钮触发 Windows 分辨率变化。具体交互待用户反馈后确定。

实现必须同步保证画面裁切、平移和输入坐标一致，工具栏点击不得传入远程会话；重置和新会话恢复初始视图。不能仅放大图片而不处理输入位置。


## DOC-CLEAN-20260905-001 — Verified（文档静态核对）

现行 README、运行库/签名基线、功能矩阵、设置页面路径、Native 进程职责及 xrdp HDC 说明已按源码修正；早期穿刺、整改、MSTSC sizing 与旧签名记录明确标为历史参考。文档索引覆盖现行入口和参考资料，缩放工具栏保持 Planned。

验证：2026-09-05，检查根 README、HarmonyOS README 与 docs 顶层 Markdown 的 75 个本地链接目标，均存在；diff 空白检查通过。本轮仅修改文档，不新增功能，不重复声明构建或真机验证。


补充复查（2026-09-05）：扫描 31 份仓库 Markdown（含历史归档、根目录 QA、商店文案、签名目录说明及本文；不改第三方源码和 skills 指令），核对 83 个本地 Markdown 链接目标均存在。补齐多设备方案首页实施状态、平板当前 Native/XComponent 导航、规划门禁脚本未实现标记、旧签名说明和商店文案设备范围。历史台账中的旧路径保留并明确标记，外部链接可达性、历史截图是否仍在本机及全量技术事实未逐项复验，不能由本次文档检查推断产品已完成发布验收。diff 检查退出码 0；客户端源码不再包含录屏编译开关。


## DOC-AUDIT-20260905-001 — 文档复核与构建验证完成

已完成外部来源核对、历史图片清点、技术声明分类和完整App构建；2张原临时截图缺失，新包端到端与商店/升级验收未执行。最新结果及原始清单见 [复核报告](audit/2026-09-05-document-verification.md)，此前的未构建/未核对描述仅代表此前阶段。缩放仍为Planned。
