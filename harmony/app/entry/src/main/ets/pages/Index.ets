import rdpNative from 'libentry.so'

type ConnectionState = 'Idle' | 'Resolving' | 'TCP connected' | 'Negotiating' | 'Authenticating' | 'Connected' | 'Bridge ready' | 'Disconnected' | 'Failed'

const PTR_FLAGS_MOVE: number = 0x0800
const PTR_FLAGS_HWHEEL: number = 0x0400
const PTR_FLAGS_WHEEL: number = 0x0200
const PTR_FLAGS_WHEEL_NEGATIVE: number = 0x0100
const PTR_FLAGS_DOWN: number = 0x8000
const PTR_FLAGS_BUTTON1: number = 0x1000
const PTR_FLAGS_BUTTON2: number = 0x2000
const PTR_FLAGS_BUTTON3: number = 0x4000
const RDP_WHEEL_DELTA: number = 0x0078
const RDP_WHEEL_DELTA_NEGATIVE: number = 0x0088
const TOUCH_DRAG_THRESHOLD: number = 8
const TOUCH_WHEEL_THRESHOLD: number = 24
const LONG_PRESS_TIMEOUT_MS: number = 550

const RDP_SCANCODE_ESCAPE: number = 0x01
const RDP_SCANCODE_BACKSPACE: number = 0x0E
const RDP_SCANCODE_TAB: number = 0x0F
const RDP_SCANCODE_ENTER: number = 0x1C
const RDP_SCANCODE_LCONTROL: number = 0x1D
const RDP_SCANCODE_LMENU: number = 0x38
const RDP_SCANCODE_LWIN: number = 0x015B
const RDP_SCANCODE_DELETE: number = 0x0153
const RDP_SCANCODE_UP: number = 0x0148
const RDP_SCANCODE_DOWN: number = 0x0150
const RDP_SCANCODE_LEFT: number = 0x014B
const RDP_SCANCODE_RIGHT: number = 0x014D
const RDP_SCANCODE_HOME: number = 0x0147
const RDP_SCANCODE_END: number = 0x014F

const OH_KEYCODE_0: number = 2000
const OH_KEYCODE_9: number = 2009
const OH_KEYCODE_DPAD_UP: number = 2012
const OH_KEYCODE_DPAD_DOWN: number = 2013
const OH_KEYCODE_DPAD_LEFT: number = 2014
const OH_KEYCODE_DPAD_RIGHT: number = 2015
const OH_KEYCODE_A: number = 2017
const OH_KEYCODE_Z: number = 2042
const OH_KEYCODE_COMMA: number = 2043
const OH_KEYCODE_PERIOD: number = 2044
const OH_KEYCODE_ALT_LEFT: number = 2045
const OH_KEYCODE_ALT_RIGHT: number = 2046
const OH_KEYCODE_SHIFT_LEFT: number = 2047
const OH_KEYCODE_SHIFT_RIGHT: number = 2048
const OH_KEYCODE_TAB: number = 2049
const OH_KEYCODE_SPACE: number = 2050
const OH_KEYCODE_ENTER: number = 2054
const OH_KEYCODE_DEL: number = 2055
const OH_KEYCODE_GRAVE: number = 2056
const OH_KEYCODE_MINUS: number = 2057
const OH_KEYCODE_EQUALS: number = 2058
const OH_KEYCODE_LEFT_BRACKET: number = 2059
const OH_KEYCODE_RIGHT_BRACKET: number = 2060
const OH_KEYCODE_BACKSLASH: number = 2061
const OH_KEYCODE_SEMICOLON: number = 2062
const OH_KEYCODE_APOSTROPHE: number = 2063
const OH_KEYCODE_SLASH: number = 2064
const OH_KEYCODE_ESCAPE: number = 2070
const OH_KEYCODE_FORWARD_DEL: number = 2071
const OH_KEYCODE_CTRL_LEFT: number = 2072
const OH_KEYCODE_CTRL_RIGHT: number = 2073
const OH_KEYCODE_META_LEFT: number = 2076
const OH_KEYCODE_META_RIGHT: number = 2077
const OH_KEYCODE_MOVE_HOME: number = 2081
const OH_KEYCODE_MOVE_END: number = 2082

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
  surfaceViewportX: number
  surfaceViewportY: number
  surfaceViewportWidth: number
  surfaceViewportHeight: number
  surfaceCreatedCount: number
  surfaceChangedCount: number
  surfaceDestroyedCount: number
  surfaceTouchCount: number
  surfacePaintCount: number
  surfaceLastPaintMessage: string
  sessionConnected: boolean
  desktopWidth: number
  desktopHeight: number
  probeJson: string
  probeError: string
  logs: string[]
}

interface PointerMapping {
  ok: boolean
  x: number
  y: number
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
  @State private surfacePixelWidth: number = 0
  @State private surfacePixelHeight: number = 0
  @State private surfaceViewportX: number = 0
  @State private surfaceViewportY: number = 0
  @State private surfaceViewportWidth: number = 0
  @State private surfaceViewportHeight: number = 0
  @State private desktopPixelWidth: number = 0
  @State private desktopPixelHeight: number = 0
  @State private remoteTextInput: string = ''
  private callbacksRegistered: boolean = false
  private pointerDown: boolean = false
  private pendingTouchActive: boolean = false
  private longPressTriggered: boolean = false
  private longPressTimer: number = -1
  private pendingTouchStartX: number = 0
  private pendingTouchStartY: number = 0
  private lastTouchX: number = 0
  private lastTouchY: number = 0
  private twoFingerScrollActive: boolean = false
  private twoFingerLastX: number = 0
  private twoFingerLastY: number = 0
  private mouseButtonFlags: number = 0
  private surfaceController: XComponentController = new XComponentController()

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
    this.probeNative()
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
            this.probeNative()
          } else if (nextState !== 'Connected') {
            this.clearModifierState()
            this.clearPointerState()
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
      this.callbacksRegistered = true
    } catch (error) {
      this.appendLog(`Native callback registration failed: ${JSON.stringify(error)}`)
    }
  }

  private probeNative(): void {
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
      const linkState = result.freeRdpLinked ? 'linked' : 'not-linked'
      const surfaceText = result.surfaceReady ? `surface ${result.surfaceWidth}x${result.surfaceHeight}` : 'surface pending'
      const viewportText = result.surfaceViewportWidth > 0 && result.surfaceViewportHeight > 0 ?
        `viewport ${result.surfaceViewportX},${result.surfaceViewportY} ${result.surfaceViewportWidth}x${result.surfaceViewportHeight}` :
        'viewport pending'
      const desktopText = result.desktopWidth > 0 && result.desktopHeight > 0 ?
        `desktop ${result.desktopWidth}x${result.desktopHeight}` : 'desktop pending'
      const paintText = result.surfacePaintCount > 0 ? `paint ${result.surfacePaintCount}` : 'paint pending'
      this.nativeProbeText = `bridge ${result.bridgeVersion} | ${result.abi} | FreeRDP ${result.freeRdpVersion} | ${linkState} | ${surfaceText} | ${viewportText} | ${desktopText} | ${paintText}`
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
      this.releasePointerButtons()
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

  private resizeNative(): void {
    const width = this.parseResolutionPart(0)
    const height = this.parseResolutionPart(1)
    if (width <= 0 || height <= 0) {
      this.appendLog(`Invalid resolution: ${this.resolution}`)
      return
    }

    try {
      const result: NativeCommandResult = rdpNative.resize({ width: width, height: height })
      this.appendLog(result.message)
      this.appendNativeLogs(result.logs)
      this.probeNative()
    } catch (error) {
      this.appendLog(`Native resize failed: ${JSON.stringify(error)}`)
    }
  }

  private isConnected(): boolean {
    return this.connectionState === 'Connected'
  }

  private sendPointer(flags: number, x: number, y: number, allowClamp: boolean = false): boolean {
    if (!this.isConnected()) {
      return false
    }

    try {
      const mapped = this.mapPointerToDesktop(x, y, allowClamp)
      if (!mapped.ok) {
        return false
      }
      const result: NativeCommandResult = rdpNative.sendPointer({
        flags: flags,
        x: mapped.x,
        y: mapped.y
      })
      if (!result.ok) {
        this.appendLog(result.message)
      }
      return result.ok
    } catch (error) {
      this.appendLog(`Native pointer input failed: ${JSON.stringify(error)}`)
      return false
    }
  }

  private parseResolutionPart(index: number): number {
    const parts = this.resolution.toLowerCase().split('x')
    if (parts.length !== 2) {
      return 0
    }

    const value = Number(parts[index])
    if (value > 0) {
      return value
    }
    return 0
  }

  private getRequestedDesktopWidth(): number {
    if (this.desktopPixelWidth > 0) {
      return this.desktopPixelWidth
    }
    return this.parseResolutionPart(0)
  }

  private getRequestedDesktopHeight(): number {
    if (this.desktopPixelHeight > 0) {
      return this.desktopPixelHeight
    }
    return this.parseResolutionPart(1)
  }

  private clampInt(value: number, minValue: number, maxValue: number): number {
    return Math.max(minValue, Math.min(maxValue, Math.round(value)))
  }

  private getRemoteViewportX(): number {
    return this.surfaceViewportWidth > 0 && this.surfaceViewportHeight > 0 ? this.surfaceViewportX : 0
  }

  private getRemoteViewportY(): number {
    return this.surfaceViewportWidth > 0 && this.surfaceViewportHeight > 0 ? this.surfaceViewportY : 0
  }

  private getRemoteViewportWidth(): number {
    return this.surfaceViewportWidth > 0 ? this.surfaceViewportWidth : this.surfacePixelWidth
  }

  private getRemoteViewportHeight(): number {
    return this.surfaceViewportHeight > 0 ? this.surfaceViewportHeight : this.surfacePixelHeight
  }

  private isPointInRemoteViewport(x: number, y: number): boolean {
    const viewportX = this.getRemoteViewportX()
    const viewportY = this.getRemoteViewportY()
    const viewportWidth = this.getRemoteViewportWidth()
    const viewportHeight = this.getRemoteViewportHeight()
    if (viewportWidth <= 0 || viewportHeight <= 0) {
      return false
    }
    return x >= viewportX && x < viewportX + viewportWidth && y >= viewportY && y < viewportY + viewportHeight
  }

  private mapPointerToDesktop(x: number, y: number, allowClamp: boolean): PointerMapping {
    const viewportX = this.getRemoteViewportX()
    const viewportY = this.getRemoteViewportY()
    const viewportWidth = this.getRemoteViewportWidth()
    const viewportHeight = this.getRemoteViewportHeight()
    const desktopWidth = this.getRequestedDesktopWidth()
    const desktopHeight = this.getRequestedDesktopHeight()
    if (viewportWidth <= 0 || viewportHeight <= 0 || desktopWidth <= 0 || desktopHeight <= 0) {
      return { ok: false, x: 0, y: 0 }
    }

    const localX = x - viewportX
    const localY = y - viewportY
    const inside = localX >= 0 && localX < viewportWidth && localY >= 0 && localY < viewportHeight
    if (!inside && !allowClamp) {
      return { ok: false, x: 0, y: 0 }
    }

    const clampedX = this.clampInt(localX, 0, viewportWidth - 1)
    const clampedY = this.clampInt(localY, 0, viewportHeight - 1)
    const mappedX = this.clampInt(clampedX * desktopWidth / viewportWidth, 0, desktopWidth - 1)
    const mappedY = this.clampInt(clampedY * desktopHeight / viewportHeight, 0, desktopHeight - 1)
    return { ok: true, x: mappedX, y: mappedY }
  }

  private sendWheel(deltaY: number, x: number, y: number): void {
    const flags = deltaY > 0 ?
      PTR_FLAGS_WHEEL | PTR_FLAGS_WHEEL_NEGATIVE | RDP_WHEEL_DELTA_NEGATIVE :
      PTR_FLAGS_WHEEL | RDP_WHEEL_DELTA
    this.sendPointer(flags, x, y)
  }

  private sendHorizontalWheel(deltaX: number, x: number, y: number): void {
    const flags = deltaX > 0 ?
      PTR_FLAGS_HWHEEL | PTR_FLAGS_WHEEL_NEGATIVE | RDP_WHEEL_DELTA_NEGATIVE :
      PTR_FLAGS_HWHEEL | RDP_WHEEL_DELTA
    this.sendPointer(flags, x, y)
  }

  private sendLeftClick(x: number, y: number): void {
    this.sendPointer(PTR_FLAGS_MOVE, x, y)
    this.sendPointer(PTR_FLAGS_BUTTON1 | PTR_FLAGS_DOWN, x, y)
    this.sendPointer(PTR_FLAGS_BUTTON1, x, y)
  }

  private sendRightClick(x: number, y: number): void {
    this.sendPointer(PTR_FLAGS_MOVE, x, y)
    this.sendPointer(PTR_FLAGS_BUTTON2 | PTR_FLAGS_DOWN, x, y)
    this.sendPointer(PTR_FLAGS_BUTTON2, x, y)
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

  private sendUnicodeEvent(code: number, down: boolean): boolean {
    if (!this.isConnected()) {
      this.appendLog('Text input skipped: no active FreeRDP session')
      return false
    }

    try {
      const result: NativeCommandResult = rdpNative.sendUnicode({ code: code, down: down })
      if (!result.ok) {
        this.appendLog(result.message)
      }
      return result.ok
    } catch (error) {
      this.appendLog(`Native unicode input failed: ${JSON.stringify(error)}`)
      return false
    }
  }

  private sendUnicodeStroke(code: number): void {
    if (this.sendUnicodeEvent(code, true)) {
      this.sendUnicodeEvent(code, false)
    }
  }

  private sendRemoteText(): void {
    if (!this.remoteTextInput.length) {
      return
    }
    if (!this.isConnected()) {
      this.appendLog('Text input skipped: no active FreeRDP session')
      return
    }

    for (let i = 0; i < this.remoteTextInput.length; i++) {
      const code = this.remoteTextInput.charCodeAt(i)
      if (code >= 0xD800 && code <= 0xDBFF) {
        if (i + 1 < this.remoteTextInput.length) {
          const nextCode = this.remoteTextInput.charCodeAt(i + 1)
          if (nextCode >= 0xDC00 && nextCode <= 0xDFFF) {
            this.appendLog('Text input skipped: non-BMP character is not supported yet')
            i++
            continue
          }
        }
      }
      if (code >= 0xDC00 && code <= 0xDFFF) {
        this.appendLog('Text input skipped: unmatched UTF-16 surrogate')
        continue
      }
      this.sendUnicodeStroke(code)
    }

    this.remoteTextInput = ''
  }

  private clearModifierState(): void {
    this.ctrlModifierDown = false
    this.altModifierDown = false
    this.winModifierDown = false
  }

  private clearPointerState(): void {
    this.cancelLongPressTimer()
    this.pendingTouchActive = false
    this.longPressTriggered = false
    this.pointerDown = false
    this.twoFingerScrollActive = false
    this.mouseButtonFlags = 0
  }

  private releasePointerButtons(): void {
    if (!this.isConnected()) {
      this.clearPointerState()
      return
    }
    if (this.pointerDown) {
      this.sendPointer(PTR_FLAGS_MOVE | PTR_FLAGS_BUTTON1, this.lastTouchX, this.lastTouchY, true)
    }
    if (this.mouseButtonFlags !== 0) {
      this.sendPointer(PTR_FLAGS_MOVE | this.mouseButtonFlags, this.lastTouchX, this.lastTouchY, true)
    }
    this.clearPointerState()
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

  private mapArkKeyCodeToRdpScancode(keyCode: number): number {
    if (keyCode >= OH_KEYCODE_A && keyCode <= OH_KEYCODE_Z) {
      const letterScancodes: number[] = [
        0x1E, 0x30, 0x2E, 0x20, 0x12, 0x21, 0x22, 0x23, 0x17, 0x24, 0x25, 0x26, 0x32,
        0x31, 0x18, 0x19, 0x10, 0x13, 0x1F, 0x14, 0x16, 0x2F, 0x11, 0x2D, 0x15, 0x2C
      ]
      return letterScancodes[keyCode - OH_KEYCODE_A]
    }
    if (keyCode >= OH_KEYCODE_0 && keyCode <= OH_KEYCODE_9) {
      const digitScancodes: number[] = [0x0B, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A]
      return digitScancodes[keyCode - OH_KEYCODE_0]
    }

    const keyMap: Map<number, number> = new Map<number, number>([
      [OH_KEYCODE_ESCAPE, RDP_SCANCODE_ESCAPE],
      [OH_KEYCODE_TAB, RDP_SCANCODE_TAB],
      [OH_KEYCODE_ENTER, RDP_SCANCODE_ENTER],
      [OH_KEYCODE_DEL, RDP_SCANCODE_BACKSPACE],
      [OH_KEYCODE_FORWARD_DEL, RDP_SCANCODE_DELETE],
      [OH_KEYCODE_SPACE, 0x39],
      [OH_KEYCODE_DPAD_UP, RDP_SCANCODE_UP],
      [OH_KEYCODE_DPAD_DOWN, RDP_SCANCODE_DOWN],
      [OH_KEYCODE_DPAD_LEFT, RDP_SCANCODE_LEFT],
      [OH_KEYCODE_DPAD_RIGHT, RDP_SCANCODE_RIGHT],
      [OH_KEYCODE_MOVE_HOME, RDP_SCANCODE_HOME],
      [OH_KEYCODE_MOVE_END, RDP_SCANCODE_END],
      [OH_KEYCODE_CTRL_LEFT, RDP_SCANCODE_LCONTROL],
      [OH_KEYCODE_CTRL_RIGHT, 0x011D],
      [OH_KEYCODE_ALT_LEFT, RDP_SCANCODE_LMENU],
      [OH_KEYCODE_ALT_RIGHT, 0x0138],
      [OH_KEYCODE_META_LEFT, RDP_SCANCODE_LWIN],
      [OH_KEYCODE_META_RIGHT, 0x015C],
      [OH_KEYCODE_SHIFT_LEFT, 0x2A],
      [OH_KEYCODE_SHIFT_RIGHT, 0x36],
      [OH_KEYCODE_GRAVE, 0x29],
      [OH_KEYCODE_MINUS, 0x0C],
      [OH_KEYCODE_EQUALS, 0x0D],
      [OH_KEYCODE_LEFT_BRACKET, 0x1A],
      [OH_KEYCODE_RIGHT_BRACKET, 0x1B],
      [OH_KEYCODE_BACKSLASH, 0x2B],
      [OH_KEYCODE_SEMICOLON, 0x27],
      [OH_KEYCODE_APOSTROPHE, 0x28],
      [OH_KEYCODE_COMMA, 0x33],
      [OH_KEYCODE_PERIOD, 0x34],
      [OH_KEYCODE_SLASH, 0x35]
    ])
    return keyMap.get(keyCode) ?? 0
  }

  private handleSurfaceKeyEvent(event: KeyEvent): boolean {
    if (!this.isConnected()) {
      return false
    }
    const scancode = this.mapArkKeyCodeToRdpScancode(event.keyCode)
    if (scancode <= 0) {
      return false
    }
    const down = event.type === KeyType.Down
    return this.sendKeyEvent(scancode, down, false)
  }

  private mouseButtonToPointerFlag(button: MouseButton): number {
    if (button === MouseButton.Left) {
      return PTR_FLAGS_BUTTON1
    }
    if (button === MouseButton.Right) {
      return PTR_FLAGS_BUTTON2
    }
    if (button === MouseButton.Middle) {
      return PTR_FLAGS_BUTTON3
    }
    return 0
  }

  private getPressedMouseButtonFlags(event: MouseEvent): number {
    let flags = this.mouseButtonFlags
    if (event.pressedButtons) {
      flags = 0
      for (let i = 0; i < event.pressedButtons.length; i++) {
        flags |= this.mouseButtonToPointerFlag(event.pressedButtons[i])
      }
    }
    return flags
  }

  private handleSurfaceMouse(event: MouseEvent): void {
    if (!this.isConnected()) {
      return
    }
    this.lastTouchX = event.x
    this.lastTouchY = event.y
    const buttonFlag = this.mouseButtonToPointerFlag(event.button)
    if (event.action === MouseAction.Press) {
      if (buttonFlag === 0 || !this.isPointInRemoteViewport(event.x, event.y)) {
        return
      }
      this.mouseButtonFlags |= buttonFlag
      this.sendPointer(PTR_FLAGS_MOVE, event.x, event.y)
      this.sendPointer(PTR_FLAGS_MOVE | buttonFlag | PTR_FLAGS_DOWN, event.x, event.y)
      return
    }
    if (event.action === MouseAction.Release) {
      const releaseFlag = buttonFlag !== 0 ? buttonFlag : this.mouseButtonFlags
      if (releaseFlag !== 0) {
        this.sendPointer(PTR_FLAGS_MOVE | releaseFlag, event.x, event.y, true)
        this.mouseButtonFlags &= ~releaseFlag
      }
      return
    }
    if (event.action === MouseAction.Move || event.action === MouseAction.Hover) {
      const pressedFlags = this.getPressedMouseButtonFlags(event)
      if (pressedFlags !== 0) {
        this.sendPointer(PTR_FLAGS_MOVE | pressedFlags | PTR_FLAGS_DOWN, event.x, event.y, true)
      } else {
        this.sendPointer(PTR_FLAGS_MOVE, event.x, event.y)
      }
      return
    }
    if (event.action === MouseAction.CANCEL) {
      if (this.mouseButtonFlags !== 0) {
        this.sendPointer(PTR_FLAGS_MOVE | this.mouseButtonFlags, event.x, event.y, true)
      }
      this.mouseButtonFlags = 0
    }
  }

  private handleSurfaceAxis(event: AxisEvent): void {
    if (!this.isConnected() || event.action !== AxisAction.UPDATE) {
      return
    }

    const deltaY = event.getVerticalAxisValue()
    const deltaX = event.getHorizontalAxisValue()
    if (Math.abs(deltaY) >= Math.abs(deltaX) && Math.abs(deltaY) > 0) {
      this.sendWheel(deltaY, event.x, event.y)
      return
    }
    if (Math.abs(deltaX) > 0) {
      this.sendHorizontalWheel(deltaX, event.x, event.y)
    }
  }

  private startPendingTouch(x: number, y: number): void {
    if (!this.isPointInRemoteViewport(x, y)) {
      this.cancelPendingTouch()
      return
    }
    this.cancelLongPressTimer()
    this.pendingTouchActive = true
    this.longPressTriggered = false
    this.pendingTouchStartX = x
    this.pendingTouchStartY = y
    this.lastTouchX = x
    this.lastTouchY = y
    this.longPressTimer = setTimeout(() => {
      if (!this.pendingTouchActive || this.pointerDown) {
        return
      }
      this.pendingTouchActive = false
      this.longPressTriggered = true
      this.longPressTimer = -1
      this.sendRightClick(this.pendingTouchStartX, this.pendingTouchStartY)
    }, LONG_PRESS_TIMEOUT_MS)
  }

  private cancelLongPressTimer(): void {
    if (this.longPressTimer >= 0) {
      clearTimeout(this.longPressTimer)
      this.longPressTimer = -1
    }
  }

  private cancelPendingTouch(): void {
    this.cancelLongPressTimer()
    this.pendingTouchActive = false
    this.longPressTriggered = false
  }

  private beginLeftDrag(x: number, y: number): void {
    this.cancelLongPressTimer()
    this.pendingTouchActive = false
    this.pointerDown = true
    this.sendPointer(PTR_FLAGS_MOVE, this.pendingTouchStartX, this.pendingTouchStartY)
    this.sendPointer(PTR_FLAGS_MOVE | PTR_FLAGS_BUTTON1 | PTR_FLAGS_DOWN, this.pendingTouchStartX,
      this.pendingTouchStartY)
    this.sendPointer(PTR_FLAGS_MOVE | PTR_FLAGS_BUTTON1 | PTR_FLAGS_DOWN, x, y, true)
  }

  private finishSingleTouch(x: number, y: number, cancelled: boolean): void {
    this.cancelLongPressTimer()

    if (this.longPressTriggered) {
      this.pendingTouchActive = false
      this.longPressTriggered = false
      this.pointerDown = false
      return
    }

    if (this.pendingTouchActive) {
      this.pendingTouchActive = false
      if (!cancelled) {
        this.sendLeftClick(x, y)
      }
      return
    }

    if (this.pointerDown) {
      this.pointerDown = false
      this.sendPointer(PTR_FLAGS_MOVE | PTR_FLAGS_BUTTON1, x, y, true)
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
      if (event.type === TouchType.Up || event.type === TouchType.Cancel) {
        this.finishSingleTouch(this.lastTouchX, this.lastTouchY, event.type === TouchType.Cancel)
      }
      return
    }

    const touch = event.touches[0]
    const x = touch.x
    const y = touch.y
    this.lastTouchX = x
    this.lastTouchY = y

    if (event.type === TouchType.Down) {
      this.startPendingTouch(x, y)
      return
    }

    if (event.type === TouchType.Move) {
      if (this.pendingTouchActive) {
        const dx = x - this.pendingTouchStartX
        const dy = y - this.pendingTouchStartY
        if (Math.abs(dx) >= TOUCH_DRAG_THRESHOLD || Math.abs(dy) >= TOUCH_DRAG_THRESHOLD) {
          this.beginLeftDrag(x, y)
        }
        return
      }

      if (this.pointerDown) {
        this.sendPointer(PTR_FLAGS_MOVE | PTR_FLAGS_BUTTON1 | PTR_FLAGS_DOWN, x, y, true)
      } else {
        this.sendPointer(PTR_FLAGS_MOVE, x, y)
      }
      return
    }

    if (event.type === TouchType.Up || event.type === TouchType.Cancel) {
      this.finishSingleTouch(x, y, event.type === TouchType.Cancel)
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

    if (this.pendingTouchActive || this.longPressTriggered) {
      this.cancelPendingTouch()
    }

    if (this.pointerDown) {
      this.pointerDown = false
      this.sendPointer(PTR_FLAGS_MOVE | PTR_FLAGS_BUTTON1, firstTouch.x, firstTouch.y, true)
    }

    if (event.type === TouchType.Down || !this.twoFingerScrollActive) {
      this.twoFingerScrollActive = true
      this.twoFingerLastX = centerX
      this.twoFingerLastY = centerY
      return true
    }

    if (event.type === TouchType.Move) {
      const deltaX = centerX - this.twoFingerLastX
      const deltaY = centerY - this.twoFingerLastY
      if (Math.abs(deltaY) >= TOUCH_WHEEL_THRESHOLD || Math.abs(deltaX) >= TOUCH_WHEEL_THRESHOLD) {
        if (Math.abs(deltaY) >= Math.abs(deltaX)) {
          this.sendWheel(deltaY, centerX, centerY)
        } else {
          this.sendHorizontalWheel(deltaX, centerX, centerY)
        }
        this.twoFingerLastX = centerX
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
        if (id === 'diagnostics') {
          this.deferUiUpdate(() => {
            this.probeNative()
          })
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
  private keyButton(label: string, scancode: number) {
    Button(label)
      .layoutWeight(1)
      .fontSize(13)
      .backgroundColor('#E8EEF7')
      .fontColor('#1D2433')
      .onClick(() => {
        this.sendKeyStroke(scancode)
      })
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
        Button('Resize')
          .height(36)
          .margin({ left: 8 })
          .backgroundColor('#E8EEF7')
          .fontColor('#1D2433')
          .onClick(() => {
            this.resizeNative()
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
            this.deferUiUpdate(() => {
              this.appendLog('XComponent loaded')
              this.probeNative()
              if (this.canPaintSurfaceTestPattern()) {
                this.paintSurfaceTestPattern()
              }
            })
          })
          .onDestroy(() => {
            this.deferUiUpdate(() => {
              this.appendLog('XComponent destroyed')
              this.probeNative()
            })
          })
          .onAreaChange(() => {
            this.deferUiUpdate(() => {
              this.probeNative()
            })
          })
          .onTouch((event: TouchEvent) => {
            this.handleSurfaceTouch(event)
          })

        if (!this.isConnected()) {
          Text('Remote desktop surface')
            .fontSize(20)
            .fontColor('#667085')
        }
      }
      .layoutWeight(1)
      .width('100%')
      .backgroundColor('#101828')
      .focusable(true)
      .focusOnTouch(true)
      .onKeyEvent((event: KeyEvent): boolean => {
        return this.handleSurfaceKeyEvent(event)
      })
      .onMouse((event: MouseEvent) => {
        this.handleSurfaceMouse(event)
      })
      .onAxisEvent((event: AxisEvent) => {
        this.handleSurfaceAxis(event)
      })

      Column() {
        Row() {
          Button('Ctrl')
            .layoutWeight(1)
            .fontSize(13)
            .backgroundColor(this.ctrlModifierDown ? '#0A84FF' : '#E8EEF7')
            .fontColor(this.ctrlModifierDown ? Color.White : '#1D2433')
            .onClick(() => {
              this.toggleCtrlModifier()
            })
          Button('Alt')
            .layoutWeight(1)
            .fontSize(13)
            .backgroundColor(this.altModifierDown ? '#0A84FF' : '#E8EEF7')
            .fontColor(this.altModifierDown ? Color.White : '#1D2433')
            .onClick(() => {
              this.toggleAltModifier()
            })
          Button('Win')
            .layoutWeight(1)
            .fontSize(13)
            .backgroundColor(this.winModifierDown ? '#0A84FF' : '#E8EEF7')
            .fontColor(this.winModifierDown ? Color.White : '#1D2433')
            .onClick(() => {
              this.toggleWinModifier()
            })
          Button('Esc')
            .layoutWeight(1)
            .fontSize(13)
            .backgroundColor('#E8EEF7')
            .fontColor('#1D2433')
            .onClick(() => {
              this.sendKeyStroke(RDP_SCANCODE_ESCAPE)
            })
          Button('Tab')
            .layoutWeight(1)
            .fontSize(13)
            .backgroundColor('#E8EEF7')
            .fontColor('#1D2433')
            .onClick(() => {
              this.sendKeyStroke(RDP_SCANCODE_TAB)
            })
          this.keyButton('Enter', RDP_SCANCODE_ENTER)
          this.keyButton('Bksp', RDP_SCANCODE_BACKSPACE)
          this.keyButton('Del', RDP_SCANCODE_DELETE)
        }
        .height(44)
        .width('100%')

        Row() {
          this.keyButton('Home', RDP_SCANCODE_HOME)
          this.keyButton('End', RDP_SCANCODE_END)
          this.keyButton('Up', RDP_SCANCODE_UP)
          this.keyButton('Down', RDP_SCANCODE_DOWN)
          this.keyButton('Left', RDP_SCANCODE_LEFT)
          this.keyButton('Right', RDP_SCANCODE_RIGHT)
        }
        .height(36)
        .width('100%')
        .margin({ top: 6 })

        Row() {
          TextInput({ placeholder: 'Text', text: this.remoteTextInput })
            .height(40)
            .layoutWeight(1)
            .onChange((value: string) => {
              this.remoteTextInput = value
            })
          Button('Send')
            .height(40)
            .width(112)
            .margin({ left: 8 })
            .backgroundColor('#0A84FF')
            .fontColor(Color.White)
            .onClick(() => {
              this.sendRemoteText()
            })
        }
        .height(48)
        .width('100%')
        .margin({ top: 6 })
      }
      .height(154)
      .width('100%')
      .padding({ left: 8, right: 8, top: 8, bottom: 8 })
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
