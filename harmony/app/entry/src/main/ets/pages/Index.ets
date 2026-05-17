import { abilityAccessCtrl, bundleManager, common, Permissions } from '@kit.AbilityKit'
import { hilog } from '@kit.PerformanceAnalysisKit'
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
const DOUBLE_TAP_TIMEOUT_MS: number = 320
const DOUBLE_TAP_DISTANCE: number = 24
const POINTER_FAILURE_LOG_INTERVAL_MS: number = 800
const INPUT_EVENT_LOG_INTERVAL_MS: number = 650
const SYNTHETIC_MOUSE_SUPPRESS_MS: number = 80
const AXIS_EVENT_DEDUP_MS: number = 12
const INPUT_PROBE_DELAY_MS: number = 200
const INPUT_PROBE_INTERVAL_MS: number = 350
const INPUT_PROBE_ON_INPUT: boolean = false
const INPUT_EVENT_UI_LOG_ENABLED: boolean = false
const INPUT_EVENT_VERBOSE_LOG_ENABLED: boolean = false
// Real-device key callback echo was observed under 30ms; keep a wider window without masking fast typing.
const BRIDGE_PRINTABLE_KEY_GUARD_MS: number = 120
const REMOTE_TEXT_BRIDGE_RESET_LENGTH: number = 32
const REMOTE_TEXT_BRIDGE_CLEAR_GUARD_MS: number = 500
const HILOG_DOMAIN: number = 0xF3D1
const HILOG_TAG: string = 'FreeRDPUi'
const REMOTE_INPUT_BRIDGE_ID: string = 'rdpRemoteInputBridge'
const REMOTE_SURFACE_FOCUS_ID: string = 'rdpRemoteSurface'

const RDP_SCANCODE_ESCAPE: number = 0x01
const RDP_SCANCODE_BACKSPACE: number = 0x0E
const RDP_SCANCODE_TAB: number = 0x0F
const RDP_SCANCODE_ENTER: number = 0x1C
const RDP_SCANCODE_LCONTROL: number = 0x1D
const RDP_SCANCODE_RCONTROL: number = 0x011D
const RDP_SCANCODE_A: number = 0x1E
const RDP_SCANCODE_C: number = 0x2E
const RDP_SCANCODE_V: number = 0x2F
const RDP_SCANCODE_X: number = 0x2D
const RDP_SCANCODE_LMENU: number = 0x38
const RDP_SCANCODE_RMENU: number = 0x0138
const RDP_SCANCODE_LWIN: number = 0x015B
const RDP_SCANCODE_RWIN: number = 0x015C
const RDP_SCANCODE_LSHIFT: number = 0x2A
const RDP_SCANCODE_RSHIFT: number = 0x36
const RDP_SCANCODE_INSERT: number = 0x0152
const RDP_SCANCODE_DELETE: number = 0x0153
const RDP_SCANCODE_UP: number = 0x0148
const RDP_SCANCODE_DOWN: number = 0x0150
const RDP_SCANCODE_LEFT: number = 0x014B
const RDP_SCANCODE_RIGHT: number = 0x014D
const RDP_SCANCODE_HOME: number = 0x0147
const RDP_SCANCODE_END: number = 0x014F
const RDP_SCANCODE_PAGE_UP: number = 0x0149
const RDP_SCANCODE_PAGE_DOWN: number = 0x0151
const RDP_SCANCODE_F1: number = 0x3B
const RDP_SCANCODE_F10: number = 0x44
const RDP_SCANCODE_F11: number = 0x57
const RDP_SCANCODE_F12: number = 0x58

const OH_KEYCODE_0: number = 2000
const OH_KEYCODE_9: number = 2009
const OH_KEYCODE_DPAD_UP: number = 2012
const OH_KEYCODE_DPAD_DOWN: number = 2013
const OH_KEYCODE_DPAD_LEFT: number = 2014
const OH_KEYCODE_DPAD_RIGHT: number = 2015
const OH_KEYCODE_A: number = 2017
const OH_KEYCODE_C: number = OH_KEYCODE_A + 2
const OH_KEYCODE_V: number = OH_KEYCODE_A + 21
const OH_KEYCODE_X: number = OH_KEYCODE_A + 23
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
const OH_KEYCODE_PAGE_UP: number = 2068
const OH_KEYCODE_PAGE_DOWN: number = 2069
const OH_KEYCODE_ESCAPE: number = 2070
const OH_KEYCODE_FORWARD_DEL: number = 2071
const OH_KEYCODE_CTRL_LEFT: number = 2072
const OH_KEYCODE_CTRL_RIGHT: number = 2073
const OH_KEYCODE_META_LEFT: number = 2076
const OH_KEYCODE_META_RIGHT: number = 2077
const OH_KEYCODE_MOVE_HOME: number = 2081
const OH_KEYCODE_MOVE_END: number = 2082
const OH_KEYCODE_INSERT: number = 2083
const OH_KEYCODE_F1: number = 2090
const OH_KEYCODE_F12: number = 2101
const OH_KEYCODE_NUMPAD_0: number = 2103
const OH_KEYCODE_NUMPAD_9: number = 2112
const OH_KEYCODE_NUMPAD_DIVIDE: number = 2113
const OH_KEYCODE_NUMPAD_MULTIPLY: number = 2114
const OH_KEYCODE_NUMPAD_SUBTRACT: number = 2115
const OH_KEYCODE_NUMPAD_ADD: number = 2116
const OH_KEYCODE_NUMPAD_DOT: number = 2117
const OH_KEYCODE_NUMPAD_ENTER: number = 2119
const READ_PASTEBOARD_PERMISSION: Permissions = 'ohos.permission.READ_PASTEBOARD'

interface NativeProbeResult {
  bridgeVersion: string
  abi: string
  freeRdpVersion: string
  winprVersion: string
  opensslVersion: string
  featureSummary: string
  audioStats: string
  renderStats: string
  graphicsStats: string
  inputDispatchMode: string
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
  inputQueueDepth: number
  inputQueuedCount: number
  inputSentCount: number
  inputDroppedCount: number
  probeJson: string
  probeError: string
  logs: string[]
}

interface PointerMapping {
  ok: boolean
  x: number
  y: number
  surfaceX: number
  surfaceY: number
  scaleX: number
  scaleY: number
  viewportX: number
  viewportY: number
  viewportWidth: number
  viewportHeight: number
  desktopWidth: number
  desktopHeight: number
  reason: string
}

interface NativeCommandResult {
  ok: boolean
  state: string
  message: string
  logs: string[]
}

interface ShortcutSyntheticModifier {
  keyCode: number
  scancode: number
}

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
  @State private nativeProbeText: string = 'Native bridge not probed'
  @State private ctrlModifierDown: boolean = false
  @State private altModifierDown: boolean = false
  @State private winModifierDown: boolean = false
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
  @State private inputQueueDepth: number = 0
  @State private inputQueuedCount: number = 0
  @State private inputSentCount: number = 0
  @State private inputDroppedCount: number = 0
  @State private nativeFeatureSummary: string = ''
  @State private nativeInputDispatchMode: string = ''
  @State private remoteTextInput: string = ''
  private rightCtrlModifierDown: boolean = false
  private rightAltModifierDown: boolean = false
  private rightWinModifierDown: boolean = false
  private leftShiftModifierDown: boolean = false
  private rightShiftModifierDown: boolean = false
  private remoteTextBridgeValue: string = ''
  private bridgePrintableKeyCode: number = 0
  private bridgePrintableKeyAtMs: number = 0
  private remoteShortcutPrintableKeysDown: number[] = []
  private shortcutSyntheticModifiers: ShortcutSyntheticModifier[] = []
  private dispatchedRemoteKeyEvents: string[] = []
  private remotePressedScancodes: number[] = []
  private suppressedTextInputChars: string[] = []
  private remoteTextBridgeClearUntilMs: number = 0
  private callbacksRegistered: boolean = false
  private pasteboardPermissionRequested: boolean = false
  private pasteboardPermissionInFlight: boolean = false
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
  private lastInputProbeAtMs: number = 0
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
            this.probeNative()
            this.focusRemoteInputBridge()
          } else if (nextState !== 'Connected') {
            this.clearModifierState()
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

  private probeNative(appendLogs: boolean = true): void {
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
      this.inputQueueDepth = result.inputQueueDepth
      this.inputQueuedCount = result.inputQueuedCount
      this.inputSentCount = result.inputSentCount
      this.inputDroppedCount = result.inputDroppedCount
      this.nativeFeatureSummary = result.featureSummary
      this.nativeInputDispatchMode = result.inputDispatchMode
      const linkState = result.freeRdpLinked ? 'linked' : 'not-linked'
      const surfaceText = result.surfaceReady ? `surface ${result.surfaceWidth}x${result.surfaceHeight}` : 'surface pending'
      const viewportText = result.surfaceViewportWidth > 0 && result.surfaceViewportHeight > 0 ?
        `viewport ${result.surfaceViewportX},${result.surfaceViewportY} ${result.surfaceViewportWidth}x${result.surfaceViewportHeight}` :
        'viewport pending'
      const layoutText = this.surfaceLayoutWidth > 0 && this.surfaceLayoutHeight > 0 ?
        `layout ${this.formatNumber(this.surfaceLayoutWidth)}x${this.formatNumber(this.surfaceLayoutHeight)}` :
        'layout pending'
      const desktopText = result.desktopWidth > 0 && result.desktopHeight > 0 ?
        `desktop ${result.desktopWidth}x${result.desktopHeight}` : 'desktop pending'
      const paintText = result.surfacePaintCount > 0 ? `paint ${result.surfacePaintCount}` : 'paint pending'
      const renderText = result.renderStats
      const graphicsText = result.graphicsStats
      const inputText = `input ${result.inputDispatchMode} q=${result.inputQueueDepth} sent=${result.inputSentCount} drop=${result.inputDroppedCount}`
      const audioText = result.audioStats.replace('OHAudio stats: ', 'audio ')
      this.nativeProbeText = `bridge ${result.bridgeVersion} | ${result.abi} | FreeRDP ${result.freeRdpVersion} | ${linkState} | ${surfaceText} | ${layoutText} | ${viewportText} | ${desktopText} | ${paintText} | ${renderText} | ${graphicsText} | ${inputText} | ${audioText}`
      if (appendLogs) {
        this.appendNativeLogs(result.logs)
      }
    } catch (error) {
      this.nativeProbeText = 'Native bridge probe failed'
      this.appendLog(`Native probe failed: ${JSON.stringify(error)}`)
    }
  }

  private scheduleInputProbe(): void {
    const now = Date.now()
    if (now - this.lastInputProbeAtMs < INPUT_PROBE_INTERVAL_MS) {
      return
    }
    this.lastInputProbeAtMs = now
    setTimeout((): void => {
      this.probeNative(false)
      this.appendLog(`Input counters: q=${this.inputQueueDepth} sent=${this.inputSentCount} drop=${this.inputDroppedCount}`)
    }, INPUT_PROBE_DELAY_MS)
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

  private areaLengthToNumber(value: Length): number {
    if (typeof value === 'number') {
      return value
    }

    const text = `${value}`
    const unitIndex = text.search(/[a-zA-Z%]/)
    const numericText = unitIndex >= 0 ? text.substring(0, unitIndex) : text
    const numeric = Number(numericText)
    return numeric > 0 ? numeric : 0
  }

  private updateSurfaceLayout(width: Length, height: Length): void {
    const nextWidth = this.areaLengthToNumber(width)
    const nextHeight = this.areaLengthToNumber(height)
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

  private getPointerScaleX(): number {
    if (this.surfaceLayoutWidth > 0 && this.surfacePixelWidth > 0) {
      return this.surfacePixelWidth / this.surfaceLayoutWidth
    }
    return 1
  }

  private getPointerScaleY(): number {
    if (this.surfaceLayoutHeight > 0 && this.surfacePixelHeight > 0) {
      return this.surfacePixelHeight / this.surfaceLayoutHeight
    }
    return 1
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
    if (INPUT_EVENT_UI_LOG_ENABLED) {
      this.appendLog(line)
    }
    const shouldLogVerbose = INPUT_EVENT_VERBOSE_LOG_ENABLED ||
      label === 'key.shortcut' ||
      label === 'paste' ||
      label === 'copy' ||
      label === 'cut' ||
      label === 'key.delete' ||
      label === 'key.repeat'
    if (shouldLogVerbose) {
      hilog.info(HILOG_DOMAIN, HILOG_TAG, '%{public}s', line)
      console.info(line)
    }
  }

  private sendPointer(flags: number, x: number, y: number, allowClamp: boolean = false,
    traceLabel: string = '', logSuccess: boolean = false): boolean {
    if (!this.isConnected()) {
      if (this.pointerMappingFailureShouldLog(traceLabel)) {
        this.appendLog(`Pointer ${traceLabel || 'input'} skipped: no active FreeRDP session`)
      }
      return false
    }

    try {
      const mapped = this.mapPointerToDesktop(x, y, allowClamp)
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
        if (INPUT_PROBE_ON_INPUT) {
          this.scheduleInputProbe()
        }
        return false
      }
      if (logSuccess && INPUT_EVENT_UI_LOG_ENABLED) {
        this.appendLog(`Pointer ${traceLabel || 'input'} queued: ${this.describePointerMapping(flags, x, y, mapped)}`)
      }
      if (INPUT_PROBE_ON_INPUT) {
        this.scheduleInputProbe()
      }
      return result.ok
    } catch (error) {
      this.appendLog(`Native pointer input failed (${traceLabel || 'input'}): ${JSON.stringify(error)}`)
      if (INPUT_PROBE_ON_INPUT) {
        this.scheduleInputProbe()
      }
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
    const parsedWidth = this.parseResolutionPart(0)
    if (parsedWidth > 0) {
      return parsedWidth
    }
    return this.getRemoteViewportWidth()
  }

  private getRequestedDesktopHeight(): number {
    if (this.desktopPixelHeight > 0) {
      return this.desktopPixelHeight
    }
    const parsedHeight = this.parseResolutionPart(1)
    if (parsedHeight > 0) {
      return parsedHeight
    }
    return this.getRemoteViewportHeight()
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
    const surfaceX = x * this.getPointerScaleX()
    const surfaceY = y * this.getPointerScaleY()
    const viewportX = this.getRemoteViewportX()
    const viewportY = this.getRemoteViewportY()
    const viewportWidth = this.getRemoteViewportWidth()
    const viewportHeight = this.getRemoteViewportHeight()
    if (viewportWidth <= 0 || viewportHeight <= 0) {
      return false
    }
    return surfaceX >= viewportX && surfaceX < viewportX + viewportWidth &&
      surfaceY >= viewportY && surfaceY < viewportY + viewportHeight
  }

  private mapPointerToDesktop(x: number, y: number, allowClamp: boolean): PointerMapping {
    const scaleX = this.getPointerScaleX()
    const scaleY = this.getPointerScaleY()
    const surfaceX = x * scaleX
    const surfaceY = y * scaleY
    const viewportX = this.getRemoteViewportX()
    const viewportY = this.getRemoteViewportY()
    const viewportWidth = this.getRemoteViewportWidth()
    const viewportHeight = this.getRemoteViewportHeight()
    const desktopWidth = this.getRequestedDesktopWidth()
    const desktopHeight = this.getRequestedDesktopHeight()
    const base: PointerMapping = {
      ok: false,
      x: 0,
      y: 0,
      surfaceX: surfaceX,
      surfaceY: surfaceY,
      scaleX: scaleX,
      scaleY: scaleY,
      viewportX: viewportX,
      viewportY: viewportY,
      viewportWidth: viewportWidth,
      viewportHeight: viewportHeight,
      desktopWidth: desktopWidth,
      desktopHeight: desktopHeight,
      reason: ''
    }
    if (viewportWidth <= 0 || viewportHeight <= 0 || desktopWidth <= 0 || desktopHeight <= 0) {
      base.reason = 'surface viewport or remote desktop size is not ready'
      return base
    }

    const localX = surfaceX - viewportX
    const localY = surfaceY - viewportY
    const inside = localX >= 0 && localX < viewportWidth && localY >= 0 && localY < viewportHeight
    if (!inside && !allowClamp) {
      base.reason = 'point is outside rendered remote viewport'
      return base
    }

    const clampedX = this.clampInt(localX, 0, viewportWidth - 1)
    const clampedY = this.clampInt(localY, 0, viewportHeight - 1)
    const mappedX = this.clampInt(clampedX * desktopWidth / viewportWidth, 0, desktopWidth - 1)
    const mappedY = this.clampInt(clampedY * desktopHeight / viewportHeight, 0, desktopHeight - 1)
    base.ok = true
    base.x = mappedX
    base.y = mappedY
    base.reason = 'ok'
    return base
  }

  private sendWheel(deltaY: number, x: number, y: number): void {
    const flags = deltaY > 0 ?
      PTR_FLAGS_WHEEL | PTR_FLAGS_WHEEL_NEGATIVE | RDP_WHEEL_DELTA_NEGATIVE :
      PTR_FLAGS_WHEEL | RDP_WHEEL_DELTA
    this.sendPointer(flags, x, y, false, 'wheel.vertical', true)
  }

  private sendHorizontalWheel(deltaX: number, x: number, y: number): void {
    const flags = deltaX > 0 ?
      PTR_FLAGS_HWHEEL | PTR_FLAGS_WHEEL_NEGATIVE | RDP_WHEEL_DELTA_NEGATIVE :
      PTR_FLAGS_HWHEEL | RDP_WHEEL_DELTA
    this.sendPointer(flags, x, y, false, 'wheel.horizontal', true)
  }

  private sendLeftButtonDown(x: number, y: number, label: string): void {
    this.sendPointer(PTR_FLAGS_MOVE, x, y, false, `${label}.move`, true)
    this.sendPointer(PTR_FLAGS_BUTTON1 | PTR_FLAGS_DOWN, x, y, false, `${label}.down`, true)
  }

  private sendLeftButtonUp(x: number, y: number, label: string): void {
    this.sendPointer(PTR_FLAGS_MOVE, x, y, true, `${label}.move`, true)
    this.sendPointer(PTR_FLAGS_BUTTON1, x, y, true, `${label}.up`, true)
  }

  private sendLeftClick(x: number, y: number, label: string = 'tap.left'): void {
    this.sendPointer(PTR_FLAGS_MOVE, x, y, false, `${label}.move`, true)
    this.sendPointer(PTR_FLAGS_BUTTON1 | PTR_FLAGS_DOWN, x, y, false, `${label}.down`, true)
    this.sendPointer(PTR_FLAGS_BUTTON1, x, y, false, `${label}.up`, true)
  }

  private sendRightClick(x: number, y: number, label: string = 'longPress.right'): void {
    this.sendPointer(PTR_FLAGS_MOVE, x, y, false, `${label}.move`, true)
    this.sendPointer(PTR_FLAGS_BUTTON2 | PTR_FLAGS_DOWN, x, y, false, `${label}.down`, true)
    this.sendPointer(PTR_FLAGS_BUTTON2, x, y, false, `${label}.up`, true)
  }

  private sendKeyStroke(scancode: number): boolean {
    const keyCode = this.mapRdpScancodeToPlatformKeyCode(scancode)
    if (keyCode > 0) {
      if (this.sendPlatformKeyEvent(keyCode, true)) {
        return this.sendPlatformKeyEvent(keyCode, false)
      }
      return false
    }
    this.appendLog(`Key stroke skipped: no platform key mapping for scancode=0x${scancode.toString(16)}`)
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
        ctrl: event ? this.isCtrlModifierPressed(event) : this.ctrlModifierDown || this.rightCtrlModifierDown,
        shift: event ? this.isShiftModifierPressed(event) : this.leftShiftModifierDown || this.rightShiftModifierDown,
        alt: event ? this.isAltModifierPressed(event) : this.altModifierDown || this.rightAltModifierDown,
        meta: event ? this.isWinModifierPressed(event) : this.winModifierDown || this.rightWinModifierDown
      })
      if (!result.ok) {
        this.appendLog(result.message)
      }
      if (result.ok && INPUT_PROBE_ON_INPUT) {
        this.scheduleInputProbe()
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
      if (result.ok && INPUT_PROBE_ON_INPUT) {
        this.scheduleInputProbe()
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
    this.logInputEvent('key.delete',
      `TextInput delete blocked direction=${value.direction} count=${Math.max(1, value.deleteValue.length)}`,
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
      this.sendKeyStroke(RDP_SCANCODE_BACKSPACE)
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
      if (result.ok && INPUT_PROBE_ON_INPUT) {
        this.scheduleInputProbe()
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

  private clearModifierState(): void {
    this.ctrlModifierDown = false
    this.altModifierDown = false
    this.winModifierDown = false
    this.rightCtrlModifierDown = false
    this.rightAltModifierDown = false
    this.rightWinModifierDown = false
    this.leftShiftModifierDown = false
    this.rightShiftModifierDown = false
    this.remoteShortcutPrintableKeysDown = []
    this.shortcutSyntheticModifiers = []
    this.remotePressedScancodes = []
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
      this.clearModifierState()
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
    this.clearModifierState()
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
    if (keyCode >= OH_KEYCODE_F1 && keyCode <= OH_KEYCODE_F12) {
      if (keyCode <= OH_KEYCODE_F1 + 9) {
        return RDP_SCANCODE_F1 + keyCode - OH_KEYCODE_F1
      }
      if (keyCode === OH_KEYCODE_F1 + 10) {
        return RDP_SCANCODE_F11
      }
      return RDP_SCANCODE_F12
    }
    if (keyCode >= OH_KEYCODE_NUMPAD_0 && keyCode <= OH_KEYCODE_NUMPAD_9) {
      const numpadScancodes: number[] = [0x52, 0x4F, 0x50, 0x51, 0x4B, 0x4C, 0x4D, 0x47, 0x48, 0x49]
      return numpadScancodes[keyCode - OH_KEYCODE_NUMPAD_0]
    }

    const keyMap: Map<number, number> = new Map<number, number>([
      [OH_KEYCODE_ESCAPE, RDP_SCANCODE_ESCAPE],
      [OH_KEYCODE_TAB, RDP_SCANCODE_TAB],
      [OH_KEYCODE_ENTER, RDP_SCANCODE_ENTER],
      [OH_KEYCODE_DEL, RDP_SCANCODE_BACKSPACE],
      [OH_KEYCODE_FORWARD_DEL, RDP_SCANCODE_DELETE],
      [OH_KEYCODE_SPACE, 0x39],
      [OH_KEYCODE_PAGE_UP, RDP_SCANCODE_PAGE_UP],
      [OH_KEYCODE_PAGE_DOWN, RDP_SCANCODE_PAGE_DOWN],
      [OH_KEYCODE_INSERT, RDP_SCANCODE_INSERT],
      [OH_KEYCODE_DPAD_UP, RDP_SCANCODE_UP],
      [OH_KEYCODE_DPAD_DOWN, RDP_SCANCODE_DOWN],
      [OH_KEYCODE_DPAD_LEFT, RDP_SCANCODE_LEFT],
      [OH_KEYCODE_DPAD_RIGHT, RDP_SCANCODE_RIGHT],
      [OH_KEYCODE_MOVE_HOME, RDP_SCANCODE_HOME],
      [OH_KEYCODE_MOVE_END, RDP_SCANCODE_END],
      [OH_KEYCODE_CTRL_LEFT, RDP_SCANCODE_LCONTROL],
      [OH_KEYCODE_CTRL_RIGHT, RDP_SCANCODE_RCONTROL],
      [OH_KEYCODE_ALT_LEFT, RDP_SCANCODE_LMENU],
      [OH_KEYCODE_ALT_RIGHT, RDP_SCANCODE_RMENU],
      [OH_KEYCODE_META_LEFT, RDP_SCANCODE_LWIN],
      [OH_KEYCODE_META_RIGHT, RDP_SCANCODE_RWIN],
      [OH_KEYCODE_SHIFT_LEFT, RDP_SCANCODE_LSHIFT],
      [OH_KEYCODE_SHIFT_RIGHT, RDP_SCANCODE_RSHIFT],
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
      [OH_KEYCODE_SLASH, 0x35],
      [OH_KEYCODE_NUMPAD_DIVIDE, 0x0135],
      [OH_KEYCODE_NUMPAD_MULTIPLY, 0x37],
      [OH_KEYCODE_NUMPAD_SUBTRACT, 0x4A],
      [OH_KEYCODE_NUMPAD_ADD, 0x4E],
      [OH_KEYCODE_NUMPAD_DOT, 0x53],
      [OH_KEYCODE_NUMPAD_ENTER, 0x011C]
    ])
    return keyMap.get(keyCode) ?? 0
  }

  private mapRdpScancodeToPlatformKeyCode(scancode: number): number {
    const keyMap: Map<number, number> = new Map<number, number>([
      [RDP_SCANCODE_BACKSPACE, OH_KEYCODE_DEL],
      [RDP_SCANCODE_DELETE, OH_KEYCODE_FORWARD_DEL],
      [RDP_SCANCODE_LCONTROL, OH_KEYCODE_CTRL_LEFT],
      [RDP_SCANCODE_RCONTROL, OH_KEYCODE_CTRL_RIGHT],
      [RDP_SCANCODE_LMENU, OH_KEYCODE_ALT_LEFT],
      [RDP_SCANCODE_RMENU, OH_KEYCODE_ALT_RIGHT],
      [RDP_SCANCODE_LWIN, OH_KEYCODE_META_LEFT],
      [RDP_SCANCODE_RWIN, OH_KEYCODE_META_RIGHT],
      [RDP_SCANCODE_LSHIFT, OH_KEYCODE_SHIFT_LEFT],
      [RDP_SCANCODE_RSHIFT, OH_KEYCODE_SHIFT_RIGHT]
    ])
    return keyMap.get(scancode) ?? 0
  }

  private isPrintableArkKeyCode(keyCode: number): boolean {
    return (keyCode >= OH_KEYCODE_A && keyCode <= OH_KEYCODE_Z) ||
      (keyCode >= OH_KEYCODE_0 && keyCode <= OH_KEYCODE_9) ||
      (keyCode >= OH_KEYCODE_NUMPAD_0 && keyCode <= OH_KEYCODE_NUMPAD_9) ||
      keyCode === OH_KEYCODE_SPACE ||
      keyCode === OH_KEYCODE_GRAVE ||
      keyCode === OH_KEYCODE_MINUS ||
      keyCode === OH_KEYCODE_EQUALS ||
      keyCode === OH_KEYCODE_LEFT_BRACKET ||
      keyCode === OH_KEYCODE_RIGHT_BRACKET ||
      keyCode === OH_KEYCODE_BACKSLASH ||
      keyCode === OH_KEYCODE_SEMICOLON ||
      keyCode === OH_KEYCODE_APOSTROPHE ||
      keyCode === OH_KEYCODE_COMMA ||
      keyCode === OH_KEYCODE_PERIOD ||
      keyCode === OH_KEYCODE_SLASH ||
      keyCode === OH_KEYCODE_NUMPAD_DIVIDE ||
      keyCode === OH_KEYCODE_NUMPAD_MULTIPLY ||
      keyCode === OH_KEYCODE_NUMPAD_SUBTRACT ||
      keyCode === OH_KEYCODE_NUMPAD_ADD ||
      keyCode === OH_KEYCODE_NUMPAD_DOT
  }

  private shouldSendPrintableAsHardwareKey(keyCode: number): boolean {
    return (keyCode >= OH_KEYCODE_0 && keyCode <= OH_KEYCODE_9) ||
      (keyCode >= OH_KEYCODE_NUMPAD_0 && keyCode <= OH_KEYCODE_NUMPAD_9) ||
      keyCode === OH_KEYCODE_NUMPAD_DIVIDE ||
      keyCode === OH_KEYCODE_NUMPAD_MULTIPLY ||
      keyCode === OH_KEYCODE_NUMPAD_SUBTRACT ||
      keyCode === OH_KEYCODE_NUMPAD_ADD ||
      keyCode === OH_KEYCODE_NUMPAD_DOT
  }

  private isRemotePlatformKeyCode(keyCode: number): boolean {
    if (this.isPrintableArkKeyCode(keyCode) || this.isFunctionKey(keyCode)) {
      return true
    }
    const keyCodes: number[] = [
      OH_KEYCODE_ESCAPE,
      OH_KEYCODE_TAB,
      OH_KEYCODE_ENTER,
      OH_KEYCODE_DEL,
      OH_KEYCODE_FORWARD_DEL,
      OH_KEYCODE_PAGE_UP,
      OH_KEYCODE_PAGE_DOWN,
      OH_KEYCODE_INSERT,
      OH_KEYCODE_DPAD_UP,
      OH_KEYCODE_DPAD_DOWN,
      OH_KEYCODE_DPAD_LEFT,
      OH_KEYCODE_DPAD_RIGHT,
      OH_KEYCODE_MOVE_HOME,
      OH_KEYCODE_MOVE_END,
      OH_KEYCODE_CTRL_LEFT,
      OH_KEYCODE_CTRL_RIGHT,
      OH_KEYCODE_ALT_LEFT,
      OH_KEYCODE_ALT_RIGHT,
      OH_KEYCODE_META_LEFT,
      OH_KEYCODE_META_RIGHT,
      OH_KEYCODE_SHIFT_LEFT,
      OH_KEYCODE_SHIFT_RIGHT
    ]
    return keyCodes.indexOf(keyCode) >= 0
  }

  private isDeletePlatformKeyCode(keyCode: number): boolean {
    return keyCode === OH_KEYCODE_DEL || keyCode === OH_KEYCODE_FORWARD_DEL
  }

  private getHardwarePrintableText(event: KeyEvent): string {
    const unicode = this.getEventUnicode(event)
    if (unicode > 0 && unicode <= 0xFFFF) {
      return String.fromCharCode(unicode)
    }
    if (event.keyCode >= OH_KEYCODE_0 && event.keyCode <= OH_KEYCODE_9) {
      return `${event.keyCode - OH_KEYCODE_0}`
    }
    if (event.keyCode >= OH_KEYCODE_NUMPAD_0 && event.keyCode <= OH_KEYCODE_NUMPAD_9) {
      return `${event.keyCode - OH_KEYCODE_NUMPAD_0}`
    }
    const keyMap: Map<number, string> = new Map<number, string>([
      [OH_KEYCODE_NUMPAD_DIVIDE, '/'],
      [OH_KEYCODE_NUMPAD_MULTIPLY, '*'],
      [OH_KEYCODE_NUMPAD_SUBTRACT, '-'],
      [OH_KEYCODE_NUMPAD_ADD, '+'],
      [OH_KEYCODE_NUMPAD_DOT, '.']
    ])
    return keyMap.get(event.keyCode) ?? ''
  }

  private getPrintableTextByKeyCode(keyCode: number): string {
    if (keyCode >= OH_KEYCODE_A && keyCode <= OH_KEYCODE_Z) {
      return String.fromCharCode('a'.charCodeAt(0) + keyCode - OH_KEYCODE_A)
    }
    if (keyCode >= OH_KEYCODE_0 && keyCode <= OH_KEYCODE_9) {
      return `${keyCode - OH_KEYCODE_0}`
    }
    if (keyCode >= OH_KEYCODE_NUMPAD_0 && keyCode <= OH_KEYCODE_NUMPAD_9) {
      return `${keyCode - OH_KEYCODE_NUMPAD_0}`
    }
    const keyMap: Map<number, string> = new Map<number, string>([
      [OH_KEYCODE_NUMPAD_DIVIDE, '/'],
      [OH_KEYCODE_NUMPAD_MULTIPLY, '*'],
      [OH_KEYCODE_NUMPAD_SUBTRACT, '-'],
      [OH_KEYCODE_NUMPAD_ADD, '+'],
      [OH_KEYCODE_NUMPAD_DOT, '.']
    ])
    return keyMap.get(keyCode) ?? ''
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
    this.addSuppressedTextInputText(this.getHardwarePrintableText(event))
    this.addSuppressedTextInputText(this.getPrintableTextByKeyCode(event.keyCode))
  }

  private isModifierPressed(event: KeyEvent, modifier: string): boolean {
    try {
      if (event.getModifierKeyState) {
        return event.getModifierKeyState([modifier])
      }
    } catch (error) {
      return false
    }
    return false
  }

  private isAnyModifierPressed(event: KeyEvent, modifiers: string[]): boolean {
    for (let i = 0; i < modifiers.length; i++) {
      if (this.isModifierPressed(event, modifiers[i])) {
        return true
      }
    }
    return false
  }

  private isCtrlModifierPressed(event: KeyEvent): boolean {
    return this.ctrlModifierDown || this.rightCtrlModifierDown ||
      this.isAnyModifierPressed(event, ['Ctrl'])
  }

  private isAltModifierPressed(event: KeyEvent): boolean {
    return this.altModifierDown || this.rightAltModifierDown ||
      this.isAnyModifierPressed(event, ['Alt'])
  }

  private isWinModifierPressed(event: KeyEvent): boolean {
    return this.winModifierDown || this.rightWinModifierDown
  }

  private updateModifierState(scancode: number, down: boolean): void {
    if (scancode === RDP_SCANCODE_LCONTROL) {
      this.ctrlModifierDown = down
    } else if (scancode === RDP_SCANCODE_RCONTROL) {
      this.rightCtrlModifierDown = down
    } else if (scancode === RDP_SCANCODE_LMENU) {
      this.altModifierDown = down
    } else if (scancode === RDP_SCANCODE_RMENU) {
      this.rightAltModifierDown = down
    } else if (scancode === RDP_SCANCODE_LWIN) {
      this.winModifierDown = down
    } else if (scancode === RDP_SCANCODE_RWIN) {
      this.rightWinModifierDown = down
    } else if (scancode === RDP_SCANCODE_LSHIFT) {
      this.leftShiftModifierDown = down
    } else if (scancode === RDP_SCANCODE_RSHIFT) {
      this.rightShiftModifierDown = down
    }
  }

  private isRemoteScancodePressed(scancode: number): boolean {
    return this.remotePressedScancodes.indexOf(scancode) >= 0
  }

  private setRemoteScancodePressed(scancode: number, down: boolean): void {
    const index = this.remotePressedScancodes.indexOf(scancode)
    if (down) {
      if (index < 0) {
        this.remotePressedScancodes.push(scancode)
      }
      return
    }
    if (index >= 0) {
      this.remotePressedScancodes.splice(index, 1)
    }
  }

  private sendMappedKeyEvent(keyCode: number, scancode: number, down: boolean,
    repeat: boolean = false, event?: KeyEvent): boolean {
    const sent = this.sendPlatformKeyEvent(keyCode, down, false, repeat, event)
    if (sent) {
      this.updateModifierState(scancode, down)
      this.setRemoteScancodePressed(scancode, down)
    }
    return sent
  }

  private isShiftModifierPressed(event: KeyEvent): boolean {
    return this.leftShiftModifierDown || this.rightShiftModifierDown ||
      this.isAnyModifierPressed(event, ['Shift'])
  }

  private hasRemoteShortcutModifier(event: KeyEvent): boolean {
    return this.isCtrlModifierPressed(event) || this.isAltModifierPressed(event) ||
      this.isWinModifierPressed(event) || this.isShiftModifierPressed(event)
  }

  private isTrackedRemoteShortcutPrintableKey(keyCode: number): boolean {
    return this.remoteShortcutPrintableKeysDown.indexOf(keyCode) >= 0
  }

  private setRemoteShortcutPrintableKeyState(keyCode: number, down: boolean): void {
    const index = this.remoteShortcutPrintableKeysDown.indexOf(keyCode)
    if (down) {
      if (index < 0) {
        this.remoteShortcutPrintableKeysDown.push(keyCode)
      }
      return
    }
    if (index >= 0) {
      this.remoteShortcutPrintableKeysDown.splice(index, 1)
    }
  }

  private isFunctionKey(keyCode: number): boolean {
    return keyCode >= OH_KEYCODE_F1 && keyCode <= OH_KEYCODE_F12
  }

  private isModifierScancode(scancode: number): boolean {
    return scancode === RDP_SCANCODE_LCONTROL || scancode === RDP_SCANCODE_RCONTROL ||
      scancode === RDP_SCANCODE_LMENU || scancode === RDP_SCANCODE_RMENU ||
      scancode === RDP_SCANCODE_LWIN || scancode === RDP_SCANCODE_RWIN ||
      scancode === RDP_SCANCODE_LSHIFT || scancode === RDP_SCANCODE_RSHIFT
  }

  private isDeleteScancode(scancode: number): boolean {
    return scancode === RDP_SCANCODE_BACKSPACE || scancode === RDP_SCANCODE_DELETE
  }

  private addScancode(scancodes: number[], scancode: number): void {
    if (scancode > 0 && scancodes.indexOf(scancode) < 0) {
      scancodes.push(scancode)
    }
  }

  private getActiveShortcutModifierScancodes(event: KeyEvent): number[] {
    const scancodes: number[] = []
    if (this.ctrlModifierDown) {
      this.addScancode(scancodes, RDP_SCANCODE_LCONTROL)
    }
    if (this.rightCtrlModifierDown) {
      this.addScancode(scancodes, RDP_SCANCODE_RCONTROL)
    }
    if (!this.ctrlModifierDown && !this.rightCtrlModifierDown && this.isCtrlModifierPressed(event)) {
      this.addScancode(scancodes, RDP_SCANCODE_LCONTROL)
    }

    if (this.altModifierDown) {
      this.addScancode(scancodes, RDP_SCANCODE_LMENU)
    }
    if (this.rightAltModifierDown) {
      this.addScancode(scancodes, RDP_SCANCODE_RMENU)
    }
    if (!this.altModifierDown && !this.rightAltModifierDown && this.isAltModifierPressed(event)) {
      this.addScancode(scancodes, RDP_SCANCODE_LMENU)
    }

    if (this.winModifierDown) {
      this.addScancode(scancodes, RDP_SCANCODE_LWIN)
    }
    if (this.rightWinModifierDown) {
      this.addScancode(scancodes, RDP_SCANCODE_RWIN)
    }
    if (!this.winModifierDown && !this.rightWinModifierDown && this.isWinModifierPressed(event)) {
      this.addScancode(scancodes, RDP_SCANCODE_LWIN)
    }

    if (this.leftShiftModifierDown) {
      this.addScancode(scancodes, RDP_SCANCODE_LSHIFT)
    }
    if (this.rightShiftModifierDown) {
      this.addScancode(scancodes, RDP_SCANCODE_RSHIFT)
    }
    if (!this.leftShiftModifierDown && !this.rightShiftModifierDown && this.isShiftModifierPressed(event)) {
      this.addScancode(scancodes, RDP_SCANCODE_LSHIFT)
    }
    return scancodes
  }

  private rememberShortcutSyntheticModifier(keyCode: number, scancode: number): void {
    for (let i = 0; i < this.shortcutSyntheticModifiers.length; i++) {
      const item = this.shortcutSyntheticModifiers[i]
      if (item.keyCode === keyCode && item.scancode === scancode) {
        return
      }
    }
    this.shortcutSyntheticModifiers.push({ keyCode: keyCode, scancode: scancode })
  }

  private ensureShortcutModifiersPressed(keyCode: number, modifierScancodes: number[]): boolean {
    let sent = true
    for (let i = 0; i < modifierScancodes.length; i++) {
      const modifierScancode = modifierScancodes[i]
      if (this.isRemoteScancodePressed(modifierScancode)) {
        continue
      }
      const modifierKeyCode = this.mapRdpScancodeToPlatformKeyCode(modifierScancode)
      sent = modifierKeyCode > 0 &&
        this.sendMappedKeyEvent(modifierKeyCode, modifierScancode, true, false) && sent
      this.rememberShortcutSyntheticModifier(keyCode, modifierScancode)
    }
    return sent
  }

  private releaseShortcutSyntheticModifiers(keyCode: number): boolean {
    let sent = true
    for (let i = this.shortcutSyntheticModifiers.length - 1; i >= 0; i--) {
      const item = this.shortcutSyntheticModifiers[i]
      if (item.keyCode !== keyCode) {
        continue
      }
      this.shortcutSyntheticModifiers.splice(i, 1)
      if (this.isRemoteScancodePressed(item.scancode)) {
        const modifierKeyCode = this.mapRdpScancodeToPlatformKeyCode(item.scancode)
        sent = modifierKeyCode > 0 &&
          this.sendMappedKeyEvent(modifierKeyCode, item.scancode, false, false) && sent
      }
    }
    return sent
  }

  private formatScancodeList(scancodes: number[]): string {
    const parts: string[] = []
    for (let i = 0; i < scancodes.length; i++) {
      parts.push(`0x${scancodes[i].toString(16)}`)
    }
    return parts.join(',')
  }

  private handleShortcutKeyEvent(event: KeyEvent, scancode: number, printable: boolean,
    down: boolean, repeat: boolean, trackedShortcut: boolean): boolean {
    if (scancode <= 0 || this.isModifierScancode(scancode)) {
      return false
    }

    const modifierScancodes = this.getActiveShortcutModifierScancodes(event)
    if (modifierScancodes.length <= 0 && !trackedShortcut) {
      return false
    }

    if (down) {
      const modifiersSent = this.ensureShortcutModifiersPressed(event.keyCode, modifierScancodes)
      const keySent = this.sendMappedKeyEvent(event.keyCode, scancode, true, repeat, event)
      if (keySent && printable) {
        this.setRemoteShortcutPrintableKeyState(event.keyCode, true)
        this.rememberHardwareTextInputSuppression(event)
      }
      this.logInputEvent('key.shortcut',
        `code=${event.keyCode} scancode=0x${scancode.toString(16)} down=true repeat=${repeat} ` +
        `modifiers=${this.formatScancodeList(modifierScancodes)} sent=${modifiersSent && keySent}`,
        true)
      return true
    }

    let keySent = true
    if (this.isRemoteScancodePressed(scancode)) {
      keySent = this.sendMappedKeyEvent(event.keyCode, scancode, false, false, event)
    }
    if (printable) {
      this.setRemoteShortcutPrintableKeyState(event.keyCode, false)
    }
    const modifiersReleased = this.releaseShortcutSyntheticModifiers(event.keyCode)
    this.logInputEvent('key.shortcut',
      `code=${event.keyCode} scancode=0x${scancode.toString(16)} down=false ` +
      `sent=${keySent && modifiersReleased}`,
      true)
    return true
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

  private sendRepeatKeyDown(keyCode: number, scancode: number, event: KeyEvent): boolean {
    const sent = this.sendMappedKeyEvent(keyCode, scancode, true, true, event)
    this.logInputEvent('key.repeat',
      `down scancode=0x${scancode.toString(16)} sent=${sent}`,
      true)
    return sent
  }

  private logKeyDiagnostic(event: KeyEvent, scancode: number, printable: boolean, shortcut: boolean,
    trackedShortcut: boolean, hardwarePrintable: boolean, fromInputBridge: boolean, repeat: boolean): void {
    if (!shortcut && !trackedShortcut && !hardwarePrintable && !this.isFunctionKey(event.keyCode) &&
      (scancode <= 0 || printable)) {
      return
    }
    this.logInputEvent('key',
      `code=${event.keyCode} type=${event.type} scancode=${scancode} printable=${printable} ` +
      `shortcut=${shortcut} tracked=${trackedShortcut} hardware=${hardwarePrintable} ` +
      `repeat=${repeat} bridge=${fromInputBridge} ` +
      `ctrl=${this.isCtrlModifierPressed(event)} alt=${this.isAltModifierPressed(event)} ` +
      `win=${this.isWinModifierPressed(event)} shift=${this.isShiftModifierPressed(event)}`,
      true)
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
    if (!this.isRemotePlatformKeyCode(event.keyCode)) {
      return false
    }
    const printable = this.isPrintableArkKeyCode(event.keyCode)
    const hardwarePrintable = printable && this.shouldSendPrintableAsHardwareKey(event.keyCode)
    const trackedShortcut = printable && this.isTrackedRemoteShortcutPrintableKey(event.keyCode)
    return !printable || hardwarePrintable || this.hasRemoteShortcutModifier(event) || trackedShortcut
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
    const printable = this.isPrintableArkKeyCode(event.keyCode)
    const shortcut = this.hasRemoteShortcutModifier(event)
    const hardwarePrintable = printable && this.shouldSendPrintableAsHardwareKey(event.keyCode)
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
    const printable = this.isPrintableArkKeyCode(event.keyCode)
    const systemShortcutModifier = this.isCtrlModifierPressed(event) || this.isAltModifierPressed(event) ||
      this.isWinModifierPressed(event)
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
    const printable = this.isPrintableArkKeyCode(event.keyCode)
    const hardwarePrintable = printable && this.shouldSendPrintableAsHardwareKey(event.keyCode)
    let shortcut = this.hasRemoteShortcutModifier(event)
    const trackedShortcut = printable && this.isTrackedRemoteShortcutPrintableKey(event.keyCode)
    if (!down && trackedShortcut) {
      shortcut = true
    }
    const platformKey = this.isRemotePlatformKeyCode(event.keyCode)
    const surfacePrintableKeyEvent = printable && !fromInputBridge
    if (!printable || shortcut || hardwarePrintable || this.isFunctionKey(event.keyCode)) {
      this.logInputEvent('key',
        `code=${event.keyCode} type=${event.type} platform=${platformKey} printable=${printable} ` +
        `shortcut=${shortcut} hardware=${hardwarePrintable} bridge=${fromInputBridge} ` +
        `ctrl=${this.isCtrlModifierPressed(event)} alt=${this.isAltModifierPressed(event)} ` +
        `win=${this.isWinModifierPressed(event)} shift=${this.isShiftModifierPressed(event)}`,
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
      if (sent && printable && shortcut) {
        this.setRemoteShortcutPrintableKeyState(event.keyCode, down)
      }
      if (sent && this.isDeletePlatformKeyCode(event.keyCode)) {
        this.logInputEvent('key.delete',
          `hardware keyCode=${event.keyCode} down=${down} sent=${sent}`,
          true)
      }
      return sent
    }

    if (!down) {
      return printable || platformKey
    }

    const unicode = this.getEventUnicode(event)
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
        this.mapPointerToDesktop(event.x, event.y, false))
      if (buttonFlag === 0 || !this.isPointInRemoteViewport(event.x, event.y)) {
        this.appendLog(`Mouse press skipped: button=${buttonFlag} inside=${this.isPointInRemoteViewport(event.x, event.y)}`)
        return
      }
      this.mouseButtonFlags |= buttonFlag
      this.sendPointer(PTR_FLAGS_MOVE, event.x, event.y, false, 'mouse.press.move', true)
      this.sendPointer(buttonFlag | PTR_FLAGS_DOWN, event.x, event.y, false, 'mouse.press.down', true)
      return
    }
    if (event.action === MouseAction.Release) {
      const releaseFlag = buttonFlag !== 0 ? buttonFlag : this.mouseButtonFlags
      if (releaseFlag !== 0) {
        this.sendPointer(PTR_FLAGS_MOVE, event.x, event.y, true, 'mouse.release.move', true)
        this.sendPointer(releaseFlag, event.x, event.y, true, 'mouse.release.up', true)
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
        this.sendPointer(PTR_FLAGS_MOVE, event.x, event.y, true, 'mouse.cancel.move', true)
        this.sendPointer(this.mouseButtonFlags, event.x, event.y, true, 'mouse.cancel.release', true)
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
    const axisX = this.isPointInRemoteViewport(event.x, event.y) ? event.x : this.lastTouchX
    const axisY = this.isPointInRemoteViewport(event.x, event.y) ? event.y : this.lastTouchY
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
    this.logPointerDiagnostic('touch.down', PTR_FLAGS_MOVE, x, y, this.mapPointerToDesktop(x, y, false))
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
        const leavingSession = this.currentTab === 'session' && id !== 'session'
        this.currentTab = id
        if (leavingSession) {
          this.releaseActiveModifiers()
        }
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
              this.probeNative()
              this.focusRemoteInputBridge()
            })
          })
          .onDestroy(() => {
            this.deferUiUpdate(() => {
              this.appendLog('XComponent destroyed')
              this.probeNative()
            })
          })
          .onAreaChange((oldArea: Area, newArea: Area) => {
            this.updateSurfaceLayout(newArea.width, newArea.height)
            this.deferUiUpdate(() => {
              this.probeNative()
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
