import { SettingsText } from '../settings/SettingsConstants'
import { SettingsAccent, SettingsResources, SettingsStatusTone, SettingsTheme } from '../settings/SettingsUi'
import { HomeText } from './HomeText'
import { HomeTheme } from './HomeTheme'

@Component
export struct HomeHeader {
  @Prop isDark: boolean = false
  @Prop xrdpServerBusy: boolean = false
  @Prop xrdpServerState: string = 'Stopped'
  @Prop xrdpServerRunning: boolean = false
  onOpenSettings: (remoteControlSection: string) => void = (_remoteControlSection: string) => {
  }
  @State private settingsHovered: boolean = false
  @State private settingsPressed: boolean = false
  private readonly headerSideWidth: number = 260

  private headerStatusTone(): SettingsStatusTone {
    return SettingsText.remoteServerStatusTone(this.xrdpServerRunning, this.xrdpServerBusy, this.xrdpServerState)
  }

  private headerStatusText(): string {
    if (this.xrdpServerBusy) {
      return SettingsText.REMOTE_SERVER_BUSY
    }
    return SettingsText.remoteServerStateLabel(this.xrdpServerRunning, this.xrdpServerState)
  }

  @Builder
  private buildHeaderStatus() {
    Row({ space: 10 }) {
      Circle()
        .width(9)
        .height(9)
        .fill(SettingsTheme.statusText(this.isDark, this.headerStatusTone()))
      Text(this.headerStatusText())
        .fontSize(15)
        .fontWeight(FontWeight.Medium)
        .fontColor(SettingsTheme.primaryText(this.isDark))
        .maxLines(1)
    }
    .alignItems(VerticalAlign.Center)
  }

  @Builder
  private buildSettingsButton() {
    Row({ space: 8 }) {
      Image(SettingsResources.SETTINGS_ICON)
        .width(19)
        .height(19)
        .fillColor(SettingsTheme.accentColor(this.isDark, SettingsAccent.BLUE))
        .draggable(false)
      Text(SettingsText.SETTINGS_TITLE)
        .fontSize(14)
        .fontWeight(FontWeight.Medium)
        .fontColor(SettingsTheme.primaryText(this.isDark))
        .maxLines(1)
    }
    .height(38)
    .padding({ left: 12, right: 12 })
    .alignItems(VerticalAlign.Center)
    .borderRadius(8)
    .backgroundColor(HomeTheme.neutralButtonBackground(this.isDark, this.settingsHovered, this.settingsPressed))
    .scale({ x: this.settingsPressed ? 0.98 : 1, y: this.settingsPressed ? 0.98 : 1 })
    .onHover((hovered: boolean) => {
      this.settingsHovered = hovered
    })
    .onTouch((event: TouchEvent) => {
      if (event.type === TouchType.Down) {
        this.settingsPressed = true
        return
      }
      if (event.type === TouchType.Up || event.type === TouchType.Cancel) {
        this.settingsPressed = false
      }
    })
    .onClick(() => {
      this.settingsPressed = false
      this.onOpenSettings('')
    })
  }

  build() {
    Row() {
      Row()
        .width(this.headerSideWidth)

      Text(HomeText.APP_TAGLINE)
        .fontSize(20)
        .fontWeight(FontWeight.Bold)
        .fontColor(SettingsTheme.primaryText(this.isDark))
        .maxLines(1)
        .textOverflow({ overflow: TextOverflow.Ellipsis })
        .textAlign(TextAlign.Center)
        .layoutWeight(1)

      Row({ space: 18 }) {
        this.buildHeaderStatus()
        Divider()
          .vertical(true)
          .height(26)
          .color(HomeTheme.borderColor(this.isDark))
        this.buildSettingsButton()
      }
      .width(this.headerSideWidth)
      .justifyContent(FlexAlign.End)
      .alignItems(VerticalAlign.Center)
    }
    .width('100%')
    .height(84)
    .padding({ left: 32, right: 30 })
    .alignItems(VerticalAlign.Center)
    .backgroundColor(HomeTheme.panelBackground(this.isDark))
    .border({
      width: { bottom: 1 },
      color: HomeTheme.borderColor(this.isDark)
    })
  }
}
