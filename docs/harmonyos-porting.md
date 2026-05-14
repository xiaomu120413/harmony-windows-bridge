# HarmonyOS porting notes

目标是把当前 demo 从“本地应用启动 FreeRDP 客户端窗口”演进为“鸿蒙应用内嵌远程桌面画面”。

## 当前 demo

当前应用分三层：

- UI 层：`app/public/*`，负责采集连接参数、展示状态、触发连接。
- 应用服务层：`app/server.js`，负责检测 FreeRDP、测试 TCP 端口、路由连接请求。
- Native bridge 层：`native/freerdp-bridge`，负责链接 FreeRDP 三方库。当前已提供探测和连接参数入口，后续补渲染/输入回调。

`wfreerdp` 进程模式只作为兼容验证路径，不是最终的鸿蒙实现方式。

## 鸿蒙应用目标结构

迁到鸿蒙后建议拆成三层：

- ArkUI 页面：连接表单、会话列表、远程画面容器、键盘鼠标/触控输入工具栏。
- Native RDP 模块：C/C++ 编译 FreeRDP core/client，封装连接、认证、图像帧、输入事件、剪贴板、断线重连。
- 渲染/输入桥：把 FreeRDP 输出的图像帧渲染到鸿蒙 surface，把 ArkUI 触控、键盘、鼠标事件转换成 RDP input events。

## 不建议的方案

不要在鸿蒙里假设可以像 Windows 一样直接执行 `wfreerdp.exe`。鸿蒙最终应该链接 FreeRDP native library，而不是启动外部桌面客户端进程。当前 `native/freerdp-bridge` 就是为了把这条边界提前固定下来。

## 需要提前验证的技术点

- FreeRDP 及其依赖库是否能用鸿蒙 NDK 交叉编译。
- TLS、证书存储、网络 socket、线程模型在目标鸿蒙设备上的兼容性。
- 远程画面帧格式和 ArkUI/Native surface 的渲染路径。
- 输入法、组合键、鼠标右键、滚轮、多指触控如何映射到 RDP。
- 剪贴板、音频、文件重定向是否作为第一版范围。

## 推荐迭代路线

1. 当前 Node 应用验证连接参数、账号格式、证书策略和网络条件。
2. 做一个 HarmonyOS ArkUI 静态界面，复用当前表单字段和状态模型。
3. 先移植 FreeRDP 到 native module，只实现连接、认证、断开和日志回调。
4. 接入远程画面渲染，只做基础鼠标和键盘输入。
5. 再补剪贴板、分辨率变化、文件共享、重连、会话管理。
