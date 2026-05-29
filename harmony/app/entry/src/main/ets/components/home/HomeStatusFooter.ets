import { SettingsRemoteControlSection } from '../settings/SettingsConstants'
import { SettingsStatusTone, SettingsTheme } from '../settings/SettingsUi'
import { HomeResources } from './HomeResources'
import { HomeTheme } from './HomeTheme'

@Component
export struct HomeStatusFooter {
  @Prop isDark: boolean = false
  @Prop xrdpServerBusy: boolean = false
  @Prop xrdpServerState: string = 'Stopped'
  @Prop xrdpServerRunning: boolean = false
  @Prop xrdpServerPort: number = 3390
  @Prop screenRecordingPermissionGranted: boolean = false
  @Prop remoteAccessCodeGateEnabled: boolean = false
  onOpenSettings: (remoteControlSection: string) => void = (_remoteControlSection: string) => {
  }
  @State private hoveredSection: string = ''
  @State private pressedSection: string = ''

  private serviceStatusTone(): SettingsStatusTone {
    if (this.xrdpServerBusy) {
      return 'info'
    }
    if (this.xrdpServerState === 'Failed' || this.xrdpServerState === 'Exited') {
      return 'danger'
    }
    if (this.xrdpServerRunning) {
      return 'ok'
    }
    return 'neutral'
  }

  private serviceStatusText(): string {
    if (this.xrdpServerBusy) {
      return '处理中'
    }
    const status = this.xrdpServerRunning ? '已启动' : '未启动'
    return `${status} / ${this.xrdpServerPort}`
  }

  private screenStatusTone(): SettingsStatusTone {
    return this.screenRecordingPermissionGranted ? 'ok' : 'warning'
  }

  private screenStatusText(): string {
    return this.screenRecordingPermissionGranted ? '已授权' : '未授权'
  }

  private accessStatusTone(): SettingsStatusTone {
    return this.remoteAccessCodeGateEnabled ? 'ok' : 'neutral'
  }

  private accessStatusText(): string {
    return this.remoteAccessCodeGateEnabled ? '已开启' : '默认关闭'
  }

  private setHoveredSection(section: string, hovered: boolean): void {
    SettingsTheme.animate(this.getUIContext(), () => {
      this.hoveredSection = hovered ? section : ''
    })
  }

  @Builder
  private buildStatusIcon(icon: Resource, tone: SettingsStatusTone) {
    Column() {
      Image(icon)
        .width(20)
        .height(20)
        .fillColor(SettingsTheme.statusText(this.isDark, tone))
        .draggable(false)
    }
    .width(32)
    .height(32)
    .borderRadius(8)
    .justifyContent(FlexAlign.Center)
    .alignItems(HorizontalAlign.Center)
    .backgroundColor(SettingsTheme.statusBackground(this.isDark, tone))
  }

  @Builder
  private buildStatusCard(label: string, value: string, icon: Resource, tone: SettingsStatusTone, section: string) {
    Row({ space: 12 }) {
      this.buildStatusIcon(icon, tone)
      Text(label)
        .fontSize(15)
        .fontWeight(FontWeight.Bold)
        .fontColor(SettingsTheme.primaryText(this.isDark))
        .maxLines(1)
      Text(value)
        .fontSize(14)
        .fontColor(SettingsTheme.secondaryText(this.isDark))
        .maxLines(1)
        .textOverflow({ overflow: TextOverflow.Ellipsis })
        .layoutWeight(1)
      Circle()
        .width(9)
        .height(9)
        .fill(SettingsTheme.statusText(this.isDark, tone))
    }
    .height(52)
    .layoutWeight(1)
    .padding({ left: 14, right: 14 })
    .alignItems(VerticalAlign.Center)
    .backgroundColor(this.hoveredSection === section || this.pressedSection === section ?
      HomeTheme.inactivePanelBackground(this.isDark, this.hoveredSection === section, this.pressedSection === section) :
      HomeTheme.panelBackground(this.isDark))
    .borderRadius(8)
    .border({
      width: 1,
      color: this.hoveredSection === section || this.pressedSection === section ?
        SettingsTheme.statusText(this.isDark, tone) : HomeTheme.mutedBorderColor(this.isDark)
    })
    .scale({
      x: this.pressedSection === section ? 0.99 : 1,
      y: this.pressedSection === section ? 0.99 : 1
    })
    .onHover((hovered: boolean) => {
      this.setHoveredSection(section, hovered)
    })
    .onTouch((event: TouchEvent) => {
      if (event.type === TouchType.Down) {
        this.pressedSection = section
        return
      }
      if (event.type === TouchType.Up || event.type === TouchType.Cancel) {
        this.pressedSection = ''
      }
    })
    .onClick(() => {
      this.pressedSection = ''
      this.onOpenSettings(section)
    })
  }

  build() {
    Row({ space: 22 }) {
      this.buildStatusCard('被控服务', this.serviceStatusText(), HomeResources.SHIELD_ICON,
        this.serviceStatusTone(), SettingsRemoteControlSection.SERVER)
      this.buildStatusCard('录屏', this.screenStatusText(), HomeResources.MONITOR_ICON, this.screenStatusTone(),
        SettingsRemoteControlSection.SCREEN)
      this.buildStatusCard('验证码', this.accessStatusText(), HomeResources.SHIELD_ICON, this.accessStatusTone(),
        SettingsRemoteControlSection.ACCESS)
      this.buildStatusCard('共享目录', '已配置', HomeResources.LOCK_ICON, 'ok', SettingsRemoteControlSection.FILES)
    }
    .width('100%')
    .height(70)
    .padding({ left: 30, right: 30, top: 8, bottom: 10 })
    .backgroundColor(HomeTheme.appBackground(this.isDark))
    .border({
      width: { top: 1 },
      color: HomeTheme.borderColor(this.isDark)
    })
  }
}
