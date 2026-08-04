import { LayoutMode } from '../../adaptive/WindowLayoutPolicy'
import { SettingsRemoteControlSection, SettingsText } from '../settings/SettingsConstants'
import { SettingsStatusTone, SettingsTheme } from '../settings/SettingsUi'
import { HomeResources } from './HomeResources'
import { HomeText } from './HomeText'
import { HomeTheme } from './HomeTheme'

interface HomeStatusCardConfig {
  section: string
  label: string
  icon: Resource
}

const HOME_STATUS_CARDS: HomeStatusCardConfig[] = [
  {
    section: SettingsRemoteControlSection.SERVER,
    label: HomeText.STATUS_SERVICE_LABEL,
    icon: HomeResources.SHIELD_ICON
  },
  {
    section: SettingsRemoteControlSection.SCREEN,
    label: HomeText.STATUS_SCREEN_LABEL,
    icon: HomeResources.MONITOR_ICON
  },
  {
    section: SettingsRemoteControlSection.ACCESS,
    label: HomeText.STATUS_ACCESS_LABEL,
    icon: HomeResources.SHIELD_ICON
  },
  {
    section: SettingsRemoteControlSection.FILES,
    label: HomeText.STATUS_FILES_LABEL,
    icon: HomeResources.LOCK_ICON
  }
]

@Component
export struct HomeStatusFooter {
  @Prop layoutMode: LayoutMode = LayoutMode.COMPACT
  @Prop isDark: boolean = false
  @Prop xrdpServerBusy: boolean = false
  @Prop xrdpServerState: string = 'Stopped'
  @Prop xrdpServerRunning: boolean = false
  @Prop xrdpServerPort: number = 3390
  @Prop screenRecordingPermissionGranted: boolean = false
  @Prop screenRecordingPermissionBusy: boolean = false
  @Prop remoteAccessCodeGateEnabled: boolean = false
  @Prop remoteControlServerAvailable: boolean = true
  onOpenSettings: (remoteControlSection: string) => void = (_remoteControlSection: string) => {
  }
  @State private hoveredSection: string = ''
  @State private pressedSection: string = ''

  private serviceStatusTone(): SettingsStatusTone {
    return SettingsText.remoteServerStatusTone(this.xrdpServerRunning, this.xrdpServerBusy, this.xrdpServerState)
  }

  private serviceStatusText(): string {
    return SettingsText.remoteServerStatusText(this.xrdpServerRunning, this.xrdpServerBusy, this.xrdpServerState,
      this.xrdpServerPort)
  }

  private screenStatusTone(): SettingsStatusTone {
    if (this.screenRecordingPermissionBusy) {
      return 'info'
    }
    return this.screenRecordingPermissionGranted ? 'ok' : 'warning'
  }

  private screenStatusText(): string {
    if (this.screenRecordingPermissionBusy) {
      return SettingsText.REMOTE_SCREEN_PERMISSION_BUSY
    }
    return this.screenRecordingPermissionGranted ? SettingsText.REMOTE_SCREEN_PERMISSION_GRANTED :
    SettingsText.REMOTE_SCREEN_PERMISSION_MISSING
  }

  private accessStatusTone(): SettingsStatusTone {
    return this.remoteAccessCodeGateEnabled ? 'ok' : 'neutral'
  }

  private accessStatusText(): string {
    return this.remoteAccessCodeGateEnabled ? SettingsText.REMOTE_ACCESS_GATE_ON : SettingsText.REMOTE_ACCESS_GATE_OFF
  }

  private cardValue(section: string): string {
    const values = new Map<string, string>()
    values.set(SettingsRemoteControlSection.SERVER, this.serviceStatusText())
    values.set(SettingsRemoteControlSection.SCREEN, this.screenStatusText())
    values.set(SettingsRemoteControlSection.ACCESS, this.accessStatusText())
    values.set(SettingsRemoteControlSection.FILES, HomeText.STATUS_FILES_CONFIGURED)
    return values.get(section) || HomeText.STATUS_FILES_CONFIGURED
  }

  private cardTone(section: string): SettingsStatusTone {
    const tones = new Map<string, SettingsStatusTone>()
    tones.set(SettingsRemoteControlSection.SERVER, this.serviceStatusTone())
    tones.set(SettingsRemoteControlSection.SCREEN, this.screenStatusTone())
    tones.set(SettingsRemoteControlSection.ACCESS, this.accessStatusTone())
    tones.set(SettingsRemoteControlSection.FILES, 'ok')
    return tones.get(section) || 'ok'
  }

  private visibleStatusCards(): HomeStatusCardConfig[] {
    if (this.remoteControlServerAvailable) {
      return HOME_STATUS_CARDS
    }
    return [HOME_STATUS_CARDS[3]]
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
  private buildStatusCard(config: HomeStatusCardConfig, expanded: boolean) {
    Row({ space: 12 }) {
      this.buildStatusIcon(config.icon, this.cardTone(config.section))
      Text(config.label)
        .fontSize(15)
        .fontWeight(FontWeight.Bold)
        .fontColor(SettingsTheme.primaryText(this.isDark))
        .maxLines(1)
      Text(this.cardValue(config.section))
        .fontSize(14)
        .fontColor(SettingsTheme.secondaryText(this.isDark))
        .maxLines(1)
        .textOverflow({ overflow: TextOverflow.Ellipsis })
        .layoutWeight(1)
      Column()
        .width(9)
        .height(9)
        .borderRadius(4.5)
        .backgroundColor(SettingsTheme.statusText(this.isDark, this.cardTone(config.section)))
    }
    .height(52)
    .width(expanded ? 'auto' : '100%')
    .layoutWeight(expanded ? 1 : 0)
    .padding({ left: 14, right: 14 })
    .alignItems(VerticalAlign.Center)
    .backgroundColor(this.hoveredSection === config.section || this.pressedSection === config.section ?
      HomeTheme.inactivePanelBackground(this.isDark, this.hoveredSection === config.section,
        this.pressedSection === config.section) :
      HomeTheme.panelBackground(this.isDark))
    .borderRadius(8)
    .border({
      width: 1,
      color: this.hoveredSection === config.section || this.pressedSection === config.section ?
        SettingsTheme.statusText(this.isDark, this.cardTone(config.section)) : HomeTheme.mutedBorderColor(this.isDark)
    })
    .scale({
      x: this.pressedSection === config.section ? 0.99 : 1,
      y: this.pressedSection === config.section ? 0.99 : 1
    })
    .onHover((hovered: boolean) => {
      this.setHoveredSection(config.section, hovered)
    })
    .onTouch((event: TouchEvent) => {
      if (event.type === TouchType.Down) {
        this.pressedSection = config.section
        return
      }
      if (event.type === TouchType.Up || event.type === TouchType.Cancel) {
        this.pressedSection = ''
      }
    })
    .onClick(() => {
      this.pressedSection = ''
      this.onOpenSettings(config.section)
    })
  }

  @Builder
  private buildExpandedFooter() {
    Row({ space: 22 }) {
      ForEach(this.visibleStatusCards(), (config: HomeStatusCardConfig) => {
        this.buildStatusCard(config, true)
      }, (config: HomeStatusCardConfig): string => config.section)
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

  @Builder
  private buildCompactFooter() {
    GridRow({ columns: 2, gutter: { x: 12, y: 12 } }) {
      ForEach(this.visibleStatusCards(), (config: HomeStatusCardConfig) => {
        GridCol({ span: 1 }) {
          this.buildStatusCard(config, false)
        }
      }, (config: HomeStatusCardConfig): string => config.section)
    }
    .width('100%')
    .padding({ left: 12, right: 12, top: 8, bottom: 10 })
    .backgroundColor(HomeTheme.appBackground(this.isDark))
    .border({
      width: { top: 1 },
      color: HomeTheme.borderColor(this.isDark)
    })
  }

  build() {
    Stack() {
      if (this.layoutMode === LayoutMode.COMPACT) {
        this.buildCompactFooter()
      } else {
        this.buildExpandedFooter()
      }
    }
    .width('100%')
  }
}
