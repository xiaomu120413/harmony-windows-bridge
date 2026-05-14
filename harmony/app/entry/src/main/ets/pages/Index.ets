import rdpNative from 'libentry.so'

type ConnectionState = 'Idle' | 'Resolving' | 'TCP connected' | 'Negotiating' | 'Authenticating' | 'Connected' | 'Bridge ready' | 'Disconnected' | 'Failed'

const PTR_FLAGS_MOVE: number = 0x0800
const PTR_FLAGS_WHEEL: number = 0x0200
const PTR_FLAGS_WHEEL_NEGATIVE: number = 0x0100
const PTR_FLAGS_DOWN: number = 0x8000
const PTR_FLAGS_BUTTON1: number = 0x1000
const RDP_WHEEL_DELTA: number = 0x0078
const RDP_WHEEL_DELTA_NEGATIVE: number = 0x0088
const TOUCH_WHEEL_THRESHOLD: number = 24

const RDP_SCANCODE_ESCAPE: number = 0x01
const RDP_SCANCODE_TAB: number = 0x0F
const RDP_SCANCODE_LCONTROL: number = 0x1D
const RDP_SCANCODE_LMENU: number = 0x38
const RDP_SCANCODE_LWIN: number = 0x015B

interface NativeProbeResult {
  bridgeVersion: string
  abi: string
  freeRdpVersion: string
  winprVersion: string
  opensslVersion: string
  freeRdpLinked: boolean
  surfaceRegistered: boolean
  surfaceReady: boolean
  surfaceId: string
  surfaceWidth: number
  surfaceHeight: number
  surfaceCreatedCount: number
  surfaceChangedCount: number
  surfaceDestroyedCount: number
  surfaceTouchCount: number
  surfacePaintCount: number
  surfaceLastPaintMessage: string
  probeJson: string
  probeError: string
  logs: string[]
}

interface NativeCommandResult {
  ok: boolean
  state: string
  message: string
  logs: string[]
}

@Entry
@Component
struct Index {
  @State private host: string = ''
  @State private port: string = '3389'
  @State private username: string = ''
  @State private password: string = ''
  @State private resolution: string = '1280x720'
  @State private certPolicy: string = 'tofu'
  @State private currentTab: string = 'connect'
  @State private connectionState: ConnectionState = 'Idle'
  @State private logText: string = 'Ready'
  @State private nativeProbeText: string = 'Native bridge not probed'
  @State private ctrlModifierDown: boolean = false
  @State private altModifierDown: boolean = false
  @State private winModifierDown: boolean = false
  private callbacksRegistered: boolean = false
  private pointerDown: boolean = false
  private twoFingerScrollActive: boolean = false
  private twoFingerLastY: number = 0
  private surfaceController: XComponentController = new XComponentController()

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
    this.probeNative()
  }

  private registerNativeCallbacks(): void {
    if (this.callbacksRegistered) {
      return
    }

    try {
      rdpNative.onState((state: string) => {
        this.connectionState = this.toConnectionState(state)
        if (state === 'Connected') {
          this.currentTab = 'session'
        } else {
          this.clearModifierState()
        }
      })
      rdpNative.onLog((line: string) => {
        this.appendLog(line)
      })
      rdpNative.onError((message: string) => {
        this.connectionState = 'Failed'
        this.appendLog(`Native error: ${message}`)
      })
      this.callbacksRegistered = true
    } catch (error) {
      this.appendLog(`Native callback registration failed: ${JSON.stringify(error)}`)
    }
  }

  private probeNative(): void {
    try {
      const result: NativeProbeResult = rdpNative.probe()
      const linkState = result.freeRdpLinked ? 'linked' : 'not-linked'
      const surfaceText = result.surfaceReady ? `surface ${result.surfaceWidth}x${result.surfaceHeight}` : 'surface pending'
      const paintText = result.surfacePaintCount > 0 ? `paint ${result.surfacePaintCount}` : 'paint pending'
      this.nativeProbeText = `bridge ${result.bridgeVersion} | ${result.abi} | FreeRDP ${result.freeRdpVersion} | ${linkState} | ${surfaceText} | ${paintText}`
      this.appendNativeLogs(result.logs)
    } catch (error) {
      this.nativeProbeText = 'Native bridge probe failed'
      this.appendLog(`Native probe failed: ${JSON.stringify(error)}`)
    }
  }

  private paintSurfaceTestPattern(): void {
    try {
      const result: NativeCommandResult = rdpNative.paintTestPattern()
      this.appendLog(result.message)
      this.appendNativeLogs(result.logs)
      this.probeNative()
    } catch (error) {
      this.connectionState = 'Failed'
      this.appendLog(`Native paint failed: ${JSON.stringify(error)}`)
    }
  }

  private canPaintSurfaceTestPattern(): boolean {
    return this.connectionState === 'Idle' || this.connectionState === 'Bridge ready' ||
      this.connectionState === 'Disconnected' || this.connectionState === 'Failed'
  }

  private connectNative(): void {
    if (!this.host.trim() || !this.username.trim() || !this.password.trim()) {
      this.connectionState = 'Idle'
      this.appendLog('Missing host, username, or password')
      return
    }

    try {
      this.connectionState = 'Resolving'
      this.currentTab = 'session'
      this.appendLog(`Calling native connect for ${this.host}:${this.port}`)
      const result: NativeCommandResult = rdpNative.connect({
        host: this.host,
        port: this.port,
        username: this.username,
        password: this.password,
        resolution: this.resolution,
        certPolicy: this.certPolicy
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

  private disconnectNative(): void {
    try {
      this.releaseActiveModifiers()
      const result: NativeCommandResult = rdpNative.disconnect()
      this.connectionState = this.toConnectionState(result.state)
      this.appendLog(result.message)
      this.appendNativeLogs(result.logs)
    } catch (error) {
      this.connectionState = 'Failed'
      this.appendLog(`Native disconnect failed: ${JSON.stringify(error)}`)
    }
  }

  private isConnected(): boolean {
    return this.connectionState === 'Connected'
  }

  private sendPointer(flags: number, x: number, y: number): void {
    if (!this.isConnected()) {
      return
    }

    try {
      const result: NativeCommandResult = rdpNative.sendPointer({
        flags: flags,
        x: Math.max(0, Math.round(x)),
        y: Math.max(0, Math.round(y))
      })
      if (!result.ok) {
        this.appendLog(result.message)
      }
    } catch (error) {
      this.appendLog(`Native pointer input failed: ${JSON.stringify(error)}`)
    }
  }

  private sendWheel(deltaY: number, x: number, y: number): void {
    const flags = deltaY > 0 ?
      PTR_FLAGS_WHEEL | PTR_FLAGS_WHEEL_NEGATIVE | RDP_WHEEL_DELTA_NEGATIVE :
      PTR_FLAGS_WHEEL | RDP_WHEEL_DELTA
    this.sendPointer(flags, x, y)
  }

  private sendKeyEvent(scancode: number, down: boolean, logDisconnected: boolean = true): boolean {
    if (!this.isConnected()) {
      if (logDisconnected) {
        this.appendLog('Key input skipped: no active FreeRDP session')
      }
      return false
    }

    try {
      const result: NativeCommandResult = rdpNative.sendKey({ scancode: scancode, down: down })
      if (!result.ok) {
        this.appendLog(result.message)
      }
      return result.ok
    } catch (error) {
      this.appendLog(`Native key input failed: ${JSON.stringify(error)}`)
      return false
    }
  }

  private sendKeyStroke(scancode: number): void {
    if (this.sendKeyEvent(scancode, true)) {
      this.sendKeyEvent(scancode, false)
    }
  }

  private clearModifierState(): void {
    this.ctrlModifierDown = false
    this.altModifierDown = false
    this.winModifierDown = false
  }

  private releaseActiveModifiers(): void {
    if (!this.isConnected()) {
      this.clearModifierState()
      return
    }

    if (this.winModifierDown) {
      this.sendKeyEvent(RDP_SCANCODE_LWIN, false, false)
    }
    if (this.altModifierDown) {
      this.sendKeyEvent(RDP_SCANCODE_LMENU, false, false)
    }
    if (this.ctrlModifierDown) {
      this.sendKeyEvent(RDP_SCANCODE_LCONTROL, false, false)
    }
    this.clearModifierState()
  }

  private toggleCtrlModifier(): void {
    const nextDown = !this.ctrlModifierDown
    if (this.sendKeyEvent(RDP_SCANCODE_LCONTROL, nextDown)) {
      this.ctrlModifierDown = nextDown
    }
  }

  private toggleAltModifier(): void {
    const nextDown = !this.altModifierDown
    if (this.sendKeyEvent(RDP_SCANCODE_LMENU, nextDown)) {
      this.altModifierDown = nextDown
    }
  }

  private toggleWinModifier(): void {
    const nextDown = !this.winModifierDown
    if (this.sendKeyEvent(RDP_SCANCODE_LWIN, nextDown)) {
      this.winModifierDown = nextDown
    }
  }

  private handleSurfaceTouch(event: TouchEvent): void {
    if (this.twoFingerScrollActive && event.touches.length < 2) {
      this.twoFingerScrollActive = false
      return
    }

    if (this.handleTwoFingerScroll(event)) {
      return
    }

    if (event.touches.length < 1) {
      return
    }

    const touch = event.touches[0]
    const x = touch.x
    const y = touch.y

    if (event.type === TouchType.Down) {
      this.pointerDown = true
      this.sendPointer(PTR_FLAGS_MOVE | PTR_FLAGS_BUTTON1 | PTR_FLAGS_DOWN, x, y)
      return
    }

    if (event.type === TouchType.Move) {
      const flags = this.pointerDown ? PTR_FLAGS_MOVE | PTR_FLAGS_BUTTON1 | PTR_FLAGS_DOWN : PTR_FLAGS_MOVE
      this.sendPointer(flags, x, y)
      return
    }

    if (event.type === TouchType.Up || event.type === TouchType.Cancel) {
      this.pointerDown = false
      this.sendPointer(PTR_FLAGS_MOVE | PTR_FLAGS_BUTTON1, x, y)
    }
  }

  private handleTwoFingerScroll(event: TouchEvent): boolean {
    if (event.touches.length < 2) {
      return false
    }

    const firstTouch = event.touches[0]
    const secondTouch = event.touches[1]
    const centerX = (firstTouch.x + secondTouch.x) / 2
    const centerY = (firstTouch.y + secondTouch.y) / 2

    if (this.pointerDown) {
      this.pointerDown = false
      this.sendPointer(PTR_FLAGS_MOVE | PTR_FLAGS_BUTTON1, firstTouch.x, firstTouch.y)
    }

    if (event.type === TouchType.Down || !this.twoFingerScrollActive) {
      this.twoFingerScrollActive = true
      this.twoFingerLastY = centerY
      return true
    }

    if (event.type === TouchType.Move) {
      const deltaY = centerY - this.twoFingerLastY
      if (Math.abs(deltaY) >= TOUCH_WHEEL_THRESHOLD) {
        this.sendWheel(deltaY, centerX, centerY)
        this.twoFingerLastY = centerY
      }
      return true
    }

    if (event.type === TouchType.Up || event.type === TouchType.Cancel) {
      this.twoFingerScrollActive = false
      return true
    }

    return true
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
      this.tabButton('diagnostics', 'Diagnostics')
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
        this.currentTab = id
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
        this.inputField('Resolution', this.resolution, (value: string) => {
          this.resolution = value
        })
        this.inputField('Certificate policy', this.certPolicy, (value: string) => {
          this.certPolicy = value
        })

        Button('Native Probe')
          .height(40)
          .width('100%')
          .backgroundColor('#E8EEF7')
          .fontColor('#1D2433')
          .margin({ bottom: 10 })
          .onClick(() => {
            this.probeNative()
          })

        Button('Connect')
          .height(44)
          .width('100%')
          .backgroundColor('#0A84FF')
          .fontColor(Color.White)
          .onClick(() => {
            this.connectNative()
          })

        Text(this.nativeProbeText)
          .fontSize(13)
          .fontColor('#475467')
          .width('100%')
          .margin({ top: 14 })

        Text(`State: ${this.connectionState}`)
          .fontSize(14)
          .fontColor('#475467')
          .width('100%')
          .margin({ top: 16 })
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
  private buildSessionPage() {
    Column() {
      Row() {
        Text(this.connectionState)
          .fontSize(16)
          .fontWeight(FontWeight.Medium)
          .fontColor('#101828')
          .layoutWeight(1)
        Button('Disconnect')
          .height(36)
          .backgroundColor('#D92D20')
          .fontColor(Color.White)
          .onClick(() => {
            this.disconnectNative()
          })
        Button('Paint')
          .height(36)
          .margin({ left: 8 })
          .backgroundColor('#E8EEF7')
          .fontColor('#1D2433')
          .onClick(() => {
            this.paintSurfaceTestPattern()
          })
      }
      .width('100%')
      .height(52)
      .padding({ left: 16, right: 16 })
      .backgroundColor('#FFFFFF')

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
            this.appendLog('XComponent loaded')
            this.probeNative()
            if (this.canPaintSurfaceTestPattern()) {
              this.paintSurfaceTestPattern()
            }
          })
          .onDestroy(() => {
            this.appendLog('XComponent destroyed')
          })
          .onTouch((event: TouchEvent) => {
            this.handleSurfaceTouch(event)
          })

        Text('Remote desktop surface')
          .fontSize(20)
          .fontColor('#667085')
      }
      .layoutWeight(1)
      .width('100%')
      .backgroundColor('#101828')

      Row() {
        Button('Ctrl')
          .layoutWeight(1)
          .backgroundColor(this.ctrlModifierDown ? '#0A84FF' : '#E8EEF7')
          .fontColor(this.ctrlModifierDown ? Color.White : '#1D2433')
          .onClick(() => {
            this.toggleCtrlModifier()
          })
        Button('Alt')
          .layoutWeight(1)
          .backgroundColor(this.altModifierDown ? '#0A84FF' : '#E8EEF7')
          .fontColor(this.altModifierDown ? Color.White : '#1D2433')
          .onClick(() => {
            this.toggleAltModifier()
          })
        Button('Win')
          .layoutWeight(1)
          .backgroundColor(this.winModifierDown ? '#0A84FF' : '#E8EEF7')
          .fontColor(this.winModifierDown ? Color.White : '#1D2433')
          .onClick(() => {
            this.toggleWinModifier()
          })
        Button('Esc')
          .layoutWeight(1)
          .backgroundColor('#E8EEF7')
          .fontColor('#1D2433')
          .onClick(() => {
            this.sendKeyStroke(RDP_SCANCODE_ESCAPE)
          })
        Button('Tab')
          .layoutWeight(1)
          .backgroundColor('#E8EEF7')
          .fontColor('#1D2433')
          .onClick(() => {
            this.sendKeyStroke(RDP_SCANCODE_TAB)
          })
      }
      .height(56)
      .width('100%')
      .padding({ left: 8, right: 8 })
      .backgroundColor('#FFFFFF')
    }
    .layoutWeight(1)
  }

  @Builder
  private buildDiagnosticsPage() {
    Scroll() {
      Column() {
        Text('Diagnostics')
          .fontSize(24)
          .fontWeight(FontWeight.Bold)
          .width('100%')
          .margin({ bottom: 16 })
        Text(`TCP target: ${this.host || '-'}:${this.port}`)
          .fontSize(15)
          .width('100%')
          .margin({ bottom: 8 })
        Text(`Native bridge: ${this.nativeProbeText}`)
          .fontSize(15)
          .width('100%')
          .margin({ bottom: 8 })
        Text('RDP negotiation: FreeRDP persistent session loop')
          .fontSize(15)
          .width('100%')
          .margin({ bottom: 8 })
        Text('Logs')
          .fontSize(18)
          .fontWeight(FontWeight.Medium)
          .width('100%')
          .margin({ top: 16, bottom: 8 })
        Text(this.logText)
          .fontSize(13)
          .fontColor('#344054')
          .width('100%')
          .backgroundColor('#FFFFFF')
          .padding(12)
      }
      .padding(20)
    }
    .layoutWeight(1)
  }

  build() {
    Column() {
      this.buildTabs()
      if (this.currentTab === 'connect') {
        this.buildConnectPage()
      } else if (this.currentTab === 'session') {
        this.buildSessionPage()
      } else {
        this.buildDiagnosticsPage()
      }
    }
    .width('100%')
    .height('100%')
    .backgroundColor('#F2F4F7')
  }
}
