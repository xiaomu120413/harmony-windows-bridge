import {
  AXIS_EVENT_DEDUP_MS,
  DOUBLE_TAP_DISTANCE,
  DOUBLE_TAP_TIMEOUT_MS,
  LONG_PRESS_TIMEOUT_MS,
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
  SYNTHETIC_MOUSE_SUPPRESS_MS,
  TOUCH_DRAG_THRESHOLD,
  TOUCH_WHEEL_THRESHOLD
} from './RdpConstants'
import { PointerMapper, PointerViewport } from './PointerMapper'
import { PointerMapping } from './RdpTypes'

export class RemotePointerInput {
  private isConnectedCallback: () => boolean
  private focusRemoteInputCallback: () => void
  private getViewportCallback: () => PointerViewport
  private refreshViewportStateCallback: (force?: boolean) => void
  private sendPointerCallback: (flags: number, x: number, y: number, allowClamp?: boolean,
    traceLabel?: string) => boolean
  private appendLogCallback: (line: string) => void
  private logInputEventCallback: (label: string, detail: string, force?: boolean) => void
  private logPointerDiagnosticCallback: (label: string, flags: number, x: number, y: number,
    mapped: PointerMapping) => void
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
  private lastAxisEventAtMs: number = 0
  private lastAxisAction: number = -1
  private lastAxisX: number = 0
  private lastAxisY: number = 0
  private lastAxisDeltaX: number = 0
  private lastAxisDeltaY: number = 0

  constructor(isConnected: () => boolean,
    focusRemoteInput: () => void,
    getViewport: () => PointerViewport,
    refreshViewportState: (force?: boolean) => void,
    sendPointer: (flags: number, x: number, y: number, allowClamp?: boolean, traceLabel?: string) => boolean,
    appendLog: (line: string) => void,
    logInputEvent: (label: string, detail: string, force?: boolean) => void,
    logPointerDiagnostic: (label: string, flags: number, x: number, y: number, mapped: PointerMapping) => void) {
    this.isConnectedCallback = isConnected
    this.focusRemoteInputCallback = focusRemoteInput
    this.getViewportCallback = getViewport
    this.refreshViewportStateCallback = refreshViewportState
    this.sendPointerCallback = sendPointer
    this.appendLogCallback = appendLog
    this.logInputEventCallback = logInputEvent
    this.logPointerDiagnosticCallback = logPointerDiagnostic
  }

  clearState(): void {
    this.cancelLongPressTimer()
    this.pendingTouchActive = false
    this.longPressTriggered = false
    this.pointerDown = false
    this.twoFingerScrollActive = false
    this.touchGestureSuppressed = false
    this.mouseButtonFlags = 0
    this.lastTapAtMs = 0
  }

  releaseButtons(): void {
    if (!this.isConnectedCallback()) {
      this.clearState()
      return
    }
    if (this.pointerDown) {
      this.sendLeftButtonUp(this.lastTouchX, this.lastTouchY, 'touch.releaseOnExit')
    }
    if (this.mouseButtonFlags !== 0) {
      this.sendPointerCallback(PTR_FLAGS_MOVE | this.mouseButtonFlags, this.lastTouchX, this.lastTouchY, true)
    }
    this.clearState()
  }

  handleMouse(event: MouseEvent): void {
    const rawButtonFlag = this.mouseButtonToPointerFlag(event.button)
    const pressedFlags = this.getPressedMouseButtonFlags(event)
    const pressedButtonFlag = this.getPrimaryPressedMouseButtonFlag(pressedFlags)
    const buttonFlag = rawButtonFlag !== 0 ? rawButtonFlag : pressedButtonFlag
    this.logInputEventCallback('mouse',
      `action=${event.action} button=${event.button} rawFlag=${RemotePointerInput.formatPointerFlags(rawButtonFlag)} ` +
        `pressed=${RemotePointerInput.formatPointerFlags(pressedFlags)} x=${RemotePointerInput.formatNumber(event.x)} ` +
        `y=${RemotePointerInput.formatNumber(event.y)}`)

    if (!this.isConnectedCallback()) {
      this.appendLogCallback('Mouse input skipped: no active FreeRDP session')
      return
    }
    if ((event.action === MouseAction.Press || event.action === MouseAction.Release) &&
      this.isSyntheticMouseAfterTouch()) {
      this.logInputEventCallback('mouse.synthetic', `suppressed action=${event.action}`, true)
      return
    }
    if (event.action === MouseAction.Press || event.action === MouseAction.Release) {
      this.refreshViewportStateCallback(true)
    }
    this.lastTouchX = event.x
    this.lastTouchY = event.y
    if (event.action === MouseAction.Press) {
      this.focusRemoteInputCallback()
      this.logPointerDiagnosticCallback('mouse.press', PTR_FLAGS_MOVE | buttonFlag | PTR_FLAGS_DOWN, event.x, event.y,
        PointerMapper.mapToDesktop(this.getViewportCallback(), event.x, event.y, false))
      if (buttonFlag === 0 ||
        !PointerMapper.isPointInRemoteViewport(this.getViewportCallback(), event.x, event.y)) {
        this.appendLogCallback(`Mouse press skipped: button=${buttonFlag} inside=${PointerMapper.isPointInRemoteViewport(this.getViewportCallback(), event.x, event.y)}`)
        return
      }
      this.mouseButtonFlags |= buttonFlag
      this.sendPointerCallback(PTR_FLAGS_MOVE, event.x, event.y, false, 'mouse.press.move')
      this.sendPointerCallback(buttonFlag | PTR_FLAGS_DOWN, event.x, event.y, false, 'mouse.press.down')
      return
    }
    if (event.action === MouseAction.Release) {
      const releaseFlag = buttonFlag !== 0 ? buttonFlag : this.mouseButtonFlags
      if (releaseFlag !== 0) {
        this.sendPointerCallback(PTR_FLAGS_MOVE, event.x, event.y, true, 'mouse.release.move')
        this.sendPointerCallback(releaseFlag, event.x, event.y, true, 'mouse.release.up')
        this.mouseButtonFlags &= ~releaseFlag
      }
      return
    }
    if (event.action === MouseAction.Move || event.action === MouseAction.Hover) {
      if (pressedFlags !== 0) {
        this.sendPointerCallback(PTR_FLAGS_MOVE | pressedFlags | PTR_FLAGS_DOWN, event.x, event.y, true,
          'mouse.drag.move')
      } else {
        this.sendPointerCallback(PTR_FLAGS_MOVE, event.x, event.y)
      }
      return
    }
    if (event.action === MouseAction.CANCEL) {
      if (this.mouseButtonFlags !== 0) {
        this.sendPointerCallback(PTR_FLAGS_MOVE, event.x, event.y, true, 'mouse.cancel.move')
        this.sendPointerCallback(this.mouseButtonFlags, event.x, event.y, true, 'mouse.cancel.release')
      }
      this.mouseButtonFlags = 0
    }
  }

  handleAxis(event: AxisEvent): void {
    const deltaY = event.getVerticalAxisValue()
    const deltaX = event.getHorizontalAxisValue()
    if (this.shouldSkipDuplicateAxisEvent(event, deltaX, deltaY)) {
      return
    }
    this.refreshViewportStateCallback(true)
    const axisX = PointerMapper.isPointInRemoteViewport(this.getViewportCallback(), event.x, event.y) ?
      event.x : this.lastTouchX
    const axisY = PointerMapper.isPointInRemoteViewport(this.getViewportCallback(), event.x, event.y) ?
      event.y : this.lastTouchY
    this.logInputEventCallback('axis',
      `action=${event.action} dx=${RemotePointerInput.formatNumber(deltaX)} dy=${RemotePointerInput.formatNumber(deltaY)} ` +
        `x=${RemotePointerInput.formatNumber(event.x)} y=${RemotePointerInput.formatNumber(event.y)} ` +
        `target=${RemotePointerInput.formatNumber(axisX)},${RemotePointerInput.formatNumber(axisY)}`)

    if (!this.isConnectedCallback() || event.action !== AxisAction.UPDATE) {
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

  handleTouch(event: TouchEvent): void {
    this.lastTouchEventAtMs = Date.now()
    const touchCount = event.touches.length
    const firstX = touchCount > 0 ? event.touches[0].x : this.lastTouchX
    const firstY = touchCount > 0 ? event.touches[0].y : this.lastTouchY
    this.logInputEventCallback('touch',
      `type=${event.type} count=${touchCount} x=${RemotePointerInput.formatNumber(firstX)} ` +
        `y=${RemotePointerInput.formatNumber(firstY)} pending=${this.pendingTouchActive} down=${this.pointerDown}`)
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
      this.refreshViewportStateCallback(true)
      this.focusRemoteInputCallback()
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
        this.sendPointerCallback(PTR_FLAGS_MOVE | PTR_FLAGS_BUTTON1 | PTR_FLAGS_DOWN, x, y, true,
          'touch.drag.move')
      } else {
        this.sendPointerCallback(PTR_FLAGS_MOVE, x, y)
      }
      return
    }

    if (event.type === TouchType.Up || event.type === TouchType.Cancel) {
      this.finishSingleTouch(x, y, event.type === TouchType.Cancel)
    }
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

  private sendWheel(deltaY: number, x: number, y: number): void {
    const flags = deltaY > 0 ?
      PTR_FLAGS_WHEEL | PTR_FLAGS_WHEEL_NEGATIVE | RDP_WHEEL_DELTA_NEGATIVE :
      PTR_FLAGS_WHEEL | RDP_WHEEL_DELTA
    this.sendPointerCallback(flags, x, y, false, 'wheel.vertical')
  }

  private sendHorizontalWheel(deltaX: number, x: number, y: number): void {
    const flags = deltaX > 0 ?
      PTR_FLAGS_HWHEEL | PTR_FLAGS_WHEEL_NEGATIVE | RDP_WHEEL_DELTA_NEGATIVE :
      PTR_FLAGS_HWHEEL | RDP_WHEEL_DELTA
    this.sendPointerCallback(flags, x, y, false, 'wheel.horizontal')
  }

  private sendLeftButtonDown(x: number, y: number, label: string): void {
    this.sendPointerCallback(PTR_FLAGS_MOVE, x, y, false, `${label}.move`)
    this.sendPointerCallback(PTR_FLAGS_BUTTON1 | PTR_FLAGS_DOWN, x, y, false, `${label}.down`)
  }

  private sendLeftButtonUp(x: number, y: number, label: string): void {
    this.sendPointerCallback(PTR_FLAGS_MOVE, x, y, true, `${label}.move`)
    this.sendPointerCallback(PTR_FLAGS_BUTTON1, x, y, true, `${label}.up`)
  }

  private sendLeftClick(x: number, y: number, label: string = 'tap.left'): void {
    this.sendPointerCallback(PTR_FLAGS_MOVE, x, y, false, `${label}.move`)
    this.sendPointerCallback(PTR_FLAGS_BUTTON1 | PTR_FLAGS_DOWN, x, y, false, `${label}.down`)
    this.sendPointerCallback(PTR_FLAGS_BUTTON1, x, y, false, `${label}.up`)
  }

  private sendRightClick(x: number, y: number, label: string = 'longPress.right'): void {
    this.sendPointerCallback(PTR_FLAGS_MOVE, x, y, false, `${label}.move`)
    this.sendPointerCallback(PTR_FLAGS_BUTTON2 | PTR_FLAGS_DOWN, x, y, false, `${label}.down`)
    this.sendPointerCallback(PTR_FLAGS_BUTTON2, x, y, false, `${label}.up`)
  }

  private startPendingTouch(x: number, y: number): void {
    this.logPointerDiagnosticCallback('touch.down', PTR_FLAGS_MOVE, x, y,
      PointerMapper.mapToDesktop(this.getViewportCallback(), x, y, false))
    if (!PointerMapper.isPointInRemoteViewport(this.getViewportCallback(), x, y)) {
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
    this.logInputEventCallback('touch.drag',
      `start ${RemotePointerInput.formatNumber(this.pendingTouchStartX)},` +
        `${RemotePointerInput.formatNumber(this.pendingTouchStartY)} -> ${RemotePointerInput.formatNumber(x)},` +
        `${RemotePointerInput.formatNumber(y)}`,
      true)
    this.sendLeftButtonDown(this.pendingTouchStartX, this.pendingTouchStartY, 'touch.drag.start')
    this.sendPointerCallback(PTR_FLAGS_MOVE | PTR_FLAGS_BUTTON1 | PTR_FLAGS_DOWN, x, y, true,
      'touch.drag.move')
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
          this.appendLogCallback(`Touch double tap: event=${RemotePointerInput.formatNumber(x)},${RemotePointerInput.formatNumber(y)}`)
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
      this.logInputEventCallback('touch.scroll',
        `start center=${RemotePointerInput.formatNumber(centerX)},${RemotePointerInput.formatNumber(centerY)}`,
        true)
      return true
    }

    if (event.type === TouchType.Move) {
      const deltaX = centerX - this.twoFingerLastX
      const deltaY = centerY - this.twoFingerLastY
      if (Math.abs(deltaY) >= TOUCH_WHEEL_THRESHOLD || Math.abs(deltaX) >= TOUCH_WHEEL_THRESHOLD) {
        this.logInputEventCallback('touch.scroll',
          `dx=${RemotePointerInput.formatNumber(deltaX)} dy=${RemotePointerInput.formatNumber(deltaY)} ` +
            `center=${RemotePointerInput.formatNumber(centerX)},${RemotePointerInput.formatNumber(centerY)}`,
          true)
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

  private static formatNumber(value: number): string {
    return `${Math.round(value * 10) / 10}`
  }

  private static formatPointerFlags(flags: number): string {
    return `0x${flags.toString(16).toUpperCase()}`
  }
}
