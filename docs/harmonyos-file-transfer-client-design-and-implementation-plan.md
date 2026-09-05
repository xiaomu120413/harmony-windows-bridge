# HarmonyOS FTP/FTPS/SFTP 客户端详细设计与实施计划

> Change ID: FT-ARCH-001  
> 状态: DesignReady（MVP Change FT-MVP-001 已获用户授权进入实施）  
> 目标平台: HarmonyOS NEXT，优先 2in1/平板，兼容手机自适应布局  
> 当前工程基线: target SDK 26，compatible SDK 22  
> 工作名称: MuTransfer（不直接使用 FileZilla 名称、代码、图标或界面资产）

## 1. 结论与核心决策

建议把产品做成一个独立的 HarmonyOS 文件传输客户端，而不是把协议逻辑塞进现有 RDP 会话模块。现有仓库可以复用多设备 App Pack、HSP、ArkTS/N-API/C++ 分层、权限桥和诊断体系的工程经验，但新产品应使用独立包名、独立 Entry HAP 和独立 Native 模块。

首版协议范围：

- 必做：SFTP、显式 FTPS、FTP；其中 SFTP 和显式 FTPS 是推荐入口，明文 FTP 默认展示强风险提示。
- 必做：远端浏览、上传、下载、批量队列、暂停/恢复、断点续传、冲突处理、失败重试、后台可见传输。
- 次期：隐式 FTPS、HTTP/SOCKS 代理、站点导入导出、目录同步、拖拽与快捷键增强。
- 后期按需求评估：WebDAV(S)、SCP、云存储协议。SMB 不纳入首版。

协议引擎首选“经过能力验证的 libcurl multi”：FTP/FTPS/SFTP 共用异步传输模型，SFTP 由 libssh2 后端提供。Phase 0 必须验证构建产物实际包含所需协议与远端文件操作；如果 libcurl 的 SFTP 元数据操作不能满足需求，则保持统一调度器不变，仅将 `SftpAdapter` 替换为直接使用 libssh2 的非阻塞实现。

## 2. 产品范围与非目标

### 2.1 MVP 用户能力

1. 保存并管理站点连接。
2. 使用密码或 SSH 私钥连接 SFTP；使用账号密码连接 FTP/FTPS。
3. 浏览远端目录，查看名称、类型、大小、时间、权限等元数据。
4. 从用户授权的本地文件/目录批量上传。
5. 批量下载到应用沙箱或用户授权目录。
6. 创建目录、重命名、删除、刷新；SFTP 支持权限修改可放在 P1。
7. 查看队列、速度、ETA、失败原因；支持暂停、继续、取消和重试。
8. 应用退至后台后，以用户可感知的数据传输长时任务继续工作。
9. 应用或进程被回收后，恢复队列并从安全检查点续传。
10. 导出经过脱敏的诊断包。

### 2.2 首版明确不做

- 不提供 SSH 终端和任意远端命令执行。
- 不绕过 HarmonyOS 沙箱，不申请“全盘扫描”式高风险权限。
- 不承诺所有服务器都支持远端哈希校验；跨 FTP/SFTP 没有统一的标准远端校验命令。
- 不在首版实现双向自动同步、计划任务或无人值守常驻保活。
- 不复用 FileZilla 源代码、品牌、图标或界面资产。若将来确需复用，必须先完成 GPL、商标及上架合规评审。

## 3. HarmonyOS 平台边界

### 3.1 本地文件访问

普通三方应用不能像桌面文件管理器一样直接遍历全部公共存储。产品必须使用以下模型：

- 用户通过系统 Picker 选择文件或目录，应用获得 URI/文件描述符。
- Native 协议层只接收 `dup` 后的文件描述符或受控 I/O 回调，不假设 URI 可以转换为普通路径。
- 对需要跨重启续传的 URI，在设备支持 `SystemCapability.FileManagement.AppFileService.FolderAuthorization` 且获批 `ohos.permission.FILE_ACCESS_PERSIST` 时持久化授权。
- 不支持持久化授权的设备采用降级方案：上传前复制到应用 staging 目录；下载先写入应用沙箱，完成后由用户导出到公共目录。
- 任何批量操作开始前检查可用空间，并明确提示 staging 会产生额外空间占用。

### 3.2 后台传输

- 整个活动队列申请一个 `dataTransfer` 长时任务，而不是每个文件申请一个任务。
- 需要声明 `ohos.permission.KEEP_BACKGROUND_RUNNING` 和 Ability 的 `backgroundModes: ["dataTransfer"]`。
- 长时任务必须显示用户可见通知，通知可进入队列页并允许暂停/取消。
- 用户移除通知、系统取消任务、网络断开或进程回收时，立即持久化检查点并安全暂停。
- 系统托管的 HTTP 上传下载代理不作为 FTP/SFTP 的核心执行器；FTP/SFTP 由应用 Native 引擎执行。
- 设计目标是“可恢复”，不是绕过系统限制实现无限保活。

### 3.3 多设备与窗口

- 2in1/平板：双栏本地/远端浏览器 + 底部/侧边传输队列，支持鼠标、键盘、多选和拖拽。
- 手机：单栏导航，本地与远端通过页签切换，队列使用独立页面。
- 共享业务和 Native 引擎放入独立 HSP；不同设备 Entry HAP 只负责启动、窗口和设备专属交互。

## 4. 目标架构

```text
ArkUI Pages / Components
        |
Application Coordinators
        |
Repositories + CredentialVault + BackgroundTask
        |
Versioned N-API Gateway
        |
Native Transfer Service
  +-----+------------------+------------------+
  | Scheduler/State Machine| Connection Pool  | Diagnostics
  | Local URI/FD I/O       | Retry/Checkpoint | Rate Limit
  +------------------------+------------------+
        |
ProtocolAdapter
  +---------------+-----------------+
  | FTP/FTPS       | SFTP            |
  | libcurl multi  | libcurl+libssh2 |
  |                | or direct libssh2 after spike
  +---------------+-----------------+
        |
OpenSSL / libssh2 / HarmonyOS socket and file APIs
```

### 4.1 模块建议

建议新建独立工程目录，避免和当前 RDP 产品耦合：

```text
harmony/filetransfer_app/
  AppScope/
  transfer_common/                  # shared HSP
    src/main/ets/
      connection/
      browser/
      transfer/
      persistence/
      security/
      background/
      diagnostics/
      components/
    src/main/cpp/
      napi/
      engine/
      protocol/
      io/
      security/
      diagnostics/
      tests/
  entry_2in1/
  entry_tablet/
  entry_phone/
harmony/third_party/curl/
harmony/third_party/libssh2/
harmony/third_party/openssl/         # 或经验证的统一 TLS 依赖
```

第三方库必须锁定版本/提交，保留 LICENSE/NOTICE、补丁目录、构建参数和 SBOM。禁止运行时从网络下载动态库。

### 4.2 ArkTS 职责

- 页面、响应式布局、交互、Picker、权限申请和后台通知。
- 连接配置、队列展示、冲突对话框、设置和诊断导出。
- RDB/Preferences 持久化与任务恢复编排。
- 将 Picker 获得的 URI 打开为 fd，将复制后的 fd 交给 Native 层。
- 不执行大文件读写，不解析 FTP/SFTP 协议，不在 UI 线程计算目录差异。

### 4.3 N-API 边界

N-API 只负责参数校验、DTO 转换、异步调用和事件桥接。建议公开下列版本化接口：

```text
initialize(config) -> EngineCapabilities
connect(profileId, endpoint, credentialToken) -> sessionId
disconnect(sessionId)
list(sessionId, remotePath, pageToken?) -> RemotePage
enqueue(batchSpec, localHandles) -> batchId
pause(jobOrBatchId)
resume(jobOrBatchId)
cancel(jobOrBatchId)
resolveConflict(jobId, decision)
getSnapshot() -> QueueSnapshot
subscribeEvents(callback)
shutdown(deadlineMs)
```

约束：

- 不把密码、私钥内容、完整 URL 放入事件和日志。
- 单次 DTO 有明确大小上限；目录分页，事件合并。
- N-API 回调只投递状态，不阻塞 Native reactor。
- 所有句柄都有 owner 和关闭规则；Native 层 `dup` fd 后独立管理生命周期。

### 4.4 Native 核心模块

- `TransferService`：引擎生命周期和线程所有权。
- `TransferScheduler`：公平排队、并发、优先级、限速和资源预算。
- `TransferStateMachine`：唯一合法状态迁移入口。
- `ConnectionPool`：按站点/协议隔离连接，限制每主机并发。
- `ProtocolAdapter`：统一远端文件系统操作。
- `CurlMultiReactor`：基于 `curl_multi_socket_action` 的事件循环。
- `LocalIoProvider`：fd/URI 流式读写、临时文件、空间检查和安全提交。
- `CheckpointStorePort`：将可恢复状态回传 ArkTS 持久化层。
- `SecurityPolicy`：TLS、SSH host key、协议白名单和路径校验。
- `DiagnosticSink`：结构化、脱敏、限量日志与指标。

## 5. 协议设计

### 5.1 协议能力矩阵

| 能力 | FTP | 显式 FTPS | SFTP |
| --- | --- | --- | --- |
| 默认推荐 | 否 | 是 | 是 |
| 传输加密 | 无 | TLS | SSH |
| 服务端身份校验 | 无 | CA/主机名/证书 | SSH host key |
| 密码认证 | 是 | 是 | 是 |
| SSH 私钥 | 否 | 否 | 是 |
| 断点下载 | SIZE + REST | SIZE + REST | stat + offset |
| 断点上传 | 需能力探测 | 需能力探测 | stat + offset |
| 原子提交 | RNFR/RNTO 尽力 | RNFR/RNTO 尽力 | rename/posix-rename |
| 标准远端哈希 | 无统一标准 | 无统一标准 | 无统一标准 |

### 5.2 libcurl 构建门禁

启动时使用 `curl_version_info()` 形成 `EngineCapabilities`，并在 CI/真机测试中断言：

- `ftp`、`ftps`、`sftp` 都存在于实际构建的 protocol list。
- TLS backend 和 libssh2 版本符合锁定清单。
- 64 位文件偏移、IPv6、异步 DNS 或受控 resolver 行为可用。
- SFTP host key 校验回调、私钥认证、目录列举、rename、mkdir、delete 和 resume 均通过真实服务器测试。

应用不得仅凭“libcurl 理论支持”就宣称功能可用；实际支持协议取决于构建参数。

### 5.3 FTP/FTPS 策略

- 优先使用 EPSV/PASV，主动模式放到高级设置并默认关闭。
- 优先 MLSD/MLST；服务器不支持时再回退 LIST，并隔离易错的文本解析器。
- 显式 FTPS 强制验证证书链和主机名，TLS 最低 1.2；TLS 1.3 由底层能力协商。
- 禁止全局关闭 `peer` 或 `host` 校验。
- 证书错误默认阻断。若未来提供例外，只能是用户明确确认的单站点、可撤销策略，且不得把过期、主机名不符和未知 CA 混为一个开关。
- URL 仅由结构化字段生成，用户名、密码不拼入 URL。
- `QUOTE`/命令参数禁止 NUL、CR、LF，远端路径使用独立安全编码器。

### 5.4 SFTP 策略

- 首次连接展示 SHA-256 host key 指纹、算法、主机和端口，用户明确确认后写入 known-hosts；不静默 TOFU。
- 已保存 host key 变化时必须硬失败并提示潜在中间人攻击，不允许自动覆盖。
- 支持密码和加密私钥；keyboard-interactive 放在 P1。
- 禁止提供任意 SSH 命令执行接口来实现“远端哈希”或文件操作。
- 递归下载默认不跟随符号链接，防止循环和越界；用户可在高级设置中选择只下载链接本身或显式跟随。

## 6. 传输与批量调度

### 6.1 状态机

```text
Queued -> Resolving -> Connecting -> Authenticating -> Preparing
       -> Transferring -> Verifying -> Committing -> Succeeded

任一活动态 -> Pausing -> Paused -> Queued/Preparing
可重试失败 -> RetryWait -> Queued
需用户选择 -> Conflict -> Queued/Cancelled
任一非终态 -> Cancelling -> Cancelled
不可恢复错误 -> Failed
```

只有状态机可以修改任务状态。每次状态变化写入单调递增 `generation`，持久化层用 `(jobId, generation)` 防止旧事件覆盖新状态。

### 6.2 数据模型

`ConnectionProfile`：

- id、显示名、协议、host、port、username、authType、secretRef、初始目录。
- TLS/host-key 策略、代理、字符集、时区提示、每主机并发数。
- 不直接存密码和私钥明文。

`TransferBatch`：

- batchId、方向、profileId、创建时间、用户优先级、冲突策略、重试策略、限速策略。

`TransferJob`：

- jobId、parentId、本地 URI 引用、远端规范化路径、文件类型。
- expectedSize、transferredBytes、sourceMtime、sourceIdentity、tempTarget。
- state、generation、attempt、nextRetryAt、errorDomain、errorCode。

`TransferCheckpoint`：

- 可安全恢复的 offset、本地/远端 size 和 mtime 快照、临时文件身份、最后确认时间。
- 密钥材料、密码和完整日志不进入检查点。

`KnownHost`：

- host、port、keyType、SHA-256 fingerprint、firstSeen、lastSeen、用户确认来源。

### 6.3 批量展开

- 目录递归使用“生产者 + 有界队列”，按页生成子任务，不一次性把十万文件加载到内存。
- 默认最大深度 128，最大单批条目数可配置并有硬上限；达到限制时暂停并提示用户。
- 目录扫描和文件传输分别限流，控制连接优先于数据连接，保证浏览 UI 不被大队列饿死。
- 删除、覆盖等破坏性操作先形成可审阅计划，再执行。

### 6.4 并发与公平性

初始建议值：

- 手机：全局 2 个活动传输；平板/2in1：全局 4 个。
- 同一站点默认 2 个；用户可在 1～6 内调整。
- 小文件队列采用大小感知调度，但不得让大文件永久饥饿；使用加权轮询。
- Wi-Fi/有线网络可提升并发，蜂窝网络和省电/高温状态降低为 1。
- 全局和单任务限速都使用 token bucket，限速在 Native 层执行。

### 6.5 断点、校验与安全提交

下载：

1. 写入同目录临时名 `.name.mutransfer.<jobId>.part`，或写入应用 staging。
2. 每个检查点保存已确认 offset、源 size/mtime 和临时文件身份。
3. 恢复前重新 stat；源已变化则进入 Conflict，不盲目拼接。
4. 完成后校验大小；若服务器提供可信哈希扩展，再做可选哈希校验。
5. fsync/close 后原子 rename；目标 Provider 不支持原子 rename 时明确采用“复制后替换”降级并保留失败清理记录。

上传：

1. 上传到远端临时名 `.name.mutransfer.<jobId>.uploading`。
2. 恢复前校验本地 source identity、size、mtime 和远端临时文件大小。
3. 完成并校验大小后 rename 为最终名。
4. 不支持原子 rename 的服务器标记为能力降级，并在异常中断后提示可能残留临时文件。

## 7. 安全详细设计

### 7.1 凭据与本地数据

- 密码、Token、私钥 passphrase 等不超过 1 KiB 的短敏感数据优先存入 HarmonyOS Asset Store，并根据产品策略设置“首次解锁后可访问”或“仅解锁时可访问”。
- 较大的 SSH 私钥文件副本使用 HUKS 生成的不可导出应用主密钥做 AES-256-GCM 信封加密。
- 每条大对象密文使用独立随机 nonce，AAD 绑定 `bundle/profileId/secretType/schemaVersion`，防止密文替换。
- RDB 只保存 `secretRef` 和密文元数据；Asset Store 数据、HUKS 主密钥和明文都不硬编码、不写入配置或日志。
- 私钥从 Picker 导入后立即加密；解密只在连接期间发生，Native 使用受控 buffer，使用后覆盖并释放。
- P1 可加入系统用户认证后解锁凭据；无认证时仍依赖应用沙箱和 HUKS。
- 站点导出默认不含凭据；显式导出凭据必须二次确认、强口令加密并提示风险。

### 7.2 网络与身份校验

- 协议白名单仅允许当前任务声明的 scheme，避免构造 URL 后意外启用 libcurl 其他协议。
- 禁止重定向到其他协议，FTP/SFTP 任务不跟随 HTTP 式重定向。
- 连接超时、认证超时、低速超时和总重试次数都有上限。
- 日志不记录密码、私钥、带凭据 URL、完整本地/远端路径或文件内容。
- FTP 明文连接必须显示持续风险标识；保存站点时再次确认。
- 远端主机和证书信任按 `host:port` 隔离，不能跨站点复用。

### 7.3 路径与文件安全

- 拒绝包含 NUL 的名称；用于协议命令的字符串拒绝 CR/LF。
- 下载时只把一个远端叶子名映射为一个本地叶子名，先做 Unicode 规范化与冲突检测。
- 在打开和提交目标前验证其仍位于用户授权根目录/目标 fd 范围内，防止 `../` 和符号链接逃逸。
- 递归操作维护 visited identity，防止符号链接环和目录环。
- 覆盖、删除、批量重命名必须经过统一冲突/破坏性操作策略，不由协议 adapter 自行决定。
- 临时文件包含不可预测 jobId，不使用仅基于文件名的固定临时路径。

### 7.4 供应链与发布安全

- curl、libssh2、OpenSSL 使用固定版本和可审计补丁，不跟随浮动分支。
- 每次发布生成 SBOM，执行许可证扫描、已知 CVE 扫描和符号/协议能力清单比对。
- Release 包关闭 Native debug/verbose 协议日志，保留必要的崩溃符号归档。
- 签名材料不放入公开仓库；本地调试签名与发布签名隔离。
- 上架前完成隐私声明、权限用途、网络传输说明和第三方 NOTICE。

## 8. 性能详细设计

### 8.1 线程和事件模型

- 一个 `CurlMultiReactor` 线程处理 socket/timer 事件，不为每个任务创建线程。
- 1～2 个有界磁盘 worker 处理可能阻塞的 fd 读写、fsync、staging copy 和可选哈希。
- Native 与 ArkTS 之间只发送合并后的状态；默认 250 ms 或进度变化超过 1% 才上报。
- UI 只渲染当前可见队列项，使用懒加载和稳定 item key。

### 8.2 内存与背压

- 每活动任务初始流缓冲 256 KiB，可在 64～512 KiB 范围调节。
- 所有传输 buffer 总预算默认 16 MiB、硬上限 32 MiB；超过预算时调度器不再启动新任务。
- 目录页默认 200 项，Native 单次事件负载设置硬上限。
- 网络快于磁盘时暂停读取；磁盘快于网络时限制预读，禁止把整个文件放入内存。
- 所有大小和 offset 使用 64 位类型，覆盖大于 4 GiB 的文件。

### 8.3 连接与网络

- 复用 DNS、TLS session 和可安全复用的连接；凭据、TLS 策略或 host key 不同的 profile 不共享。
- 默认连接超时 15 秒；低速窗口建议 30 秒，具体阈值可配置。
- 网络切换后不直接沿用旧 socket，任务转入 RetryWait，重新解析和校验后续传。
- 重试使用指数退避加随机抖动，例如 1、2、4、8、16、30 秒封顶；认证失败、证书失败、host key 变化不自动重试。

## 9. 用户体验设计

### 9.1 核心页面

- 站点管理：快速连接、收藏站点、最近连接、安全状态。
- 文件浏览：本地授权根与远端目录、面包屑、多选、搜索/过滤、排序。
- 传输队列：活动、等待、失败、已完成；批量控制和单项控制。
- 安全中心：known hosts、证书异常历史、明文 FTP 站点、凭据锁定设置。
- 设置：并发、限速、蜂窝网络策略、冲突策略、临时文件清理、诊断。

### 9.2 冲突策略

统一支持：询问、覆盖、跳过、自动重命名、符合条件时续传。用户可以“仅此文件”或“应用到本批次”。目录与文件类型冲突不自动覆盖，必须进入人工决策。

### 9.3 错误表达

底层错误映射为稳定的产品错误域：

- Network、DNS、TLS、SSHHostKey、Authentication、Permission、LocalIO、RemoteIO、Conflict、Quota、Cancelled、Internal。
- UI 展示可执行建议，不直接展示含路径或凭据的底层错误字符串。
- 诊断包保留内部错误码、阶段、耗时和库版本，但默认对 host、username 和 path 哈希化/脱敏。

## 10. 持久化与恢复

- 使用 RDB 存 profile 元数据、known hosts、batch/job/checkpoint 和迁移版本；Preferences 只存轻量 UI 设置。
- 任务事件批量事务写入，进度不必每 250 ms 落盘；默认每 2 秒、每 4 MiB 或状态变化时写检查点。
- 启动时把遗留活动态标记为 `Interrupted`，重新校验权限、source identity、远端状态和临时文件后再入队。
- 数据库迁移必须向前兼容；密文 schema 单独版本化并支持密钥轮换。
- 成功历史按数量/时间清理，失败和安全事件保留策略由用户可见设置控制。

## 11. 测试与验收

### 11.1 自动化测试

- ArkTS 单测：连接校验、权限降级、冲突策略、恢复编排、UI view model。
- C++ 单测：状态机、调度公平性、退避、限速、路径规范化、临时文件提交、错误映射。
- 协议集成：vsftpd FTP、带受信/自签/过期证书的 FTPS、OpenSSH SFTP。
- 故障注入：断网、切网、丢包、延迟、半连接、服务器重启、磁盘满、权限撤销、进程被杀。
- 模糊测试：LIST/MLSD 输出、远端文件名、N-API DTO、路径编码和诊断脱敏。
- Sanitizer：host 侧 ASan/UBSan；真机使用可用的 Native 内存和线程诊断工具。

### 11.2 功能验收基线

- 0 B、1 B、4 GiB 以上文件均可正确传输。
- 1 万文件批次可以分页展开和执行，UI 不冻结且不一次性占用线性大内存。
- 中断上传/下载后能安全续传；源变化时不会错误拼接。
- 重启应用后队列和冲突状态可恢复；最终文件不出现静默损坏。
- FTPS 证书错误、SFTP host key 变化均硬失败。
- 权限被撤销后任务进入可理解的 Permission 状态，不循环重试。
- plain FTP 风险持续可见，不能被默认模板无意启用。

### 11.3 性能验收基线

在固定真机、固定局域网和同一服务器上，以同版本命令行/测试 harness 的单流能力为基准：

- 单个大文件 FTP/FTPS 吞吐达到基准的 80% 以上。
- 单个大文件 SFTP 吞吐达到基准的 75% 以上。
- 4 个并发任务时，传输 buffer 与队列元数据增量内存不超过 32 MiB。
- Native 进度事件不高于 4 Hz/活动任务，队列滚动保持流畅。
- 暂停/取消指令 P95 在 500 ms 内被引擎确认；不要求网络端已在该时限内完成关闭。
- 后台运行 8 小时或完成指定大批次后，无持续增长的 fd、线程和 Native heap 泄漏。

## 12. 实施计划

### Phase 0：能力与风险验证（1～2 周）

交付：可在真机运行的 Native spike，不做完整 UI。

- 交叉编译 curl + OpenSSL + libssh2，输出版本、协议和特性清单。
- 真实验证 FTP、显式 FTPS、SFTP 连接、目录列举、上传、下载、rename、delete、resume。
- 验证 SFTP host key 回调和 FTPS 证书校验不可绕过。
- 验证 URI -> fd -> Native 回调式 I/O，测试 4 GiB 以上文件。
- 验证 `dataTransfer` 长时任务、进程回收和重启恢复路径。
- 验证目录授权持久化能力和 compatible SDK 22 的降级分支。

退出门禁：形成能力矩阵并决定 SFTP 使用 libcurl 还是直接 libssh2。任何关键能力未证明前不进入大规模 UI 开发。

### Phase 1：工程骨架与安全基础（2 周）

- 新建独立 App Pack、多设备 Entry 和 `transfer_common` HSP。
- 建立 N-API 版本化网关、Native 生命周期和线程模型。
- 建立 RDB schema、CredentialVault、known-hosts 与脱敏日志。
- 接入第三方 NOTICE、版本锁定、SBOM 生成脚本。

退出门禁：空队列生命周期、密钥加解密、权限/Picker 流程和真机 HAP 构建通过。

### Phase 2：站点与远端浏览（2 周）

- 站点管理、快速连接、安全确认 UI。
- FTP/FTPS/SFTP connect/list/stat/mkdir/rename/delete。
- 目录分页、刷新、错误映射、连接池和超时。

退出门禁：三协议在基准服务器上完成浏览和基本远端文件操作；证书/host key 负例通过。

### Phase 3：单文件可靠传输（2 周）

- fd 流式 I/O、临时文件、上传/下载、进度和取消。
- 大文件、空间检查、大小校验和安全提交。
- 前台错误恢复和诊断。

退出门禁：0 B 到 4 GiB+ 文件通过，断网不产生错误最终文件。

### Phase 4：批量队列与断点恢复（3 周）

- 目录懒展开、调度器、公平性、并发和限速。
- pause/resume/retry/conflict，任务与检查点持久化。
- 进程重启、服务器重启、权限撤销和源变化处理。

退出门禁：1 万文件批次、网络故障注入和应用重启恢复通过。

### Phase 5：后台、多设备与体验完善（2～3 周）

- `dataTransfer` 长时任务、通知和取消回调。
- 2in1/平板双栏、手机单栏、键鼠、多选和拖拽。
- 安全中心、设置、历史、诊断导出和可访问性。

退出门禁：前后台切换、低电量/高温/蜂窝策略、多窗口和旋转测试通过。

### Phase 6：安全与性能硬化、发布（2～3 周）

- 安全审计、fuzz、CVE/许可证/SBOM、隐私与权限审查。
- 性能基准、内存/fd/线程泄漏、长稳测试。
- 灰度策略、崩溃符号、第三方 NOTICE 和上架材料。

退出门禁：本设计第 11 节全部验收项有证据，所有高危安全问题关闭。

### 12.1 粗略投入

- 1 名资深 HarmonyOS/ArkTS + 1 名资深 C++ 网络工程师：约 10～14 个日历周完成可发布 MVP。
- 单人全栈实现：约 4～6 个月，且协议、安全和真机长稳测试风险明显更高。
- 若首版仅做 SFTP + 单文件传输，可在 4～6 周形成内测版，但不应宣称为完整 FileZilla 类产品。

## 13. 风险清单与缓解

| 风险 | 影响 | 缓解措施 |
| --- | --- | --- |
| libcurl 的 HarmonyOS 构建未包含 SFTP/FTPS | 核心协议不可用 | Phase 0 以实际 protocol list 和真机集成为门禁 |
| libcurl SFTP 元数据操作不完整 | 浏览/管理能力受限 | 保持 Adapter 接口，切换为直接 libssh2 非阻塞实现 |
| 公共目录权限不能跨重启 | 后台/恢复失败 | FileShare 持久授权；不支持时 staging 降级 |
| 系统回收后台进程 | 传输中断 | 长时任务 + 检查点 + 临时文件 + 重启恢复 |
| FTP 服务端方言差异 | LIST/续传不稳定 | MLSD 优先、能力探测、服务器兼容测试矩阵 |
| 小文件过多导致 UI/DB 压力 | 卡顿、内存升高 | 懒展开、分页、有界队列、事务批写、可见项渲染 |
| SSH host key/FTPS 证书被错误忽略 | 中间人攻击 | 默认严格校验，变化硬失败，无全局忽略开关 |
| 第三方库漏洞 | 发布安全风险 | 固定版本、SBOM、CVE 门禁和快速升级流程 |
| FileZilla 品牌/代码复用 | 许可证与商标风险 | 独立品牌和实现；复用前单独法务/开源合规评审 |

## 14. 进入编码前需要确认的产品决策

1. 首发设备：只做 2in1/平板，还是同时包含手机 Entry。
2. 首发协议：建议 SFTP + 显式 FTPS + FTP；是否接受先 SFTP 再扩协议。
3. 产品形态：独立应用（推荐）还是现有 MuHub 中的一个功能入口。
4. 源码与商业策略：闭源、开放核心，还是完整开源；这决定第三方依赖和发布合规策略。
5. 是否要求目录同步、站点批量导入、代理和蜂窝后台传输进入 MVP。

## 15. 参考依据

- [OpenHarmony 长时任务](https://gitee.com/openharmony/docs/blob/master/zh-cn/application-dev/task-management/continuous-task.md)：数据传输属于用户可感知的长时任务场景。
- [HarmonyOS FileShare 授权持久化](https://developer.huawei.com/consumer/cn/doc/HarmonyOS-Guides/native-fileshare-guidelines)：Picker URI、文件/目录授权持久化能力。
- [HarmonyOS Asset Store Kit](https://developer.huawei.com/consumer/cn/doc/harmonyos-guides-V14/asset-store-kit-overview-V14)：账号密码、Token 和其他短敏感数据的安全存储与访问控制。
- [OpenHarmony HUKS](https://gitee.com/openharmony/security_huks/blob/master/README.md)：大对象信封加密所需的应用密钥生成、存储和密码学操作。
- [libcurl multi](https://curl.se/libcurl/c/libcurl-multi.html) 与 [curl_version_info](https://curl.se/libcurl/c/curl_version_info.html)：异步批量传输和构建时协议能力探测。
- [libssh2 官方文档](https://libssh2.org/docs.html)：SFTP、known-hosts、host key 和非阻塞 SSH 能力。

以上外部能力均应以项目锁定的 HarmonyOS SDK 和第三方库版本在真机上的实际结果为准。

## 16. MVP 实施台账

### 16.1 变更登记

| 字段 | 内容 |
| --- | --- |
| Change ID | FT-MVP-001 |
| 授权范围 | 在全新目录创建可运行的 HarmonyOS 文件传输 MVP，不修改现有 RDP 产品逻辑 |
| 设计状态 | DesignReady |
| 实施状态 | NotStarted |
| 目标目录 | `harmony/filetransfer_app/` |
| MVP 协议 | 以本机 SDK/第三方库探测结果为准；目标为 FTP、显式 FTPS、SFTP |
| 核心能力 | 站点配置、远端浏览、上传、下载、批量队列、暂停/继续/取消、基础断点、错误与进度展示 |
| 安全能力 | 严格 FTPS 校验、SFTP host key 策略入口、明文 FTP 风险提示、日志脱敏、凭据安全存储适配 |
| 性能能力 | Native 异步工作线程、有界队列、有界进度事件、64 位大小/偏移、批量任务不阻塞 UI |

### 16.2 文件级实施清单

计划创建：

- `harmony/filetransfer_app/AppScope/`：独立包名、应用资源和全局配置。
- `harmony/filetransfer_app/entry/`：UIAbility、权限、后台数据传输声明和启动入口。
- `harmony/filetransfer_app/transfer_common/src/main/ets/model/`：站点、远端条目、任务和状态 DTO。
- `harmony/filetransfer_app/transfer_common/src/main/ets/service/`：Native 网关、配置/凭据、队列协调和后台任务。
- `harmony/filetransfer_app/transfer_common/src/main/ets/pages/`：站点、双栏浏览和传输队列 MVP 页面。
- `harmony/filetransfer_app/transfer_common/src/main/cpp/napi/`：仅含注册、参数转换和事件转发。
- `harmony/filetransfer_app/transfer_common/src/main/cpp/engine/`：队列状态机、调度、任务线程和检查点。
- `harmony/filetransfer_app/transfer_common/src/main/cpp/protocol/`：协议接口和 curl adapter。
- `harmony/filetransfer_app/transfer_common/src/main/cpp/tests/`：状态机、路径/协议策略和调度单测。
- `harmony/filetransfer_app/scripts/`：安全的构建和 host 侧测试入口。
- `harmony/filetransfer_app/README.md`：能力、构建、运行、测试、限制和安全说明。

不修改：

- `harmony/app/` 现有 RDP App Pack。
- `harmony/third_party/FreeRDP/` 与 `harmony/third_party/xrdp/`。
- 现有 bundle `com.muhub.desktop` 的权限、签名和发布配置。

### 16.3 MVP 验收 ID

| ID | 验收条件 |
| --- | --- |
| FT-A01 | 新工程使用独立 bundle/module，能够完成 Hvigor 配置解析和 HAP 构建（环境具备 SDK 时） |
| FT-A02 | 可以新增站点并对 FTP 明文配置显示风险提示；密码不会进入日志或普通配置对象 |
| FT-A03 | Native 能力探测明确返回实际支持的协议，不把未编译协议报告为可用 |
| FT-A04 | 支持连接、列举远端目录，并能将错误映射到稳定错误域 |
| FT-A05 | 支持单/多文件上传与下载任务，展示字节进度、速度和最终状态 |
| FT-A06 | 队列支持暂停、继续、取消和失败重试，状态迁移由单一状态机约束 |
| FT-A07 | 下载使用临时文件；续传前校验已存在大小，完成后再提交最终文件 |
| FT-A08 | UI 和 N-API 不执行协议循环或大文件全量缓冲；Native 使用 64 位 offset |
| FT-A09 | FTPS 默认启用证书/主机名校验；SFTP 提供 host key 严格校验配置入口 |
| FT-A10 | C++ host 单测覆盖合法/非法状态迁移、协议白名单、CR/LF 路径拒绝和取消行为 |
| FT-A11 | 构建/测试结果、未验证能力和环境限制回写本台账，不把未真机验证能力写成完成 |

### 16.4 实施结果与证据

等待代码与测试完成后回写。只有相应验收真实通过，实施状态才可依次更新为 `Implemented`、`Verified`。
