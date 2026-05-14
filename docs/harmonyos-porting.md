# HarmonyOS RDP App 整体计划

本文档记录从当前 Windows FreeRDP demo 迁移到 HarmonyOS App 的整体方案、目标架构、迭代路线和风险边界。

核心结论：鸿蒙 App 不能继续走 `mstsc` / `wfreerdp.exe` 进程模式。当前 Windows demo 的价值是验证连接参数、RDP 环境、账号格式和网络诊断；鸿蒙版需要把“连接层”替换为 App 内的 C++ native library。

## 目标架构

```text
ArkUI 页面
  ↓ N-API
C++ Native RDP 模块
  ↓
FreeRDP library
  ↓
Windows RDP 服务
```

```text
HarmonyOS App
├─ ArkTS / ArkUI
│  ├─ 连接表单
│  ├─ 会话页面
│  ├─ 远程桌面画面容器 XComponent
│  ├─ 键盘/鼠标/触控工具栏
│  └─ 网络诊断页
│
├─ N-API Bridge
│  ├─ connect(params)
│  ├─ disconnect()
│  ├─ resize(width, height)
│  ├─ sendPointer(...)
│  ├─ sendKey(...)
│  └─ onState / onLog / onError
│
└─ C++ Native
   ├─ FreeRDP core
   ├─ WinPR
   ├─ OpenSSL / zlib / cJSON
   ├─ RDP connect/auth loop
   ├─ frame update callback
   └─ render to NativeWindow
```

## 当前 Windows Demo 的定位

当前仓库里的 Windows demo 继续保留，定位为诊断工具和参数验证工具：

- 验证目标 Windows 是否开启 RDP。
- 验证 TCP `3389` 是否可达。
- 验证用户名格式、密码、域、证书策略。
- 验证网络环境、路由、防火墙、VPN 或 Guest Wi-Fi 限制。
- 作为鸿蒙版连接参数和诊断页的参考实现。

它不是鸿蒙最终实现方案。鸿蒙版本不应依赖外部桌面客户端进程，而应直接链接 FreeRDP native library。

## 开发和构建环境分工

当前方案调整为 Windows + WSL 双环境：

- Windows：保留现有 demo、连接参数验证、RDP 环境排查、DevEco 工程管理、HAP 签名和产物归档。
- WSL Ubuntu：负责 OpenSSL、zlib、cJSON、WinPR、FreeRDP 的鸿蒙 native 交叉编译。
- HarmonyOS 工程：消费 WSL 编译出的 `.so`、头文件和 CMake package，不在 Windows 上直接编 FreeRDP 三方库。

边界要求：

- Windows 上不再尝试直接交叉编译 FreeRDP 及其依赖。
- WSL 里必须使用 Linux 版 OpenHarmony / HarmonyOS NDK toolchain；不能直接拿 Windows 版 DevEco SDK 里的 Windows 可执行文件当作 WSL 编译器。
- WSL 通过 `/mnt/c/Users/mu/Desktop/code/demo` 访问当前仓库，编译产物落到 `harmony/out/ohos-arm64/` 或等价本地输出目录。
- HAP 签名仍在 Windows 侧使用 `tools/hapsigner/Sign-NormalApp.ps1`，签名产物不提交。

WSL 当前入口：

```powershell
wsl.exe -d Ubuntu-24.04 -- bash -lc "cd /mnt/c/Users/mu/Desktop/code/demo && ./harmony/scripts/wsl/build-freerdp-ohos.sh"
```

## UI 范围

ArkUI 负责业务界面和输入事件采集：

- 连接页：IP、端口、用户名、密码、分辨率、证书策略。
- 会话页：全屏远程桌面画面。
- 工具栏：Ctrl、Alt、Win、Esc、Tab、方向键、软键盘。
- 诊断页：TCP `3389`、RDP negotiation、路由和网络提示。

远程桌面画面使用 `XComponent` 承载：

- ArkUI 页面放置一个 `XComponent`。
- Native C++ 获取对应 `NativeWindow`。
- FreeRDP 收到远程桌面帧后转换为 `RGBA8888`。
- C++ 写入 `NativeWindow` buffer。
- flush 到屏幕。

## Native Bridge 范围

N-API Bridge 第一阶段只暴露最小闭环：

```ts
probe(): ProbeResult
connect(params: ConnectParams): void
disconnect(): void
resize(width: number, height: number): void
sendPointer(event: PointerEvent): void
sendKey(event: KeyEvent): void
onState(callback): void
onLog(callback): void
onError(callback): void
```

Native 侧职责：

- 维护 RDP 连接线程和状态机。
- 封装 FreeRDP 初始化、连接、认证、断开。
- 将 FreeRDP 日志和错误映射回 ArkTS。
- 接收 ArkUI 输入事件并转成 RDP input events。
- 接收帧更新并渲染到 `NativeWindow`。
- 处理 ArkUI 生命周期中的暂停、恢复、销毁。

## FreeRDP 编译策略

FreeRDP 交叉编译统一放在 WSL Ubuntu 内执行。Windows 只做脚本触发、DevEco 工程管理和签名，不作为 FreeRDP 编译 host。

第一阶段只启用最小能力：

- RDP core。
- TLS / NLA。
- WinPR。
- OpenSSL。
- zlib。
- cJSON。
- 目标 ABI 先做 `arm64`。

第一版暂时关闭：

- 音频。
- 文件重定向。
- 智能卡。
- 打印机。
- 多显示器。
- 视频编解码。
- RD Gateway。
- 复杂剪贴板。

交叉编译优先目标：

1. FreeRDP version OK。
2. OpenSSL OK。
3. WinPR OK。
4. TCP connect OK。
5. RDP negotiation OK。
6. NLA/auth OK。

## 输入事件映射

触控和键盘输入先做基础映射：

- 单指点击：鼠标左键点击。
- 单指拖动：鼠标左键拖动。
- 双指滑动：鼠标滚轮。
- 长按：鼠标右键。
- ArkUI 键盘输入：FreeRDP keyboard events。
- 工具栏组合键：显式发送 Ctrl、Alt、Win、Esc、Tab、方向键。

后续再补：

- 输入法组合文本。
- 长按菜单细节。
- 横竖屏切换后的坐标映射。
- 远端分辨率变化后的 pointer 坐标缩放。

## 迭代路线

### M0：仓库整理

- 保留当前 `freerdp-control-demo`。
- 新增 `harmony/` 或 `openharmony/` 工程目录。
- 当前 Windows demo 继续作为诊断工具。
- 文档明确：Windows demo 是环境验证，不是鸿蒙最终连接模式。

### M1：鸿蒙 UI 骨架

- 建 ArkTS 工程。
- 实现连接页、会话页、诊断页。
- 会话页先放置 `XComponent` 占位。
- native 连接状态先 mock。
- 不接 FreeRDP。

目标：先把 App 交互流跑通。

### M2：Native Bridge 打通

- 新建 N-API C++ 模块。
- ArkTS 能调用：
  - `native.probe()`
  - `native.connect(params)`
  - `native.disconnect()`
- C++ 先返回版本、平台信息和模拟日志。
- ArkUI 能接收 `onState`、`onLog`、`onError`。

目标：证明 ArkTS 到 C++ 的调用链和回调链都通了。

### M3：FreeRDP 交叉编译

- 在 WSL Ubuntu 内用 Linux 版鸿蒙 NDK / CMake 编译 FreeRDP 和依赖。
- 先实现 native probe：
  - FreeRDP version OK。
  - OpenSSL OK。
  - WinPR OK。
- 再做 TCP/RDP negotiation。

目标：证明 FreeRDP 可以作为鸿蒙 native library 被加载和调用。

### M4：只连接不渲染

- C++ 完成 RDP connect/auth。
- ArkUI 只展示连接状态：
  - Resolving
  - TCP connected
  - Negotiating
  - Authenticating
  - Connected
  - Failed
- 暂不显示桌面画面。

目标：能连上 Windows RDP 服务。

### M5：画面渲染

- FreeRDP update callback 获取 frame。
- 转换为 `RGBA8888`。
- 写入 `XComponent` 对应的 `NativeWindow`。
- 先不追求性能，只要求能显示远程桌面。

目标：App 内显示远程 Windows 桌面。

### M6：输入控制

- 鼠标点击。
- 鼠标拖动。
- 滚轮。
- 键盘输入。
- 工具栏组合键。
- 分辨率变化。

目标：完成最基础可操作远程桌面。

### M7：增强功能

- 剪贴板。
- 自动重连。
- 会话保存。
- 证书 TOFU。
- 全屏/横屏。
- dirty rect。
- 双缓冲。
- GPU texture 渲染优化。

## 可执行实施拆解

下面把 M0 到 M7 拆成可直接执行的工作项。每个阶段都必须同时交付代码、验收记录、遗留问题和影响说明。

### M0 执行拆解：仓库整理

目标：明确 Windows demo 与 HarmonyOS App 的边界，避免后续把 `wfreerdp.exe` 进程模式误当成鸿蒙方案。

修改范围：

- `docs/harmonyos-porting.md`：维护鸿蒙整体计划。
- `README.md`：补充 `harmony/` 或 `openharmony/` 目录说明。
- `docs/repository-notes.md`：说明不提交构建产物、三方源码压缩包和本地密钥。
- `harmony/README.md` 或 `openharmony/README.md`：新增鸿蒙工程入口说明。
- 不修改 `app/` 的 Windows demo 行为。
- 不修改 `native/freerdp-bridge/` 的现有桌面 bridge 行为。

执行步骤：

1. 创建 `harmony/` 目录，作为 DevEco / OpenHarmony 工程根目录。
2. 在 `harmony/README.md` 写明当前阶段、构建方式、依赖要求。
3. 更新根 `README.md`，说明 Windows demo 只负责诊断和参数验证。
4. 更新 `.gitignore`，排除 `harmony/**/build/`、`.hvigor/`、`.idea/`、三方依赖编译输出、签名私钥和本地配置。
5. 保持 `npm start` 仍可启动当前 Windows demo。

伪码：

```text
repo/
  app/                         # Windows demo, keep
  native/freerdp-bridge/        # desktop stand-in bridge, keep
  docs/
  harmony/
    README.md
    entry/
    AppScope/
```

验收标准：

- `harmony/` 目录存在，且文档说明这是鸿蒙 App 入口。
- 根 `README.md` 能区分 Windows demo 和 HarmonyOS final architecture。
- 当前 Windows demo 启动方式不变。
- `git status --short` 只出现预期文档和目录变更。

阶段遗留问题和影响：

- 遗留问题：尚未创建真实 ArkTS 工程。
- 影响：M1 开始前仍需要用 DevEco Studio 或 OpenHarmony 模板初始化工程。

### M1 执行拆解：鸿蒙 UI 骨架

目标：先把 App 交互流跑通，不接 FreeRDP，不接真实 native 连接。

修改范围：

- `harmony/AppScope/app.json5`
- `harmony/entry/src/main/module.json5`
- `harmony/entry/src/main/ets/pages/ConnectPage.ets`
- `harmony/entry/src/main/ets/pages/SessionPage.ets`
- `harmony/entry/src/main/ets/pages/DiagnosticsPage.ets`
- `harmony/entry/src/main/ets/components/SessionToolbar.ets`
- `harmony/entry/src/main/ets/model/RdpTypes.ets`
- `harmony/entry/src/main/ets/services/MockRdpClient.ets`
- `harmony/entry/src/main/resources/`

执行步骤：

1. 使用 DevEco Studio 创建基础 ArkTS 工程。
2. 建立 `ConnectPage`，包含 host、port、username、password、resolution、certPolicy。
3. 建立 `SessionPage`，包含状态栏、`XComponent` 占位容器、输入工具栏、断开按钮。
4. 建立 `DiagnosticsPage`，展示 TCP 端口检查、RDP negotiation 检查和建议文案。
5. 建立 `MockRdpClient`，模拟 `Resolving -> TCP connected -> Negotiating -> Authenticating -> Connected`。
6. 页面间传递 `ConnectParams`，但不保存密码到持久化文件。

ArkTS 伪码：

```ts
type CertPolicy = 'tofu' | 'ignore' | 'strict'

interface ConnectParams {
  host: string
  port: number
  username: string
  password: string
  domain?: string
  width: number
  height: number
  certPolicy: CertPolicy
}

class MockRdpClient {
  onState?: (state: string) => void
  onLog?: (line: string) => void
  onError?: (message: string) => void

  async connect(params: ConnectParams): Promise<void> {
    validateConnectParams(params)
    for (const state of [
      'Resolving',
      'TCP connected',
      'Negotiating',
      'Authenticating',
      'Connected'
    ]) {
      this.onState?.(state)
      this.onLog?.(`[mock] ${state}`)
      await sleep(300)
    }
  }

  disconnect(): void {
    this.onState?.('Disconnected')
  }
}

function validateConnectParams(params: ConnectParams): void {
  if (!params.host.trim()) throw new Error('Host is required')
  if (params.port <= 0 || params.port > 65535) throw new Error('Port is invalid')
  if (!params.username.trim()) throw new Error('Username is required')
  if (!params.password) throw new Error('Password is required')
}
```

会话页伪码：

```ts
@Entry
@Component
struct SessionPage {
  @State connectionState: string = 'Idle'
  @State logs: string[] = []

  build() {
    Column() {
      Row() {
        Text(this.connectionState)
        Button('Disconnect').onClick(() => rdpClient.disconnect())
      }

      XComponent({
        id: 'rdpSurface',
        type: XComponentType.SURFACE
      })
      .width('100%')
      .height('100%')

      SessionToolbar({
        onSpecialKey: (key) => rdpClient.sendKey({ key, down: true })
      })
    }
  }
}
```

验收标准：

- App 能打开连接页、会话页、诊断页。
- 连接页字段校验可用。
- 点击连接后，状态按 mock 流转并进入会话页。
- 会话页存在 `XComponent` 占位区域。
- 点击断开后状态变为 `Disconnected`。
- 未接入 FreeRDP、未发起真实网络连接。

阶段遗留问题和影响：

- 遗留问题：没有真实 N-API 调用、没有真实 RDP 连接。
- 影响：M1 只能验证 UI 流程，不能证明 native、FreeRDP、认证和渲染可行。

### M2 执行拆解：Native Bridge 打通

目标：证明 ArkTS 能调用 C++，C++ 能把状态、日志、错误回传给 ArkTS。

修改范围：

- `harmony/entry/src/main/cpp/CMakeLists.txt`
- `harmony/entry/src/main/cpp/native_rdp.cpp`
- `harmony/entry/src/main/cpp/rdp_session.h`
- `harmony/entry/src/main/cpp/rdp_session.cpp`
- `harmony/entry/src/main/cpp/rdp_types.h`
- `harmony/entry/src/main/ets/services/NativeRdpClient.ets`
- `harmony/entry/src/main/ets/model/RdpTypes.ets`
- `harmony/entry/src/main/cpp/types/` 或工程模板对应的 native module 类型声明目录。

执行步骤：

1. 新建 C++ N-API 模块。
2. 暴露 `probe()`，返回 native module 版本、平台、mock FreeRDP 状态。
3. 暴露 `connect(params)`，先不创建 FreeRDP，只启动 native worker 线程并回调 mock 状态。
4. 暴露 `disconnect()`，能取消 worker 线程。
5. 暴露 `onState`、`onLog`、`onError` 回调注册。
6. ArkTS 将 M1 的 `MockRdpClient` 替换为 `NativeRdpClient`。

ArkTS 调用伪码：

```ts
import nativeRdp from 'libnative_rdp.so'

export class NativeRdpClient {
  constructor() {
    nativeRdp.onState((state: string) => AppBus.emit('rdp.state', state))
    nativeRdp.onLog((line: string) => AppBus.emit('rdp.log', line))
    nativeRdp.onError((message: string) => AppBus.emit('rdp.error', message))
  }

  probe(): ProbeResult {
    return nativeRdp.probe()
  }

  connect(params: ConnectParams): void {
    nativeRdp.connect(JSON.stringify(params))
  }

  disconnect(): void {
    nativeRdp.disconnect()
  }
}
```

C++ N-API 伪码：

```cpp
struct NativeCallbacks {
  ThreadSafeCallback onState;
  ThreadSafeCallback onLog;
  ThreadSafeCallback onError;
};

class RdpSession {
public:
  void Connect(ConnectParams params) {
    StopExistingWorkerIfNeeded();
    running_ = true;
    worker_ = std::thread([this, params]() {
      EmitState("Resolving");
      EmitLog("native bridge received params");
      SleepMs(200);

      EmitState("TCP connected");
      SleepMs(200);

      EmitState("Negotiating");
      SleepMs(200);

      EmitState("Authenticating");
      SleepMs(200);

      if (!running_) return;
      EmitState("Connected");
    });
  }

  void Disconnect() {
    running_ = false;
    JoinWorker();
    EmitState("Disconnected");
  }
};

napi_value Probe(napi_env env, napi_callback_info info) {
  return JsonObject({
    {"ok", true},
    {"module", "native_rdp"},
    {"freerdpLinked", false},
    {"version", "0.1.0"}
  });
}
```

验收标准：

- ArkTS 调用 `probe()` 能拿到 native 返回值。
- ArkTS 调用 `connect(params)` 后能收到 native 发出的状态和日志。
- `disconnect()` 后 native worker 退出，重复连接不会创建失控线程。
- 页面关闭或返回时不会崩溃。
- 当前阶段仍不链接 FreeRDP。

阶段遗留问题和影响：

- 遗留问题：N-API 已通，但 FreeRDP 未接入，`connect()` 仍是 mock。
- 影响：M3 才能验证三方库链接；M4 才能验证真实账号认证。

### M3 执行拆解：FreeRDP 交叉编译

目标：在 WSL Ubuntu 内证明 FreeRDP、WinPR、OpenSSL、zlib、cJSON 能以鸿蒙 native library 形式被编译、链接、加载和调用。

修改范围：

- `harmony/third_party/README.md`：记录依赖来源、版本、构建方式。
- `harmony/scripts/wsl/bootstrap-build-env.sh`：检查 WSL 内构建依赖和 OHOS NDK。
- `harmony/scripts/wsl/build-openssl-ohos.sh`
- `harmony/scripts/wsl/build-zlib-ohos.sh`
- `harmony/scripts/wsl/build-cjson-ohos.sh`
- `harmony/scripts/wsl/build-freerdp-ohos.sh`
- `harmony/scripts/wsl/package-native-libs.sh`
- `harmony/scripts/windows/run-wsl-freerdp-build.ps1`：可选，只负责从 Windows 调 WSL。
- `harmony/entry/src/main/cpp/CMakeLists.txt`
- `harmony/entry/src/main/cpp/freerdp_probe.cpp`
- `harmony/entry/src/main/cpp/freerdp_probe.h`
- `.gitignore`：排除三方源码解压目录和编译产物。

执行步骤：

1. 在 WSL 内确认基础工具：`cmake`、`ninja`、`pkg-config`、`perl`、`python3`、`git`。
2. 在 WSL 内配置 Linux 版 OHOS NDK 路径，例如 `OHOS_NDK_HOME=$HOME/ohos-sdk/native`。
3. 固定依赖版本：FreeRDP、WinPR、OpenSSL、zlib、cJSON。
4. 编译 OpenSSL for OHOS `arm64-v8a`。
5. 编译 zlib 和 cJSON。
6. 编译 FreeRDP，关闭不需要的 client channels 和复杂模块。
7. 将头文件、`.so` 和 CMake config 归档到 `harmony/out/ohos-arm64/`。
8. 在 `native_rdp` 中链接 WSL 产出的 FreeRDP / WinPR。
9. 扩展 `probe()`，返回 FreeRDP、OpenSSL、WinPR 版本。
10. 在真机或模拟器加载 `.so`，确认无动态库缺失。

WSL 构建脚本伪码：

```bash
#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
OUT_DIR="$REPO_ROOT/harmony/out/ohos-arm64"
DEPS_DIR="$REPO_ROOT/harmony/third_party"

: "${OHOS_NDK_HOME:?Set OHOS_NDK_HOME to the Linux OHOS native SDK path}"
: "${OHOS_ARCH:=arm64-v8a}"

cmake --version
ninja --version

build_zlib() {
  cmake -S "$DEPS_DIR/zlib" -B "$OUT_DIR/build/zlib" -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE="$OHOS_NDK_HOME/build/cmake/ohos.toolchain.cmake" \
    -DOHOS_ARCH="$OHOS_ARCH" \
    -DCMAKE_INSTALL_PREFIX="$OUT_DIR/sysroot"
  cmake --build "$OUT_DIR/build/zlib"
  cmake --install "$OUT_DIR/build/zlib"
}

build_cjson() {
  cmake -S "$DEPS_DIR/cjson" -B "$OUT_DIR/build/cjson" -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE="$OHOS_NDK_HOME/build/cmake/ohos.toolchain.cmake" \
    -DOHOS_ARCH="$OHOS_ARCH" \
    -DCMAKE_INSTALL_PREFIX="$OUT_DIR/sysroot" \
    -DENABLE_CJSON_TEST=OFF
  cmake --build "$OUT_DIR/build/cjson"
  cmake --install "$OUT_DIR/build/cjson"
}

build_freerdp() {
  cmake -S "$DEPS_DIR/FreeRDP" -B "$OUT_DIR/build/freerdp" -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE="$OHOS_NDK_HOME/build/cmake/ohos.toolchain.cmake" \
    -DOHOS_ARCH="$OHOS_ARCH" \
    -DCMAKE_INSTALL_PREFIX="$OUT_DIR/sysroot" \
    -DWITH_SERVER=OFF \
    -DWITH_CLIENT=ON \
    -DWITH_CHANNELS=OFF \
    -DWITH_ALSA=OFF \
    -DWITH_PULSE=OFF \
    -DWITH_CUPS=OFF \
    -DWITH_FFMPEG=OFF
  cmake --build "$OUT_DIR/build/freerdp"
  cmake --install "$OUT_DIR/build/freerdp"
}

build_zlib
build_cjson
build_freerdp
```

Windows 触发 WSL 伪码：

```powershell
$Repo = "C:\Users\mu\Desktop\code\demo"
wsl.exe -d Ubuntu-24.04 -- bash -lc "cd /mnt/c/Users/mu/Desktop/code/demo && ./harmony/scripts/wsl/build-freerdp-ohos.sh"
```

CMake 伪码：

```cmake
add_library(native_rdp SHARED
  native_rdp.cpp
  rdp_session.cpp
  freerdp_probe.cpp
)

target_include_directories(native_rdp PRIVATE
  ${CMAKE_SOURCE_DIR}/../../../out/ohos-arm64/sysroot/include
)

target_link_directories(native_rdp PRIVATE
  ${CMAKE_SOURCE_DIR}/../../../out/ohos-arm64/sysroot/lib
)

target_link_libraries(native_rdp PRIVATE
  freerdp
  winpr
  ssl
  crypto
  z
  cjson
)

target_compile_definitions(native_rdp PRIVATE
  RDP_ENABLE_MINIMAL_CLIENT=1
)
```

C++ probe 伪码：

```cpp
ProbeResult ProbeFreerdp() {
  ProbeResult result;
  result.ok = true;
  result.freerdpVersion = freerdp_get_version_string();
  result.opensslVersion = OpenSslVersionString();
  result.winprAvailable = TryCallWinPrApi();
  result.features = {
    {"rdp", true},
    {"tls", true},
    {"nla", true},
    {"clipboard", false},
    {"audio", false},
    {"driveRedirect", false}
  };
  return result;
}
```

验收标准：

- `wsl.exe -d Ubuntu-24.04` 能进入当前仓库目录。
- WSL 内 `cmake`、`ninja` 可用。
- WSL 内 `OHOS_NDK_HOME` 指向 Linux 版 OHOS native SDK。
- `native_rdp.so` 能成功加载。
- `probe()` 返回 FreeRDP version、OpenSSL version、WinPR available。
- WSL 构建产物只在 `harmony/out/ohos-arm64/` 或本地输出目录生成，不提交到仓库。
- 关闭音频、文件重定向、智能卡、打印机、多显示器、RD Gateway。
- 依赖版本和构建命令写入文档。

阶段遗留问题和影响：

- 遗留问题：WSL 编译通过只证明库能加载，不证明能完成 RDP 登录。
- 影响：如果 WSL 内 Linux 版 OHOS NDK、OpenSSL 或 FreeRDP 编译失败，M4-M6 全部阻塞；需要优先降级为先做 TCP/RDP negotiation probe。

### M4 执行拆解：只连接不渲染

目标：C++ 完成真实 RDP connect/auth，ArkUI 只展示状态和日志，暂不显示桌面。

修改范围：

- `harmony/entry/src/main/cpp/rdp_session.cpp`
- `harmony/entry/src/main/cpp/rdp_session.h`
- `harmony/entry/src/main/cpp/rdp_settings_mapper.cpp`
- `harmony/entry/src/main/cpp/rdp_settings_mapper.h`
- `harmony/entry/src/main/cpp/rdp_event_loop.cpp`
- `harmony/entry/src/main/cpp/rdp_event_loop.h`
- `harmony/entry/src/main/ets/services/NativeRdpClient.ets`
- `harmony/entry/src/main/ets/pages/SessionPage.ets`
- `harmony/entry/src/main/ets/pages/DiagnosticsPage.ets`

执行步骤：

1. 将 `ConnectParams` 映射到 FreeRDP settings。
2. 实现连接状态机。
3. 建立 RDP worker 线程，避免阻塞 ArkUI。
4. 在 worker 线程内完成 TCP、RDP negotiation、TLS/NLA、认证。
5. 状态和错误通过线程安全回调返回 ArkTS。
6. 断开连接时安全释放 FreeRDP instance、context、event handles。
7. 失败时给出可诊断错误：网络不可达、认证失败、证书失败、NLA 失败、协议失败。

状态机伪码：

```text
Idle
  -> Resolving
  -> TcpConnecting
  -> TcpConnected
  -> Negotiating
  -> Authenticating
  -> Connected
  -> Disconnecting
  -> Disconnected

AnyState
  -> Failed(errorCode, message)
```

C++ 连接伪码：

```cpp
void RdpSession::WorkerMain(ConnectParams params) {
  EmitState("Resolving");

  freerdp* instance = freerdp_new();
  if (!instance) return Fail("FREERDP_INIT_FAILED");

  ConfigureContext(instance);

  rdpSettings* settings = instance->settings;
  ApplyBasicSettings(settings, params);
  ApplySecuritySettings(settings, params.certPolicy);
  DisableDeferredFeatures(settings);

  EmitState("TcpConnecting");
  if (!CheckTcpReachable(params.host, params.port, 5000)) {
    return Fail("TCP_CONNECT_FAILED");
  }

  EmitState("Negotiating");
  bool connected = freerdp_connect(instance);
  if (!connected) {
    return Fail(MapFreerdpError(instance));
  }

  EmitState("Connected");

  while (running_) {
    int status = freerdp_check_fds(instance);
    if (status != 0) {
      return Fail(MapFreerdpError(instance));
    }
    WaitForFreerdpEvents(instance, 16);
  }

  EmitState("Disconnecting");
  freerdp_disconnect(instance);
  freerdp_free(instance);
  EmitState("Disconnected");
}
```

ArkTS 状态处理伪码：

```ts
nativeClient.onState((state) => {
  this.state = state
  this.canDisconnect = ['TcpConnecting', 'Negotiating', 'Authenticating', 'Connected'].includes(state)
})

nativeClient.onError((error) => {
  this.state = 'Failed'
  this.errorMessage = toUserFacingMessage(error)
  this.showTroubleshootingHint(error.code)
})
```

验收标准：

- 正确账号能进入 `Connected` 状态。
- 错误密码能进入 `Failed`，错误信息能区分认证失败。
- 错误 IP 或端口能进入 `Failed`，错误信息能区分 TCP 失败。
- 证书策略能影响连接结果，至少支持 `ignore` 和后续可扩展 `tofu`。
- 点击断开能结束 RDP worker 线程，不崩溃、不泄露明显线程。
- 仍不渲染远程桌面。

阶段遗留问题和影响：

- 遗留问题：已连接但没有画面，用户不能操作远端桌面。
- 影响：M4 可作为认证和网络诊断版本；不能作为第一版产品交付。

### M5 执行拆解：画面渲染

目标：FreeRDP 收到远程画面后转为 `RGBA8888`，写入 `XComponent` 对应的 `NativeWindow`。

修改范围：

- `harmony/entry/src/main/ets/pages/SessionPage.ets`
- `harmony/entry/src/main/cpp/xcomponent_manager.cpp`
- `harmony/entry/src/main/cpp/xcomponent_manager.h`
- `harmony/entry/src/main/cpp/native_window_renderer.cpp`
- `harmony/entry/src/main/cpp/native_window_renderer.h`
- `harmony/entry/src/main/cpp/frame_converter.cpp`
- `harmony/entry/src/main/cpp/frame_converter.h`
- `harmony/entry/src/main/cpp/rdp_update_callbacks.cpp`
- `harmony/entry/src/main/cpp/rdp_update_callbacks.h`
- `harmony/entry/src/main/cpp/rdp_session.cpp`

执行步骤：

1. ArkUI 在会话页创建 `XComponent`，固定 id，例如 `rdpSurface`。
2. Native 注册 XComponent 生命周期回调，拿到 surface / NativeWindow。
3. FreeRDP 注册 update callback。
4. 每次远端帧更新时获取 dirty rect。
5. 将 FreeRDP frame buffer 转换为 `RGBA8888`。
6. 写入 NativeWindow buffer 并 flush。
7. 处理窗口 resize、横竖屏、App 后台/恢复。

ArkTS surface 伪码：

```ts
XComponent({
  id: 'rdpSurface',
  type: XComponentType.SURFACE
})
.width('100%')
.height('100%')
.onLoad(() => nativeClient.bindSurface('rdpSurface'))
.onDestroy(() => nativeClient.unbindSurface('rdpSurface'))
```

C++ XComponent 伪码：

```cpp
void OnSurfaceCreated(XComponentHandle component, NativeWindowHandle window) {
  SurfaceRegistry::SetWindow("rdpSurface", window);
  RdpSession::Current()->AttachRenderer(window);
}

void OnSurfaceChanged(int width, int height) {
  RdpSession::Current()->ResizeLocalSurface(width, height);
}

void OnSurfaceDestroyed() {
  RdpSession::Current()->DetachRenderer();
  SurfaceRegistry::ClearWindow("rdpSurface");
}
```

渲染伪码：

```cpp
void OnRdpFrameUpdated(const RdpFrame& frame, const DirtyRect& rect) {
  if (!renderer.HasWindow()) return;

  RgbaFrame rgba = ConvertToRgba8888(frame, rect);

  NativeWindowBuffer buffer = renderer.LockBuffer();
  CopyRect(
    source = rgba.pixels,
    target = buffer.pixels,
    sourceStride = rgba.stride,
    targetStride = buffer.stride,
    rect = rect
  );
  renderer.UnlockAndFlush(buffer);
}
```

验收标准：

- 连接成功后能在 App 内看到远程 Windows 桌面。
- 画面不是黑屏、白屏、花屏或明显颜色通道错乱。
- 横竖屏或窗口尺寸变化后不会崩溃。
- 断开后画面停止刷新并释放 NativeWindow 引用。
- 初版允许全帧拷贝，性能优化不作为 M5 验收条件。

阶段遗留问题和影响：

- 遗留问题：性能可能较差，dirty rect、双缓冲、GPU texture 尚未优化。
- 影响：M5 可证明产品闭环，但长时间使用和高分辨率场景可能卡顿、发热或耗电高。

### M6 执行拆解：输入控制

目标：将 ArkUI 触控、键盘和工具栏事件映射到 FreeRDP input events，让用户能操作远端桌面。

修改范围：

- `harmony/entry/src/main/ets/pages/SessionPage.ets`
- `harmony/entry/src/main/ets/components/SessionToolbar.ets`
- `harmony/entry/src/main/ets/services/InputMapper.ets`
- `harmony/entry/src/main/cpp/rdp_input.cpp`
- `harmony/entry/src/main/cpp/rdp_input.h`
- `harmony/entry/src/main/cpp/rdp_session.cpp`
- `harmony/entry/src/main/cpp/rdp_settings_mapper.cpp`

执行步骤：

1. 在 `XComponent` 外层采集触控事件。
2. 单指点击映射为左键 down/up。
3. 单指移动映射为 mouse move；拖动时保持左键 down。
4. 双指滑动映射为 wheel。
5. 长按映射为右键 click。
6. ArkUI 键盘事件映射为 RDP scancode 或 virtual key。
7. 工具栏发送 Ctrl、Alt、Win、Esc、Tab、方向键。
8. 根据远端桌面分辨率和本地显示区域做坐标缩放。

ArkTS 输入伪码：

```ts
function mapLocalToRemote(x: number, y: number): Point {
  return {
    x: Math.round(x * remoteWidth / surfaceWidth),
    y: Math.round(y * remoteHeight / surfaceHeight)
  }
}

function onTouch(event: TouchEvent): void {
  if (event.touchCount === 1 && event.type === 'Down') {
    const p = mapLocalToRemote(event.x, event.y)
    nativeClient.sendPointer({ type: 'button', button: 'left', down: true, x: p.x, y: p.y })
  }

  if (event.touchCount === 1 && event.type === 'Move') {
    const p = mapLocalToRemote(event.x, event.y)
    nativeClient.sendPointer({ type: 'move', x: p.x, y: p.y })
  }

  if (event.touchCount === 1 && event.type === 'Up') {
    const p = mapLocalToRemote(event.x, event.y)
    nativeClient.sendPointer({ type: 'button', button: 'left', down: false, x: p.x, y: p.y })
  }

  if (event.touchCount === 2 && event.type === 'Move') {
    nativeClient.sendPointer({ type: 'wheel', delta: event.deltaY })
  }
}
```

C++ 输入伪码：

```cpp
void RdpSession::SendPointer(const PointerEvent& event) {
  if (!IsConnected()) return;

  if (event.type == PointerType::Move) {
    freerdp_input_send_mouse_event(input_, PTR_FLAGS_MOVE, event.x, event.y);
  }

  if (event.type == PointerType::Button) {
    uint16_t flags = ButtonToRdpFlags(event.button);
    flags |= event.down ? PTR_FLAGS_DOWN : 0;
    freerdp_input_send_mouse_event(input_, flags, event.x, event.y);
  }

  if (event.type == PointerType::Wheel) {
    freerdp_input_send_mouse_event(input_, WheelToRdpFlags(event.delta), event.x, event.y);
  }
}

void RdpSession::SendKey(const KeyEvent& event) {
  if (!IsConnected()) return;
  uint16_t scancode = MapArkKeyToRdpScancode(event.key);
  freerdp_input_send_keyboard_event(input_, event.down ? KBD_FLAGS_DOWN : KBD_FLAGS_RELEASE, scancode);
}
```

验收标准：

- 点击远端开始菜单、按钮、文本框有效。
- 单指拖动窗口有效。
- 双指滚动远端页面有效。
- 软键盘输入普通英文字符有效。
- Esc、Tab、方向键有效。
- Ctrl/Alt/Win 组合键能按工具栏设计发送。
- 断开后输入事件被忽略，不崩溃。

阶段遗留问题和影响：

- 遗留问题：输入法组合文本、中文输入、复杂快捷键和安全注意序列仍需专项验证。
- 影响：第一版可以远程操作，但中文输入和部分系统快捷键体验可能不完整。

### M7 执行拆解：增强功能

目标：在最小可用版本稳定后，再增强会话体验、安全策略和性能。

修改范围：

- `harmony/entry/src/main/ets/services/SessionStore.ets`
- `harmony/entry/src/main/ets/services/CertificateStore.ets`
- `harmony/entry/src/main/ets/services/ReconnectManager.ets`
- `harmony/entry/src/main/cpp/clipboard_channel.cpp`
- `harmony/entry/src/main/cpp/reconnect_policy.cpp`
- `harmony/entry/src/main/cpp/cert_policy.cpp`
- `harmony/entry/src/main/cpp/native_window_renderer.cpp`
- `harmony/entry/src/main/cpp/gpu_renderer.cpp`，仅在决定走 GPU texture 后新增。

执行步骤：

1. 实现会话保存，但不保存明文密码。
2. 实现证书 TOFU：首次信任、后续校验指纹变更。
3. 实现自动重连：网络抖动后限次重连。
4. 实现剪贴板基础文本同步。
5. 实现横屏/全屏体验。
6. 优化渲染：dirty rect、双缓冲、减少全帧 copy。
7. 评估 GPU texture 路线，只在 CPU 渲染瓶颈明确后实施。

TOFU 伪码：

```ts
function verifyCertificate(host: string, fingerprint: string): CertDecision {
  const saved = certificateStore.get(host)
  if (!saved) {
    certificateStore.save(host, fingerprint)
    return 'trusted-first-use'
  }

  if (saved.fingerprint !== fingerprint) {
    return 'blocked-fingerprint-changed'
  }

  return 'trusted-known'
}
```

自动重连伪码：

```ts
class ReconnectManager {
  attempts = 0
  maxAttempts = 3

  async onDisconnected(reason: string) {
    if (!isRecoverable(reason)) return
    while (this.attempts < this.maxAttempts) {
      this.attempts += 1
      await sleep(backoffMs(this.attempts))
      try {
        await nativeClient.connect(lastSafeParamsWithoutPlainPassword)
        return
      } catch (error) {
        logReconnectFailure(error)
      }
    }
    showReconnectFailed()
  }
}
```

渲染优化伪码：

```cpp
void RenderDirtyRects(const Frame& frame, const std::vector<Rect>& dirtyRects) {
  NativeWindowBuffer buffer = renderer.LockBuffer();
  for (const Rect& rect : dirtyRects) {
    CopyConvertedRect(frame, buffer, rect);
  }
  renderer.UnlockAndFlush(buffer);
}
```

验收标准：

- 会话配置可保存和再次选择，密码不落明文。
- 首次证书信任和证书变更拦截可验证。
- 网络短暂断开后能按策略自动重连。
- 基础文本剪贴板可用。
- 横屏和全屏不会破坏坐标映射。
- 高分辨率下帧率、CPU、内存有明确优化数据。

阶段遗留问题和影响：

- 遗留问题：音频、文件共享、多显示器、打印机、智能卡、RD Gateway 仍不在第一版范围。
- 影响：第一版适合基础远程控制，不适合完整企业级 RDP 客户端替代。

## 阶段交付报告模板

每完成一个阶段，必须输出一份阶段报告，避免只提交代码但没有验收依据。

```md
# 阶段报告：M?

## 完成内容

- 

## 修改范围

- 

## 验收结果

- 验收项：
- 结果：
- 证据：截图 / 日志 / 构建命令 / 测试设备 / 目标 Windows 版本

## 遗留问题

- 问题：
- 原因：
- 后续处理阶段：

## 影响评估

- 对当前 Windows demo 的影响：
- 对 HarmonyOS App 的影响：
- 对安全性的影响：
- 对性能的影响：
- 对后续里程碑的影响：
```

## 当前遗留问题和影响

以下问题在当前阶段尚未解决，需要在后续实施中持续报告。

1. WSL 内 FreeRDP 鸿蒙交叉编译尚未验证。
   - 影响：M3 是最大技术门槛；如果 WSL 内 Linux 版 OHOS NDK 或依赖编译失败，M4-M6 全部阻塞。

2. OpenSSL / NTLM / NLA 兼容性尚未验证。
   - 影响：即使 TCP 可达，也可能在认证阶段失败；需要保留详细日志和错误码。

3. WSL 编译产物与 DevEco 工程集成路径未固化。
   - 影响：需要明确 `.so`、头文件和 CMake config 从 `harmony/out/ohos-arm64/` 进入 ArkTS native module 的方式，否则构建可复现性差。

4. `XComponent` / `NativeWindow` 的实际 API 需要按目标 SDK 校准。
   - 影响：M5 的 native surface 绑定和 buffer flush 可能需要按 HarmonyOS / OpenHarmony 版本调整。

5. 当前 `native/freerdp-bridge` 是 Windows demo 的桌面 stand-in。
   - 影响：它能复用参数模型和 FreeRDP 调用思路，但不能直接作为鸿蒙 N-API 模块使用。

6. 密码、证书和会话配置的安全存储方案未定。
   - 影响：第一版不能保存明文密码；TOFU 和安全存储需要在 M7 前明确。

7. 输入法、中文输入和复杂组合键未验证。
   - 影响：M6 第一版只能保证基础键鼠操作，中文输入体验可能不足。

8. 渲染性能未知。
   - 影响：M5 先接受 CPU 全帧/局部拷贝；如果高分辨率卡顿，M7 必须投入 dirty rect、双缓冲或 GPU texture。

9. 缺少真机测试矩阵。
   - 影响：模拟器通过不代表真机可用；需要至少覆盖一台目标 HarmonyOS 设备、一个 Windows 10/11 Pro RDP 目标和一个弱网场景。

## 第一版范围

第一版只做最小可用远程桌面：

- 输入 IP、端口、用户名、密码。
- 连接 Windows RDP 服务。
- App 内显示远程桌面。
- 支持点击、拖动、键盘输入。
- 支持断开连接。
- 支持基础网络诊断和连接状态日志。

第一版暂缓：

- 音频。
- 文件共享。
- 多显示器。
- 打印机。
- 智能卡。
- 复杂剪贴板。
- RD Gateway。
- 高性能视频编解码。

## 最大风险点

- WSL 内 Linux 版 OHOS NDK、FreeRDP、OpenSSL、WinPR 的交叉编译链路。
- WSL 产物同步到 Windows / DevEco 工程后的链接和打包路径。
- OpenSSL / NTLM / NLA 兼容性。
- `NativeWindow` 渲染性能。
- 输入法和组合键映射。
- 后台线程和 ArkUI 生命周期同步。
- 证书策略和安全存储。
- 不同 HarmonyOS 版本的 NDK / XComponent 行为差异。

## 下一步建议

下一步仍然先做 M1 + M2，同时把 M3 的 WSL 编译环境准备好，不在 Windows 上尝试直接编完整 FreeRDP：

1. 创建 `harmony/` 或 `openharmony/` 工程骨架。
2. 做 ArkTS 连接页、会话页、诊断页。
3. 新建 N-API native bridge。
4. 实现 `probe()`、`connect(params)`、`disconnect()` 的 mock/native 调用。
5. 在 WSL Ubuntu 内确认 `cmake`、`ninja`、Linux 版 OHOS NDK 路径。
6. 新增 `harmony/scripts/wsl/build-freerdp-ohos.sh`，先只做到依赖版本检查和空构建目录初始化。
7. 让 C++ 返回状态和日志。

只要 ArkUI 能调用 C++，C++ 能回传状态和日志，同时 WSL 能稳定产出 native 依赖，后续接 FreeRDP 就有明确落点。

## 参考依据

- FreeRDP 官方仓库：https://github.com/FreeRDP/FreeRDP
- OpenHarmony XComponent / NativeWindow 文档。
- 当前仓库 Windows demo 的连接参数、诊断脚本和 native bridge 骨架。
