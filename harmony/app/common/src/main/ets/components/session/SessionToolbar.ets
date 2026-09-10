import { SettingsTheme } from '../settings/SettingsTheme'

export interface SessionDisplayState {
  connected: boolean
  multimon: boolean
  pending: boolean
  mode: string
  status: string
  requestedWidth: number
  requestedHeight: number
  actualWidth: number
  actualHeight: number
}

export interface SessionDisplayResult {
  ok: boolean
  state: string
  message: string
}

export function emptySessionDisplayState(): SessionDisplayState {
  return { connected: false, multimon: false, pending: false, mode: 'follow', status: 'Idle',
    requestedWidth: 0, requestedHeight: 0, actualWidth: 0, actualHeight: 0 }
}

export function unavailableDisplayResult(): SessionDisplayResult {
  return { ok: false, state: 'Unavailable', message: '' }
}

const RESOLUTION_LABELS: string[] = ['跟随窗口', '1920 × 1080', '1920 × 1200', '2560 × 1440', '2560 × 1600']
const RESOLUTION_WIDTHS: number[] = [0, 1920, 1920, 2560, 2560]
const RESOLUTION_HEIGHTS: number[] = [0, 1080, 1200, 1440, 1600]

@Component
export struct SessionToolbar {
  getDisplayState: () => SessionDisplayState = emptySessionDisplayState
  setResolution: (mode: string, width: number, height: number) => SessionDisplayResult = unavailableDisplayResult
  refreshDisplay: () => SessionDisplayResult = unavailableDisplayResult
  releaseInput: () => void = () => {}
  @Prop isDark: boolean = false
  @State private expanded: boolean = false
  @State private pinned: boolean = false
  @State private selected: number = 0
  @State private available: boolean = false
  @State private pending: boolean = false
  @State private actual: string = '等待画面'
  @State private statusText: string = ''
  @State private feedback: string = ''
  private hovering: boolean = false
  private focused: boolean = false
  private pressing: boolean = false
  private disposed: boolean = false
  private hoverTimer: number = -1
  private hideTimer: number = -1
  private pollTimer: number = -1

  aboutToAppear(): void { this.disposed = false }

  aboutToDisappear(): void {
    this.disposed = true
    this.clearTimers()
  }

  private clearTimers(): void {
    clearTimeout(this.hoverTimer)
    clearTimeout(this.hideTimer)
    clearInterval(this.pollTimer)
    this.hoverTimer = this.hideTimer = this.pollTimer = -1
  }

  private readState(): void {
    if (this.disposed) { return }
    const state: SessionDisplayState = this.getDisplayState()
    this.available = state.connected && !state.multimon
    this.pending = state.pending
    this.actual = state.actualWidth > 0 ? `${state.actualWidth} × ${state.actualHeight}` : '等待画面'
    if (state.multimon) { this.statusText = '多屏会话暂不支持手动分辨率'; return }
    if (!state.connected) { this.statusText = '连接后可调整分辨率'; return }
    if (state.status === 'Deferred') { this.statusText = '等待远端显示通道'; return }
    if (state.pending) { this.statusText = '正在调整，等待新画面…'; return }
    if (state.status === 'Timeout') { this.statusText = '未确认生效，可重试或选择跟随窗口'; return }
    if (state.status === 'Unsupported') { this.statusText = '远端不支持动态分辨率'; return }
    if (state.status === 'Failed') { this.statusText = '调整失败，请重试'; return }
    this.statusText = state.mode === 'fixed' ? '固定分辨率' : '跟随窗口'
  }

  private open(): void {
    if (this.disposed || this.expanded) { return }
    this.expanded = true
    this.feedback = ''
    this.releaseInput()
    const state = this.getDisplayState()
    this.selected = 0
    if (state.mode === 'fixed') {
      for (let i = 1; i < RESOLUTION_WIDTHS.length; i++) {
        if (RESOLUTION_WIDTHS[i] === state.requestedWidth && RESOLUTION_HEIGHTS[i] === state.requestedHeight) {
          this.selected = i
          break
        }
      }
    }
    this.readState()
    this.pollTimer = setInterval(() => { this.readState() }, 250)
  }

  private close(): void {
    this.expanded = false
    this.focused = false
    this.pressing = false
    this.clearTimers()
  }

  private scheduleHide(): void {
    clearTimeout(this.hideTimer)
    this.hideTimer = setTimeout(() => {
      if (!this.expanded || this.disposed || this.pinned || this.hovering || this.focused || this.pressing) { return }
      if (this.pending) { this.scheduleHide(); return }
      this.close()
    }, 800)
  }

  private hover(inside: boolean): void {
    this.hovering = inside
    clearTimeout(this.hoverTimer)
    if (inside) {
      clearTimeout(this.hideTimer)
      this.hoverTimer = setTimeout(() => { if (this.hovering) { this.open() } }, 250)
    } else { this.scheduleHide() }
  }

  private apply(): void {
    this.releaseInput()
    const result = this.setResolution(this.selected === 0 ? 'follow' : 'fixed',
      RESOLUTION_WIDTHS[this.selected], RESOLUTION_HEIGHTS[this.selected])
    this.feedback = result.ok ? '' : '请求未被接受，请查看当前状态'
    this.readState()
  }

  build() {
    Column() {
      if (!this.expanded) {
        Button('显示设置')
          .height(48)
          .fontSize(13)
          .fontColor(SettingsTheme.primaryText(this.isDark))
          .backgroundColor(SettingsTheme.cardBackground(this.isDark))
          .borderRadius({ bottomLeft: 12, bottomRight: 12 })
          .accessibilityText('展开远程显示设置')
          .hitTestBehavior(HitTestMode.Block)
          .onHover((inside: boolean) => { this.hover(inside) })
          .onClick(() => { this.open() })
      } else {
        Column({ space: 8 }) {
          Row({ space: 8 }) {
            Text('远程显示')
              .fontSize(16).fontWeight(FontWeight.Medium)
              .fontColor(SettingsTheme.primaryText(this.isDark)).layoutWeight(1)
            Button(this.pinned ? '取消固定' : '固定')
              .height(48).fontSize(13)
              .fontColor(SettingsTheme.primaryText(this.isDark))
              .backgroundColor(SettingsTheme.cardBackground(this.isDark))
              .border({ width: 1, color: SettingsTheme.borderColor(this.isDark) })
              .onFocus(() => { this.focused = true })
              .onBlur(() => { this.focused = false; this.scheduleHide() })
              .onClick(() => { this.pinned = !this.pinned })
            Button('收起').height(48).fontSize(13)
              .fontColor(SettingsTheme.primaryText(this.isDark))
              .backgroundColor(SettingsTheme.cardBackground(this.isDark))
              .onFocus(() => { this.focused = true })
              .onBlur(() => { this.focused = false; this.scheduleHide() })
              .onClick(() => { this.close() })
          }.width('100%')
          Flex({ wrap: FlexWrap.Wrap, alignItems: ItemAlign.Center }) {
            Select(RESOLUTION_LABELS.map((label: string): SelectOption => { return { value: label } }))
              .selected(this.selected).value(RESOLUTION_LABELS[this.selected])
              .font({ size: 14 }).width(210).height(48)
              .enabled(this.available)
              .onSelect((index: number) => { this.selected = index; this.feedback = '' })
              .onFocus(() => { this.focused = true })
              .onBlur(() => { this.focused = false; this.scheduleHide() })
            Button('应用').height(48).margin({ left: 8 }).enabled(this.available)
              .onFocus(() => { this.focused = true })
              .onBlur(() => { this.focused = false; this.scheduleHide() })
              .onClick(() => { this.apply() })
            Button('重绘画面').height(48).margin({ left: 8 }).enabled(this.available && !this.pending)
              .fontColor(SettingsTheme.primaryText(this.isDark))
              .backgroundColor(SettingsTheme.cardBackground(this.isDark))
              .border({ width: 1, color: SettingsTheme.borderColor(this.isDark) })
              .onFocus(() => { this.focused = true })
              .onBlur(() => { this.focused = false; this.scheduleHide() })
              .onClick(() => {
                const result = this.refreshDisplay()
                this.feedback = result.ok ? '已请求重绘当前画面' : '当前画面暂不能重绘'
              })
          }.width('100%')
          Text(`当前画面 ${this.actual} · ${this.statusText}`)
            .fontSize(12).fontColor(SettingsTheme.secondaryText(this.isDark))
            .width('100%')
          if (this.feedback.length > 0) {
            Text(this.feedback).fontSize(12).fontColor(SettingsTheme.secondaryText(this.isDark)).width('100%')
          }
        }
        .padding(12).width('94%').constraintSize({ maxWidth: 560 })
        .backgroundColor(SettingsTheme.cardBackground(this.isDark))
        .border({ width: 1, color: SettingsTheme.borderColor(this.isDark) })
        .borderRadius(12).shadow(SettingsTheme.shadow(this.isDark, false))
        .hitTestBehavior(HitTestMode.Block)
        .onHover((inside: boolean) => { this.hover(inside) })
        .onTouch((event: TouchEvent) => {
          this.pressing = event.type === TouchType.Down || event.type === TouchType.Move
          if (event.type === TouchType.Down) { this.releaseInput() }
        })
      }
    }
    .width('100%').alignItems(HorizontalAlign.Center)
    .hitTestBehavior(HitTestMode.Transparent)
  }
}
