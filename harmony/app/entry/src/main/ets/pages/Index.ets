import { common } from '@kit.AbilityKit'
import { hilog } from '@kit.PerformanceAnalysisKit'
import {
  HILOG_DOMAIN,
  HILOG_TAG,
  INPUT_EVENT_LOG_INTERVAL_MS,
  POINTER_FAILURE_LOG_INTERVAL_MS,
  REMOTE_INPUT_BRIDGE_ID,
  REMOTE_SURFACE_FOCUS_ID
} from '../rdp/RdpConstants'
import { PointerMapper, PointerViewport } from '../rdp/PointerMapper'
import { RemotePointerInput } from '../rdp/RemotePointerInput'
import { RdpPermissionCallbackDelegate, RdpPermissionManager } from '../rdp/RdpPermissions'
import { ConnectionState, NativeCommandResult, NativeProbeResult, PointerMapping } from '../rdp/RdpTypes'
import rdpNative from 'libentry.so'

@Entry
@Component
struct Index {
  @State private host: string = '192.168.43.28'
  @State private port: string = '3389'
  @State private username: string = 'DESKTOP-UH7T7O1\\aoqiduan'
  @State private password: string = 'aaaa'
  @State private resolution: string = 'auto'
  @State private certPolicy: string = 'ignore'
  @State private graphicsMode: string = 'rdpgfx-h264'
  @State private currentTab: string = 'connect'
  @State private connectionState: ConnectionState = 'Idle'
  @State private logText: string = 'Ready'
  @State private surfacePixelWidth: number = 0
  @State private surfacePixelHeight: number = 0
  @State private surfaceLayoutWidth: number = 0
  @State private surfaceLayoutHeight: number = 0
  @State private surfaceViewportX: number = 0
  @State private surfaceViewportY: number = 0
  @State private surfaceViewportWidth: number = 0
  @State private surfaceViewportHeight: number = 0
  @State private desktopPixelWidth: number = 0
  @State private desktopPixelHeight: number = 0
  @State private remoteTextInput: string = ''
  private callbacksRegistered: boolean = false
  private lastPointerFailureLogAtMs: number = 0
  private lastInputEventLogAtMs: number = 0
  private surfacePixelScaleX: number = 0
  private surfacePixelScaleY: number = 0
  private surfaceController: XComponentController = new XComponentController()
  private remoteInputController: TextInputController = new TextInputController()
  private pointerInput: RemotePointerInput | null = null
  private permissionManager: RdpPermissionManager | null = null

  private deferUiUpdate(task: () => void): void {
    setTimeout((): void => {
      task()
    }, 0)
  }

  private appendLog(line: string): void {
    this.logText = `${this.logText}\n${line}`
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
    this.getPointerInput().releaseButtons()
    this.releaseActiveModifiers()
  }

  private getUiAbilityContext(): common.UIAbilityContext | null {
    try {
      return getContext(this) as common.UIAbilityContext
    } catch (error) {
      this.appendLog(`UIAbilityContext unavailable: ${JSON.stringify(error)}`)
      return null
    }
  }

  private getPointerInput(): RemotePointerInput {
    if (this.pointerInput === null) {
      this.pointerInput = new RemotePointerInput(
        (): boolean => this.isConnected(),
        (): void => this.focusRemoteInputBridge(),
        (): PointerViewport => this.getPointerViewport(),
        (flags: number, x: number, y: number, allowClamp?: boolean, traceLabel?: string): boolean =>
          this.sendPointer(flags, x, y, !!allowClamp, traceLabel ?? ''),
        (line: string): void => this.appendLog(line),
        (label: string, detail: string, force?: boolean): void =>
          this.logInputEvent(label, detail, !!force),
        (label: string, flags: number, x: number, y: number, mapped: PointerMapping): void =>
          this.logPointerDiagnostic(label, flags, x, y, mapped)
      )
    }
    return this.pointerInput
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
          const nextState = this.toConnectionState(state)
          const wasConnected = this.isConnected()
          this.connectionState = nextState
          if (nextState === 'Connected' && !wasConnected) {
            this.currentTab = 'session'
            this.refreshNativeState()
            this.focusRemoteInputBridge()
          } else if (nextState !== 'Connected') {
            this.getPointerInput().clearState()
            this.clearRemoteInputBridge()
          }
        })
      })
      rdpNative.onLog((line: string) => {
        this.deferUiUpdate(() => {
          this.appendLog(line)
        })
      })
      rdpNative.onError((message: string) => {
        this.deferUiUpdate(() => {
          this.connectionState = 'Failed'
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

  private refreshNativeState(appendLogs: boolean = false): void {
    try {
      const result: NativeProbeResult = rdpNative.probe()
      this.surfacePixelWidth = result.surfaceWidth
      this.surfacePixelHeight = result.surfaceHeight
      this.surfaceViewportX = result.surfaceViewportX
      this.surfaceViewportY = result.surfaceViewportY
      this.surfaceViewportWidth = result.surfaceViewportWidth
      this.surfaceViewportHeight = result.surfaceViewportHeight
      this.desktopPixelWidth = result.desktopWidth
      this.desktopPixelHeight = result.desktopHeight
      if (appendLogs) {
        this.appendNativeLogs(result.logs)
      }
    } catch (error) {
      this.appendLog(`Native state refresh failed: ${JSON.stringify(error)}`)
    }
  }

  private normalizeCertPolicy(value: string): string {
    const normalized = value.trim().toLowerCase()
    if (normalized === 'strict' || normalized === 'verify' || normalized === 'valid-ca' ||
      normalized === 'deny' || normalized === 'reject') {
      return 'strict'
    }
    if (normalized === 'ignore' || normalized === 'accept' || normalized === 'insecure') {
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
      this.connectionState = 'Idle'
      this.appendLog('Missing host, username, or password')
      return
    }

    try {
      const policy = this.normalizeCertPolicy(this.certPolicy)
      this.certPolicy = policy
      this.connectionState = 'Resolving'
      this.currentTab = 'session'
      this.appendLog(`Calling native connect for ${this.host}:${this.port}`)
      const result: NativeCommandResult = rdpNative.connect({
        host: this.host,
        port: this.port,
        username: this.username,
        password: this.password,
        resolution: this.resolution,
        certPolicy: policy,
        graphicsMode: this.graphicsMode,
        appFilesDir: this.getAppFilesDir()
      })
      this.connectionState = this.toConnectionState(result.state)
      this.appendLog(result.message)
      this.appendNativeLogs(result.logs)
      if (result.ok) {
        this.currentTab = 'session'
      }
    } catch (error) {
      this.connectionState = 'Failed'
      this.appendLog(`Native connect failed: ${JSON.stringify(error)}`)
    }
  }

  private isConnected(): boolean {
    return this.connectionState === 'Connected'
  }

  private formatNumber(value: number): string {
    return `${Math.round(value * 10) / 10}`
  }

  private formatPointerFlags(flags: number): string {
    return `0x${flags.toString(16).toUpperCase()}`
  }

  private updateSurfaceLayout(width: Length, height: Length): void {
    const nextWidth = PointerMapper.areaLengthToNumber(width)
    const nextHeight = PointerMapper.areaLengthToNumber(height)
    if (nextWidth <= 0 || nextHeight <= 0) {
      return
    }

    let pixelScaleX = this.surfacePixelScaleX
    let pixelScaleY = this.surfacePixelScaleY
    let nativeWidth = 0
    let nativeHeight = 0
    try {
      const probe: NativeProbeResult = rdpNative.probe()
      if (probe.surfaceReady && probe.surfaceWidth > 0 && probe.surfaceHeight > 0) {
        nativeWidth = probe.surfaceWidth
        nativeHeight = probe.surfaceHeight
        pixelScaleX = probe.surfaceWidth / nextWidth
        pixelScaleY = probe.surfaceHeight / nextHeight
      }
    } catch (error) {
      this.appendLog(`Native surface scale probe failed: ${JSON.stringify(error)}`)
    }

    if (nativeWidth <= 0 || nativeHeight <= 0) {
      if (pixelScaleX <= 0 && this.surfaceLayoutWidth > 0 && this.surfacePixelWidth > 0) {
        pixelScaleX = this.surfacePixelWidth / this.surfaceLayoutWidth
      }
      if (pixelScaleY <= 0 && this.surfaceLayoutHeight > 0 && this.surfacePixelHeight > 0) {
        pixelScaleY = this.surfacePixelHeight / this.surfaceLayoutHeight
      }
      nativeWidth = Math.round(nextWidth * (pixelScaleX > 0 ? pixelScaleX : 1))
      nativeHeight = Math.round(nextHeight * (pixelScaleY > 0 ? pixelScaleY : 1))
    }

    if (this.surfaceLayoutWidth === nextWidth && this.surfaceLayoutHeight === nextHeight &&
      this.surfacePixelWidth === nativeWidth && this.surfacePixelHeight === nativeHeight) {
      return
    }

    this.surfacePixelScaleX = pixelScaleX > 0 ? pixelScaleX : this.surfacePixelScaleX
    this.surfacePixelScaleY = pixelScaleY > 0 ? pixelScaleY : this.surfacePixelScaleY
    this.surfaceLayoutWidth = nextWidth
    this.surfaceLayoutHeight = nextHeight
    this.surfacePixelWidth = nativeWidth
    this.surfacePixelHeight = nativeHeight
    try {
      rdpNative.notifySurfaceLayout({
        width: nativeWidth,
        height: nativeHeight
      })
    } catch (error) {
      this.appendLog(`Native surface layout notify failed: ${JSON.stringify(error)}`)
    }
  }

  private getPointerViewport(): PointerViewport {
    return {
      resolution: this.resolution,
      surfacePixelWidth: this.surfacePixelWidth,
      surfacePixelHeight: this.surfacePixelHeight,
      surfaceLayoutWidth: this.surfaceLayoutWidth,
      surfaceLayoutHeight: this.surfaceLayoutHeight,
      surfaceViewportX: this.surfaceViewportX,
      surfaceViewportY: this.surfaceViewportY,
      surfaceViewportWidth: this.surfaceViewportWidth,
      surfaceViewportHeight: this.surfaceViewportHeight,
      desktopPixelWidth: this.desktopPixelWidth,
      desktopPixelHeight: this.desktopPixelHeight
    }
  }

  private pointerMappingFailureShouldLog(traceLabel: string): boolean {
    if (traceLabel.length > 0) {
      return true
    }

    const now = Date.now()
    if (now - this.lastPointerFailureLogAtMs >= POINTER_FAILURE_LOG_INTERVAL_MS) {
      this.lastPointerFailureLogAtMs = now
      return true
    }
    return false
  }

  private describePointerMapping(flags: number, eventX: number, eventY: number, mapped: PointerMapping): string {
    const inputText = `event=${this.formatNumber(eventX)},${this.formatNumber(eventY)}`
    const surfaceText = `surface=${this.formatNumber(mapped.surfaceX)},${this.formatNumber(mapped.surfaceY)}`
    const scaleText = `scale=${this.formatNumber(mapped.scaleX)}x${this.formatNumber(mapped.scaleY)}`
    const viewportText = `viewport=${mapped.viewportX},${mapped.viewportY} ${mapped.viewportWidth}x${mapped.viewportHeight}`
    const desktopText = `desktop=${mapped.desktopWidth}x${mapped.desktopHeight}`
    const remoteText = mapped.ok ? ` remote=${mapped.x},${mapped.y}` : ''
    return `flags=${this.formatPointerFlags(flags)} ${inputText} ${surfaceText} ${scaleText} ${viewportText} ${desktopText}${remoteText}`
  }

  private logPointerDiagnostic(label: string, flags: number, x: number, y: number, mapped: PointerMapping): void {
    const status = mapped.ok ? 'mapped' : `blocked: ${mapped.reason}`
    this.appendLog(`Pointer ${label} ${status}: ${this.describePointerMapping(flags, x, y, mapped)}`)
  }

  private logInputEvent(label: string, detail: string, force: boolean = false): void {
    const now = Date.now()
    if (!force && now - this.lastInputEventLogAtMs < INPUT_EVENT_LOG_INTERVAL_MS) {
      return
    }
    this.lastInputEventLogAtMs = now
    const line = `Input ${label}: ${detail}`
    const shouldLogHilog = force ||
      label === 'paste' ||
      label === 'copy' ||
      label === 'cut' ||
      label === 'key.delete' ||
      label === 'key.repeat'
    if (shouldLogHilog) {
      hilog.info(HILOG_DOMAIN, HILOG_TAG, '%{public}s', line)
      console.info(line)
    }
  }

  private sendPointer(flags: number, x: number, y: number, allowClamp: boolean = false,
    traceLabel: string = ''): boolean {
    if (!this.isConnected()) {
      if (this.pointerMappingFailureShouldLog(traceLabel)) {
        this.appendLog(`Pointer ${traceLabel || 'input'} skipped: no active FreeRDP session`)
      }
      return false
    }

    try {
      const mapped = PointerMapper.mapToDesktop(this.getPointerViewport(), x, y, allowClamp)
      if (!mapped.ok) {
        if (this.pointerMappingFailureShouldLog(traceLabel)) {
          this.logPointerDiagnostic(traceLabel || 'input', flags, x, y, mapped)
        }
        return false
      }
      const result: NativeCommandResult = rdpNative.sendPointer({
        flags: flags,
        x: mapped.x,
        y: mapped.y
      })
      if (!result.ok) {
        this.appendLog(`Pointer ${traceLabel || 'input'} failed: ${result.message}; ${this.describePointerMapping(flags, x, y, mapped)}`)
        return false
      }
      return result.ok
    } catch (error) {
      this.appendLog(`Native pointer input failed (${traceLabel || 'input'}): ${JSON.stringify(error)}`)
      return false
    }
  }

  private focusRemoteInputBridge(): void {
    if (!this.isConnected()) {
      return
    }
    this.deferUiUpdate(() => {
      const focused = focusControl.requestFocus(REMOTE_INPUT_BRIDGE_ID)
      if (!focused) {
        this.appendLog('Remote input bridge focus failed')
      }
    })
  }

  private handleInputBridgeWillInsert(value: InsertValue): boolean {
    const insertedText = value.insertValue
    if (!insertedText.length) {
      return false
    }
    this.sendRemoteTextFragment(insertedText)
    this.remoteTextInput = ''
    return false
  }

  private sendRemoteTextFragment(value: string): boolean {
    if (!value.length) {
      return false
    }
    if (!this.isConnected()) {
      this.appendLog('Text input skipped: no active FreeRDP session')
      return false
    }

    try {
      const result: NativeCommandResult = rdpNative.sendText({ text: value })
      if (!result.ok) {
        this.appendLog(result.message)
      }
      return result.ok
    } catch (error) {
      this.appendLog(`Native committed text input failed: ${JSON.stringify(error)}`)
      return false
    }
  }

  private clearRemoteInputBridge(): void {
    this.remoteTextInput = ''
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

  private shouldRouteRemoteKeyboard(): boolean {
    return this.currentTab === 'session'
  }

  private stopKeyPropagation(event: KeyEvent): void {
    try {
      event.stopPropagation()
    } catch (error) {
      this.logInputEvent('key.stop', `failed ${JSON.stringify(error)}`, true)
    }
  }

  private isModifierPressed(event: KeyEvent, modifier: string): boolean {
    try {
      return !!event.getModifierKeyState && event.getModifierKeyState([modifier])
    } catch (error) {
      return false
    }
  }

  private forwardRemoteKeyEvent(event: KeyEvent): boolean {
    if (!this.shouldRouteRemoteKeyboard() || !this.isConnected()) {
      return false
    }
    if (event.type !== KeyType.Down && event.type !== KeyType.Up) {
      return false
    }

    const down = event.type === KeyType.Down
    try {
      const result: NativeCommandResult = rdpNative.sendPlatformKey({
        keyCode: event.keyCode,
        down: down,
        repeat: false,
        ctrl: this.isModifierPressed(event, 'ctrl'),
        shift: this.isModifierPressed(event, 'shift'),
        alt: this.isModifierPressed(event, 'alt'),
        meta: this.isModifierPressed(event, 'meta') || this.isModifierPressed(event, 'win')
      })
      this.logInputEvent('key.forward',
        `code=${event.keyCode} type=${event.type} down=${down} ok=${result.ok}`)
      if (!result.ok) {
        this.appendLog(result.message)
      }
      this.stopKeyPropagation(event)
      return true
    } catch (error) {
      this.appendLog(`Native platform key input failed: ${JSON.stringify(error)}`)
      return false
    }
  }

  private toConnectionState(value: string): ConnectionState {
    if (value === 'Resolving') {
      return 'Resolving'
    }
    if (value === 'TCP connected') {
      return 'TCP connected'
    }
    if (value === 'Negotiating') {
      return 'Negotiating'
    }
    if (value === 'Authenticating') {
      return 'Authenticating'
    }
    if (value === 'Bridge ready') {
      return 'Bridge ready'
    }
    if (value === 'Disconnected') {
      return 'Disconnected'
    }
    if (value === 'Failed') {
      return 'Failed'
    }
    if (value === 'Connected') {
      return 'Connected'
    }
    return 'Idle'
  }

  @Builder
  private buildTabs() {
    Row() {
      this.tabButton('connect', 'Connect')
      this.tabButton('session', 'Session')
    }
    .height(48)
    .width('100%')
    .backgroundColor('#FFFFFF')
    .padding({ left: 16, right: 16 })
  }

  @Builder
  private tabButton(id: string, label: string) {
    Button(label)
      .height(36)
      .layoutWeight(1)
      .margin({ right: 8 })
      .backgroundColor(this.currentTab === id ? '#0A84FF' : '#E8EEF7')
      .fontColor(this.currentTab === id ? Color.White : '#1D2433')
      .onClick(() => {
        const leavingSession = this.currentTab === 'session' && id !== 'session'
        this.currentTab = id
        if (leavingSession) {
          this.releaseActiveModifiers()
        }
      })
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
      Stack() {
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
              this.refreshNativeState()
              this.focusRemoteInputBridge()
            })
          })
          .onDestroy(() => {
            this.deferUiUpdate(() => {
              this.appendLog('XComponent destroyed')
            })
          })
          .onAreaChange((oldArea: Area, newArea: Area) => {
            this.updateSurfaceLayout(newArea.width, newArea.height)
            this.deferUiUpdate(() => {
              this.refreshNativeState()
            })
          })
          .onAxisEvent((event: AxisEvent) => {
            this.getPointerInput().handleAxis(event)
          })

        TextInput({ text: this.remoteTextInput, controller: this.remoteInputController })
          .id(REMOTE_INPUT_BRIDGE_ID)
          .width(1)
          .height(1)
          .position({ x: 0, y: 0 })
          .opacity(0.01)
          .fontSize(1)
          .fontColor('#00000000')
          .backgroundColor('#00000000')
          .caretColor('#00000000')
          .focusable(true)
          .copyOption(CopyOptions.None)
          .onWillInsert((value: InsertValue): boolean => {
            return this.handleInputBridgeWillInsert(value)
          })
          .onKeyEventDispatch((event: KeyEvent): boolean => {
            return this.forwardRemoteKeyEvent(event)
          })
          .onAxisEvent((event: AxisEvent) => {
            this.getPointerInput().handleAxis(event)
          })

        if (!this.isConnected()) {
          Text('Remote desktop surface')
            .fontSize(20)
            .fontColor('#667085')
        }
      }
      .id(REMOTE_SURFACE_FOCUS_ID)
      .layoutWeight(1)
      .width('100%')
      .backgroundColor('#101828')
      .focusable(true)
      .focusOnTouch(true)
      .onTouch((event: TouchEvent) => {
        this.getPointerInput().handleTouch(event)
      })
      .onKeyEventDispatch((event: KeyEvent): boolean => {
        return this.forwardRemoteKeyEvent(event)
      })
      .onMouse((event: MouseEvent) => {
        this.getPointerInput().handleMouse(event)
      })
      .onAxisEvent((event: AxisEvent) => {
        this.getPointerInput().handleAxis(event)
      })
    }
    .layoutWeight(1)
    .focusable(true)
    .onKeyEventDispatch((event: KeyEvent): boolean => {
      return this.forwardRemoteKeyEvent(event)
    })
  }

  build() {
    Column() {
      this.buildTabs()
      if (this.currentTab === 'connect') {
        this.buildConnectPage()
      } else {
        this.buildSessionPage()
      }
    }
    .width('100%')
    .height('100%')
    .backgroundColor('#F2F4F7')
  }
}
