import {
  AXIS_EVENT_DEDUP_MS,
  DOUBLE_TAP_DISTANCE,
  DOUBLE_TAP_TIMEOUT_MS,
  LONG_PRESS_TIMEOUT_MS,
  SYNTHETIC_MOUSE_SUPPRESS_MS,
  TOUCH_DRAG_THRESHOLD,
  TOUCH_WHEEL_THRESHOLD
} from './RdpConstants'

export interface RemotePointerEvent {
  action: string
  button?: string
  buttons?: number
  x: number
  y: number
  delta?: number
  allowClamp?: boolean
  traceLabel?: string
}

const REMOTE_BUTTON_NONE: number = 0
const REMOTE_BUTTON_LEFT: number = 1
const REMOTE_BUTTON_RIGHT: number = 2
const REMOTE_BUTTON_MIDDLE: number = 4

export class RemotePointerInput {
  private isConnectedCallback: () => boolean
  private focusRemoteInputCallback: () => void
  private refreshViewportStateCallback: (force?: boolean) => void
  private sendPointerCallback: (event: RemotePointerEvent) => boolean
  private appendLogCallback: (line: string) => void
  private logInputEventCallback: (label: string, detail: string, force?: boolean) => void
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
    refreshViewportState: (force?: boolean) => void,
    sendPointer: (event: RemotePointerEvent) => boolean,
    appendLog: (line: string) => void,
    logInputEvent: (label: string, detail: string, force?: boolean) => void) {
    this.isConnectedCallback = isConnected
    this.focusRemoteInputCallback = focusRemoteInput
    this.refreshViewportStateCallback = refreshViewportState
    this.sendPointerCallback = sendPointer
    this.appendLogCallback = appendLog
    this.logInputEventCallback = logInputEvent
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
      this.releaseMouseButtons(this.mouseButtonFlags, this.lastTouchX, this.lastTouchY, 'mouse.releaseOnExit')
    }
    this.clearState()
  }

  handleMouse(event: MouseEvent): void {
    const rawButtonMask = this.mouseButtonToPointerMask(event.button)
    const pressedMask = this.getPressedMouseButtonMask(event)
    const pressedButtonMask = this.getPrimaryPressedMouseButtonMask(pressedMask)
    const buttonMask = rawButtonMask !== 0 ? rawButtonMask : pressedButtonMask
    this.logInputEventCallback('mouse',
      `action=${event.action} button=${event.button} raw=${RemotePointerInput.formatButtonMask(rawButtonMask)} ` +
        `pressed=${RemotePointerInput.formatButtonMask(pressedMask)} x=${RemotePointerInput.formatNumber(event.x)} ` +
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
      if (buttonMask === 0) {
        this.appendLogCallback('Mouse press skipped: no mapped button')
        return
      }
      this.sendMove(event.x, event.y, REMOTE_BUTTON_NONE, false, 'mouse.press.move')
      if (this.sendButton(buttonMask, true, event.x, event.y, false, 'mouse.press.down')) {
        this.mouseButtonFlags |= buttonMask
      }
      return
    }
    if (event.action === MouseAction.Release) {
      const releaseMask = buttonMask !== 0 ? buttonMask : this.mouseButtonFlags
      if (releaseMask !== 0) {
        this.sendMove(event.x, event.y, REMOTE_BUTTON_NONE, true, 'mouse.release.move')
        this.releaseMouseButtons(releaseMask, event.x, event.y, 'mouse.release')
      }
      return
    }
    if (event.action === MouseAction.Move || event.action === MouseAction.Hover) {
      if (pressedMask !== 0) {
        this.sendMove(event.x, event.y, pressedMask, true, 'mouse.drag.move')
      } else {
        this.sendMove(event.x, event.y, REMOTE_BUTTON_NONE, false, 'mouse.move')
      }
      return
    }
    if (event.action === MouseAction.CANCEL) {
      if (this.mouseButtonFlags !== 0) {
        this.sendMove(event.x, event.y, REMOTE_BUTTON_NONE, true, 'mouse.cancel.move')
        this.releaseMouseButtons(this.mouseButtonFlags, event.x, event.y, 'mouse.cancel')
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
    const axisX = event.x
    const axisY = event.y
    this.logInputEventCallback('axis',
      `action=${event.action} dx=${RemotePointerInput.formatNumber(deltaX)} dy=${RemotePointerInput.formatNumber(deltaY)} ` +
        `x=${RemotePointerInput.formatNumber(axisX)} y=${RemotePointerInput.formatNumber(axisY)}`)

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
        this.sendMove(x, y, REMOTE_BUTTON_LEFT, true, 'touch.drag.move')
      } else {
        this.sendMove(x, y, REMOTE_BUTTON_NONE, false, 'touch.move')
      }
      return
    }

    if (event.type === TouchType.Up || event.type === TouchType.Cancel) {
      this.finishSingleTouch(x, y, event.type === TouchType.Cancel)
    }
  }

  private mouseButtonToPointerMask(button: MouseButton): number {
    if (button === MouseButton.Left) {
      return REMOTE_BUTTON_LEFT
    }
    if (button === MouseButton.Right) {
      return REMOTE_BUTTON_RIGHT
    }
    if (button === MouseButton.Middle) {
      return REMOTE_BUTTON_MIDDLE
    }
    return REMOTE_BUTTON_NONE
  }

  private getPressedMouseButtonMask(event: MouseEvent): number {
    let mask = this.mouseButtonFlags
    if (event.pressedButtons) {
      mask = REMOTE_BUTTON_NONE
      for (let i = 0; i < event.pressedButtons.length; i++) {
        mask |= this.mouseButtonToPointerMask(event.pressedButtons[i])
      }
    }
    return mask
  }

  private getPrimaryPressedMouseButtonMask(mask: number): number {
    if ((mask & REMOTE_BUTTON_LEFT) !== 0) {
      return REMOTE_BUTTON_LEFT
    }
    if ((mask & REMOTE_BUTTON_RIGHT) !== 0) {
      return REMOTE_BUTTON_RIGHT
    }
    if ((mask & REMOTE_BUTTON_MIDDLE) !== 0) {
      return REMOTE_BUTTON_MIDDLE
    }
    return REMOTE_BUTTON_NONE
  }

  private isSyntheticMouseAfterTouch(): boolean {
    return Date.now() - this.lastTouchEventAtMs < SYNTHETIC_MOUSE_SUPPRESS_MS
  }

  private sendMove(x: number, y: number, buttons: number = REMOTE_BUTTON_NONE,
    allowClamp: boolean = false, label: string = 'pointer.move'): boolean {
    return this.sendPointerCallback({
      action: 'move',
      buttons: buttons,
      x: x,
      y: y,
      allowClamp: allowClamp,
      traceLabel: label
    })
  }

  private sendButton(buttonMask: number, down: boolean, x: number, y: number,
    allowClamp: boolean, label: string): boolean {
    let sent = false
    sent = this.sendSingleButton(REMOTE_BUTTON_LEFT, buttonMask, down, x, y, allowClamp, label) || sent
    sent = this.sendSingleButton(REMOTE_BUTTON_RIGHT, buttonMask, down, x, y, allowClamp, label) || sent
    sent = this.sendSingleButton(REMOTE_BUTTON_MIDDLE, buttonMask, down, x, y, allowClamp, label) || sent
    return sent
  }

  private sendSingleButton(button: number, buttonMask: number, down: boolean, x: number, y: number,
    allowClamp: boolean, label: string): boolean {
    if ((buttonMask & button) === 0) {
      return false
    }
    return this.sendPointerCallback({
      action: down ? 'buttonDown' : 'buttonUp',
      button: RemotePointerInput.buttonName(button),
      buttons: button,
      x: x,
      y: y,
      allowClamp: allowClamp,
      traceLabel: label
    })
  }

  private releaseMouseButtons(buttonMask: number, x: number, y: number, label: string): void {
    if (this.sendButton(buttonMask, false, x, y, true, `${label}.up`)) {
      this.mouseButtonFlags &= ~buttonMask
    }
  }

  private sendWheel(deltaY: number, x: number, y: number): void {
    this.sendPointerCallback({
      action: 'wheelVertical',
      x: x,
      y: y,
      delta: deltaY,
      traceLabel: 'wheel.vertical'
    })
  }

  private sendHorizontalWheel(deltaX: number, x: number, y: number): void {
    this.sendPointerCallback({
      action: 'wheelHorizontal',
      x: x,
      y: y,
      delta: deltaX,
      traceLabel: 'wheel.horizontal'
    })
  }

  private sendLeftButtonDown(x: number, y: number, label: string): void {
    this.sendMove(x, y, REMOTE_BUTTON_NONE, false, `${label}.move`)
    if (this.sendButton(REMOTE_BUTTON_LEFT, true, x, y, false, `${label}.down`)) {
      this.pointerDown = true
    }
  }

  private sendLeftButtonUp(x: number, y: number, label: string): void {
    this.sendMove(x, y, REMOTE_BUTTON_NONE, true, `${label}.move`)
    this.sendButton(REMOTE_BUTTON_LEFT, false, x, y, true, `${label}.up`)
  }

  private sendLeftClick(x: number, y: number, label: string = 'tap.left'): void {
    this.sendMove(x, y, REMOTE_BUTTON_NONE, false, `${label}.move`)
    this.sendButton(REMOTE_BUTTON_LEFT, true, x, y, false, `${label}.down`)
    this.sendButton(REMOTE_BUTTON_LEFT, false, x, y, false, `${label}.up`)
  }

  private sendRightClick(x: number, y: number, label: string = 'longPress.right'): void {
    this.sendMove(x, y, REMOTE_BUTTON_NONE, false, `${label}.move`)
    this.sendButton(REMOTE_BUTTON_RIGHT, true, x, y, false, `${label}.down`)
    this.sendButton(REMOTE_BUTTON_RIGHT, false, x, y, false, `${label}.up`)
  }

  private startPendingTouch(x: number, y: number): void {
    this.logInputEventCallback('touch.down',
      `x=${RemotePointerInput.formatNumber(x)} y=${RemotePointerInput.formatNumber(y)}`,
      true)
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
    this.logInputEventCallback('touch.drag',
      `start ${RemotePointerInput.formatNumber(this.pendingTouchStartX)},` +
        `${RemotePointerInput.formatNumber(this.pendingTouchStartY)} -> ${RemotePointerInput.formatNumber(x)},` +
        `${RemotePointerInput.formatNumber(y)}`,
      true)
    this.sendLeftButtonDown(this.pendingTouchStartX, this.pendingTouchStartY, 'touch.drag.start')
    this.sendMove(x, y, REMOTE_BUTTON_LEFT, true, 'touch.drag.move')
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

  private static buttonName(button: number): string {
    if (button === REMOTE_BUTTON_LEFT) {
      return 'left'
    }
    if (button === REMOTE_BUTTON_RIGHT) {
      return 'right'
    }
    if (button === REMOTE_BUTTON_MIDDLE) {
      return 'middle'
    }
    return ''
  }

  private static formatNumber(value: number): string {
    return `${Math.round(value * 10) / 10}`
  }

  private static formatButtonMask(mask: number): string {
    return `0x${mask.toString(16).toUpperCase()}`
  }
}
