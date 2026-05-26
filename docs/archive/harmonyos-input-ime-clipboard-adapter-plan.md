# HarmonyOS FreeRDP 输入、IME、剪贴板适配正式方案

日期：2026-05-17

本文档用于记录 HarmonyOS FreeRDP 客户端的输入、IME 和剪贴板正式适配方案，并给出从当前实现迁移到目标结构的可执行步骤。核心目标是对齐 FreeRDP Android、Wayland、X11 等平台客户端的设计，把平台相关逻辑收敛到“鸿蒙 FreeRDP client 适配层”，避免 ArkTS 继续直接处理 RDP scancode、组合键和剪贴板协议语义。

## 1. 当前问题

当前键盘链路被拆在三处：

- ArkTS 中维护 `OH_KEYCODE -> RDP_SCANCODE` 映射。
- TextInput 回调里有删除、字符回显、时间窗口 guard 等兜底逻辑。
- Native 输入队列最终再发送 FreeRDP keyboard/unicode event。

这导致几个反复出现的问题：

- Ctrl+A/C/V/X/Z 等组合键语义不稳定。
- 数字、Delete、Backspace 长按依赖 ArkUI 是否重复下发 `Down`，真实设备上不可靠。
- TextInput 的删除逻辑会和硬件 Delete/Backspace 冲突。
- 输入日志分散，问题定位不到一条稳定链路。

新的目标链路：

```text
ArkTS KeyEvent
  -> N-API 平台按键事件
  -> HarmonyOS native keyboard adapter
  -> Windows VK
  -> WinPR scancode / FreeRDP remap
  -> FreeRDP input PDU
```

IME 和剪贴板单独成链：

```text
ArkTS IME 已提交文本
  -> native unicode text adapter
  -> FreeRDP unicode keyboard event

HarmonyOS OH_Pasteboard
  <-> native cliprdr bridge
  <-> Windows clipboard
```

## 2. 其他 FreeRDP 平台的参考结论

FreeRDP 其他平台客户端没有让 UI 层直接发送 RDP scancode，而是在平台 client 适配层完成平台事件到 FreeRDP 输入事件的转换。

### Android

参考文件：

- `harmony/third_party/FreeRDP/client/Android/Studio/freeRDPCore/src/main/java/com/freerdp/freerdpcore/utils/KeyboardMapper.java`
- `harmony/third_party/FreeRDP/client/Android/Studio/freeRDPCore/src/main/cpp/android_freerdp.c`
- `harmony/third_party/FreeRDP/client/Android/Studio/freeRDPCore/src/main/cpp/android_cliprdr.c`

做法：

- Java 层接 Android `KeyEvent`。
- `KeyboardMapper` 转 Windows virtual key。
- Native 使用 `GetVirtualScanCodeFromVirtualKeyCode()` 转 scancode。
- Native 把事件排队到 FreeRDP worker 线程。
- 剪贴板是 Android 系统 clipboard + native `cliprdr` 回调。

### Wayland

参考文件：

- `harmony/third_party/FreeRDP/client/Wayland/wlf_input.c`
- `harmony/third_party/FreeRDP/client/Wayland/wlf_cliprdr.c`

做法：

- Wayland/UWAC 给 raw key。
- client 转 WinPR virtual key。
- client 转 scancode 并应用 FreeRDP remap。
- 发送 `freerdp_input_send_keyboard_event_ex()`。
- 剪贴板在 Wayland client 适配层实现，不改 FreeRDP core。

### X11

参考文件：

- `harmony/third_party/FreeRDP/client/X11/xf_event.c`
- `harmony/third_party/FreeRDP/client/X11/xf_keyboard.c`
- `harmony/third_party/FreeRDP/client/X11/xf_cliprdr.c`

做法：

- X11 client 自己维护按键状态。
- auto-repeat 由 client 根据平台 KeyPress/KeyRelease 行为识别。
- 如果 XIM/lookup 得到字符，则走 Unicode。
- 否则走 scancode keyboard event。
- 剪贴板也在 X11 client 适配层实现。

结论：HarmonyOS 也应该有自己的 client 平台适配层，而不是继续在 ArkTS 页面里直接写 RDP 输入语义。

## 3. 目标职责边界

### ArkTS / ArkUI

负责：

- 页面 UI、会话状态展示、焦点请求。
- XComponent 容器和触摸手势识别。
- 获取 ArkTS `KeyEvent`，只转发平台字段到 native。
- 通过 TextInput 获取 IME 已提交文本，转发给 native。
- 剪贴板权限申请和用户提示。

不负责：

- 不维护 RDP scancode 表。
- 不解释 Ctrl+C/V/A/X/Z 为本地剪贴板动作。
- 不用 TextInput delete 兜底硬件 Delete/Backspace。
- 不实现 FreeRDP 协议细节。

### N-API Bridge

负责：

- 传输结构化平台事件。
- API 命名保持平台语义，不暴露 RDP scancode 给 ArkTS。

目标接口：

```ts
native.sendPlatformKey({
  keyCode: number,
  down: boolean,
  repeat: boolean,
  ctrl: boolean,
  shift: boolean,
  alt: boolean,
  meta: boolean
})

native.sendUnicodeText(text: string)
native.releaseAllKeys()
```

旧的 `sendKey(scancode, down, repeat)` 可以暂时保留用于诊断对比，但迁移完成后 session 页面不再调用。

### Native Harmony Adapter

建议新增：

```text
harmony/app/entry/src/main/cpp/input/ohos_keyboard_adapter.h
harmony/app/entry/src/main/cpp/input/ohos_keyboard_adapter.cpp
harmony/app/entry/src/main/cpp/input/ohos_ime_adapter.h
harmony/app/entry/src/main/cpp/input/ohos_ime_adapter.cpp
```

键盘 adapter 负责：

- `OH_KEYCODE -> Windows VK`。
- 使用 WinPR 转 scancode。
- 应用 FreeRDP keyboard remap。
- 调用 `freerdp_input_send_keyboard_event_ex()`。
- 维护 pressed-key table。
- 维护 modifier 状态。
- 维护 repeat 状态。
- focus lost、disconnect、页面销毁、后台时释放全部按键。
- 输出统一日志。

日志格式建议：

```text
ohos.key keyCode=... vk=0x.. sc=0x.. rdp=0x.. down=true repeat=false ctrl=false shift=false alt=false meta=false sent=true
```

IME adapter 负责：

- 接收 ArkTS 传入的已提交 UTF-16 文本。
- BMP 字符走 `freerdp_input_send_unicode_keyboard_event()`。
- 非 BMP/surrogate 暂时记录日志并跳过。
- 保持 IME 文本输入和硬件键盘输入完全分离。

### Native Clipboard Adapter

当前方向保持不变，继续放在：

```text
harmony/app/entry/src/main/cpp/channels/clipboard_bridge.cpp
harmony/app/entry/src/main/cpp/channels/clipboard_pasteboard.cpp
harmony/app/entry/src/main/cpp/channels/clipboard_client_messages.cpp
harmony/app/entry/src/main/cpp/channels/clipboard_format.cpp
```

负责：

- 使用 `OH_Pasteboard` 读写 HarmonyOS 本地剪贴板文本。
- 实现 FreeRDP `cliprdr` 回调和 format negotiation。
- UTF-8 与 CF_UNICODETEXT 互转。
- 远端写入本地 pasteboard 后抑制本地 change echo。
- 第一版只做纯文本。

ArkTS 不实现 RDP 剪贴板语义。Ctrl+C/V 是键盘事件；剪贴板同步是 `cliprdr` 通道事件。

后续为了更接近 FreeRDP Android、Wayland、X11 的结构，可以把这一组整理成 `client/OHOS` clipboard backend。这个动作不应该和当前键盘迁移混在一起，建议在文本剪贴板稳定后单独做。

短期保留当前位置：

```text
harmony/app/entry/src/main/cpp/channels/clipboard_*.cpp
```

中期整理为 app 内的 OHOS client backend 目录，先不移动到 FreeRDP 子模块：

```text
harmony/app/entry/src/main/cpp/client/ohos/ohos_cliprdr.cpp
harmony/app/entry/src/main/cpp/client/ohos/ohos_cliprdr.h
harmony/app/entry/src/main/cpp/client/ohos/ohos_pasteboard.cpp
harmony/app/entry/src/main/cpp/client/ohos/ohos_pasteboard.h
harmony/app/entry/src/main/cpp/client/ohos/ohos_clipboard_format.cpp
harmony/app/entry/src/main/cpp/client/ohos/ohos_clipboard_format.h
```

长期如果 FreeRDP 子模块需要形成完整 OHOS client，可再迁移到：

```text
harmony/third_party/FreeRDP/client/OHOS/ohos_cliprdr.c
harmony/third_party/FreeRDP/client/OHOS/ohos_cliprdr.h
```

迁移原则：

- ETS 仍只负责权限和用户提示。
- `OH_Pasteboard` backend 可以进入 OHOS client backend。
- `CliprdrClientContext` 回调注册、format list、data request/response 都放在 OHOS client backend。
- N-API session runner 只负责初始化/销毁 OHOS client backend，不直接写剪贴板协议细节。
- 第一轮只整理文件边界和接口，不扩大功能范围到 HTML、图片、文件剪贴板。

### FreeRDP Core / WinPR

本阶段不改 FreeRDP core，除非发现真实平台兼容 bug。

允许使用：

- `GetVirtualScanCodeFromVirtualKeyCode()`
- `GetVirtualKeyCodeFromKeycode()`，如后续需要
- `freerdp_keyboard_remap_key()`
- `freerdp_input_send_keyboard_event_ex()`
- `freerdp_input_send_unicode_keyboard_event()`
- `CliprdrClientContext` callbacks

### 其余适配点归属结论

| 适配点 | 当前状态 | 正式归属 | 后续是否整理成 `client/OHOS` |
| --- | --- | --- | --- |
| Keyboard | `Index.ets` 仍有 RDP scancode map、组合键、repeat、delete guard | native `input/ohos_keyboard_adapter.*` | 是，稳定后可并入 app 内 `client/ohos/ohos_keyboard.*` |
| IME | `Index.ets` 隐藏 TextInput 处理文本、删除和回显抑制 | ETS 只接 IME 提交文本，native `ohos_ime_adapter.*` 发 Unicode | 部分是。TextInput 入口留 ETS，Unicode/RDP 发送进入 OHOS client adapter |
| Clipboard | ETS 权限，native `OH_Pasteboard`，native `cliprdr` bridge | 当前拆分方向正确 | 是，文本稳定后整理为 `client/ohos/ohos_clipboard_backend.*` |
| Pointer/Touch | ArkTS 识别 tap、drag、wheel、right click 后直接 `sendPointer` | ArkTS 保留手势识别，native 可加 `ohos_pointer_adapter.*` 做坐标/按钮规范化 | 可选。鼠标问题优先级低于键盘和 IME |
| Focus/Lifecycle | `aboutToDisappear()` 释放 modifier，native session 生命周期分散在 runner/core/input | native session owner 统一 release keys、stop repeat、clear queue、detach surface | 是，放在 app 内 `client/ohos/ohos_session_adapters.*` 或 session runner |
| Display resize | XComponent layout 触发 native surface resize 和 `disp` channel | 保持 native display-control channel bridge | 不放输入方案主线，后续可单独整理为 `ohos_display_adapter.*` |
| Audio | 已走 FreeRDP `rdpsnd sys:ohos` 方向 | FreeRDP rdpsnd OHOS backend + Harmony audio kit | 不属于本文档，单独音频方案跟进 |
| Graphics/H.264 | rdpgfx/AVCodec surface 路线已有代码 | graphics/AVCodec OHOS backend | 不属于本文档，单独图形性能方案跟进 |

本方案当前只执行 Keyboard、IME、Clipboard、Pointer/Lifecycle 中和输入稳定性直接相关的部分。Audio、Graphics/H.264、Display resize 保持现状，不混入本轮输入迁移提交。

## 4. 正式迁移方案

### 4.1 迁移目标

迁移完成后的目标不是简单“移动文件”，而是把当前 ArkTS、N-API、native bridge 中混在一起的输入语义拆成稳定的平台 client adapter：

```text
harmony/app/entry/src/main/ets/
  pages/Index.ets
    只保留 UI、焦点、IME 文本入口、权限入口

harmony/app/entry/src/main/cpp/
  input/
    ohos_keyboard_adapter.*
    ohos_ime_adapter.*
    ohos_pointer_adapter.*        可选，第二轮整理
  client/ohos/
    ohos_clipboard_backend.*
    ohos_cliprdr.*
    ohos_pasteboard.*
    ohos_clipboard_format.*
    ohos_session_lifecycle.*      可选，统一 release/stop/clear
  rdp_session_input.*
    只保留 worker 线程安全输入队列和 FreeRDP dispatch
  freerdp_session_runner.*
    只负责 session 生命周期和 adapter 初始化/销毁
```

最终边界：

- ArkTS 不再持有 RDP scancode。
- ArkTS 不再解释远端 Ctrl+C/V/A/X/Z。
- Native keyboard adapter 统一处理 VK、scancode、repeat、modifier。
- Native IME adapter 统一处理 Unicode 文本。
- Native clipboard backend 统一处理 `OH_Pasteboard` 和 `cliprdr`。
- FreeRDP core 只作为协议库使用，不因为本次迁移改 core。

### 4.2 当前文件到目标结构的迁移映射

| 当前位置 | 当前职责 | 目标位置 | 迁移方式 |
| --- | --- | --- | --- |
| `harmony/app/entry/src/main/ets/pages/Index.ets` | UI、焦点、触摸、键盘映射、IME TextInput、部分兜底逻辑 | `Index.ets` + native adapter | 保留 UI/焦点/IME入口；删除 RDP scancode map 和远端快捷键语义 |
| `Index.ets::mapArkKeyCodeToRdpScancode()` | ArkTS 手写 RDP scancode | `input/ohos_keyboard_adapter.cpp` | 改为 `OH_KEYCODE -> Windows VK -> WinPR scancode` |
| `Index.ets` TextInput delete guard | 处理软键盘删除，同时影响硬件 Delete/Backspace | `input/ohos_ime_adapter.cpp` + `ohos_keyboard_adapter.cpp` | 软键盘删除走 IME adapter；硬件删除走 keyboard adapter |
| `Index.ets::handleSurfaceMouse/Touch/Axis` | 手势识别和 pointer flags 组装 | `Index.ets` + 可选 `input/ohos_pointer_adapter.cpp` | ArkTS 保留手势识别；native 负责坐标边界、按钮状态、队列背压 |
| `Index.ets::releaseActiveModifiers()` | ArkTS 用 scancode 释放 modifier | `input/ohos_keyboard_adapter.cpp` | 改为 native `releaseAllKeys()`，不再从 ArkTS 发 RDP scancode |
| `harmony/app/entry/src/main/cpp/rdp_session_input.*` | 输入队列和 FreeRDP dispatch | 原地保留 | 保留线程安全队列，增加平台 key/unicode event 类型或统一 event payload |
| `harmony/app/entry/src/main/cpp/freerdp_runtime.*` | FreeRDP symbol/runtime API | 原地保留 | 补齐 WinPR keyboard helper 或 FreeRDP remap 所需入口 |
| `harmony/app/entry/src/main/cpp/channels/clipboard_*` | Pasteboard、格式转换、cliprdr 回调混在 channels 目录 | `client/ohos/ohos_clipboard_*` | 文本剪贴板稳定后做文件边界整理，不和键盘迁移混做 |
| `harmony/app/entry/src/main/cpp/freerdp_session_runner.cpp` | session 生命周期、channel 初始化、clipboard bridge 持有 | 原地保留 | 后续只持有 `OhosClipboardBackend`、`OhosKeyboardAdapter` 生命周期 |
| `harmony/third_party/FreeRDP/client/OHOS/` | 当前不存在 | 可选长期目标 | 只有当需要把 OHOS client backend 归入 FreeRDP 子模块时再迁入 |

### 4.3 迁移原则

- 先新增新链路，再切调用方，最后删除旧链路。
- 每个阶段都必须能 build，并能通过最小真机验证。
- 不在同一阶段同时重构键盘、IME、剪贴板，避免问题归因困难。
- 不用兜底逻辑掩盖输入问题；优先让事件按正确语义进入 native adapter。
- 剪贴板当前方向正确，先稳定文本复制粘贴，再整理为 `client/OHOS` backend。
- 旧 `sendKey(scancode)` 只短期保留作日志对比，迁移完成后从 session 主路径移除。
- 每阶段单独提交，报告完成度和风险。

### 4.4 迁移阶段总览

| 阶段 | 目标 | 行为变化 | 是否需要真机验证 |
| --- | --- | --- | --- |
| Migrate-1 | 新增 native keyboard adapter 骨架 | 无 | 只需确认日志 |
| Migrate-2 | OH keycode 映射 Windows VK | 仅新增日志和新能力 | 需要校验键值 |
| Migrate-3 | WinPR scancode + FreeRDP dispatch | 开始具备新输入链路 | 需要远端文本框验证 |
| Migrate-4 | ArkTS session 键盘切到 `sendPlatformKey` | 有 | 必须验证组合键和普通输入 |
| Migrate-5 | native repeat 和 pressed-key table | 有 | 必须验证长按数字/Delete/Backspace |
| Migrate-6 | IME 边界清理 | 有 | 必须验证中文/英文/软键盘删除 |
| Migrate-7 | 剪贴板日志和文本同步补强 | 有 | 必须验证双向文本剪贴板 |
| Migrate-8 | 剪贴板整理为 OHOS client backend | 理论无行为变化 | 必须做迁移前后对比 |

### 4.5 迁移兼容和回退策略

键盘迁移期间保留两条 native 能力：

```text
sendKey(scancode, down, repeat)        旧链路，只用于对比和临时回退
sendPlatformKey(event)                 新链路，正式主路径
```

切换策略：

- Phase 1-3：只新增新链路，ArkTS 主路径仍可保持旧逻辑。
- Phase 4：session 页面切换到新链路，旧 `sendKey` 不再被正常 session 输入调用。
- Phase 5-6：删除或隔离 ArkTS 中影响硬件输入的兜底逻辑。
- Phase 7-8：剪贴板不依赖旧键盘链路，按 clipboard backend 自己的阶段推进。

回退策略：

- 如果 Phase 4 后远端完全无法输入，可以临时把 session 键盘入口切回旧 `sendKey`，但必须保留新链路日志用于定位。
- 如果 repeat 出现 stuck key，优先禁用 native repeat timer，保留普通 key down/up。
- 如果 IME 提交异常，保留硬件键盘新链路，只回退 TextInput 到原 Unicode 发送方式。
- 如果剪贴板 backend 迁移后异常，回退到当前 `channels/clipboard_*` 文件布局，不影响键盘链路。

禁止回退方式：

- 不通过 ArkTS 重新拦截 Ctrl+C/V/A/X/Z 做本地语义兜底。
- 不把硬件 Delete/Backspace 重新塞回 TextInput delete guard。
- 不在 FreeRDP core 里写 HarmonyOS UI/输入逻辑。

### 4.6 迁移完成判定

键盘迁移完成：

- `Index.ets` session 主路径不再调用 `mapArkKeyCodeToRdpScancode()`。
- 远端 Ctrl+A/C/V/X/Z 在 Notepad 或浏览器输入框中可用。
- 数字、Delete、Backspace 长按可用。
- focus lost、disconnect、页面销毁时 native 能 release all keys。

IME 迁移完成：

- 中文 IME 提交文本能进入远端。
- 英文软键盘输入可用。
- 软键盘 Backspace 可用。
- 硬件 Delete/Backspace 不依赖 TextInput delete。

剪贴板迁移完成：

- Windows 复制文本到 HarmonyOS pasteboard 正常。
- HarmonyOS 复制文本到 Windows 粘贴正常。
- `freerdp_session_runner.cpp` 不再直接依赖具体 clipboard message/format 细节。
- clipboard 日志前缀和生命周期清晰。

Pointer/生命周期迁移完成：

- 单指点击、拖动、长按右键、双指滚轮行为和迁移前一致。
- 鼠标左/右/中键 down/up 成对发送。
- pointer move 高频输入不会压垮 native queue。
- 页面销毁、断开、后台时能停止 repeat、释放全部按键、清空 pending pointer 状态。
- surface 销毁后不再继续写入窗口或发送依赖旧 surface 的更新。

## 5. 可执行迁移步骤

### Phase 1：新增 native keyboard adapter 骨架

修改范围：

- 新增 `input/ohos_keyboard_adapter.h`
- 新增 `input/ohos_keyboard_adapter.cpp`
- 修改 `harmony/app/entry/src/main/cpp/CMakeLists.txt`
- N-API 增加 log-only `sendPlatformKey`

伪码：

```cpp
struct OhosKeyEvent {
    int32_t keyCode;
    bool down;
    bool repeat;
    bool ctrl;
    bool shift;
    bool alt;
    bool meta;
};

class OhosKeyboardAdapter {
public:
    bool SendPlatformKey(const OhosKeyEvent& event, rdpContext* context);
    void ReleaseAll(rdpContext* context);
};
```

验收：

- App 能 build。
- 调用 `sendPlatformKey()` 能输出平台按键日志。
- 当前远端输入行为不变。

风险：

- 低。此阶段只做增量骨架。

### Phase 2：实现 OH keycode 到 Windows VK 映射

修改范围：

- 在 native adapter 中实现 `MapOhosKeyCodeToWindowsVk()`。
- 覆盖以下按键：
  - A-Z
  - 0-9
  - numpad 0-9
  - Backspace、Delete、Enter、Tab、Esc、Space
  - 方向键、Insert、Home、End、PageUp、PageDown
  - F1-F12
  - 左右 Ctrl、Shift、Alt、Win
  - HarmonyOS SDK 可确认的常见标点键

伪码：

```cpp
uint32_t MapOhosKeyCodeToWindowsVk(int32_t keyCode)
{
    if (keyCode >= OH_KEYCODE_A && keyCode <= OH_KEYCODE_Z)
        return 0x41 + keyCode - OH_KEYCODE_A;
    if (keyCode >= OH_KEYCODE_0 && keyCode <= OH_KEYCODE_9)
        return 0x30 + keyCode - OH_KEYCODE_0;

    switch (keyCode) {
    case OH_KEYCODE_DEL: return VK_BACK;
    case OH_KEYCODE_FORWARD_DEL: return VK_DELETE;
    case OH_KEYCODE_ENTER: return VK_RETURN;
    case OH_KEYCODE_TAB: return VK_TAB;
    case OH_KEYCODE_ESCAPE: return VK_ESCAPE;
    case OH_KEYCODE_DPAD_LEFT: return VK_LEFT;
    default: return 0;
    }
}
```

验收：

- 日志能显示目标按键对应的 VK。
- 未识别按键只记录日志，不发送无效输入。

风险：

- 中。HarmonyOS keycode 常量需要按当前 SDK 头文件和真机日志校对。

### Phase 3：接入 WinPR 和 FreeRDP 发送

修改范围：

- 确认 WinPR keyboard helper 的头文件和链接方式。
- 通过 `GetVirtualScanCodeFromVirtualKeyCode()` 转 scancode。
- 如果能拿到 remap table，则应用 `freerdp_keyboard_remap_key()`。
- 通过 `freerdp_input_send_keyboard_event_ex()` 发送。

伪码：

```cpp
bool OhosKeyboardAdapter::SendVirtualKey(uint32_t vk, bool down, bool repeat, rdpContext* context)
{
    DWORD scancode = GetVirtualScanCodeFromVirtualKeyCode(vk, WINPR_KBD_TYPE_IBM_ENHANCED);
    DWORD rdpScancode = remapTable != nullptr
        ? freerdp_keyboard_remap_key(remapTable, scancode)
        : scancode;

    if (rdpScancode == RDP_SCANCODE_UNKNOWN)
        return true;

    return freerdp_input_send_keyboard_event_ex(
        context->input,
        down ? TRUE : FALSE,
        repeat ? TRUE : FALSE,
        rdpScancode) == TRUE;
}
```

验收：

- session 键盘事件不再使用 ArkTS 手写 scancode。
- native 日志包含 `keyCode/vk/scancode/rdpScancode/down/repeat/sent`。
- 远端文本应用中 Ctrl+A/C/V/X/Z 生效。

风险：

- 中。当前 runtime 动态加载可能需要补 WinPR keyboard helper 符号，或者调整为直接链接。

### Phase 4：ArkTS session 键盘改为平台事件转发

修改范围：

- 废弃 `Index.ets` 中的 `mapArkKeyCodeToRdpScancode()`。
- session 页面键盘调用改为 `native.sendPlatformKey(...)`。
- connect 页面 TextInput 保持系统默认行为。
- TextInput 不再兜底发送硬件 Delete/Backspace。
- TextInput 只保留 IME 已提交文本桥。

伪码：

```ts
private handleSurfaceKeyEvent(event: KeyEvent): boolean {
  if (!this.shouldRouteRemoteKeyboard() || !this.isConnected()) {
    return false
  }

  return native.sendPlatformKey({
    keyCode: event.keyCode,
    down: event.type === KeyType.Down,
    repeat: false,
    ctrl: this.isCtrlModifierPressed(event),
    shift: this.isShiftModifierPressed(event),
    alt: this.isAltModifierPressed(event),
    meta: this.isWinModifierPressed(event)
  })
}
```

验收：

- connect 页面本地 Ctrl+C/V/A 仍由系统处理。
- session 页面按键全部进入 native keyboard adapter。
- ArkTS 正常 session 输入不再查 RDP scancode 表。

风险：

- 中。XComponent 和隐藏 TextInput 的焦点路由需要真机验证。

### Phase 5：native pressed-key table 和 repeat

修改范围：

- native 中维护 `pressedKeys`。
- key-down 时记录状态。
- repeatable key 首次按下后启动 repeat。
- key-up 停止 repeat。
- focus lost、disconnect、页面销毁、后台时停止 repeat 并释放所有按键。

伪码：

```cpp
void OhosKeyboardAdapter::OnKeyDown(uint32_t vk)
{
    const bool alreadyDown = pressedKeys_.contains(vk);
    pressedKeys_.insert(vk);
    SendVirtualKey(vk, true, alreadyDown, context);

    if (!alreadyDown && IsRepeatable(vk))
        StartRepeat(vk);
}

void OhosKeyboardAdapter::StartRepeat(uint32_t vk)
{
    repeatThread_.PostDelayed(initialDelayMs_, [this, vk]() {
        while (IsPressed(vk)) {
            SendVirtualKey(vk, true, true, context);
            Sleep(repeatIntervalMs_);
        }
    });
}

void OhosKeyboardAdapter::OnKeyUp(uint32_t vk)
{
    StopRepeat(vk);
    pressedKeys_.erase(vk);
    SendVirtualKey(vk, false, false, context);
}
```

验收：

- 长按数字能连续输入。
- 长按 Backspace 能连续删除。
- 长按 Delete 能连续向前删除。
- 松手后 repeat 立即停止。
- `releaseAllKeys()` 能清理所有 stuck modifier。

风险：

- 高。生命周期清理必须完整，否则远端可能认为按键一直处于按下状态。

### Phase 6：IME 边界清理

修改范围：

- 隐藏 TextInput 只作为 IME composition/commit 桥。
- `onChange` 提取已提交文本后走 `native.sendUnicodeText(text)`。
- `onWillDelete` 只处理软键盘删除，不处理硬件删除回显。
- 删除硬件路径上的时间窗口 delete suppression。

伪码：

```ts
private handleRemoteTextInputChange(value: string): void {
  const committed = this.extractCommittedText(value)
  if (committed.length > 0) {
    native.sendUnicodeText(committed)
  }
}

private handleInputBridgeWillDelete(value: DeleteValue): boolean {
  if (this.isHardwareKeyDispatchActive()) {
    return true
  }

  const keyCode = value.direction === TextDeleteDirection.FORWARD
    ? OH_KEYCODE_FORWARD_DEL
    : OH_KEYCODE_DEL

  native.sendPlatformKey({ keyCode, down: true, repeat: false, ctrl: false, shift: false, alt: false, meta: false })
  native.sendPlatformKey({ keyCode, down: false, repeat: false, ctrl: false, shift: false, alt: false, meta: false })
  return true
}
```

验收：

- 英文硬件输入正常。
- 中文 IME 提交文本能进入远端应用。
- 软键盘 Backspace 可用。
- 硬件 Delete/Backspace 不依赖 TextInput delete 回调。

风险：

- 中。ArkUI TextInput composition 行为需要真机验证。

### Phase 7：剪贴板日志和稳定性补强

修改范围：

- 保留 native `channels/clipboard_*` 归属。
- 补充关键日志：
  - local pasteboard changed
  - local format list sent
  - remote format list received
  - remote data request sent
  - remote data copied to pasteboard
- 验证远端写入本地 pasteboard 后的 echo guard。

伪码：

```cpp
void ClipboardPasteboard::HandlePasteboardChanged(Pasteboard_NotifyType type)
{
    if (type != NOTIFY_LOCAL_DATA_CHANGE)
        return;

    if (ConsumeIgnoreLocalChange())
        return;

    onLocalChange_();
}
```

验收：

- Windows 复制文本，HarmonyOS pasteboard 能收到。
- HarmonyOS 复制文本，Windows Ctrl+V 能粘贴。
- 远端 session 中 Ctrl+C/V 只是键盘事件，不被 ArkTS 当成本地剪贴板命令拦截。
- 无剪贴板 echo 死循环。

风险：

- 中。第一阶段只支持纯文本；HTML、图片、文件剪贴板后续另做。

### Phase 8：整理为 OHOS client clipboard backend

修改范围：

- 新增 app 内 `client/ohos/` 目录。
- 将现有 `channels/clipboard_*` 按职责迁移或拆分到 `ohos_cliprdr`、`ohos_pasteboard`、`ohos_clipboard_format`。
- `freerdp_session_runner.cpp` 只持有一个 `OhosClipboardBackend` 生命周期对象。
- `rdp_channel_config.cpp` 仍只负责请求 `cliprdr` channel，不写回调细节。
- 保持 ETS 权限逻辑不变。

目标接口：

```cpp
class OhosClipboardBackend {
public:
    bool Initialize(rdpContext* context, FreerdpRuntimeApi& api, const LogFn& log, std::string& error);
    void Uninitialize();
};
```

目标目录：

```text
harmony/app/entry/src/main/cpp/client/ohos/ohos_clipboard_backend.cpp
harmony/app/entry/src/main/cpp/client/ohos/ohos_clipboard_backend.h
harmony/app/entry/src/main/cpp/client/ohos/ohos_cliprdr.cpp
harmony/app/entry/src/main/cpp/client/ohos/ohos_pasteboard.cpp
harmony/app/entry/src/main/cpp/client/ohos/ohos_clipboard_format.cpp
```

验收：

- 迁移前后的 Windows -> HarmonyOS 文本复制行为一致。
- 迁移前后的 HarmonyOS -> Windows 文本粘贴行为一致。
- `freerdp_session_runner.cpp` 不再依赖具体 clipboard message/format 细节。
- 日志前缀统一为 `ohos.cliprdr` 或 `ohos.pasteboard`。
- 不改变 HAP 权限声明，不新增无关权限。

风险：

- 中。纯文件迁移容易引入 CMake 链接、include path、生命周期顺序问题。
- 中。如果后续直接迁入 FreeRDP 子模块，需要处理 OHOS Pasteboard 头文件和 FreeRDP CMake 对 HarmonyOS SDK 的发现方式。

### Phase 9：Pointer 和 lifecycle 输入收口

修改范围：

- 保留 ArkTS tap、drag、wheel、right click 手势识别。
- native 增加可选 `ohos_pointer_adapter.*`，集中处理坐标 clamp、按钮状态、wheel flags 和日志。
- ArkTS 的 `releaseActiveModifiers()` 改为调用 native `releaseAllKeys()`。
- 页面销毁、disconnect、后台、surface destroyed 时统一调用：
  - `releaseAllKeys()`
  - `stopAllRepeat()`
  - `clearPointerState()`
  - `RdpSessionInput::Clear()`

伪码：

```ts
aboutToDisappear(): void {
  native.releaseAllKeys()
  this.clearPointerState()
  this.clearRemoteInputBridge()
}
```

```cpp
class OhosInputLifecycle {
public:
    void OnFocusLost();
    void OnDisconnecting();
    void OnSurfaceDestroyed();

private:
    OhosKeyboardAdapter* keyboard_;
    RdpSessionInput* inputQueue_;
};

void OhosInputLifecycle::OnDisconnecting()
{
    keyboard_->ReleaseAll();
    keyboard_->StopAllRepeat();
    inputQueue_->Clear();
}
```

验收：

- 单指点击、拖动、右键、滚轮行为不退化。
- 长按键时返回页面，远端没有 stuck key。
- 鼠标拖动中断开连接，远端不会保持鼠标按下状态。
- native 日志能看到 lifecycle release。

风险：

- 中。Pointer 行为当前相对可用，迁移时要避免为了整理结构引入触控回归。
- 高。生命周期释放顺序如果不对，可能导致 native queue 清空过早或 key-up 没有发出。

## 6. 执行顺序

推荐顺序：

1. 新增 native keyboard adapter 骨架并 build。
2. 增加 VK 映射和 native 日志。
3. 接入 WinPR/FreeRDP 发送。
4. ArkTS session 键盘改为 `sendPlatformKey`。
5. 增加 native pressed-key table 和 repeat。
6. 清理 IME 边界，移除硬件删除兜底。
7. 复测并补强文本剪贴板日志。
8. 文本剪贴板稳定后，单独整理为 OHOS client clipboard backend。
9. 收口 Pointer 和 lifecycle，统一释放按键、repeat 和 pending pointer 状态。

每个阶段完成后单独提交，并报告完成度、验证结果、遗留风险。

## 7. 真机验收清单

远端验证应用优先使用 Notepad、浏览器输入框、Explorer 重命名。不要只用 `cmd.exe` 验证组合键，因为控制台快捷键行为和普通 Windows 文本控件不同。

键盘：

- A-Z 输入。
- 0-9 输入。
- 长按 0-9 repeat。
- Backspace 单击。
- Backspace 长按 repeat。
- Delete 单击。
- Delete 长按 repeat。
- Enter、Tab、Esc。
- 方向键。
- F1-F12。
- Ctrl+A、Ctrl+C、Ctrl+V、Ctrl+X、Ctrl+Z。
- Shift+字母和常见标点。
- Alt+Tab、Alt+F4 只在专项测试时验证。

IME：

- 英文软键盘输入。
- 中文 IME 组合和提交。
- 软键盘 Backspace。
- 远端区域失焦和重新聚焦。

剪贴板：

- 远端复制到本地 pasteboard。
- 本地 pasteboard 粘贴到远端。
- 多次复制/粘贴无 echo loop。

生命周期：

- modifier 按下时断开连接。
- 按键按下时返回页面。
- 按键按下时 App 进入后台。
- 断开后重新连接。

## 8. 已知遗留问题和影响

- 第一阶段不做完整 IME composition 范围控制。
  影响：提交文本应可用，但候选词编辑、光标范围等高级行为可能还需要设备专项适配。
- 剪贴板第一阶段只做纯文本。
  影响：富文本、图片、文件、目录剪贴板暂不支持。
- native repeat 参数需要真机调优。
  影响：初版长按速度可能和系统键盘手感不完全一致。
- HarmonyOS keycode 常量可能受 SDK/API 版本影响。
  影响：VK 映射需要结合头文件和真机日志校对。
- 硬件键盘、软键盘、外接键盘事件序列可能不同。
  影响：验收至少覆盖当前真机和当前输入法。

## 9. 阶段报告模板

每个阶段完成后按以下格式报告：

```text
阶段：
修改文件：
构建结果：
真机验证：
完成度：
遗留风险：
下一步：
提交：
```
