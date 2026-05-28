import { common } from '@kit.AbilityKit'
import { hilog } from '@kit.PerformanceAnalysisKit'
import {
  HILOG_DOMAIN,
  HILOG_TAG,
  REMOTE_SURFACE_FOCUS_ID
} from '../rdp/RdpConstants'
import { RdpPermissionCallbackDelegate, RdpPermissionManager } from '../rdp/RdpPermissions'
import { RemoteFilesDirectory } from '../rdp/RemoteFilesDirectory'
import {
  XrdpServerController,
  XrdpServerDisplayMessages,
  XrdpServerStartOptions,
  XrdpServerStatus
} from '../rdp/XrdpServerController'
import { SettingsPage } from '../components/SettingsPage'
import { SettingsText } from '../components/settings/SettingsConstants'
import { SettingsAccent, SettingsAppearanceMode, SettingsResources, SettingsTheme } from '../components/settings/SettingsUi'
import rdpNative, {
  NativeCommandResult
} from 'libentry.so'

@Entry
@Component
struct Index {
  @State private host: string = ''
  @State private port: string = '3389'
  @State private username: string = ''
  @State private password: string = ''
  // 暂时固定使用 TOFU 证书策略，后续需要严格校验时再恢复策略选择入口。
  private certPolicy: string = 'tofu'
  @State private remoteAccessCode: string = '000000'
  @State private remoteAccessCodeGateEnabled: boolean = false
  @State private screenRecordingPermissionGranted: boolean = false
  @State private xrdpServerRunning: boolean = false
  @State private xrdpServerState: string = 'Stopped'
  @State private xrdpServerPort: number = 3390
  @State private xrdpServerMessage: string = ''
  @State private xrdpServerBusy: boolean = false
  @State private showSession: boolean = false
  @State private showSettings: boolean = false
  @State private settingsButtonHovered: boolean = false
  @State private settingsButtonPressed: boolean = false
  @StorageLink('settingsSystemDark') private systemDark: boolean = false
  @StorageLink('settingsAppearanceMode') private appearanceMode: string = 'system'
  private connected: boolean = false
  private callbacksRegistered: boolean = false
  private xrdpServerStartRequested: boolean = false
  private remoteAccessCodeInitialized: boolean = false
  private surfaceController: XComponentController = new XComponentController()
  private permissionManager: RdpPermissionManager | null = null
  private xrdpServerController: XrdpServerController | null = null
  private xrdpServerStartRequest: Promise<XrdpServerStatus> | null = null

  private deferUiUpdate(task: () => void): void {
    setTimeout((): void => {
      task()
    }, 0)
  }

  private appendLog(line: string): void {
    hilog.info(HILOG_DOMAIN, HILOG_TAG, '%{private}s', line)
  }

  private appendNativeLogs(lines: string[]): void {
    for (let i = 0; i < lines.length; i++) {
      this.appendLog(lines[i])
    }
  }

  aboutToAppear(): void {
    this.registerNativeCallbacks()
    this.refreshScreenRecordingPermissionState()
    this.refreshXrdpServerDiagnostics()
    this.ensureXrdpServerStarted('page appear')
  }

  onPageShow(): void {
    this.refreshScreenRecordingPermissionState()
    this.refreshXrdpServerDiagnostics()
  }

  aboutToDisappear(): void {
    this.releaseActiveModifiers()
  }

  private getUiAbilityContext(): common.UIAbilityContext | null {
    try {
      const hostContext = this.getUIContext().getHostContext()
      if (!hostContext) {
        return null
      }
      return hostContext as common.UIAbilityContext
    } catch (error) {
      this.appendLog(`UIAbilityContext unavailable: ${JSON.stringify(error)}`)
      return null
    }
  }

  private getPermissionManager(): RdpPermissionManager {
    if (this.permissionManager === null) {
      const delegate = new RdpPermissionCallbackDelegate(
        (): common.UIAbilityContext | null => this.getUiAbilityContext(),
        (line: string): void => this.appendLog(line)
      )
      this.permissionManager = new RdpPermissionManager(delegate)
    }
    return this.permissionManager
  }

  private getXrdpServerController(): XrdpServerController {
    if (this.xrdpServerController === null) {
      this.xrdpServerController = new XrdpServerController(
        this.xrdpServerDisplayMessages(),
        (line: string): void => this.appendLog(line),
        (lines: string[]): void => this.appendNativeLogs(lines)
      )
    }
    return this.xrdpServerController
  }

  private xrdpServerDisplayMessages(): XrdpServerDisplayMessages {
    return {
      active: SettingsText.REMOTE_SERVER_MESSAGE_ACTIVE,
      listening: SettingsText.REMOTE_SERVER_MESSAGE_LISTENING,
      stopped: SettingsText.REMOTE_SERVER_MESSAGE_STOPPED,
      exited: SettingsText.REMOTE_SERVER_MESSAGE_EXITED,
      failed: SettingsText.REMOTE_SERVER_MESSAGE_FAILED
    }
  }

  private registerNativeCallbacks(): void {
    if (this.callbacksRegistered) {
      return
    }

    try {
      rdpNative.onState((state: string) => {
        this.deferUiUpdate(() => {
          const wasConnected = this.isConnected()
          const nextConnected = state === 'Connected'
          if (!nextConnected && (state === 'Disconnected' || state === 'Failed' || state === 'Idle')) {
            this.setSessionVisible(false)
          }
          this.connected = nextConnected
          if (nextConnected && !wasConnected) {
            this.setSessionVisible(true)
            this.focusRemoteSurface()
          }
        })
      })
      rdpNative.onError((message: string) => {
        this.deferUiUpdate(() => {
          this.setSessionVisible(false)
          this.connected = false
          this.appendLog(`Native error: ${message}`)
        })
      })
      rdpNative.onMicrophonePermissionRequest((requestIdText: string) => {
        this.deferUiUpdate(() => {
          this.handleNativePermissionRequest(
            requestIdText,
            'Remote audio capture requested microphone permission',
            'remote audio capture',
            (trigger: string): Promise<boolean> => this.getPermissionManager().ensureMicrophonePermission(trigger),
            (requestId: number, granted: boolean): NativeCommandResult =>
              rdpNative.completeMicrophonePermissionRequest({ requestId: requestId, granted: granted })
          )
        })
      })
      rdpNative.onClipboardPermissionRequest((requestIdText: string) => {
        this.deferUiUpdate(() => {
          this.handleNativePermissionRequest(
            requestIdText,
            'Clipboard sync requested pasteboard permission',
            'clipboard read',
            (trigger: string): Promise<boolean> => this.getPermissionManager().ensurePasteboardReadPermission(trigger),
            (requestId: number, granted: boolean): NativeCommandResult =>
              rdpNative.completeClipboardPermissionRequest({ requestId: requestId, granted: granted })
          )
        })
      })
      rdpNative.onLocationPermissionRequest((requestIdText: string) => {
        this.deferUiUpdate(() => {
          this.handleNativePermissionRequest(
            requestIdText,
            'RDP location channel requested location permission',
            'remote location redirection',
            (trigger: string): Promise<boolean> => this.getPermissionManager().ensureLocationPermission(trigger),
            (requestId: number, granted: boolean): NativeCommandResult =>
              rdpNative.completeLocationPermissionRequest({ requestId: requestId, granted: granted })
          )
        })
      })
      this.callbacksRegistered = true
    } catch (error) {
      this.appendLog(`Native callback registration failed: ${JSON.stringify(error)}`)
    }
  }

  private getAppFilesDir(): string {
    const context = this.getUiAbilityContext()
    if (context === null) {
      return ''
    }
    return context.filesDir
  }

  private openRemoteFilesDirectoryFromSettings(): void {
    RemoteFilesDirectory.openSharedDirectory(
      this.getUiAbilityContext(),
      (line: string): void => this.appendLog(line)
    )
      .catch((error: Error) => {
        this.appendLog(`remote files directory open failed: ${JSON.stringify(error)}`)
      })
  }

  private handleNativePermissionRequest(requestIdText: string, label: string, trigger: string,
    ensurePermission: (trigger: string) => Promise<boolean>,
    completeRequest: (requestId: number, granted: boolean) => NativeCommandResult): void {
    const requestId = Number(requestIdText)
    const safeRequestId = isNaN(requestId) ? 0 : requestId
    this.appendLog(`${label}: requestId=${requestIdText}`)
    ensurePermission(trigger)
      .then((granted: boolean) => {
        const result: NativeCommandResult = completeRequest(safeRequestId, granted)
        this.appendLog(`${result.message}: granted=${granted}`)
        this.appendNativeLogs(result.logs)
      })
      .catch((error: Error) => {
        this.appendLog(`${label} failed: ${JSON.stringify(error)}`)
        const result: NativeCommandResult = completeRequest(safeRequestId, false)
        this.appendLog(`${result.message}: granted=false`)
        this.appendNativeLogs(result.logs)
      })
  }

  private generateRemoteAccessCode(): string {
    const value = Math.floor(100000 + Math.random() * 900000)
    return `${value}`
  }

  private ensureRemoteAccessCode(): string {
    if (!this.remoteAccessCodeInitialized || this.remoteAccessCode === '000000') {
      this.remoteAccessCode = this.generateRemoteAccessCode()
      this.remoteAccessCodeInitialized = true
    }
    return this.remoteAccessCode
  }

  private hasScreenRecordingPermission(): boolean {
    return this.getPermissionManager().hasScreenRecordingPermission()
  }

  private refreshScreenRecordingPermissionState(): boolean {
    this.screenRecordingPermissionGranted = this.hasScreenRecordingPermission()
    return this.screenRecordingPermissionGranted
  }

  private requestScreenRecordingPermissionFromSettings(): Promise<boolean> {
    return this.getPermissionManager().requestScreenRecordingPermission('remote control settings')
      .then((granted: boolean) => {
        const allowed = granted || this.refreshScreenRecordingPermissionState()
        this.screenRecordingPermissionGranted = allowed
        if (!allowed) {
          return false
        }
        return this.ensureXrdpServerStarted('screen recording permission granted')
          .then((_status: XrdpServerStatus) => {
            return this.refreshScreenRecordingPermissionState()
          })
      })
      .catch((error: Error) => {
        this.appendLog(`screen recording permission request from settings failed: ${JSON.stringify(error)}`)
        return this.refreshScreenRecordingPermissionState()
      })
  }

  private currentXrdpServerStatus(): XrdpServerStatus {
    return {
      running: this.xrdpServerRunning,
      state: this.xrdpServerState,
      port: this.xrdpServerPort,
      message: this.xrdpServerMessage
    }
  }

  private applyXrdpServerStatus(status: XrdpServerStatus): XrdpServerStatus {
    this.xrdpServerRunning = status.running
    this.xrdpServerState = status.state
    this.xrdpServerPort = status.port
    this.xrdpServerMessage = status.message
    return status
  }

  private refreshXrdpServerDiagnostics(): XrdpServerStatus {
    return this.applyXrdpServerStatus(this.getXrdpServerController().diagnostics())
  }

  private startXrdpServerFromSettings(): Promise<XrdpServerStatus> {
    return this.ensureXrdpServerStarted('remote control settings button', false, true)
  }

  private setRemoteAccessCodeGateFromSettings(enabled: boolean): string {
    this.setRemoteAccessCodeGateEnabled(enabled)
    return this.remoteAccessCode
  }

  private regenerateRemoteAccessCodeFromSettings(): string {
    this.remoteAccessCode = this.generateRemoteAccessCode()
    this.remoteAccessCodeInitialized = true
    if (this.remoteAccessCodeGateEnabled) {
      this.ensureXrdpServerStarted('access code regenerated', true)
    }
    return this.remoteAccessCode
  }

  private xrdpServerStartOptions(restartIfRunning: boolean = false): XrdpServerStartOptions {
    return {
      appFilesDir: this.getAppFilesDir(),
      accessCode: this.remoteAccessCodeGateEnabled ? this.ensureRemoteAccessCode() : '',
      accessCodeGateEnabled: this.remoteAccessCodeGateEnabled,
      restartIfRunning: restartIfRunning
    }
  }

  private ensureXrdpServerStarted(trigger: string, restartIfRunning: boolean = false,
    requestScreenRecordingImmediately: boolean = false): Promise<XrdpServerStatus> {
    if (this.xrdpServerStartRequest !== null && !restartIfRunning) {
      if (requestScreenRecordingImmediately) {
        this.getPermissionManager().requestScreenRecordingPermission(`xrdp ${trigger}`)
          .catch((error: Error) => {
            this.appendLog(`xrdp immediate permission request failed: ${JSON.stringify(error)}`)
          })
      }
      return this.xrdpServerStartRequest
    }
    if (this.xrdpServerStartRequested && !restartIfRunning) {
      return Promise.resolve(this.currentXrdpServerStatus())
    }
    this.xrdpServerStartRequested = true
    this.xrdpServerBusy = true
    const screenRecordingPermissionRequest = requestScreenRecordingImmediately ?
      this.getPermissionManager().requestScreenRecordingPermission(`xrdp ${trigger}`) :
      this.getPermissionManager().ensureScreenRecordingPermission(`xrdp ${trigger}`)
    this.xrdpServerStartRequest = screenRecordingPermissionRequest
      .then((granted: boolean) => {
        const allowed = granted || this.refreshScreenRecordingPermissionState()
        this.screenRecordingPermissionGranted = allowed
        if (!allowed) {
          this.appendLog(`xrdp start skipped: screen recording permission denied: ${trigger}`)
          return this.refreshXrdpServerDiagnostics()
        }
        return this.startXrdpServerAfterPermissions(trigger, restartIfRunning)
      })
      .catch((error: Error) => {
        this.appendLog(`xrdp permission check failed: ${JSON.stringify(error)}`)
        return this.refreshXrdpServerDiagnostics()
      })
      .finally(() => {
        this.xrdpServerStartRequested = false
        this.xrdpServerBusy = false
        this.xrdpServerStartRequest = null
      })
    return this.xrdpServerStartRequest
  }

  private startXrdpServerAfterPermissions(trigger: string, restartIfRunning: boolean): XrdpServerStatus {
    const status = this.getXrdpServerController().start(this.xrdpServerStartOptions(restartIfRunning), trigger)
    return this.applyXrdpServerStatus(status)
  }

  private setRemoteAccessCodeGateEnabled(enabled: boolean): void {
    if (this.remoteAccessCodeGateEnabled === enabled) {
      return
    }
    this.remoteAccessCodeGateEnabled = enabled
    if (enabled) {
      this.ensureRemoteAccessCode()
    }
    this.ensureXrdpServerStarted(enabled ? 'access code gate enabled' : 'access code gate disabled', true)
  }

  private connectNative(): void {
    if (!this.host.trim() || !this.username.trim() || !this.password.trim()) {
      this.connected = false
      this.appendLog('Missing host, username, or password')
      return
    }

    const policy = this.certPolicy
    const host = this.host
    const port = this.port
    const username = this.username
    const password = this.password
    const appFilesDir = this.getAppFilesDir()
    this.connected = false
    this.setSessionVisible(true)
    this.appendLog('Opening session surface')
    this.queueNativeConnect(host, port, username, password, policy, appFilesDir)
  }

  private queueNativeConnect(host: string, port: string, username: string, password: string, policy: string,
    appFilesDir: string): void {
    setTimeout((): void => {
      this.startNativeConnect(host, port, username, password, policy, appFilesDir)
    }, 0)
  }

  private startNativeConnect(host: string, port: string, username: string, password: string,
    policy: string, appFilesDir: string): void {
    try {
      this.appendLog('Calling native connect')
      const result: NativeCommandResult = rdpNative.connect({
        host: host,
        port: port,
        username: username,
        password: password,
        certPolicy: policy,
        appFilesDir: appFilesDir
      })
      this.appendLog(result.message)
      this.appendNativeLogs(result.logs)
      if (result.ok) {
        this.connected = result.state === 'Connected'
        this.setSessionVisible(true)
      } else {
        this.setSessionVisible(false)
        this.connected = false
      }
    } catch (error) {
      this.setSessionVisible(false)
      this.connected = false
      this.appendLog(`Native connect failed: ${JSON.stringify(error)}`)
    }
  }

  private setSessionVisible(visible: boolean): void {
    if (!visible && this.showSession && this.isConnected()) {
      this.releaseActiveModifiers()
    }
    this.showSession = visible
  }

  private isConnected(): boolean {
    return this.connected
  }

  private focusRemoteSurface(): void {
    if (!this.isConnected()) {
      return
    }
    this.deferUiUpdate(() => {
      const focused = focusControl.requestFocus(REMOTE_SURFACE_FOCUS_ID)
      if (!focused) {
        this.appendLog('Remote surface focus failed')
      }
    })
  }

  private releaseActiveModifiers(): void {
    if (!this.isConnected()) {
      return
    }

    try {
      const result: NativeCommandResult = rdpNative.releaseAllKeys()
      if (!result.ok) {
        this.appendLog(result.message)
      }
    } catch (error) {
      this.appendLog(`Native release all keys failed: ${JSON.stringify(error)}`)
    }
  }

  private setSettingsButtonHovered(hovered: boolean): void {
    SettingsTheme.animate(this.getUIContext(), () => {
      this.settingsButtonHovered = hovered
    })
  }

  private getAppearanceMode(): SettingsAppearanceMode {
    return SettingsTheme.normalizeAppearanceMode(this.appearanceMode)
  }

  private isAppDark(): boolean {
    const mode = this.getAppearanceMode()
    if (mode === 'dark') {
      return true
    }
    if (mode === 'light') {
      return false
    }
    return this.systemDark
  }

  @Builder
  private buildConnectPage() {
    Scroll() {
      Column() {
        Row() {
          Text(SettingsText.HOME_TITLE)
            .fontSize(20)
            .fontWeight(FontWeight.Bold)
            .fontColor(SettingsTheme.primaryText(this.isAppDark()))
            .lineHeight(26)
            .maxLines(2)
            .textOverflow({ overflow: TextOverflow.Ellipsis })
            .layoutWeight(1)

          Row({ space: 6 }) {
            Image(SettingsResources.SETTINGS_ICON)
              .width(18)
              .height(18)
              .fillColor(SettingsTheme.accentColor(this.isAppDark(), SettingsAccent.BLUE))
              .draggable(false)

            Text(SettingsText.SETTINGS_BUTTON)
              .fontSize(14)
              .fontWeight(FontWeight.Medium)
              .fontColor(SettingsTheme.primaryText(this.isAppDark()))
              .maxLines(1)
          }
            .width(86)
            .height(40)
            .alignItems(VerticalAlign.Center)
            .justifyContent(FlexAlign.Center)
            .borderRadius(SettingsTheme.CARD_RADIUS)
            .backgroundColor(SettingsTheme.subtleButton(this.isAppDark(), this.settingsButtonHovered,
              this.settingsButtonPressed))
            .border({
              width: 1,
              color: SettingsTheme.borderColor(this.isAppDark())
            })
            .margin({ left: 12 })
            .scale({
              x: this.settingsButtonPressed ? 0.98 : 1,
              y: this.settingsButtonPressed ? 0.98 : 1
            })
            .onHover((isHover: boolean) => {
              this.setSettingsButtonHovered(isHover)
            })
            .onTouch((event: TouchEvent) => {
              if (event.type === TouchType.Down) {
                this.settingsButtonPressed = true
                return
              }
              if (event.type === TouchType.Up || event.type === TouchType.Cancel) {
                this.settingsButtonPressed = false
              }
            })
            .onClick(() => {
              this.settingsButtonPressed = false
              this.refreshScreenRecordingPermissionState()
              this.refreshXrdpServerDiagnostics()
              this.showSettings = true
            })
        }
          .alignItems(VerticalAlign.Center)
          .width('100%')
          .margin({ bottom: 18 })

        this.inputField(SettingsText.HOME_HOST_PLACEHOLDER, this.host, (value: string) => {
          this.host = value
        })
        this.inputField(SettingsText.HOME_PORT_PLACEHOLDER, this.port, (value: string) => {
          this.port = value
        })
        this.inputField(SettingsText.HOME_USERNAME_PLACEHOLDER, this.username, (value: string) => {
          this.username = value
        })
        TextInput({ placeholder: SettingsText.HOME_PASSWORD_PLACEHOLDER, text: this.password })
          .type(InputType.Password)
          .height(44)
          .width('100%')
          .margin({ bottom: 12 })
          .fontSize(14)
          .fontColor(SettingsTheme.primaryText(this.isAppDark()))
          .placeholderColor(SettingsTheme.mutedText(this.isAppDark()))
          .backgroundColor(SettingsTheme.cardBackground(this.isAppDark()))
          .borderRadius(SettingsTheme.CARD_RADIUS)
          .border({
            width: 1,
            color: SettingsTheme.borderColor(this.isAppDark()),
            style: BorderStyle.Solid
          })
          .onChange((value: string) => {
            this.password = value
          })
        Button(SettingsText.HOME_CONNECT_BUTTON)
          .type(ButtonType.Normal)
          .height(44)
          .width('100%')
          .borderRadius(SettingsTheme.BUTTON_RADIUS)
          .backgroundColor('#0A84FF')
          .fontColor(Color.White)
          .stateEffect(false)
          .onClick(() => {
            this.connectNative()
          })
      }
      .padding(20)
    }
    .layoutWeight(1)
  }

  @Builder
  private inputField(label: string, value: string, onChange: (value: string) => void) {
    TextInput({ placeholder: label, text: value })
      .height(44)
      .width('100%')
      .margin({ bottom: 12 })
      .fontSize(14)
      .fontColor(SettingsTheme.primaryText(this.isAppDark()))
      .placeholderColor(SettingsTheme.mutedText(this.isAppDark()))
      .backgroundColor(SettingsTheme.cardBackground(this.isAppDark()))
      .borderRadius(SettingsTheme.CARD_RADIUS)
      .border({
        width: 1,
        color: SettingsTheme.borderColor(this.isAppDark()),
        style: BorderStyle.Solid
      })
      .onChange(onChange)
  }

  @Builder
  private buildSessionPage() {
    Column() {
        XComponent({
          id: 'rdpSurface',
          type: XComponentType.SURFACE,
          libraryname: 'entry',
          controller: this.surfaceController
        })
          .width('100%')
          .height('100%')
          .onLoad(() => {
            this.deferUiUpdate(() => {
              this.appendLog('XComponent loaded')
              this.focusRemoteSurface()
            })
          })
          .onDestroy(() => {
            this.deferUiUpdate(() => {
              this.appendLog('XComponent destroyed')
            })
          })
          .id(REMOTE_SURFACE_FOCUS_ID)
          .focusable(true)
          .focusOnTouch(true)
          .defaultFocus(true)
          .backgroundColor(Color.Black)
          .renderFit(RenderFit.CENTER)
    }
    .width('100%')
    .height('100%')
    .backgroundColor(Color.Black)
  }

  build() {
    Column() {
      if (this.showSession) {
        this.buildSessionPage()
      } else if (this.showSettings) {
        SettingsPage({
          remoteAccessCode: this.remoteAccessCode,
          remoteAccessCodeGateEnabled: this.remoteAccessCodeGateEnabled,
          screenRecordingPermissionGranted: this.screenRecordingPermissionGranted,
          xrdpServerRunning: this.xrdpServerRunning,
          xrdpServerState: this.xrdpServerState,
          xrdpServerPort: this.xrdpServerPort,
          xrdpServerMessage: this.xrdpServerMessage,
          xrdpServerBusy: this.xrdpServerBusy,
          onRemoteAccessCodeGateChange: (enabled: boolean): string => this.setRemoteAccessCodeGateFromSettings(enabled),
          onRemoteAccessCodeRegenerate: (): string => this.regenerateRemoteAccessCodeFromSettings(),
          onRequestScreenRecordingPermission: (): Promise<boolean> => this.requestScreenRecordingPermissionFromSettings(),
          onRefreshScreenRecordingPermission: (): boolean => this.refreshScreenRecordingPermissionState(),
          onRefreshXrdpServerStatus: (): XrdpServerStatus => this.refreshXrdpServerDiagnostics(),
          onStartXrdpServer: (): Promise<XrdpServerStatus> => this.startXrdpServerFromSettings(),
          onOpenRemoteFilesDirectory: (): void => this.openRemoteFilesDirectoryFromSettings(),
          onClose: () => {
            this.showSettings = false
          }
        })
      } else {
        this.buildConnectPage()
      }
    }
    .width('100%')
    .height('100%')
    .backgroundColor(SettingsTheme.pageBackground(this.isAppDark()))
  }
}
