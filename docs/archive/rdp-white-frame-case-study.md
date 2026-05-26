# RDP 连接白帧问题定位复盘

日期：2026-05-18

状态：白帧问题已定位并修复。后续全屏视频场景的粉色帧属于 codec negotiation / fallback 路径问题，应该单独成案，不和这个白帧问题混在一起。

## 结论

RDP 连接启动阶段出现一帧全白，不是 ArkTS 背景、不是 `XComponent` 外层 `Stack`、也不是 GLES clear color。真正原因是 native 在 `rdpgfx connected` 之后主动请求了一次当前帧 repaint，但当时 FreeRDP 的 GDI primary buffer 还没有收到任何真实 remote update，里面还是 resize/init 后的白色像素。

最终修复不是把画面涂黑，也不是隐藏页面，而是给 FreeRDP GDI primary 加 readiness gate：只有真实 `HarmonyEndPaint` 到达后，当前 GDI buffer 才允许被当成可渲染帧。

## 现象

用户可见顺序：

1. 会话页面打开。
2. 顶部能看到 debug 状态文字。
3. 出现一帧全白。
4. 接着出现黑色画面。
5. 最后远端桌面正常显示。

关键点：白帧发生时 session surface 已经显示，但远端首帧还没有真正进入渲染链路。

## 当时为什么做了规避

### 1. ArkTS first-frame gate

当时判断：

白帧发生在连接早期，又和 `XComponent` 显示时机接近，所以直觉上像是 ArkUI 页面、`XComponent` 默认背景、或者 surface 还没准备好时露出的一帧。

做过的方向：

用 ArkTS 状态控制 session 页面或 remote surface 的显示，尝试等第一帧之后再展示。

为什么这是规避：

它只改变了“什么时候显示”，没有证明白色像素来自哪里。如果白色已经来自 native 的 remote frame buffer，隐藏/延迟只是挡住现象，不是修正数据契约。

反思：

单帧颜色问题不能先从 UI 可见性入手。应该先打像素采样，确认白色来自 ArkUI、NativeWindow、GLES 还是 FreeRDP source buffer。

### 2. 去掉 XComponent 外层 Stack / wrapper

当时判断：

外层 `Stack` 或 wrapper 可能带了默认背景，或者布局切换时让白色区域露出来。

做过的方向：

简化 ArkUI 结构，把 `XComponent` 外面的包装层去掉。

为什么这是规避：

清理 UI 层级本身可以做，但它仍然没有回答“GLES 上传前那一帧是不是白的”。如果 native 已经上传白色 frame，去掉 wrapper 不会解决根因。

反思：

UI 结构调整应该基于证据。只有证明源帧正常、屏幕异常，才应该优先怀疑 ArkUI composition / XComponent wrapper。

### 3. native 层 created/change 时刷黑

当时判断：

`XComponent` / `NativeWindow` 可能在首个 EGL buffer swap 前展示系统默认白色 buffer，所以试图在 surface created 时先刷一帧黑。后来也讨论过 changed 是否要处理。

做过的方向：

尝试在 native surface created / changed 时主动 clear 黑色。

为什么这是规避：

如果问题是 NativeWindow 的未初始化 buffer，刷黑可能有效。但后面日志证明，白色是在一次显式 render 中从 FreeRDP GDI primary 上传出来的。也就是说，即使 created 时刷黑，只要后面主动 repaint 了白色 GDI buffer，白帧仍然会出现。

反思：

clear color 只能解决“没有内容时显示什么”，不能解决“错误内容被当成有效帧上传”。用户指出“不要猜，先加文字和日志”是对的。

## 正确定位链路

后面改为打通整条渲染链路，而不是继续猜 UI 层。

### ArkTS 状态线

在 ArkTS 页面上加 debug line，展示：

- sequence；
- timestamp；
- raw RDP state；
- mapped RDP state；
- session 是否显示；
- native log 转发。

目的：把肉眼看到的白帧和 RDP 状态、surface 生命周期、native 日志放到同一条时间线上。

### GDI source 采样

在 native 侧采样：

- `DesktopResize` 后的 `gdi->primary_buffer`；
- 第一次真实 `HarmonyEndPaint`；
- GDI frame queue 前；
- render thread 上传前。

关键日志形态：

```text
FreeRDP desktop resize GDI primary sample:
sample tl=255,255,255,255 mid=255,255,255,255 br=255,255,255,255

Render thread painted frame ...
sample tl=255,255,255,255 mid=255,255,255,255 br=255,255,255,255
```

随后真实远端 paint 到达：

```text
FreeRDP GDI primary frame is now renderable:
sample tl=0,0,0,255 mid=0,0,0,255 br=0,0,0,255

Render thread painted frame ...
sample tl=0,0,0,255 mid=0,0,0,255 br=0,0,0,255
```

这条链路证明：白色在 GLES 上传前就已经存在于 FreeRDP GDI primary buffer。问题不在 ArkTS 背景，也不在 `XComponent` 外层。

### 触发源

同时看到早期事件会触发当前帧 repaint：

```text
Surface repaint queued/skipped after rdpgfx connected ...
```

当时 GDI context 已存在、desktop size 已有、primary buffer 已分配，但还没有任何真实 remote paint。旧逻辑把“buffer 存在”误当成“buffer 可渲染”。

## 根因

`RequestCurrentFrameRender()` 的条件太宽。

当时它只关心：

- active context 存在；
- GDI 存在；
- primary buffer 存在；
- width/height/stride 有效；
- surface queue 可用。

但这些条件只能说明内存和尺寸准备好了，不能说明这个 buffer 已经包含远端有效画面。

真正缺失的条件是：

是否已经收到过真实 remote update。

## 修复

增加 native readiness gate：

- `PostConnect` 清空 `g_rdpPrimaryFrameReady`；
- disconnect / clear desktop size 时清空；
- desktop size 真实变化时清空；
- 只有 `HarmonyEndPaint` 收到真实 invalidated remote paint 后才置为 true；
- `RequestCurrentFrameRender()` 在 readiness 为 false 时直接跳过。

涉及位置：

- `harmony/app/entry/src/main/cpp/freerdp_gdi_bridge.cpp`
  - 维护 `g_rdpPrimaryFrameReady`；
  - 暴露 `RdpPrimaryFrameReady()`；
  - 在真实 `HarmonyEndPaint` 后标记 primary renderable。

- `harmony/app/entry/src/main/cpp/rdp_session_channels.cpp`
  - 如果 GDI primary 没有 remote update，拒绝 current-GDI repaint。

修复后的生产行为：

- `rdpgfx connected` 触发 current-GDI repaint 时，如果还没有真实 remote update，直接静默跳过；
- 不再为这个预期内的首帧等待打印 `Surface repaint skipped` 或像素采样日志；
- 第一帧真实远端更新仍然由 `HarmonyEndPaint` 正常入队渲染。

## 为什么最终方案不是规避

最终方案没有设置颜色，也没有隐藏 UI，而是修正“什么数据可以被渲染”的边界。

正确的数据契约应该是：

- GDI context 存在，不代表可以渲染；
- desktop size 存在，不代表可以渲染；
- primary buffer 分配了，不代表可以渲染；
- rdpgfx channel connected，不代表可以渲染；
- 只有收到真实 remote paint 后，GDI primary 才是可渲染的远端帧。

这个契约和 FreeRDP 的 GDI paint 生命周期一致。

## 经验沉淀

### 单帧异常色先找像素来源

以后遇到白帧、粉帧、黑帧、绿帧这类问题，第一步不是改颜色，而是沿链路采样：

1. 录屏或截图确认异常帧；
2. GLES upload 前采样；
3. render queue 入队前采样；
4. source buffer 采样；
5. 记录触发这次 render 的事件。

如果 source buffer 已经异常，就不要先改 ArkUI、`XComponent`、clear color、wrapper。

如果 source buffer 正常，但屏幕异常，再查 NativeWindow、EGL swap、GL texture upload、ArkUI composition。

### 不要让“看起来合理”的解释替代证据

这个问题里，UI 背景、surface 初始 buffer、Stack wrapper 都看起来合理，但它们都没有直接解释“GLES 上传前为什么已经是白色”。

以后判断标准要更硬一点：

- 没有 source sample，不下结论；
- 没有 render reason，不改生命周期；
- 没有对比前后帧，不做隐藏/延迟；
- 临时诊断可以加，但修复必须落在最早出错的契约点。

## 下次需要提供的信息

类似问题再次出现时，最好同时提供：

- 录屏或连续截图，包含异常帧前后一帧；
- 发生阶段：连接中、刚 `Connected`、resize 后、全屏视频后、断开重连后；
- 图形模式：`gdi`、`rdpgfx`、`rdpgfx-h264`；
- RDP 状态时间线：`Resolving`、`TCP connected`、`Negotiating`、`Authenticating`、`Bridge ready`、`Connected`；
- `XComponent` 生命周期：created、changed、destroyed、layout changed；
- rdpgfx caps confirm；
- 最近的 rdpgfx surface command；
- GDI / render queue / GLES upload 的像素采样；
- 触发 repaint 的原因：surface created、surface changed、rdpgfx connected、desktop resize、真实 `EndPaint`。

临时定位时建议日志关键字：

```text
Native log:
FreeRDP desktop resize GDI primary sample
FreeRDP GDI primary frame is now renderable
FreeRDP GDI frame queued
Render thread painted frame
Surface repaint skipped
Surface repaint queued
rdpgfx caps confirm
rdpgfx surface command
```

## 后续处理建议

- 白帧修复保留 readiness gate。
- 调试 overlay 和高频像素采样在问题稳定后要收口，避免长期污染 HAP demo。
- 粉色帧不要套用白帧结论；它已经表现为 GDI primary 里出现粉色内容，应该按 rdpgfx codec/fallback 独立定位。
