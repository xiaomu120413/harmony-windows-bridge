import { common } from '@kit.AbilityKit'
import { hilog } from '@kit.PerformanceAnalysisKit'
import {
  HILOG_DOMAIN,
  HILOG_TAG,
  REMOTE_SURFACE_FOCUS_ID
} from '../rdp/RdpConstants'
import { RdpPermissionCallbackDelegate, RdpPermissionManager } from '../rdp/RdpPermissions'
import rdpNative, { NativeCommandResult, NativeXrdpServerParams, NativeXrdpServerResult } from 'libentry.so'

@Entry
@Component
struct Index {
  @State private host: string = '172.20.10.3'
  @State private port: string = '3389'
  @State private username: string = 'DESKTOP-UH7T7O1\\aoqiduan'
  @State private password: string = 'aaaa'
  @State private certPolicy: string = 'ignore'
  @State private showSession: boolean = false
  private connected: boolean = false
  private callbacksRegistered: boolean = false
  private surfaceController: XComponentController = new XComponentController()
  private permissionManager: RdpPermissionManager | null = null

  private deferUiUpdate(task: () => void): void {
    setTimeout((): void => {
      task()
    }, 0)
  }

  private appendLog(line: string): void {
    hilog.info(HILOG_DOMAIN, HILOG_TAG, '%{public}s', line)
  }

  private appendNativeLogs(lines: string[]): void {
    for (let i = 0; i < lines.length; i++) {
      this.appendLog(lines[i])
    }
  }

  aboutToAppear(): void {
    this.registerNativeCallbacks()
    this.getPermissionManager().ensurePasteboardReadPermission('page appear')
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
      rdpNative.onLog((_line: string) => {
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
          const requestId = Number(requestIdText)
          this.appendLog(`Remote audio capture requested microphone permission: requestId=${requestIdText}`)
          this.getPermissionManager().ensureMicrophonePermission('remote audio capture')
            .then((granted: boolean) => {
              const result: NativeCommandResult = rdpNative.completeMicrophonePermissionRequest({
                requestId: isNaN(requestId) ? 0 : requestId,
                granted: granted
              })
              this.appendLog(`${result.message}: granted=${granted}`)
              this.appendNativeLogs(result.logs)
            })
        })
      })
      this.callbacksRegistered = true
    } catch (error) {
      this.appendLog(`Native callback registration failed: ${JSON.stringify(error)}`)
    }
  }

  private normalizeCertPolicy(value: string): string {
    const normalized = value.trim().toLowerCase()
    if (normalized === 'strict') {
      return 'strict'
    }
    if (normalized === 'ignore') {
      return 'ignore'
    }
    return 'tofu'
  }

  private getAppFilesDir(): string {
    const context = this.getUiAbilityContext()
    if (context === null) {
      return ''
    }
    return context.filesDir
  }

  private connectNative(): void {
    if (!this.host.trim() || !this.username.trim() || !this.password.trim()) {
      this.connected = false
      this.appendLog('Missing host, username, or password')
      return
    }

    const policy = this.normalizeCertPolicy(this.certPolicy)
    const host = this.host
    const port = this.port
    const username = this.username
    const password = this.password
    const appFilesDir = this.getAppFilesDir()
    this.certPolicy = policy
    this.connected = false
    this.setSessionVisible(true)
    this.appendLog(`Opening session surface for ${host}:${port}`)
    setTimeout((): void => {
      this.startNativeConnect(host, port, username, password, policy, appFilesDir)
    }, 0)
  }

  private startNativeConnect(host: string, port: string, username: string, password: string,
    policy: string, appFilesDir: string): void {
    try {
      this.appendLog(`Calling native connect for ${host}:${port}`)
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

  private xrdpParams(): NativeXrdpServerParams {
    const appFilesDir = this.getAppFilesDir()
    return {
      appFilesDir: appFilesDir,
      port: 3390
    }
  }

  private probeXrdpNative(): void {
    try {
      const result: NativeXrdpServerResult = rdpNative.probeXrdpServer(this.xrdpParams())
      this.appendLog(result.message)
      this.appendNativeLogs(result.logs)
    } catch (error) {
      this.appendLog(`xrdp probe failed: ${JSON.stringify(error)}`)
    }
  }

  private startXrdpNative(): void {
    try {
      const result: NativeXrdpServerResult = rdpNative.startXrdpServer(this.xrdpParams())
      this.appendLog(result.message)
      this.appendNativeLogs(result.logs)
    } catch (error) {
      this.appendLog(`xrdp start failed: ${JSON.stringify(error)}`)
    }
  }

  private stopXrdpNative(): void {
    try {
      const result: NativeXrdpServerResult = rdpNative.stopXrdpServer()
      this.appendLog(result.message)
      this.appendNativeLogs(result.logs)
    } catch (error) {
      this.appendLog(`xrdp stop failed: ${JSON.stringify(error)}`)
    }
  }

  private pushXrdpTestFrame(): void {
    try {
      const params = this.xrdpParams()
      const result: NativeXrdpServerResult = rdpNative.pushXrdpTestFrame({
        appFilesDir: params.appFilesDir,
        port: params.port,
        width: 960,
        height: 540
      })
      this.appendLog(result.message)
      this.appendNativeLogs(result.logs)
    } catch (error) {
      this.appendLog(`xrdp frame push failed: ${JSON.stringify(error)}`)
    }
  }

  @Builder
  private buildConnectPage() {
    Scroll() {
      Column() {
        Text('FreeRDP HarmonyOS')
          .fontSize(24)
          .fontWeight(FontWeight.Bold)
          .fontColor('#101828')
          .width('100%')
          .margin({ bottom: 18 })

        this.inputField('Windows host', this.host, (value: string) => {
          this.host = value
        })
        this.inputField('Port', this.port, (value: string) => {
          this.port = value
        })
        this.inputField('Username', this.username, (value: string) => {
          this.username = value
        })
        TextInput({ placeholder: 'Password', text: this.password })
          .type(InputType.Password)
          .height(44)
          .width('100%')
          .margin({ bottom: 12 })
          .onChange((value: string) => {
            this.password = value
          })
        Row() {
          this.certPolicyButton('tofu', 'TOFU')
          this.certPolicyButton('strict', 'Strict')
          this.certPolicyButton('ignore', 'Ignore')
        }
        .height(40)
        .width('100%')
        .margin({ bottom: 12 })

        Button('Connect')
          .height(44)
          .width('100%')
          .backgroundColor('#0A84FF')
          .fontColor(Color.White)
          .onClick(() => {
            this.connectNative()
          })

        Row() {
          Button('Probe xrdp')
            .height(40)
            .layoutWeight(1)
            .fontSize(13)
            .backgroundColor('#E8EEF7')
            .fontColor('#1D2433')
            .onClick(() => {
              this.probeXrdpNative()
            })
          Button('Start xrdp')
            .height(40)
            .layoutWeight(1)
            .fontSize(13)
            .backgroundColor('#0A84FF')
            .fontColor(Color.White)
            .onClick(() => {
              this.startXrdpNative()
            })
          Button('Stop xrdp')
            .height(40)
            .layoutWeight(1)
            .fontSize(13)
            .backgroundColor('#E8EEF7')
            .fontColor('#1D2433')
            .onClick(() => {
              this.stopXrdpNative()
            })
        }
        .height(42)
        .width('100%')
        .margin({ top: 12 })

        Button('Push xrdp frame')
          .height(40)
          .width('100%')
          .fontSize(13)
          .backgroundColor('#E8EEF7')
          .fontColor('#1D2433')
          .margin({ top: 8 })
          .onClick(() => {
            this.pushXrdpTestFrame()
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
      .onChange(onChange)
  }

  @Builder
  private certPolicyButton(value: string, label: string) {
    Button(label)
      .height(36)
      .layoutWeight(1)
      .fontSize(13)
      .backgroundColor(this.certPolicy === value ? '#0A84FF' : '#E8EEF7')
      .fontColor(this.certPolicy === value ? Color.White : '#1D2433')
      .onClick(() => {
        this.certPolicy = value
      })
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
      } else {
        this.buildConnectPage()
      }
    }
    .width('100%')
    .height('100%')
    .backgroundColor('#F2F4F7')
  }
}
