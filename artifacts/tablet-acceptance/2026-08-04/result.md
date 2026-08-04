# MuHub 平板/2in1 真机验收记录

日期：2026-08-04

## 环境

- 设备：HUAWEI MateBook Pro（`HAD-W32`）
- 设备序列号：`3QC0124C11000711`
- 设备类型：`2in1`
- 系统 API：26
- 屏幕：3120 × 2080 px
- 应用窗口：2432 × 1444 px，对应当前 manifest 最小窗口 1280 × 760 vp
- 包名：`com.muhub.desktop`
- 安装产物：`harmony/app/entry/build/default/outputs/default/entry-default-signed.hap`
- 产物大小：41782896 字节

## 已通过

1. 自动 `SignHap`、HNP 重签及 `hdc install -r` 均成功，系统验签和 HNP 解包通过。
2. Home Expanded：设备列表、连接详情、设置入口和四个状态入口同时可见，无横向溢出或遮挡。
3. Settings Expanded：左侧导航和右侧内容区同时可见；概览、基础设置、远控设置、项目帮助四个路由均可达，内容成功加载。
4. 真实触控点击 Windows host 输入框后系统输入面板出现；输入 `10.0.0.1` 后 TextInput 状态正确更新，应用窗口尺寸和布局未跳变。
5. ArkTS 平板策略测试：`tools/run_tablet_arkts_tests.ps1` 退出码 0，Hvigor `BUILD SUCCESSFUL`。

## 未通过或尚未覆盖

1. Compact 真机布局被配置阻塞：`module.json5` 的 `minWindowWidth=1280`、`minWindowHeight=760` 已经是当前窗口下限，鼠标及触控拖拽均不能继续缩小，无法在该设备触发小于 840vp 的 Compact 断点。
2. tablet、竖屏、分屏和旋转未覆盖：当前 HAP 只声明 `2in1`，本次也没有 tablet 真机。不能据此声称 tablet/rotation 已适配。
3. XComponent 会话、远端 resize、GDI/AVC420/AVC444、远端输入与会话内虚拟键盘未覆盖：没有可用的 RDP 服务端地址和凭据。
4. 中文 IME 组合提交、删除键、软键盘 Backspace 未覆盖；本次只证明输入面板显示和 ASCII 文本提交。
5. 设置页内使用系统 Back 键会把应用退到桌面，没有走应用内设置返回；左上角应用内返回按钮工作正常。该行为需要单独设计后再决定是否修改。
6. Tab 焦点注入未从 ArkUI dump 中获得控件级 focused 证据，焦点顺序暂不判通过。

## 结论

当前候选包只能证明 API 26 2in1 的 Expanded 首页、Expanded 设置页、基础触控输入和签名安装通过。`TAB-D-01`、`TAB-D-02` 继续保持 `Implemented`，不能升级为 `Verified`。下一步应先完成会话 resize/fallback 底座，再按设计降低最小窗口、声明 tablet/旋转/分屏，之后补 Compact、tablet、XComponent、触控和 IME 完整矩阵。

## 证据

- `muhub-home-expanded.jpeg`
- `muhub-settings-real.jpeg`
- `muhub-ime-open.jpeg`
- `muhub-layout-home.json`
- `muhub-layout-settings.json`
- `muhub-layout-settings-basic.json`
- `muhub-layout-settings-remote.json`
- `muhub-layout-settings-help.json`
- `muhub-layout-ime-text.json`
- `layout-matrix.csv`
