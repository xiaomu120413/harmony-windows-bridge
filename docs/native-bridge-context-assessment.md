# native_bridge_context god object 评估结论（M5）

## 评估结论
M5 的"god object"描述**被夸大**，不建议强行迁移。依据 2026-09-15 只读评估：

## 事实
- `native_bridge_context.cpp` **493 行**（在 500 行软上限内，远低于 `ohos-native-cpp-module-guidelines.md` 1000 行硬上限）。
- 公共 API 仅 7 个函数（`native_bridge_context.h` 23 行）。
- `ohos-native-cpp-module-guidelines.md` 第 3 行明确：「native_bridge_context 连接依赖项并序列化显示调度」——显示调度序列化**被指定保留在此文件**。文档第 313–325 行的迁移计划（6 个拆分）已基本完成，native_bridge_context 被设计为剩余的轻量连线外壳。
- 之前审计报告称"文档要求 resize/monitor 编排归 rdp_display_*"——**转述有误**，文档无此要求。

## 真正违反点（仅两处）
1. **全局变量**（28–37 行）：9 个单例实例（g_events/g_surface/g_session 等）作为文件全局变量，违反 guidelines 第 250 行"避免在拥有模块之外使用全局可变状态"。
2. **resize 调度算法**（~149 行，职责 #3）：表面快照→请求构建→coalescer 调度→锁应用+协调器/IME 重置。guidelines 批准留在此处（连线逻辑），但它是唯一远超"纯连线"的逻辑块。

## 不迁移的理由
- **风险高**：resize 调度涉及 6 个全局变量 + 递归互斥锁，横跨 surface/resize/IME/渲染。编排变更会**静默影响** resize/surface/IME/渲染行为，无真机验证下无法发现回归。
- **收益微**：493 行已在限制内，迁移后降至 ~300 行，但合规收益不显著（连线逻辑本就批准留此）。
- **需新建模块**：职责 #3 无现有承接模块（rdp_display_* 4 文件家族都不拥有 surface-snapshot→request→coalesce→apply 调度），迁移要新建 `rdp_display_dispatch` + 参数化 6 个全局变量依赖（为 host 可测），超出"移动代码"范畴。

## 可选低风险改进（如未来要做）
1. 全局变量（28–37）移到拥有者模块访问器（`RdpSession::Instance()` 等），触及 5 个调用点（napi_exports/display_settings_exports）。机械操作但需排序单例构造。风险中。
2. resize 调度（~149行）提取到新 `rdp_display_dispatch` 模块 + 参数化依赖。风险高，建议有真机 + C++ host 测试框架后单独立项。
3. XComponent 原始注册结构体（~30行）移到 `surface/xcomponent_native_host.*`（已存在）。风险低-中，机械移动。

## 状态
M5 评估完成，**建议保留现状**（493 行在限制内，文档批准连线逻辑留此）。真正违反点（全局变量 + resize 调度）迁移风险高收益微，留后续单独立项（需真机 + host 测试）。不作为当前修复项强行执行。

## 关联
- 评估依据：`ohos-native-cpp-module-guidelines.md` 第 3、157-161、169-178、192-198、250、313-325 行
- 相关文件：`harmony/app/common/src/main/cpp/napi/native_bridge_context.cpp/.h`、`session/rdp_display_resize_coordinator.h`、`session/rdp_display_request_coalescer.h`、`session/rdp_display_layout_monitor.h`、`surface/surface_bridge.h`、`surface/xcomponent_native_host.h`
