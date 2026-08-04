# MuHub 平板/2in1 真机验收记录

日期：2026-08-04

## 环境

- 设备：HUAWEI MateBook Pro（`HAD-W32`）
- 设备序列号：`3QC0124C11000711`
- 设备类型：`2in1`
- 系统 API：26
- 屏幕：3120 × 2080 px
- 应用窗口：Expanded 2432 × 1444 px（1280 × 760 vp）；解除应用声明下限后，Compact 证据窗口 1437 × 1229 px（约 756 × 647 vp，密度 1.9）
- 包名：`com.muhub.desktop`
- 安装产物：`harmony/app/entry/build/default/outputs/default/entry-default-signed.hap`
- 产物大小：41788522 字节

## 已通过

1. 自动 `SignHap`、HNP 重签及 `hdc install -r` 均成功，系统验签和 HNP 解包通过。
2. Home Expanded：设备列表、连接详情、设置入口和四个状态入口同时可见，无横向溢出或遮挡。
3. Settings Expanded：左侧导航和右侧内容区同时可见；概览、基础设置、远控设置、项目帮助四个路由均可达，内容成功加载。
4. 真实触控点击 Windows host 输入框后系统输入面板出现；输入 `10.0.0.1` 后 TextInput 状态正确更新，应用窗口尺寸和布局未跳变。
5. ArkTS 平板策略测试：`tools/run_tablet_arkts_tests.ps1` 退出码 0，Hvigor `BUILD SUCCESSFUL`。
6. `TAB-D-04` 连接表单改为组件宽度响应式：2in1 Expanded 保持3/9标签与输入分栏，固定190vp标签列及36～52vp文本行高已清理；完整 HAP 构建、HNP 重签、覆盖安装和 Expanded 截图回归通过。
7. `TAB-D-05` 设置页6处代码约束已改为最小48vp：Expanded桌面导航实测91～92px；Compact本机网络刷新和4个远控操作按钮原始边界均为91px（密度1.9取整对应48vp）。基础设置、远控设置及底部共享目录操作均可滚动到达。
8. `TAB-C-01` 已删除应用声明的 `minWindowWidth/minWindowHeight`。同一包在真机可缩至约756×647vp，Home切换为Compact单页设备列表，Settings切换为无桌面侧栏的Compact概览，跨断点状态监听生效。

## 未通过或尚未覆盖

1. 精确600/839/840vp、最小高度480vp和1.75字体尚未覆盖；当前Compact真机证据为约756×647vp，不能替代完整边界矩阵。
2. tablet、竖屏、分屏和旋转未覆盖：当前 HAP 只声明 `2in1`，本次也没有 tablet 真机。不能据此声称 tablet/rotation 已适配。
3. XComponent 会话、远端 resize、GDI/AVC420/AVC444、远端输入与会话内虚拟键盘未覆盖：没有可用的 RDP 服务端地址和凭据。解除窗口下限不代表小窗远程会话已可发布。
4. 中文 IME 组合提交、删除键、软键盘 Backspace 未覆盖；本次只证明输入面板显示和 ASCII 文本提交。
5. 设置页内使用系统 Back 键会把应用退到桌面，没有走应用内设置返回；左上角应用内返回按钮工作正常。该行为需要单独设计后再决定是否修改。
6. Tab 焦点注入未从 ArkUI dump 中获得控件级 focused 证据，焦点顺序暂不判通过。

## 结论

当前候选包已证明 API 26 2in1 的 Expanded 与约756×647vp Compact 首页/设置页、设置按钮热区、基础触控输入和签名安装通过。`TAB-D-01`、`TAB-D-02`、`TAB-D-05`、`TAB-C-01` 保持 `Implemented`，精确边界、1.75字体、tablet和小窗XComponent会话未通过前不能升级为 `Verified`。下一步仍需完成会话 resize/fallback 底座，再声明 tablet/旋转/分屏并补齐完整矩阵。

## 证据

- `muhub-home-expanded.jpeg`
- `muhub-settings-real.jpeg`
- `muhub-ime-open.jpeg`
- `muhub-d04-expanded.jpeg`
- `muhub-d05-settings-expanded.jpeg`
- `muhub-d05-basic-compact.jpeg`
- `muhub-d05-remote-compact-scrolled.jpeg`
- `muhub-c01-home-compact.jpeg`
- `muhub-c01-settings-compact.jpeg`
- `muhub-layout-home.json`
- `muhub-layout-settings.json`
- `muhub-layout-settings-basic.json`
- `muhub-layout-settings-remote.json`
- `muhub-layout-settings-help.json`
- `muhub-layout-ime-text.json`
- `muhub-layout-d05-basic-compact.json`
- `muhub-layout-d05-remote-compact.json`
- `muhub-layout-c01-home-compact.json`
- `muhub-layout-c01-settings-compact.json`
- `layout-matrix.csv`
