# 会话状态机协调器拆分设计

将 `Index.ets`（950 行）内的连接/会话状态机（`onState`/`onError` 决策树、`connectNative`/`startNativeConnect`、会话通知与反馈文案）提取到独立可测的 `RdpSessionStateCoordinator`，复刻已落地+已测的 `RemoteControlCoordinator` 范式（构造注入 + snapshot + onChange）。台账：TAB-A-09。

## 策略

保守提取：**逐字搬移** `onState`/`onError`/`startNativeConnect` 决策树的 if/else 分支、顺序与 feedback 文本，只改产出目标——从直接写 ArkUI `@State` 改为写协调器内部态 + `publish()`（经 `onChange` 回调由 Index 落地 `@State`）。不改变状态机语义，不调 ArkUI API（`focusControl.requestFocus` 留 Index），不直接写 `@State`。

## 提取映射（Index → Coordinator）

| Coordinator 方法 | Index 原位置 | 搬移内容 |
|---|---|---|
| `register()` | `registerNativeCallbacks`（489-551）onState/onError 体内联 | 调 `rdpClientController.registerCallbacks({onState, onError})`，幂等（`callbacksRegistered`）；onState→`handleState`，onError→`handleError` |
| `handleState(state, wasConnected)` | onState 决策树（496-527） | 逐字搬移：nextConnected 计算；终态非连接(Disconnected/Failed/Idle)→wasConnected 则 releaseInput + sessionVisible=false；updateSessionNoticeForNativeState；首连(nextConnected&&!wasConnected)→sessionVisible=true+清 lastError+feedback SUCCESS+onPersistProfile+清 pending；Failed→清 pending+feedback CREDENTIALS；Disconnected/Idle+pending→清 pending+feedback NO_SESSION；其余 log |
| `handleError(message, wasConnected)` | onError 决策树（528-540） | 逐字搬移：wasConnected→releaseInput；sessionVisible=false；clearSessionNotice；清 pending；lastError=message；feedback danger |
| `connect(input)` | `connectNative`（696-733） | 校验（`RdpConnectionValidator.validate` 静态）；校验失败→beginConnect+清 pending+feedback warning+return；成功→构造 pendingSave+beginConnect+初态 notice/feedback CONNECTING+sessionVisible=true+queueNativeConnect |
| `queueNativeConnect` / `handleConnectResult(result)` | `queueNativeConnect`（735-740）/`startNativeConnect`（742-777） | setTimeout 调 rdpClientController.connect + handleConnectResult；ok→sessionVisible=true+isConnected 则 SUCCESS+persistProfile；!ok→sessionVisible=false+clearNotice+清 pending+lastError+feedback danger；catch→同 !ok 但 fallback CREDENTIALS |
| `updateSessionNoticeForNativeState(state)` | （563-597） | 纯函数搬入：RemoteLoginWaiting/Connected/Resolving/TCP connected/Negotiating/Authenticating 的 title/subtitle 映射 |
| `connectionFailureMessage` / `errorMessage` | （458-464）/（466-472） | 纯函数搬入 |
| `snapshot()` / `publish()` | 新增（对齐 RemoteControlCoordinator） | snapshot 返回 6 字段；publish 调 onChange |

Index 删除：`queueNativeConnect`/`startNativeConnect`/`updateSessionNoticeForNativeState`/`connectionFailureMessage`/`errorMessage`/`isConnectedNativeState`/`clearSessionNotice`/`set-clearConnectionFeedback`/`persistPendingConnectionProfile`/`isConnected`/`setSessionVisible`（移到 Coordinator 或 applySessionStateSnapshot）。

## snapshot 字段（`RdpSessionStateSnapshot`）

| 字段 | Index @State | 初始值 |
|---|---|---|
| `sessionVisible` | `showSession`（76） | `false` |
| `remoteLoginWaiting` | `remoteLoginWaiting`（80） | `false` |
| `sessionNoticeTitle` | `sessionNoticeTitle`（81） | `''` |
| `sessionNoticeSubtitle` | `sessionNoticeSubtitle`（82） | `''` |
| `connectionFeedbackText` | `connectionFeedbackText`（83） | `''` |
| `connectionFeedbackTone` | `connectionFeedbackTone`（84） | `'neutral'` |

## 注入依赖（构造参数，对齐 RemoteControlCoordinator）

| 依赖 | 类型 | 用途 |
|---|---|---|
| `rdpClientController` | `RdpClientController` | registerCallbacks/connect/beginConnect/isConnected |
| `messages` | `SessionStateMessages` | 文案捆绑（SESSION_CONNECTING_TITLE/SUBTITLE、CONNECTION_SUCCESS/CONNECTING、CONNECTION_FAILURE/FAILURE_CREDENTIALS/FAILURE_NO_SESSION、connectionFailureWithDetail、各 SESSION_*_TITLE/SUBTITLE） |
| `appFilesDir` | `() => string` | 传给 native connect 的 appFilesDir |
| `onPersistProfile` | `(input: WindowsConnectionSaveInput) => void` | 首连成功时持久化连接配置 |
| `onChange` | `(snapshot: RdpSessionStateSnapshot) => void` | 状态变更通知 Index |
| `releaseInput` | `() => void = () => {}` | 终态/onError 释放输入（Index 注入 `releaseActiveInput`） |

## 边沿触发语义（Index `applySessionStateSnapshot`）

- `sessionVisible` 上升沿（`false→true`）：`deferUiUpdate(() => this.focusRemoteSurface())`——首连获焦（原 onState `focusRemoteSurface()`）。
- `sessionVisible` 下降沿（`true→false`）且 `rdpClientController.isConnected()`：`releaseActiveInput()`——原 `setSessionVisible(false)` 中 `!visible && showSession && isConnected()` 分支。
- 上述边沿由 Index 在写 `@State` 后判断，协调器只产出 `sessionVisible` 值；协调器自身在 `handleState`/`handleError` 中对 `wasConnected` 的显式 `releaseInput` 调用保留（与原 `if (wasConnected) releaseActiveInput()` 一致；终态时 `isConnected()` 已为 false，下降沿不会二次触发）。

## 不变项

- `onState`/`onError` 决策树语义逐字保留：只搬移位置，不改 if/else 分支/顺序/feedback 文本。
- `handleState`/`handleError`/`handleConnectResult`/`connect` 须 public（H3 测试入口）。
- 协调器不调 ArkUI API（`focusControl.requestFocus` 留 Index `focusRemoteSurface`）。
- 协调器不直接写 `@State`（经 snapshot + onChange 由 Index 落地）。
- 保留所有 `@State` 字段名（build 绑定不变）。
- `applySessionStateSnapshot` 写 `@State` 与原 `setSessionVisible`/`setConnectionFeedback` 等语义一致。
- `focusRemoteSurface` 仍走 `deferUiUpdate`。
- `RdpClientController` 的 `connected` 单一状态源不变（TAB-A-07 9.1.1）。

## 验收

- AC-ARCH：`Index.ets` 净减约 140 行；`RdpSessionStateCoordinator.ets` 约 220-280 行；`onState`/`onError` 决策树不在 Index 内联。
- AC-TEST：`RdpSessionStateCoordinator.test.ets` 覆盖 7+ native 状态转移（Idle/Resolving/Connected/RemoteLoginWaiting/RemoteDesktopReady/Disconnected/Failed）、首连 SUCCESS+persistProfile、Failed CREDENTIALS+清 pending、Disconnected/Idle+pending NO_SESSION、onError danger+sessionVisible=false、connect 校验失败/成功、handleConnectResult ok/!ok。
- 构建：`build_app` debug/hap/default 0 error + signed。
- 测试：`hvigor test --mode module -p module=common@default` 全部通过。
- 真机：connect→connected→disconnect + connect→failed 冒烟后升 Verified。

## 风险与回退

- 决策树逐字搬移，语义不变；错误表现为编译/测试失败，可即时修。
- `connect` 不再回写 `host`/`port`/`username` 的 trimmed 值到 `@State`（原 `connectNative` 的 `this.host = validation.host` 等），属可接受的行为微调：trimmed 值仍用于 pendingSave 与 native connect，表单显示保留用户输入原文，下次从 profile 加载时为 trimmed 值。
- 独立 git 提交，可回退。
