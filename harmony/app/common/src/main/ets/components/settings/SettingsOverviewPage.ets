import { LayoutMode } from '../../adaptive/WindowLayoutPolicy'
import {
  SettingsAppearanceText,
  SettingsNetworkText,
  SettingsRemoteText,
  SettingsShellText
} from './SettingsConstants'
import {
  SettingsAccent,
  SettingsAccentName,
  SettingsAppearanceMode,
  SettingsKeyValueRow,
  SettingsListItem,
  SettingsResources,
  SettingsStatusChip,
  SettingsStatusTone,
  SettingsTheme
} from './SettingsUi'

@Component
export struct SettingsOverviewPage {
  @Prop layoutMode: LayoutMode = LayoutMode.COMPACT
  @Prop isDark: boolean = false
  @Prop appearanceMode: SettingsAppearanceMode = 'system'
  @Prop localIpAddress: string = SettingsNetworkText.LOADING
  @Prop remoteControlServerAvailable: boolean = true
  @Prop screenRecordingPermissionGranted: boolean = false
  @Prop screenRecordingPermissionBusy: boolean = false
  @Prop inputInjectionPermissionGranted: boolean = false
  @Prop inputInjectionPermissionBusy: boolean = false
  @Prop xrdpServerRunning: boolean = false
  @Prop xrdpServerState: string = 'Stopped'
  @Prop xrdpServerPort: number = 3390
  @Prop xrdpServerBusy: boolean = false
  onOpenBasic: () => void = () => {
  }
  onOpenRemoteControl: () => void = () => {
  }
  onOpenHelp: () => void = () => {
  }
  @State private currentStateCardHovered: boolean = false
  @State private currentStateCardPressed: boolean = false

  private xrdpStatusTone(): SettingsStatusTone {
    return SettingsRemoteText.serverStatusTone(this.xrdpServerRunning, this.xrdpServerBusy, this.xrdpServerState)
  }

  private xrdpStatusText(): string {
    return SettingsRemoteText.serverStatusText(this.xrdpServerRunning, this.xrdpServerBusy, this.xrdpServerState,
      this.xrdpServerPort)
  }

  private permissionTone(granted: boolean, busy: boolean): SettingsStatusTone {
    if (busy) {
      return 'info'
    }
    return granted ? 'ok' : 'warning'
  }

  private permissionText(granted: boolean, busy: boolean): string {
    if (busy) {
      return SettingsRemoteText.PERMISSION_BUSY
    }
    return granted ? SettingsRemoteText.PERMISSION_GRANTED : SettingsRemoteText.PERMISSION_MISSING
  }

  private setCurrentStateCardHovered(hovered: boolean): void {
    SettingsTheme.animate(this.getUIContext(), () => {
      this.currentStateCardHovered = hovered
    })
  }

  private connectionAddress(): string {
    if (this.localIpAddress === SettingsNetworkText.LOADING ||
      this.localIpAddress === SettingsNetworkText.EMPTY ||
      this.localIpAddress === SettingsNetworkText.NO_IP ||
      this.localIpAddress === SettingsNetworkText.READ_FAILED) {
      return SettingsRemoteText.CONNECTION_ADDRESS_WAITING
    }
    const host = this.localIpAddress.indexOf(':') >= 0 ? `[${this.localIpAddress}]` : this.localIpAddress
    const port = this.xrdpServerPort > 0 ? this.xrdpServerPort : 3390
    return `${host}:${port}`
  }

  @Builder
  private overviewEntry(title: string, description: string, iconResource: Resource, accentName: SettingsAccentName,
    value: string, onPress: () => void) {
    SettingsListItem({
      title: title,
      description: description,
      iconResource: iconResource,
      accentName: accentName,
      value: value,
      isDark: this.isDark,
      onPress: onPress
    })
  }

  @Builder
  private currentStateCard() {
    Column() {
      Row() {
        Text(SettingsShellText.CURRENT_STATE)
          .fontSize(15)
          .fontWeight(FontWeight.Bold)
          .fontColor(SettingsTheme.primaryText(this.isDark))
          .layoutWeight(1)
        SettingsStatusChip({
          text: this.xrdpStatusText(),
          tone: this.xrdpStatusTone(),
          isDark: this.isDark
        })
      }
      .alignItems(VerticalAlign.Center)
      .width('100%')
      .margin({ bottom: 14 })

      if (this.layoutMode === LayoutMode.EXPANDED) {
        Row({ space: 24 }) {
          Column() {
            this.connectionRows()
          }
          .layoutWeight(1)
          Column() {
            this.permissionRows()
          }
          .layoutWeight(1)
        }
        .width('100%')
      } else {
        Column() {
          this.connectionRows()
          this.permissionRows()
        }
        .width('100%')
      }
    }
    .width('100%')
    .padding(18)
    .backgroundColor(this.currentStateCardHovered || this.currentStateCardPressed ?
      SettingsTheme.cardHoverBackground(this.isDark) : SettingsTheme.cardBackground(this.isDark))
    .borderRadius(SettingsTheme.CARD_RADIUS)
    .border({ width: 1, color: SettingsTheme.borderColor(this.isDark) })
    .shadow(SettingsTheme.shadow(this.isDark, this.currentStateCardHovered))
    .translate({ y: this.currentStateCardHovered ? -5 : 0 })
    .scale({
      x: this.currentStateCardPressed ? 0.99 : this.currentStateCardHovered ? 1.012 : 1,
      y: this.currentStateCardPressed ? 0.99 : this.currentStateCardHovered ? 1.012 : 1
    })
    .margin({ bottom: 18 })
    .onHover((isHover: boolean) => {
      this.setCurrentStateCardHovered(isHover)
    })
    .onTouch((event: TouchEvent) => {
      if (event.type === TouchType.Down) {
        this.currentStateCardPressed = true
        return
      }
      if (event.type === TouchType.Up || event.type === TouchType.Cancel) {
        this.currentStateCardPressed = false
      }
    })
  }

  @Builder
  private connectionRows() {
    SettingsKeyValueRow({
      label: SettingsRemoteText.CONNECTION_ADDRESS_LABEL,
      value: this.connectionAddress(),
      isDark: this.isDark
    })
    SettingsKeyValueRow({
      label: SettingsRemoteText.SERVER_STATUS_LABEL,
      value: this.xrdpStatusText(),
      tone: this.xrdpStatusTone(),
      isDark: this.isDark
    })
  }

  @Builder
  private permissionRows() {
    SettingsKeyValueRow({
      label: SettingsRemoteText.SCREEN_SECTION,
      value: this.permissionText(this.screenRecordingPermissionGranted, this.screenRecordingPermissionBusy),
      tone: this.permissionTone(this.screenRecordingPermissionGranted, this.screenRecordingPermissionBusy),
      isDark: this.isDark
    })
    SettingsKeyValueRow({
      label: SettingsRemoteText.INPUT_SECTION,
      value: this.permissionText(this.inputInjectionPermissionGranted, this.inputInjectionPermissionBusy),
      tone: this.permissionTone(this.inputInjectionPermissionGranted, this.inputInjectionPermissionBusy),
      isDark: this.isDark
    })
  }

  build() {
    Column() {
      Text(SettingsShellText.OVERVIEW_TITLE)
        .fontSize(24)
        .fontWeight(FontWeight.Bold)
        .fontColor(SettingsTheme.primaryText(this.isDark))
        .width('100%')
        .margin({ bottom: 6 })
      Text(this.remoteControlServerAvailable ? SettingsShellText.OVERVIEW_SUBTITLE :
        SettingsShellText.OVERVIEW_CLIENT_SUBTITLE)
        .fontSize(13)
        .fontColor(SettingsTheme.mutedText(this.isDark))
        .lineHeight(20)
        .width('100%')
        .margin({ bottom: 18 })

      if (this.remoteControlServerAvailable) {
        this.currentStateCard()
      }

      Column() {
        this.overviewEntry(SettingsShellText.BASIC_TITLE, SettingsShellText.BASIC_DESC,
          SettingsResources.BASIC_ICON, SettingsAccent.CYAN, SettingsAppearanceText.value(this.appearanceMode),
          this.onOpenBasic)
        this.overviewEntry(SettingsShellText.REMOTE_TITLE, this.remoteControlServerAvailable ?
          SettingsShellText.REMOTE_DESC : SettingsShellText.REMOTE_CLIENT_DESC,
          SettingsResources.REMOTE_CONTROL_ICON, SettingsAccent.BLUE,
          this.remoteControlServerAvailable ? this.xrdpStatusText() : '', this.onOpenRemoteControl)
        this.overviewEntry(SettingsShellText.HELP_TITLE, this.remoteControlServerAvailable ?
          SettingsShellText.HELP_DESC : SettingsShellText.HELP_CLIENT_DESC,
          SettingsResources.PROJECT_HELP_ICON, SettingsAccent.PURPLE, '', this.onOpenHelp)
      }
      .alignItems(HorizontalAlign.Start)
      .width('100%')
    }
    .alignItems(HorizontalAlign.Start)
    .justifyContent(FlexAlign.Start)
    .width('100%')
    .padding(22)
  }
}
