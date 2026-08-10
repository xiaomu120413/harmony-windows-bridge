import { SettingsRemoteControlSection, SettingsRemoteText } from './SettingsConstants'
import { SettingsAccent, SettingsResources, SettingsTheme } from './SettingsUi'

@Component
export struct DiagnosticsCard {
  @Prop isDark: boolean = false
  @Prop targetSection: string = ''
  onExportDiagnostics: () => Promise<string> = () => Promise.resolve('')
  @State private hovered: boolean = false
  @State private pressed: boolean = false
  @State private busy: boolean = false
  @State private message: string = ''

  private isTarget(): boolean {
    return this.targetSection === SettingsRemoteControlSection.DIAGNOSTICS
  }

  private exportDiagnostics(): void {
    if (this.busy) {
      return
    }
    this.busy = true
    this.onExportDiagnostics()
      .then((message: string): void => {
        this.message = message
      })
      .catch((_error: Error): void => {
        this.message = '诊断导出失败'
      })
      .finally((): void => {
        this.busy = false
      })
  }

  build() {
    Row({ space: 14 }) {
      Column() {
        Image(SettingsResources.PROJECT_HELP_ICON)
          .width(SettingsTheme.ICON_SIZE)
          .height(SettingsTheme.ICON_SIZE)
          .fillColor(SettingsTheme.accentColor(this.isDark, SettingsAccent.CYAN))
          .draggable(false)
      }
      .width(SettingsTheme.ICON_TILE_SIZE)
      .height(SettingsTheme.ICON_TILE_SIZE)
      .justifyContent(FlexAlign.Center)
      .alignItems(HorizontalAlign.Center)
      .borderRadius(SettingsTheme.BUTTON_RADIUS)
      .backgroundColor(SettingsTheme.accentBackground(this.isDark, SettingsAccent.CYAN))

      Column({ space: 5 }) {
        Text(SettingsRemoteText.DIAGNOSTICS_TITLE)
          .fontSize(15)
          .fontWeight(FontWeight.Medium)
          .fontColor(SettingsTheme.primaryText(this.isDark))
          .width('100%')
        Text(SettingsRemoteText.DIAGNOSTICS_DESC)
          .fontSize(13)
          .fontColor(SettingsTheme.mutedText(this.isDark))
          .lineHeight(19)
          .width('100%')
          .maxLines(3)
          .textOverflow({ overflow: TextOverflow.Ellipsis })
        if (this.message.length > 0) {
          Text(this.message)
            .fontSize(12)
            .fontColor(SettingsTheme.mutedText(this.isDark))
            .maxLines(2)
            .textOverflow({ overflow: TextOverflow.Ellipsis })
            .width('100%')
        }
      }
      .layoutWeight(1)

      Button(this.busy ? SettingsRemoteText.DIAGNOSTICS_EXPORTING : SettingsRemoteText.DIAGNOSTICS_ACTION)
        .type(ButtonType.Normal)
        .constraintSize({ minHeight: SettingsTheme.SECONDARY_ACTION_HEIGHT })
        .borderRadius(SettingsTheme.BUTTON_RADIUS)
        .backgroundColor(this.busy ? SettingsTheme.disabledButton(this.isDark) :
          SettingsTheme.subtleButton(this.isDark, false, false))
        .fontColor(this.busy ? SettingsTheme.disabledText(this.isDark) : SettingsTheme.primaryText(this.isDark))
        .fontSize(12)
        .stateEffect(false)
        .enabled(!this.busy)
        .onClick(() => this.exportDiagnostics())
    }
    .alignItems(VerticalAlign.Center)
    .width('100%')
    .padding(16)
    .backgroundColor(this.hovered || this.pressed || this.isTarget() ?
      SettingsTheme.cardHoverBackground(this.isDark) : SettingsTheme.cardBackground(this.isDark))
    .borderRadius(SettingsTheme.CARD_RADIUS)
    .border({
      width: 1,
      color: this.isTarget() ? SettingsTheme.accentColor(this.isDark, SettingsAccent.CYAN) :
        SettingsTheme.borderColor(this.isDark)
    })
    .shadow(SettingsTheme.shadow(this.isDark, this.hovered || this.isTarget()))
    .translate({ y: this.hovered ? -5 : 0 })
    .scale({
      x: this.pressed ? 0.99 : this.hovered ? 1.012 : 1,
      y: this.pressed ? 0.99 : this.hovered ? 1.012 : 1
    })
    .margin({ bottom: 12 })
    .onHover((isHover: boolean) => {
      SettingsTheme.animate(this.getUIContext(), () => this.hovered = isHover)
    })
    .onTouch((event: TouchEvent) => {
      this.pressed = event.type === TouchType.Down
    })
  }
}
