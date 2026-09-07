# 鸿蒙客户端会话缩放与悬浮功能球调研

日期：2026-09-07；Change ID：CHG-20260907-002；关联：SESSION-ZOOM-001。
状态：ResearchComplete / Proposed，尚未实现。源码基线：`3b174ae`。

## 1. 范围与结论

用户确认问题方向为**鸿蒙连接 Windows，查看 Windows 桌面时显示太大**，此前又出现过太小。目标用户是2in1/平板上的远程桌面使用者；本轮研究当前实现和首版交互，不涉及Windows MSTSC控制鸿蒙，也不重新调整已验收的分包方式。

建议采用**会话内可拖动、贴边收起的功能球，点击展开带文字的短面板**。首版提供缩小、放大、当前比例、适应窗口、原始比例、移动画面；不做环形菜单，不增加系统级悬浮窗权限，不自动改变Windows分辨率。面板复用当前SettingsTheme及描边图标风格，视觉稿仍需后续确定。

“太大”的具体原因尚未复现。源码已确认存在两个独立影响因素：本地等比适配会放大小画面；显示请求还包含远端desktopScaleFactor。必须记录两者，不能认定功能球已经修复DPI问题，也不能继续靠改一个默认分辨率反复试大小。

## 2. 已核对的实现

路径均相对仓库根目录，列出的代码事实为本轮静态核对，不代表新增真机结果。

| 位置 | 当前行为 | 对设计的影响 |
| --- | --- | --- |
| `harmony/app/common/src/main/ets/components/session/RdpSessionPage.ets` | Stack中放置ContentSlot承载Native XComponent，上层已有连接提示；键盘避让为NONE | 可增加同一窗口内的兄弟控件；保留Surface实际尺寸，不用工具栏挤压Surface |
| `harmony/app/common/src/main/cpp/surface/remote_content_geometry.cpp` | `FitRemoteContentIntoTarget`等比居中；接近原始尺寸时有16px容差，否则允许放大到目标区域 | 当前没有用户可控zoom/pan；小源画面也可能被放大 |
| `surface/native_rgba_copy.cpp`（同一cpp根目录） | `FitFrameIntoTarget`复用上述计算 | 不能只改UI比例而保留旧Native几何 |
| `surface/avc420_gpu_compositor_internal.cpp`、`avc444_gpu_compositor_internal.cpp`、`gpu_rgba_renderer.h`、`native_window_rgba_painter.cpp` | AVC420、AVC444、RGBA GPU和CPU后备分别使用输出viewport | 四条呈现路径必须共用一个视图变换；AVC444中间重建pass不能跟着缩放 |
| `session/rdp_session_input.cpp` | `BuildOhosPointerViewport`读取SurfaceSnapshot并生成FreeRDP指针viewport | 显示与点击映射必须绑定到同一已呈现状态 |
| `harmony/third_party/FreeRDP/client/OHOS/ohos_pointer.c` | 通过viewport位置/大小将本地坐标映射到远端；参数使用无符号坐标 | 不能把负的平移偏移直接塞进现有接口，否则会溢出/错误夹取 |
| `session/rdp_display_layout_monitor.cpp` | 以densityDPI/160计算远端desktopScaleFactor，并选择支持档位 | 远端UI大小与本地像素缩放不同，应独立诊断 |
| `napi/native_bridge_context.cpp` | `CurrentSurfaceResizeRequest`使用Surface尺寸；单屏时附带monitor的物理尺寸和scale | 功能球的zoom操作不能调用这里，也不能用布局放大Surface引发此链路 |
| `input/xcomponent_native_gesture.cpp` | 单指拖动、双指滚动、单/双击、长按已绑定Native识别器 | 直接叠加全屏ArkTS双指手势会争抢既有输入，首版先不做 |

没有本次异常截图、实际协商尺寸和DPI日志，不能确定此次过大是DPI变化、等比放大、窗口适配或远端应用自己的缩放造成。现有诊断入口包括 `freerdp/freerdp_gdi_bridge.cpp` 的scale日志和 `native_bridge_context.cpp` 的display request日志，先利用这些，必要时再补信息。

## 3. 同类产品与平台依据

| 来源 | 核对内容 | 本项目采用方式 |
| --- | --- | --- |
| [AnyDesk Display settings](https://support.anydesk.com/docs/display) | 查看模式与远端自动分辨率调整是不同选项；会话工具栏可随时调节显示 | 保留会话内入口，把本地查看和远端设置分开；不采用非等比拉伸 |
| [Windows App输入说明](https://learn.microsoft.com/en-us/windows-app/input-keyboard-mouse-touch-pen) | iOS/iPadOS鼠标指针模式明确区分本地捏合缩放和发送到远端的手势 | 明确本地/远端事件归属；不能把本地缩放转成远端Ctrl+滚轮 |
| [Microsoft RDP属性](https://learn.microsoft.com/en-us/azure/virtual-desktop/rdp-properties) | smart sizing与dynamic resolution分为两个属性 | 仅作为概念区分依据，不照搬AVD客户端属性到FreeRDP，也不将属性弃用说明等同协议字段弃用 |
| [Huawei触摸穿透FAQ](https://developer.huawei.com/consumer/cn/doc/doccenter-dev-faq/faqs-arkui-863) | 官方搜索摘要说明Transparent可能使下层也响应，建议Default/Block；正文读取失败 | 作为待验证风险，不作为所有XComponent事件可自动隔离的证明；本地SDK有hitTestBehavior声明 |

公开社区搜索也出现“画面太小”“窗口边缘自动平移”等讨论，但缺少可比设备和复现条件，不据此估算发生频率或判断本项目根因。首要证据仍是用户反馈及本项目代码。

## 4. 功能球与操作流程（建议）

### 收起与展开

- 仅远程会话显示。圆形视觉40vp、可点击区域至少48vp，默认右侧中上部，避开远端标题栏常用按钮；具体默认纵向位置需真机确认。
- 可拖动，松手贴最近左右边，保存边和归一化纵向位置；全屏/窗口/旋转后重新限制在安全区域。球不可拖出可见范围，拖动结束不触发点击。
- 点击球展开短面板；靠左向右展开、靠右向左展开。窄窗使用两行而非缩小控件；复用深浅主题、圆角、描边图标与文字标签。
- 不自动展开，不默认隐藏成难以点击的半圆。点击关闭或面板外区域收起；用于收起的那次按下/抬起整段由本地消费，不能顺便点击远端。
- 球、面板和状态提示不改变Surface尺寸，不添加系统悬浮窗权限。离开会话必须销毁，不能留在设置页或被控端页面。

### 首版动作

| 控件 | 行为 | 显示与边界 |
| --- | --- | --- |
| 缩小 / 放大 | 当前本地比例÷1.25 / ×1.25，保持画面中心所对应的远端点 | 临界值禁用，不改变Windows分辨率/DPI |
| 比例 | 显示实际源像素到Surface像素比例，如75%、125% | 百分比不是Windows显示缩放；高级说明写“画面缩放” |
| 适应窗口 | 保持长宽比，完整显示并居中；允许放大 | 显示“适应 · 75%”一类状态，而不是把所有适配都称为100% |
| 原始比例 | 1个远端像素对应1个Surface物理像素，超出时裁切且允许平移 | “原始比例100%”不表示物理尺寸相同，也不改变系统字体大小 |
| 移动画面 | 切入本地浏览模式，拖动画面只改变平移 | 常驻“移动画面中”提示和“完成”；退出后才恢复远端操作 |

建议手动缩放范围为25%～300%，作为待验产品参数；适应窗口可超出此范围以保证超大桌面仍能完整显示。若当前适配值低于25%，放大沿1.25倍逐步接近25%，缩小禁用；高于300%则缩小逐步回到范围，放大禁用，避免一步跳到边界。

首版新会话默认“适应窗口”。手动比例及平移不跨连接永久保存，避免另一台电脑/不同方向打开就过大；功能球位置可本机保存。后续如需要记忆比例，应按连接与显示环境保存，而非全局一个倍率。重置统一等价于“适应窗口并居中”，无需再放一个含义重复的按钮。

**重要限制**：本地缩小只把现有远端画面整体缩小，不能让Windows在同一桌面内容纳更多内容。如果用户说的“太大”实际是Windows字体/控件过大，仍需处理远端DPI或分辨率；不能用缩小加黑边冒充解决。

### 输入隔离

- 普通模式保留现有单指拖动远端窗口、双指滚动、长按右键和鼠标滚轮行为。首版不抢占普通Ctrl+滚轮，因为远端浏览器/编辑器已使用它。
- 浏览模式下，鼠标或单指拖动只平移画面；滚轮和触控操作不得发送远端。功能球不随画面一起缩放或平移。
- 进入浏览模式或本地交互前安全释放已按下的远端按钮/按键，结束时不能补发一次远端点击；失焦、断线、取消也释放输入。控件关闭后不要把本地拖动的残留抬起交给远端。
- 笔默认继续走远端手写路径；画面缩放后笔坐标同样必须变换。第一版不让捏合改变缩放，第二阶段才考虑在Native识别层加入捏合与双指滚动的仲裁。
- 测试真实Surface的叠层、触摸、鼠标、Axis和键盘焦点；仅设置一次hitTestBehavior不算验收。全屏容器不能无条件Block导致远端完全不能操作。

## 5. 技术方案与坐标约定

推荐在现有Native呈现层实现视图变换，ArkTS只负责功能球、按钮、状态呈现及发送意图。首版保留已有FreeRDP和xrdp协议代码，不改分包、签名身份和Native库归属。

### 一个变换、一份已呈现状态

用Surface**物理像素**作为渲染/指针共同坐标；ArkTS功能球布局仍用vp，两者不要混用。

设远端尺寸Wr/Hr，Surface尺寸Ws/Hs；原始比例s=1，适应比例sFit=min(Ws/Wr,Hs/Hr)。图像左上角为tx/ty：

```text
surfaceX = remoteX * s + tx
surfaceY = remoteY * s + ty
remoteX = (surfaceX - tx) / s
remoteY = (surfaceY - ty) / s
```

每轴独立限位：若远端绘制宽度D≤Surface宽度S，偏移固定为(S-D)/2；否则偏移限制为[S-D, 0]。缩放以当前可见中心为锚点：先求旧变换下的远端中心，再计算新tx/ty并限位。所有偏移用浮点/有符号数，禁止沿用uint32内容偏移表示放大后的负坐标。

新增纯策略对象 `SessionViewTransform`，记录mode、scale、offset、远端尺寸、Surface尺寸、revision。各渲染路径在最终输出阶段应用同一变换，裁剪到Surface并清黑边；不要对AVC444中间解码/色彩重建纹理进行平移。CPU后备也必须支持源区域采样，否则切换后画面会跳回原比例。

输入读取最后成功呈现的变换快照，不能先更新点击坐标、后显示旧画面。静态桌面点击缩放也需请求缓存帧重绘；若该codec路径不能重绘，作为实现阻断项处理，不能等远端下一次动一下才更新。

现有FreeRDP pointer viewport无法直接表达裁剪源区域和负偏移。建议应用Native层先完成逆变换，再以远端尺寸的恒等viewport调用现有输入接口，避免第二次缩放；需要同时覆盖mouse/touch/pen及文本命中检测，并验证此前的边界和按钮释放语义。不能只改其中一个事件入口。

远端指针若单独绘制，也使用同一正变换；若已合成到帧内则避免重复处理。IME候选定位或文本命中检测凡涉及远端坐标都需排查；不改变现有KeyboardAvoidMode.NONE，软键盘出现不自动请求远端resize。功能球与面板应避开键盘遮挡区域。

### 与动态分辨率的关系

按钮缩放：只改变view transform，**不调用**CurrentSurfaceResizeRequest、不改变XComponent布局尺寸、不发disp。

用户改变窗口/旋转：保留现有动态分辨率链路。Fit模式按最终桌面尺寸重新适配；100%保持1:1；手动模式尽量保持倍率和远端归一化中心，收到新桌面尺寸后重新限位。远端DPI是否需要用户配置留作独立问题，不能在功能球中偷偷改变。

## 6. 文件级落点（均为建议，尚未修改）

| 层 | 文件/新增模块 | 职责 |
| --- | --- | --- |
| ArkTS UI | 修改`components/session/RdpSessionPage.ets`，新增同目录`SessionFloatingControls.ets` | 贴边位置、展开、主题、可访问性、控件点击与本地交互状态 |
| ArkTS会话边界 | `rdp/NativeRdpGateway.ets`、`rdp/RdpClientController.ets`及Native类型声明 | 有类型的setViewMode、zoomBy、panBy/getViewState等候选接口；最终签名实施前确定 |
| N-API | `napi/napi_exports.cpp`注册/转发，新增小型view接口模块 | 参数验证与转发，不放缩放数学/手势状态机 |
| Native策略 | 新增`surface/session_view_transform.h/.cpp`；更新geometry和Surface快照 | 统一缩放、平移、边界、revision及已呈现快照 |
| 呈现 | AVC420/AVC444 compositor最终pass、RGBA renderer和CPU painter、render output owner | 每条路径共用变换、重绘缓存帧、发布已呈现revision |
| 输入 | `session/rdp_session_input.cpp`、`input/xcomponent_native_gesture.cpp`及mouse/pen/text命中模块 | 本地浏览模式、逆变换、取消/释放、防止穿透 |
| 验证与文档 | Native几何测试、现行适配方案/交互文档/功能矩阵、Release包门禁 | 落实测试与回写证据，维持公共HSP导出稳定 |

单独给ContentSlot加ArkTS scale实现较短，但Surface合成、命中坐标、裁剪和系统尺寸回调不能据此保证一致，因此不推荐为正式方案。修改远端分辨率/DPI能影响Windows布局，但与“本地查看大小”不是同一功能，不作为功能球按钮实现。

## 7. 实施顺序与验收

1. **尺寸诊断与小型验证**：记录Surface物理尺寸、收到的远端帧尺寸、显示请求scale、实际协商scale、内容矩形、渲染路径。抓一次“太大”的现场；确认现有缓存帧能否重绘，验证Surface上层按钮不会穿透。这一步先于大范围UI改动。
2. **本地缩放闭环**：统一Native变换和输入映射，完成Fit/100%/手动缩放/平移。四条呈现路径都覆盖，先通过自动几何测试，再上机。
3. **功能球交互**：落地贴边、面板、浏览模式、焦点和可访问性；按用户选定视觉样式制作图标与布局。保持已验收分包方式，最后出Release正式签名包。
4. **后续增强**：视反馈加入Native捏合仲裁、每连接显示偏好、键盘/全屏等更多功能。第一版不塞断开连接等高风险动作，不做无条件自动边缘平移。

| 验收项 | 通过条件 |
| --- | --- |
| 大/小桌面 | 1280×720、1920×1080、3840×2160及非整数窗口比例；Fit可完整看到，100%真实1:1，缩小/放大双向可用 |
| 定位 | 缩放和平移后九点点击/拖动/笔一致；换算误差≤1远端像素；黑边不产生新远端按下，拖出范围后的释放不丢 |
| 渲染 | AVC420、AVC444、RGBA GPU、CPU后备结果一致；静态帧立即响应缩放；切换路径不跳比例，不拉伸变形 |
| 生命周期 | 窗口/全屏/旋转/断连/重连/键盘出现后球仍可触达，无跨会话残留变换；Surface零尺寸时安全禁用 |
| 输入隔离 | 球拖动、面板点击、关闭面板、浏览模式各执行20次，远端无额外点击、滚轮、按键或卡住的按钮 |
| 协议分界 | 缩放/平移时没有额外disp请求；实际窗口变化仍遵循现有debounce和远端支持能力 |
| UI | 普通/深色主题、鼠标和触摸、48vp热区、大字体和窄窗可操作；球不挡主要内容且能换边 |
| 交付 | Native/ArkTS检查、Release正式签名、HSP ABC门禁通过；只交付一个最终App，不把构建通过等同真机缩放通过 |

本轮完成源码与公开资料调研、提出方案和验收口径；未更改产品代码，未安装或重建App，未宣称“太大”根因已确认。用户频率数据缺失，优先级依据当前明确反馈：P1缩放可控与输入正确；P1防穿透；P2捏合和记忆偏好。
