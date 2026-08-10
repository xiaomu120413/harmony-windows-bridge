import { SettingsRemoteControlSection, SettingsRemoteText } from './SettingsConstants'
import { SettingsAccent, SettingsResources, SettingsTheme } from './SettingsUi'

@Component
export struct RemoteFilesCard {
  @Prop isDark: boolean = false
  @Prop targetSection: string = ''
  onOpenRemoteFilesDirectory: () => void = () => {
  }
  @State private hovered: boolean = false
  @State private pressed: boolean = false

  private isTarget(): boolean {
    return this.targetSection === SettingsRemoteControlSection.FILES
  }

  private setHovered(hovered: boolean): void {
    SettingsTheme.animate(this.getUIContext(), () => {
      this.hovered = hovered
    })
  }

  build() {
    Row({ space: 14 }) {
      Column() {
        Image(SettingsResources.REMOTE_FILES_ICON)
          .width(SettingsTheme.ICON_SIZE)
          .height(SettingsTheme.ICON_SIZE)
          .fillColor(SettingsTheme.accentColor(this.isDark, SettingsAccent.PURPLE))
          .draggable(false)
      }
      .width(SettingsTheme.ICON_TILE_SIZE)
      .height(SettingsTheme.ICON_TILE_SIZE)
      .justifyContent(FlexAlign.Center)
      .alignItems(HorizontalAlign.Center)
      .borderRadius(SettingsTheme.BUTTON_RADIUS)
      .backgroundColor(SettingsTheme.accentBackground(this.isDark, SettingsAccent.PURPLE))

      Column({ space: 5 }) {
        Text(SettingsRemoteText.FILES_TITLE)
          .fontSize(15)
          .fontWeight(FontWeight.Medium)
          .fontColor(SettingsTheme.primaryText(this.isDark))
          .width('100%')
        Text(SettingsRemoteText.FILES_DESC)
          .fontSize(13)
          .fontColor(SettingsTheme.mutedText(this.isDark))
          .lineHeight(19)
          .width('100%')
          .maxLines(2)
          .textOverflow({ overflow: TextOverflow.Ellipsis })
      }
      .layoutWeight(1)

      Column() {
        Button(SettingsRemoteText.FILES_ACTION)
          .type(ButtonType.Normal)
          .constraintSize({ minHeight: SettingsTheme.SECONDARY_ACTION_HEIGHT })
          .borderRadius(SettingsTheme.BUTTON_RADIUS)
          .backgroundColor(SettingsTheme.subtleButton(this.isDark, false, false))
          .fontColor(SettingsTheme.primaryText(this.isDark))
          .fontSize(12)
          .stateEffect(false)
          .onClick(() => {
            this.onOpenRemoteFilesDirectory()
          })
      }
    }
    .alignItems(VerticalAlign.Center)
    .width('100%')
    .padding(16)
    .backgroundColor(this.hovered || this.pressed || this.isTarget() ?
      SettingsTheme.cardHoverBackground(this.isDark) : SettingsTheme.cardBackground(this.isDark))
    .borderRadius(SettingsTheme.CARD_RADIUS)
    .border({
      width: 1,
      color: this.isTarget() ? SettingsTheme.accentColor(this.isDark, SettingsAccent.PURPLE) :
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
      this.setHovered(isHover)
    })
    .onTouch((event: TouchEvent) => {
      if (event.type === TouchType.Down) {
        this.pressed = true
        return
      }
      if (event.type === TouchType.Up || event.type === TouchType.Cancel) {
        this.pressed = false
      }
    })
  }
}
