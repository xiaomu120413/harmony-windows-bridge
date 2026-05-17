import { abilityAccessCtrl, bundleManager, common } from '@kit.AbilityKit'
import { hilog } from '@kit.PerformanceAnalysisKit'
import {
  AXIS_EVENT_DEDUP_MS,
  BRIDGE_PRINTABLE_KEY_GUARD_MS,
  DOUBLE_TAP_DISTANCE,
  DOUBLE_TAP_TIMEOUT_MS,
  HILOG_DOMAIN,
  HILOG_TAG,
  INPUT_EVENT_LOG_INTERVAL_MS,
  LONG_PRESS_TIMEOUT_MS,
  MICROPHONE_PERMISSION,
  OH_KEYCODE_DEL,
  POINTER_FAILURE_LOG_INTERVAL_MS,
  PTR_FLAGS_BUTTON1,
  PTR_FLAGS_BUTTON2,
  PTR_FLAGS_BUTTON3,
  PTR_FLAGS_DOWN,
  PTR_FLAGS_HWHEEL,
  PTR_FLAGS_MOVE,
  PTR_FLAGS_WHEEL,
  PTR_FLAGS_WHEEL_NEGATIVE,
  RDP_WHEEL_DELTA,
  RDP_WHEEL_DELTA_NEGATIVE,
  READ_PASTEBOARD_PERMISSION,
  REMOTE_INPUT_BRIDGE_ID,
  REMOTE_SURFACE_FOCUS_ID,
  REMOTE_TEXT_BRIDGE_CLEAR_GUARD_MS,
  REMOTE_TEXT_BRIDGE_RESET_LENGTH,
  SYNTHETIC_MOUSE_SUPPRESS_MS,
  TOUCH_DRAG_THRESHOLD,
  TOUCH_WHEEL_THRESHOLD
} from '../rdp/RdpConstants'
import { PointerMapper, PointerViewport } from '../rdp/PointerMapper'
import { RdpKeyboard } from '../rdp/RdpKeyboard'
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
  private remoteTextBridgeValue: string = ''
  private bridgePrintableKeyCode: number = 0
  private bridgePrintableKeyAtMs: number = 0
  private dispatchedRemoteKeyEvents: string[] = []
  private suppressedTextInputChars: string[] = []
  private remoteTextBridgeClearUntilMs: number = 0
  private callbacksRegistered: boolean = false
  private pasteboardPermissionRequested: boolean = false
  private pasteboardPermissionInFlight: boolean = false
  private microphonePermissionInFlight: boolean = false
  private microphonePermissionRequest: Promise<boolean> | null = null
  private pointerDown: boolean = false
  private pendingTouchActive: boolean = false
  private longPressTriggered: boolean = false
  private longPressTimer: number = -1
  private pendingTouchStartX: number = 0
  private pendingTouchStartY: number = 0
  private lastTouchX: number = 0
  private lastTouchY: number = 0
  private twoFingerScrollActive: boolean = false
  private touchGestureSuppressed: boolean = false
  private lastTouchEventAtMs: number = 0
  private twoFingerLastX: number = 0
  private twoFingerLastY: number = 0
  private mouseButtonFlags: number = 0
  private lastTapAtMs: number = 0
  private lastTapX: number = 0
  private lastTapY: number = 0
  private lastPointerFailureLogAtMs: number = 0
  private lastInputEventLogAtMs: number = 0
  private lastAxisEventAtMs: number = 0
  private lastAxisAction: number = -1
  private lastAxisX: number = 0
  private lastAxisY: number = 0
  private lastAxisDeltaX: number = 0
  private lastAxisDeltaY: number = 0
  private surfacePixelScaleX: number = 0
  private surfacePixelScaleY: number = 0
  private surfaceController: XComponentController = new XComponentController()
  private remoteInputController: TextInputController = new TextInputController()

  private deferUiUpdate(task: () => void): void {
    setTimeout((): void => {
      task()
    }, 0)
  }

  private appendLog(line: string): void {
    this.logText = `${this.logText}\n${line}`
  }

  private appendPasteboardPermissionLog(line: string): void {
    this.appendLog(line)
    hilog.info(HILOG_DOMAIN, HILOG_TAG, '%{public}s', line)
  }

  private appendNativeLogs(lines: string[]): void {
    for (let i = 0; i < lines.length; i++) {
      this.appendLog(lines[i])
    }
  }

  aboutToAppear(): void {
    this.registerNativeCallbacks()
    this.ensurePasteboardReadPermission('page appear')
  }

  aboutToDisappear(): void {
    this.releasePointerButtons()
    this.releaseActiveModifiers()
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
            this.clearPointerState()
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
          this.ensureMicrophonePermission('remote audio capture')
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

  private getSelfAccessTokenId(): number {
    const bundleInfo = bundleManager.getBundleInfoForSelfSync(
      bundleManager.BundleFlag.GET_BUNDLE_INFO_WITH_APPLICATION
    )
    return bundleInfo.appInfo.accessTokenId
  }

  private hasPasteboardReadPermission(): boolean {
    try {
      const atManager = abilityAccessCtrl.createAtManager()
      const status = atManager.checkAccessTokenSync(this.getSelfAccessTokenId(), READ_PASTEBOARD_PERMISSION)
      return status === abilityAccessCtrl.GrantStatus.PERMISSION_GRANTED
    } catch (error) {
      this.appendPasteboardPermissionLog(`Pasteboard permission check failed: ${JSON.stringify(error)}`)
      return false
    }
  }

  private ensurePasteboardReadPermission(trigger: string): void {
    if (this.hasPasteboardReadPermission()) {
      this.appendPasteboardPermissionLog(`Pasteboard read permission granted: ${trigger}`)
      return
    }
    if (this.pasteboardPermissionRequested || this.pasteboardPermissionInFlight) {
      return
    }

    let context: common.UIAbilityContext
    try {
      context = getContext(this) as common.UIAbilityContext
    } catch (error) {
      this.appendPasteboardPermissionLog(`Pasteboard permission request skipped: no UIAbilityContext; ${JSON.stringify(error)}`)
      return
    }

    this.pasteboardPermissionRequested = true
    this.pasteboardPermissionInFlight = true
    const atManager = abilityAccessCtrl.createAtManager()
    atManager.requestPermissionsFromUser(context, [READ_PASTEBOARD_PERMISSION])
      .then((result) => {
        const authResult = Array.isArray(result.authResults) && result.authResults.length > 0 ?
          result.authResults[0] : abilityAccessCtrl.GrantStatus.PERMISSION_DENIED
        const granted = authResult === abilityAccessCtrl.GrantStatus.PERMISSION_GRANTED ||
          this.hasPasteboardReadPermission()
        this.appendPasteboardPermissionLog(`Pasteboard read permission request result: granted=${granted} auth=${authResult}`)
      })
      .catch((error: Error) => {
        this.appendPasteboardPermissionLog(`Pasteboard read permission request failed: ${JSON.stringify(error)}`)
      })
      .finally(() => {
        this.pasteboardPermissionInFlight = false
      })
  }

  private hasMicrophonePermission(): boolean {
    try {
      const atManager = abilityAccessCtrl.createAtManager()
      const status = atManager.checkAccessTokenSync(this.getSelfAccessTokenId(), MICROPHONE_PERMISSION)
      return status === abilityAccessCtrl.GrantStatus.PERMISSION_GRANTED
    } catch (error) {
      this.appendLog(`Microphone permission check failed: ${JSON.stringify(error)}`)
      return false
    }
  }

  private ensureMicrophonePermission(trigger: string): Promise<boolean> {
    if (this.hasMicrophonePermission()) {
      this.appendLog(`Microphone permission granted: ${trigger}`)
      return Promise.resolve(true)
    }
    if (this.microphonePermissionRequest !== null) {
      return this.microphonePermissionRequest
    }

    let context: common.UIAbilityContext
    try {
      context = getContext(this) as common.UIAbilityContext
    } catch (error) {
      this.appendLog(`Microphone permission request skipped: no UIAbilityContext; ${JSON.stringify(error)}`)
      return Promise.resolve(false)
    }

    this.microphonePermissionInFlight = true
    const atManager = abilityAccessCtrl.createAtManager()
    this.microphonePermissionRequest = atManager.requestPermissionsFromUser(context, [MICROPHONE_PERMISSION])
      .then((result) => {
        const authResult = Array.isArray(result.authResults) && result.authResults.length > 0 ?
          result.authResults[0] : abilityAccessCtrl.GrantStatus.PERMISSION_DENIED
        const granted = authResult === abilityAccessCtrl.GrantStatus.PERMISSION_GRANTED ||
          this.hasMicrophonePermission()
        this.appendLog(`Microphone permission request result: granted=${granted} auth=${authResult}`)
        return granted
      })
      .catch((error: Error) => {
        this.appendLog(`Microphone permission request failed: ${JSON.stringify(error)}`)
        return false
      })
      .finally(() => {
        this.microphonePermissionInFlight = false
        this.microphonePermissionRequest = null
      })
    return this.microphonePermissionRequest
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
    try {
      const context = getContext(this) as common.UIAbilityContext
      return context.filesDir
    } catch (error) {
      this.appendLog(`App files dir unavailable: ${JSON.stringify(error)}`)
      return ''
    }
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

  private sendWheel(deltaY: number, x: number, y: number): void {
    const flags = deltaY > 0 ?
      PTR_FLAGS_WHEEL | PTR_FLAGS_WHEEL_NEGATIVE | RDP_WHEEL_DELTA_NEGATIVE :
      PTR_FLAGS_WHEEL | RDP_WHEEL_DELTA
    this.sendPointer(flags, x, y, false, 'wheel.vertical')
  }

  private sendHorizontalWheel(deltaX: number, x: number, y: number): void {
    const flags = deltaX > 0 ?
      PTR_FLAGS_HWHEEL | PTR_FLAGS_WHEEL_NEGATIVE | RDP_WHEEL_DELTA_NEGATIVE :
      PTR_FLAGS_HWHEEL | RDP_WHEEL_DELTA
    this.sendPointer(flags, x, y, false, 'wheel.horizontal')
  }

  private sendLeftButtonDown(x: number, y: number, label: string): void {
    this.sendPointer(PTR_FLAGS_MOVE, x, y, false, `${label}.move`)
    this.sendPointer(PTR_FLAGS_BUTTON1 | PTR_FLAGS_DOWN, x, y, false, `${label}.down`)
  }

  private sendLeftButtonUp(x: number, y: number, label: string): void {
    this.sendPointer(PTR_FLAGS_MOVE, x, y, true, `${label}.move`)
    this.sendPointer(PTR_FLAGS_BUTTON1, x, y, true, `${label}.up`)
  }

  private sendLeftClick(x: number, y: number, label: string = 'tap.left'): void {
    this.sendPointer(PTR_FLAGS_MOVE, x, y, false, `${label}.move`)
    this.sendPointer(PTR_FLAGS_BUTTON1 | PTR_FLAGS_DOWN, x, y, false, `${label}.down`)
    this.sendPointer(PTR_FLAGS_BUTTON1, x, y, false, `${label}.up`)
  }

  private sendRightClick(x: number, y: number, label: string = 'longPress.right'): void {
    this.sendPointer(PTR_FLAGS_MOVE, x, y, false, `${label}.move`)
    this.sendPointer(PTR_FLAGS_BUTTON2 | PTR_FLAGS_DOWN, x, y, false, `${label}.down`)
    this.sendPointer(PTR_FLAGS_BUTTON2, x, y, false, `${label}.up`)
  }

  private sendPlatformKeyStroke(keyCode: number): boolean {
    if (this.sendPlatformKeyEvent(keyCode, true)) {
      return this.sendPlatformKeyEvent(keyCode, false)
    }
    return false
  }

  private sendPlatformKeyEvent(keyCode: number, down: boolean, logDisconnected: boolean = true,
    repeat: boolean = false, event?: KeyEvent): boolean {
    if (!this.isConnected()) {
      if (logDisconnected) {
        this.appendLog('Key input skipped: no active FreeRDP session')
      }
      return false
    }

    try {
      const result: NativeCommandResult = rdpNative.sendPlatformKey({
        keyCode: keyCode,
        down: down,
        repeat: repeat,
        ctrl: event ? RdpKeyboard.isCtrlModifierPressed(event) : false,
        shift: event ? RdpKeyboard.isShiftModifierPressed(event) : false,
        alt: event ? RdpKeyboard.isAltModifierPressed(event) : false,
        meta: event ? RdpKeyboard.isWinModifierPressed(event) : false
      })
      if (!result.ok) {
        this.appendLog(result.message)
      }
      return result.ok
    } catch (error) {
      this.appendLog(`Native platform key input failed: ${JSON.stringify(error)}`)
      return false
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

  private sendUnicodeStroke(code: number): boolean {
    if (this.sendUnicodeEvent(code, true)) {
      return this.sendUnicodeEvent(code, false)
    }
    return false
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

  private consumeSuppressedTextInputFragment(value: string): string {
    if (!value.length || this.suppressedTextInputChars.length <= 0) {
      return value
    }

    let remaining = ''
    let suppressed = ''
    for (let i = 0; i < value.length; i++) {
      const char = value.charAt(i)
      const index = this.suppressedTextInputChars.indexOf(char)
      if (index >= 0) {
        this.suppressedTextInputChars.splice(index, 1)
        suppressed += char
      } else {
        remaining += char
      }
    }
    if (suppressed.length > 0) {
      this.logInputEvent('key.hardware',
        `suppressed TextInput echo chars=${suppressed.length} remaining=${remaining.length}`, true)
    }
    return remaining
  }

  private handleInputBridgeWillInsert(value: InsertValue): boolean {
    const insertedText = value.insertValue
    if (!insertedText.length) {
      return true
    }
    const remaining = this.consumeSuppressedTextInputFragment(insertedText)
    return remaining.length === insertedText.length
  }

  private handleInputBridgeWillDelete(value: DeleteValue): boolean {
    const deleteCount = Math.max(1, value.deleteValue.length)
    for (let i = 0; i < deleteCount; i++) {
      this.sendPlatformKeyStroke(OH_KEYCODE_DEL)
    }
    this.logInputEvent('key.delete',
      `TextInput delete routed direction=${value.direction} count=${deleteCount}`,
      true)
    return false
  }

  private handleRemoteTextInputChange(value: string): void {
    if (!value.length && Date.now() <= this.remoteTextBridgeClearUntilMs) {
      this.remoteTextBridgeValue = ''
      this.remoteTextInput = ''
      return
    }

    const oldValue = this.remoteTextBridgeValue
    if (value === oldValue) {
      return
    }

    let prefixLength = 0
    while (prefixLength < oldValue.length && prefixLength < value.length &&
      oldValue.charAt(prefixLength) === value.charAt(prefixLength)) {
      prefixLength++
    }

    let suffixLength = 0
    while (suffixLength < oldValue.length - prefixLength &&
      suffixLength < value.length - prefixLength &&
      oldValue.charAt(oldValue.length - 1 - suffixLength) === value.charAt(value.length - 1 - suffixLength)) {
      suffixLength++
    }

    const deleteCount = oldValue.length - prefixLength - suffixLength
    const insertedText = value.substring(prefixLength, value.length - suffixLength)
    const remoteDeleteCount = deleteCount
    const remoteInsertedText = this.consumeSuppressedTextInputFragment(insertedText)

    for (let i = 0; i < remoteDeleteCount; i++) {
      this.sendPlatformKeyStroke(OH_KEYCODE_DEL)
    }
    this.sendRemoteTextFragment(remoteInsertedText)

    this.remoteTextBridgeValue = value
    this.remoteTextInput = value
    this.resetRemoteTextBridgeIfNeeded()
  }

  private resetRemoteTextBridgeIfNeeded(): void {
    if (this.remoteTextBridgeValue.length <= REMOTE_TEXT_BRIDGE_RESET_LENGTH) {
      return
    }
    this.remoteTextBridgeClearUntilMs = Date.now() + REMOTE_TEXT_BRIDGE_CLEAR_GUARD_MS
    this.remoteTextBridgeValue = ''
    this.remoteTextInput = ''
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
    this.remoteTextBridgeValue = ''
    this.remoteTextInput = ''
    this.suppressedTextInputChars = []
  }

  private clearPointerState(): void {
    this.cancelLongPressTimer()
    this.pendingTouchActive = false
    this.longPressTriggered = false
    this.pointerDown = false
    this.twoFingerScrollActive = false
    this.touchGestureSuppressed = false
    this.mouseButtonFlags = 0
    this.lastTapAtMs = 0
  }

  private releasePointerButtons(): void {
    if (!this.isConnected()) {
      this.clearPointerState()
      return
    }
    if (this.pointerDown) {
      this.sendLeftButtonUp(this.lastTouchX, this.lastTouchY, 'touch.releaseOnExit')
    }
    if (this.mouseButtonFlags !== 0) {
      this.sendPointer(PTR_FLAGS_MOVE | this.mouseButtonFlags, this.lastTouchX, this.lastTouchY, true)
    }
    this.clearPointerState()
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

  private addSuppressedTextInputText(value: string): void {
    if (!value.length) {
      return
    }
    for (let i = 0; i < value.length; i++) {
      const char = value.charAt(i)
      this.suppressedTextInputChars.push(char)
      const lower = char.toLowerCase()
      const upper = char.toUpperCase()
      if (lower !== char) {
        this.suppressedTextInputChars.push(lower)
      }
      if (upper !== char && upper !== lower) {
        this.suppressedTextInputChars.push(upper)
      }
    }
    while (this.suppressedTextInputChars.length > 32) {
      this.suppressedTextInputChars.shift()
    }
  }

  private rememberHardwareTextInputSuppression(event: KeyEvent): void {
    if (event.type !== KeyType.Down) {
      return
    }
    this.addSuppressedTextInputText(RdpKeyboard.getHardwarePrintableText(event))
    this.addSuppressedTextInputText(RdpKeyboard.getPrintableTextByKeyCode(event.keyCode))
  }

  private keyEventId(event: KeyEvent): string {
    return `${event.timestamp}:${event.keyCode}:${event.type}`
  }

  private rememberDispatchedRemoteKeyEvent(event: KeyEvent): void {
    this.dispatchedRemoteKeyEvents.push(this.keyEventId(event))
    if (this.dispatchedRemoteKeyEvents.length > 32) {
      this.dispatchedRemoteKeyEvents.shift()
    }
  }

  private consumeDispatchedRemoteKeyEvent(event: KeyEvent): boolean {
    const index = this.dispatchedRemoteKeyEvents.indexOf(this.keyEventId(event))
    if (index < 0) {
      return false
    }
    this.dispatchedRemoteKeyEvents.splice(index, 1)
    return true
  }

  private hasDispatchedRemoteKeyEvent(event: KeyEvent): boolean {
    return this.dispatchedRemoteKeyEvents.indexOf(this.keyEventId(event)) >= 0
  }

  private getEventUnicode(event: KeyEvent): number {
    if (event.unicode && event.unicode > 0) {
      return event.unicode
    }
    if (event.keyText.length === 1) {
      return event.keyText.charCodeAt(0)
    }
    return 0
  }

  private rememberInputBridgePrintableKey(event: KeyEvent): void {
    if (event.type === KeyType.Down) {
      this.bridgePrintableKeyCode = event.keyCode
      this.bridgePrintableKeyAtMs = Date.now()
    }
  }

  private shouldSkipBridgedPrintableKey(event: KeyEvent, printable: boolean, shortcut: boolean): boolean {
    if (event.type !== KeyType.Down || !printable || shortcut) {
      return false
    }
    if (event.keyCode !== this.bridgePrintableKeyCode) {
      return false
    }
    if (Date.now() - this.bridgePrintableKeyAtMs > BRIDGE_PRINTABLE_KEY_GUARD_MS) {
      return false
    }
    this.bridgePrintableKeyCode = 0
    this.bridgePrintableKeyAtMs = 0
    return true
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

  private shouldDispatchRemoteKey(event: KeyEvent): boolean {
    if (!this.shouldRouteRemoteKeyboard() || !this.isConnected()) {
      return false
    }
    if (!RdpKeyboard.isPlatformKeyCode(event.keyCode)) {
      return false
    }
    const printable = RdpKeyboard.isPrintableKeyCode(event.keyCode)
    const hardwarePrintable = printable && RdpKeyboard.shouldSendPrintableAsHardwareKey(event.keyCode)
    return !printable || hardwarePrintable || RdpKeyboard.hasShortcutModifier(event)
  }

  private handleRemoteKeyDispatch(event: KeyEvent, fromInputBridge: boolean): boolean {
    if (!this.shouldRouteRemoteKeyboard()) {
      return false
    }
    if (this.hasDispatchedRemoteKeyEvent(event)) {
      this.stopKeyPropagation(event)
      return true
    }
    if (!this.shouldDispatchRemoteKey(event)) {
      return false
    }
    const sent = this.handleSurfaceKeyEvent(event, fromInputBridge)
    if (!sent) {
      return false
    }
    this.rememberDispatchedRemoteKeyEvent(event)
    this.stopKeyPropagation(event)
    return true
  }

  private handleInputBridgeKeyEvent(event: KeyEvent): boolean {
    if (!this.shouldRouteRemoteKeyboard()) {
      return false
    }
    if (this.consumeDispatchedRemoteKeyEvent(event)) {
      return true
    }
    const printable = RdpKeyboard.isPrintableKeyCode(event.keyCode)
    const shortcut = RdpKeyboard.hasShortcutModifier(event)
    const hardwarePrintable = printable && RdpKeyboard.shouldSendPrintableAsHardwareKey(event.keyCode)
    if (printable && !shortcut && !hardwarePrintable) {
      this.rememberInputBridgePrintableKey(event)
      return false
    }
    return this.handleSurfaceKeyEvent(event, true)
  }

  private handleInputBridgePreImeKeyEvent(event: KeyEvent): boolean {
    if (!this.shouldRouteRemoteKeyboard()) {
      return false
    }
    const printable = RdpKeyboard.isPrintableKeyCode(event.keyCode)
    const systemShortcutModifier = RdpKeyboard.isCtrlModifierPressed(event) ||
      RdpKeyboard.isAltModifierPressed(event) || RdpKeyboard.isWinModifierPressed(event)
    if (printable && !systemShortcutModifier) {
      return false
    }
    return this.handleRemoteKeyDispatch(event, true)
  }

  private handleSurfaceKeyEvent(event: KeyEvent, fromInputBridge: boolean = false): boolean {
    if (!this.shouldRouteRemoteKeyboard()) {
      return false
    }
    if (!fromInputBridge && this.consumeDispatchedRemoteKeyEvent(event)) {
      return true
    }
    if (!this.isConnected()) {
      return false
    }
    const down = event.type === KeyType.Down
    const printable = RdpKeyboard.isPrintableKeyCode(event.keyCode)
    const hardwarePrintable = printable && RdpKeyboard.shouldSendPrintableAsHardwareKey(event.keyCode)
    const shortcut = RdpKeyboard.hasShortcutModifier(event)
    const platformKey = RdpKeyboard.isPlatformKeyCode(event.keyCode)
    const surfacePrintableKeyEvent = printable && !fromInputBridge
    if (!printable || shortcut || hardwarePrintable || RdpKeyboard.isFunctionKey(event.keyCode)) {
      this.logInputEvent('key',
        `code=${event.keyCode} type=${event.type} platform=${platformKey} printable=${printable} ` +
        `shortcut=${shortcut} hardware=${hardwarePrintable} bridge=${fromInputBridge} ` +
        `ctrl=${RdpKeyboard.isCtrlModifierPressed(event)} alt=${RdpKeyboard.isAltModifierPressed(event)} ` +
        `win=${RdpKeyboard.isWinModifierPressed(event)} shift=${RdpKeyboard.isShiftModifierPressed(event)}`,
        true)
    }
    if (!fromInputBridge && this.shouldSkipBridgedPrintableKey(event, printable, shortcut)) {
      return true
    }

    if (platformKey && (!printable || shortcut || hardwarePrintable || surfacePrintableKeyEvent)) {
      const sent = this.sendPlatformKeyEvent(event.keyCode, down, false, false, event)
      if (sent && hardwarePrintable) {
        this.rememberHardwareTextInputSuppression(event)
      }
      if (sent && RdpKeyboard.isDeleteKeyCode(event.keyCode)) {
        this.logInputEvent('key.delete',
          `hardware keyCode=${event.keyCode} down=${down} sent=${sent}`,
          true)
      }
      return sent
    }

    if (!down) {
      return printable || platformKey
    }

    const unicode = RdpKeyboard.getEventUnicode(event)
    if (unicode > 0 && unicode <= 0xFFFF) {
      this.sendUnicodeStroke(unicode)
      return true
    }

    return platformKey
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

  private getPrimaryPressedMouseButtonFlag(flags: number): number {
    if ((flags & PTR_FLAGS_BUTTON1) !== 0) {
      return PTR_FLAGS_BUTTON1
    }
    if ((flags & PTR_FLAGS_BUTTON2) !== 0) {
      return PTR_FLAGS_BUTTON2
    }
    if ((flags & PTR_FLAGS_BUTTON3) !== 0) {
      return PTR_FLAGS_BUTTON3
    }
    return 0
  }

  private isSyntheticMouseAfterTouch(): boolean {
    return Date.now() - this.lastTouchEventAtMs < SYNTHETIC_MOUSE_SUPPRESS_MS
  }

  private handleSurfaceMouse(event: MouseEvent): void {
    const rawButtonFlag = this.mouseButtonToPointerFlag(event.button)
    const pressedFlags = this.getPressedMouseButtonFlags(event)
    const pressedButtonFlag = this.getPrimaryPressedMouseButtonFlag(pressedFlags)
    const buttonFlag = rawButtonFlag !== 0 ? rawButtonFlag : pressedButtonFlag
    this.logInputEvent('mouse',
      `action=${event.action} button=${event.button} rawFlag=${this.formatPointerFlags(rawButtonFlag)} ` +
      `pressed=${this.formatPointerFlags(pressedFlags)} x=${this.formatNumber(event.x)} y=${this.formatNumber(event.y)}`)

    if (!this.isConnected()) {
      if (this.pointerMappingFailureShouldLog('')) {
        this.appendLog('Mouse input skipped: no active FreeRDP session')
      }
      return
    }
    if ((event.action === MouseAction.Press || event.action === MouseAction.Release) &&
      this.isSyntheticMouseAfterTouch()) {
      this.logInputEvent('mouse.synthetic', `suppressed action=${event.action}`, true)
      return
    }
    this.lastTouchX = event.x
    this.lastTouchY = event.y
    if (event.action === MouseAction.Press) {
      this.focusRemoteInputBridge()
      this.logPointerDiagnostic('mouse.press', PTR_FLAGS_MOVE | buttonFlag | PTR_FLAGS_DOWN, event.x, event.y,
        PointerMapper.mapToDesktop(this.getPointerViewport(), event.x, event.y, false))
      if (buttonFlag === 0 ||
        !PointerMapper.isPointInRemoteViewport(this.getPointerViewport(), event.x, event.y)) {
        this.appendLog(`Mouse press skipped: button=${buttonFlag} inside=${PointerMapper.isPointInRemoteViewport(this.getPointerViewport(), event.x, event.y)}`)
        return
      }
      this.mouseButtonFlags |= buttonFlag
      this.sendPointer(PTR_FLAGS_MOVE, event.x, event.y, false, 'mouse.press.move')
      this.sendPointer(buttonFlag | PTR_FLAGS_DOWN, event.x, event.y, false, 'mouse.press.down')
      return
    }
    if (event.action === MouseAction.Release) {
      const releaseFlag = buttonFlag !== 0 ? buttonFlag : this.mouseButtonFlags
      if (releaseFlag !== 0) {
        this.sendPointer(PTR_FLAGS_MOVE, event.x, event.y, true, 'mouse.release.move')
        this.sendPointer(releaseFlag, event.x, event.y, true, 'mouse.release.up')
        this.mouseButtonFlags &= ~releaseFlag
      }
      return
    }
    if (event.action === MouseAction.Move || event.action === MouseAction.Hover) {
      if (pressedFlags !== 0) {
        this.sendPointer(PTR_FLAGS_MOVE | pressedFlags | PTR_FLAGS_DOWN, event.x, event.y, true, 'mouse.drag.move')
      } else {
        this.sendPointer(PTR_FLAGS_MOVE, event.x, event.y)
      }
      return
    }
    if (event.action === MouseAction.CANCEL) {
      if (this.mouseButtonFlags !== 0) {
        this.sendPointer(PTR_FLAGS_MOVE, event.x, event.y, true, 'mouse.cancel.move')
        this.sendPointer(this.mouseButtonFlags, event.x, event.y, true, 'mouse.cancel.release')
      }
      this.mouseButtonFlags = 0
    }
  }

  private handleSurfaceAxis(event: AxisEvent): void {
    const deltaY = event.getVerticalAxisValue()
    const deltaX = event.getHorizontalAxisValue()
    if (this.shouldSkipDuplicateAxisEvent(event, deltaX, deltaY)) {
      return
    }
    const axisX = PointerMapper.isPointInRemoteViewport(this.getPointerViewport(), event.x, event.y) ?
      event.x : this.lastTouchX
    const axisY = PointerMapper.isPointInRemoteViewport(this.getPointerViewport(), event.x, event.y) ?
      event.y : this.lastTouchY
    this.logInputEvent('axis',
      `action=${event.action} dx=${this.formatNumber(deltaX)} dy=${this.formatNumber(deltaY)} ` +
      `x=${this.formatNumber(event.x)} y=${this.formatNumber(event.y)} target=${this.formatNumber(axisX)},${this.formatNumber(axisY)}`)

    if (!this.isConnected() || event.action !== AxisAction.UPDATE) {
      return
    }

    if (Math.abs(deltaY) >= Math.abs(deltaX) && Math.abs(deltaY) > 0) {
      this.sendWheel(deltaY, axisX, axisY)
      return
    }
    if (Math.abs(deltaX) > 0) {
      this.sendHorizontalWheel(deltaX, axisX, axisY)
    }
  }

  private startPendingTouch(x: number, y: number): void {
    this.logPointerDiagnostic('touch.down', PTR_FLAGS_MOVE, x, y,
      PointerMapper.mapToDesktop(this.getPointerViewport(), x, y, false))
    if (!PointerMapper.isPointInRemoteViewport(this.getPointerViewport(), x, y)) {
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
    this.logInputEvent('touch.drag', `start ${this.formatNumber(this.pendingTouchStartX)},${this.formatNumber(this.pendingTouchStartY)} -> ${this.formatNumber(x)},${this.formatNumber(y)}`, true)
    this.sendLeftButtonDown(this.pendingTouchStartX, this.pendingTouchStartY, 'touch.drag.start')
    this.sendPointer(PTR_FLAGS_MOVE | PTR_FLAGS_BUTTON1 | PTR_FLAGS_DOWN, x, y, true, 'touch.drag.move')
  }

  private isDoubleTap(x: number, y: number): boolean {
    const now = Date.now()
    if (now - this.lastTapAtMs > DOUBLE_TAP_TIMEOUT_MS) {
      return false
    }

    const dx = x - this.lastTapX
    const dy = y - this.lastTapY
    return Math.abs(dx) <= DOUBLE_TAP_DISTANCE && Math.abs(dy) <= DOUBLE_TAP_DISTANCE
  }

  private recordTap(x: number, y: number): void {
    this.lastTapAtMs = Date.now()
    this.lastTapX = x
    this.lastTapY = y
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
        if (this.isDoubleTap(x, y)) {
          this.appendLog(`Touch double tap: event=${this.formatNumber(x)},${this.formatNumber(y)}`)
          this.lastTapAtMs = 0
          this.sendLeftClick(x, y, 'tap.double')
        } else {
          this.recordTap(x, y)
          this.sendLeftClick(x, y)
        }
      }
      return
    }

    if (this.pointerDown) {
      this.pointerDown = false
      this.sendLeftButtonUp(x, y, 'touch.drag.release')
    }
  }

  private handleSurfaceTouch(event: TouchEvent): void {
    this.lastTouchEventAtMs = Date.now()
    const touchCount = event.touches.length
    const firstX = touchCount > 0 ? event.touches[0].x : this.lastTouchX
    const firstY = touchCount > 0 ? event.touches[0].y : this.lastTouchY
    this.logInputEvent('touch',
      `type=${event.type} count=${touchCount} x=${this.formatNumber(firstX)} y=${this.formatNumber(firstY)} ` +
      `pending=${this.pendingTouchActive} down=${this.pointerDown}`)
    if (this.touchGestureSuppressed && event.touches.length < 2) {
      if (event.type === TouchType.Up || event.type === TouchType.Cancel || event.touches.length === 0) {
        this.touchGestureSuppressed = false
      }
      this.cancelPendingTouch()
      return
    }

    if (this.twoFingerScrollActive && event.touches.length < 2) {
      this.twoFingerScrollActive = false
      this.touchGestureSuppressed = true
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
      this.focusRemoteInputBridge()
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
        this.sendPointer(PTR_FLAGS_MOVE | PTR_FLAGS_BUTTON1 | PTR_FLAGS_DOWN, x, y, true, 'touch.drag.move')
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
      this.sendLeftButtonUp(firstTouch.x, firstTouch.y, 'touch.scroll.releaseDrag')
    }

    if (event.type === TouchType.Down || !this.twoFingerScrollActive) {
      this.twoFingerScrollActive = true
      this.touchGestureSuppressed = true
      this.twoFingerLastX = centerX
      this.twoFingerLastY = centerY
      this.logInputEvent('touch.scroll',
        `start center=${this.formatNumber(centerX)},${this.formatNumber(centerY)}`, true)
      return true
    }

    if (event.type === TouchType.Move) {
      const deltaX = centerX - this.twoFingerLastX
      const deltaY = centerY - this.twoFingerLastY
      if (Math.abs(deltaY) >= TOUCH_WHEEL_THRESHOLD || Math.abs(deltaX) >= TOUCH_WHEEL_THRESHOLD) {
        this.logInputEvent('touch.scroll',
          `dx=${this.formatNumber(deltaX)} dy=${this.formatNumber(deltaY)} ` +
          `center=${this.formatNumber(centerX)},${this.formatNumber(centerY)}`, true)
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

  private shouldSkipDuplicateAxisEvent(event: AxisEvent, deltaX: number, deltaY: number): boolean {
    const now = Date.now()
    const duplicate = now - this.lastAxisEventAtMs <= AXIS_EVENT_DEDUP_MS &&
      this.lastAxisAction === event.action &&
      Math.abs(this.lastAxisX - event.x) < 0.01 &&
      Math.abs(this.lastAxisY - event.y) < 0.01 &&
      Math.abs(this.lastAxisDeltaX - deltaX) < 0.01 &&
      Math.abs(this.lastAxisDeltaY - deltaY) < 0.01
    this.lastAxisEventAtMs = now
    this.lastAxisAction = event.action
    this.lastAxisX = event.x
    this.lastAxisY = event.y
    this.lastAxisDeltaX = deltaX
    this.lastAxisDeltaY = deltaY
    return duplicate
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
            this.handleSurfaceAxis(event)
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
          .onWillDelete((value: DeleteValue): boolean => {
            return this.handleInputBridgeWillDelete(value)
          })
          .onChange((value: string) => {
            this.handleRemoteTextInputChange(value)
          })
          .onKeyPreIme((event: KeyEvent): boolean => {
            return this.handleInputBridgePreImeKeyEvent(event)
          })
          .onKeyEvent((event: KeyEvent): boolean => {
            return this.handleInputBridgeKeyEvent(event)
          })
          .onKeyEventDispatch((event: KeyEvent): boolean => {
            return this.handleRemoteKeyDispatch(event, true)
          })
          .onAxisEvent((event: AxisEvent) => {
            this.handleSurfaceAxis(event)
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
        this.handleSurfaceTouch(event)
      })
      .onKeyEvent((event: KeyEvent): boolean => {
        return this.handleSurfaceKeyEvent(event)
      })
      .onKeyEventDispatch((event: KeyEvent): boolean => {
        return this.handleRemoteKeyDispatch(event, false)
      })
      .onMouse((event: MouseEvent) => {
        this.handleSurfaceMouse(event)
      })
      .onAxisEvent((event: AxisEvent) => {
        this.handleSurfaceAxis(event)
      })
    }
    .layoutWeight(1)
    .focusable(true)
    .onKeyEvent((event: KeyEvent): boolean => {
      return this.handleSurfaceKeyEvent(event)
    })
    .onKeyEventDispatch((event: KeyEvent): boolean => {
      return this.handleRemoteKeyDispatch(event, false)
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
