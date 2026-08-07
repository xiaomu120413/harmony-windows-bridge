import { connection } from '@kit.NetworkKit'
import { BasicSettingsPage } from './settings/BasicSettingsPage'
import { LayoutMode } from '../adaptive/WindowLayoutPolicy'
import { ProjectHelpPage } from './settings/ProjectHelpPage'
import { RemoteControlSettingsPage } from './settings/RemoteControlSettingsPage'
import { XrdpServerStatus } from '../rdp/RemoteControlCoordinator'
import {
  SettingsAppearanceText,
  SettingsNetworkText,
  SettingsRemoteControlSection,
  SettingsRemoteText,
  SettingsRoute,
  SettingsShellText
} from './settings/SettingsConstants'
import {
  SettingsAccent,
  SettingsAccentName,
  SettingsAppearanceMode,
  SettingsBackButton,
  SettingsDesktopNavItem,
  SettingsKeyValueRow,
  SettingsListItem,
  SettingsResources,
  SettingsStatusChip,
  SettingsStatusTone,
  SettingsTheme
} from './settings/SettingsUi'

@Component
export struct SettingsPage {
  @Prop layoutMode: LayoutMode = LayoutMode.COMPACT
  onClose: () => void = () => {
  }
  onRemoteAccessCodeGateChange: (enabled: boolean) => string = (_enabled: boolean) => {
    return ''
  }
  onRemoteAccessCodeRegenerate: () => string = () => {
    return ''
  }
  onRequestScreenRecordingPermission: () => Promise<boolean> = () => {
    return Promise.resolve(false)
  }
  onRefreshScreenRecordingPermission: () => boolean = () => {
    return false
  }
  onRequestInputInjectionPermission: () => Promise<boolean> = () => {
    return Promise.resolve(false)
  }
  onRefreshInputInjectionPermission: () => boolean = () => {
    return false
  }
  onRefreshXrdpServerStatus: () => XrdpServerStatus = () => {
    return {
      running: false,
      state: 'Stopped',
      port: 3390,
      message: ''
    }
  }
  onStartXrdpServer: () => Promise<XrdpServerStatus> = () => {
    const status: XrdpServerStatus = {
      running: false,
      state: 'Stopped',
      port: 3390,
      message: ''
    }
    return Promise.resolve(status)
  }
  onOpenRemoteFilesDirectory: () => void = () => {
  }
  @Prop initialPageName: string = SettingsRoute.SETTINGS
  @Prop initialRemoteControlSection: string = ''
  @Prop remoteAccessCode: string = '000000'
  @Prop remoteAccessCodeGateEnabled: boolean = false
  @Prop screenRecordingPermissionGranted: boolean = false
  @Prop inputInjectionPermissionGranted: boolean = false
  @Prop xrdpServerRunning: boolean = false
  @Prop xrdpServerState: string = 'Stopped'
  @Prop xrdpServerPort: number = 3390
  @Prop xrdpServerMessage: string = ''
  @Prop xrdpServerBusy: boolean = false
  @Prop screenRecordingPermissionBusy: boolean = false
  @Prop inputInjectionPermissionBusy: boolean = false
  @Prop remoteControlServerAvailable: boolean = true
  @State private pageName: string = SettingsRoute.SETTINGS
  @State private remoteControlTargetSection: string = ''
  @State private appearanceMode: SettingsAppearanceMode = SettingsTheme.getStoredAppearanceMode()
  @State private localIpAddress: string = SettingsNetworkText.LOADING
  @State private currentStateCardHovered: boolean = false
  @State private currentStateCardPressed: boolean = false
  @StorageLink('settingsSystemDark') private systemDark: boolean = false

  aboutToAppear(): void {
    this.appearanceMode = SettingsTheme.getStoredAppearanceMode()
    this.pageName = this.initialPageName.length > 0 ? this.initialPageName : SettingsRoute.SETTINGS
    if (!this.remoteControlServerAvailable && this.pageName === SettingsRoute.REMOTE_CONTROL) {
      this.pageName = SettingsRoute.SETTINGS
    }
    this.remoteControlTargetSection = this.pageName === SettingsRoute.REMOTE_CONTROL ?
      this.initialRemoteControlSection : ''
    if (this.remoteControlServerAvailable) {
      this.refreshLocalIpAddress()
    }
  }

  private isDarkMode(): boolean {
    if (this.appearanceMode === 'system') {
      return this.systemDark
    }
    return SettingsTheme.isDark(this.getUIContext(), this.appearanceMode)
  }

  private xrdpStatusTone(): SettingsStatusTone {
    return SettingsRemoteText.serverStatusTone(this.xrdpServerRunning, this.xrdpServerBusy, this.xrdpServerState)
  }

  private xrdpStatusText(): string {
    return SettingsRemoteText.serverStatusText(this.xrdpServerRunning, this.xrdpServerBusy, this.xrdpServerState,
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
      return SettingsRemoteText.PERMISSION_BUSY
    }
    return this.screenRecordingPermissionGranted ? SettingsRemoteText.PERMISSION_GRANTED :
      SettingsRemoteText.PERMISSION_MISSING
  }

  private inputStatusTone(): SettingsStatusTone {
    if (this.inputInjectionPermissionBusy) {
      return 'info'
    }
    return this.inputInjectionPermissionGranted ? 'ok' : 'warning'
  }

  private setCurrentStateCardHovered(hovered: boolean): void {
    SettingsTheme.animate(this.getUIContext(), () => {
      this.currentStateCardHovered = hovered
    })
  }

  private inputStatusText(): string {
    if (this.inputInjectionPermissionBusy) {
      return SettingsRemoteText.PERMISSION_BUSY
    }
    return this.inputInjectionPermissionGranted ? SettingsRemoteText.PERMISSION_GRANTED :
      SettingsRemoteText.PERMISSION_MISSING
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

  private async refreshLocalIpAddress(): Promise<void> {
    this.localIpAddress = SettingsNetworkText.LOADING
    try {
      const netHandles = await connection.getAllNets()
      if (netHandles.length === 0) {
        this.localIpAddress = SettingsNetworkText.EMPTY
        return
      }
      let bestAddress = ''
      let bestScore = -1
      for (let netIndex = 0; netIndex < netHandles.length; netIndex++) {
        const netHandle = netHandles[netIndex]
        const capabilities = await connection.getNetCapabilities(netHandle)
        const properties = await connection.getConnectionProperties(netHandle)
        let bearerScore = 10
        if (capabilities.bearerTypes.indexOf(connection.NetBearType.BEARER_ETHERNET) >= 0) {
          bearerScore = 300
        } else if (capabilities.bearerTypes.indexOf(connection.NetBearType.BEARER_WIFI) >= 0) {
          bearerScore = 200
        } else if (capabilities.bearerTypes.indexOf(connection.NetBearType.BEARER_VPN) >= 0) {
          bearerScore = 0
        }
        for (let addressIndex = 0; addressIndex < properties.linkAddresses.length; addressIndex++) {
          const address = properties.linkAddresses[addressIndex].address.address
          if (!address || address === '::1' || address.indexOf('127.') === 0) {
            continue
          }
          let addressScore = address.indexOf(':') < 0 ? 30 : 5
          if (address.indexOf('192.168.') === 0 || address.indexOf('10.') === 0) {
            addressScore = 80
          } else if (address.indexOf('172.') === 0) {
            const secondPart = Number(address.split('.')[1])
            if (secondPart >= 16 && secondPart <= 31) {
              addressScore = 70
            }
          }
          const score = bearerScore + addressScore
          if (score > bestScore) {
            bestScore = score
            bestAddress = address
          }
        }
      }
      this.localIpAddress = bestAddress.length > 0 ? bestAddress : SettingsNetworkText.NO_IP
    } catch (_error) {
      this.localIpAddress = SettingsNetworkText.READ_FAILED
    }
  }

  @Builder
  private buildDesktopNav() {
    Column() {
      Row() {
        SettingsBackButton({
          isDark: this.isDarkMode(),
          onBack: () => {
            this.onClose()
          }
        })

        Column() {
          Text(SettingsShellText.TITLE)
            .fontSize(20)
            .fontWeight(FontWeight.Bold)
            .fontColor(SettingsTheme.primaryText(this.isDarkMode()))
            .width('100%')
        }
        .alignItems(HorizontalAlign.Start)
        .layoutWeight(1)
      }
      .alignItems(VerticalAlign.Center)
      .width('100%')
      .margin({ bottom: 20 })

      Column({ space: 8 }) {
        SettingsDesktopNavItem({
          title: SettingsShellText.OVERVIEW_TITLE,
          iconResource: SettingsResources.OVERVIEW_ICON,
          accentName: SettingsAccent.BLUE,
          selected: this.pageName === SettingsRoute.SETTINGS,
          isDark: this.isDarkMode(),
          onPress: () => {
            this.pageName = SettingsRoute.SETTINGS
            this.remoteControlTargetSection = ''
          }
        })
        SettingsDesktopNavItem({
          title: SettingsShellText.BASIC_TITLE,
          iconResource: SettingsResources.BASIC_ICON,
          accentName: SettingsAccent.CYAN,
          selected: this.pageName === SettingsRoute.BASIC,
          isDark: this.isDarkMode(),
          onPress: () => {
            this.pageName = SettingsRoute.BASIC
            this.remoteControlTargetSection = ''
          }
        })
        if (this.remoteControlServerAvailable) {
          SettingsDesktopNavItem({
            title: SettingsShellText.REMOTE_TITLE,
            iconResource: SettingsResources.REMOTE_CONTROL_ICON,
            accentName: SettingsAccent.BLUE,
            selected: this.pageName === SettingsRoute.REMOTE_CONTROL,
            isDark: this.isDarkMode(),
            onPress: () => {
              this.pageName = SettingsRoute.REMOTE_CONTROL
              this.remoteControlTargetSection = ''
            }
          })
        }
        SettingsDesktopNavItem({
          title: SettingsShellText.HELP_TITLE,
          iconResource: SettingsResources.PROJECT_HELP_ICON,
          accentName: SettingsAccent.PURPLE,
          selected: this.pageName === SettingsRoute.PROJECT_HELP,
          isDark: this.isDarkMode(),
          onPress: () => {
            this.pageName = SettingsRoute.PROJECT_HELP
            this.remoteControlTargetSection = ''
          }
        })
      }
      .width('100%')
    }
    .width(196)
    .height('100%')
    .padding(14)
    .backgroundColor(SettingsTheme.cardBackground(this.isDarkMode()))
    .borderRadius(16)
    .border({
      width: 1,
      color: SettingsTheme.borderColor(this.isDarkMode())
    })
  }

  @Builder
  private buildCompactTopBar() {
    Row({ space: 12 }) {
      SettingsBackButton({
        isDark: this.isDarkMode(),
        onBack: () => {
          this.onClose()
        }
      })

      Text(SettingsShellText.TITLE)
        .fontSize(20)
        .fontWeight(FontWeight.Bold)
        .fontColor(SettingsTheme.primaryText(this.isDarkMode()))
        .layoutWeight(1)
        .maxLines(1)
        .textOverflow({ overflow: TextOverflow.Ellipsis })
    }
    .width('100%')
    .padding({ left: 16, right: 16, top: 8, bottom: 8 })
    .alignItems(VerticalAlign.Center)
    .backgroundColor(SettingsTheme.pageBackground(this.isDarkMode()))
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
      isDark: this.isDarkMode(),
      onPress: onPress
    })
  }

  @Builder
  private buildOverviewPage() {
    Column() {
      Text(SettingsShellText.OVERVIEW_TITLE)
        .fontSize(24)
        .fontWeight(FontWeight.Bold)
        .fontColor(SettingsTheme.primaryText(this.isDarkMode()))
        .width('100%')
        .margin({ bottom: 6 })
      Text(this.remoteControlServerAvailable ? SettingsShellText.OVERVIEW_SUBTITLE :
        SettingsShellText.OVERVIEW_CLIENT_SUBTITLE)
        .fontSize(13)
        .fontColor(SettingsTheme.mutedText(this.isDarkMode()))
        .lineHeight(20)
        .width('100%')
        .margin({ bottom: 18 })

      if (this.remoteControlServerAvailable) {
        Column() {
          Row() {
            Text(SettingsShellText.CURRENT_STATE)
              .fontSize(15)
              .fontWeight(FontWeight.Bold)
              .fontColor(SettingsTheme.primaryText(this.isDarkMode()))
              .layoutWeight(1)

            SettingsStatusChip({
              text: this.xrdpStatusText(),
              tone: this.xrdpStatusTone(),
              isDark: this.isDarkMode()
            })
          }
          .alignItems(VerticalAlign.Center)
          .width('100%')
          .margin({ bottom: 14 })

          if (this.layoutMode === LayoutMode.EXPANDED) {
            Row({ space: 24 }) {
              Column() {
                SettingsKeyValueRow({
                  label: SettingsRemoteText.CONNECTION_ADDRESS_LABEL,
                  value: this.connectionAddress(),
                  isDark: this.isDarkMode()
                })
                SettingsKeyValueRow({
                  label: SettingsRemoteText.SERVER_STATUS_LABEL,
                  value: this.xrdpStatusText(),
                  tone: this.xrdpStatusTone(),
                  isDark: this.isDarkMode()
                })
              }
              .layoutWeight(1)

              Column() {
                SettingsKeyValueRow({
                  label: SettingsRemoteText.SCREEN_SECTION,
                  value: this.screenStatusText(),
                  tone: this.screenStatusTone(),
                  isDark: this.isDarkMode()
                })
                SettingsKeyValueRow({
                  label: SettingsRemoteText.INPUT_SECTION,
                  value: this.inputStatusText(),
                  tone: this.inputStatusTone(),
                  isDark: this.isDarkMode()
                })
              }
              .layoutWeight(1)
            }
            .width('100%')
          } else {
            Column() {
              SettingsKeyValueRow({
                label: SettingsRemoteText.CONNECTION_ADDRESS_LABEL,
                value: this.connectionAddress(),
                isDark: this.isDarkMode()
              })
              SettingsKeyValueRow({
                label: SettingsRemoteText.SERVER_STATUS_LABEL,
                value: this.xrdpStatusText(),
                tone: this.xrdpStatusTone(),
                isDark: this.isDarkMode()
              })
              SettingsKeyValueRow({
                label: SettingsRemoteText.SCREEN_SECTION,
                value: this.screenStatusText(),
                tone: this.screenStatusTone(),
                isDark: this.isDarkMode()
              })
              SettingsKeyValueRow({
                label: SettingsRemoteText.INPUT_SECTION,
                value: this.inputStatusText(),
                tone: this.inputStatusTone(),
                isDark: this.isDarkMode()
              })
            }
            .width('100%')
          }
        }
        .width('100%')
        .padding(18)
        .backgroundColor(this.currentStateCardHovered || this.currentStateCardPressed ?
          SettingsTheme.cardHoverBackground(this.isDarkMode()) : SettingsTheme.cardBackground(this.isDarkMode()))
        .borderRadius(SettingsTheme.CARD_RADIUS)
        .border({
          width: 1,
          color: SettingsTheme.borderColor(this.isDarkMode())
        })
        .shadow(SettingsTheme.shadow(this.isDarkMode(), this.currentStateCardHovered))
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

      Column() {
        this.overviewEntry(SettingsShellText.BASIC_TITLE, SettingsShellText.BASIC_DESC,
          SettingsResources.BASIC_ICON, SettingsAccent.CYAN, SettingsAppearanceText.value(this.appearanceMode),
          () => {
            this.pageName = SettingsRoute.BASIC
            this.remoteControlTargetSection = ''
          })
        if (this.remoteControlServerAvailable) {
          this.overviewEntry(SettingsShellText.REMOTE_TITLE, SettingsShellText.REMOTE_DESC,
            SettingsResources.REMOTE_CONTROL_ICON, SettingsAccent.BLUE, this.xrdpStatusText(), () => {
              this.pageName = SettingsRoute.REMOTE_CONTROL
              this.remoteControlTargetSection = ''
            })
        } else {
          this.overviewEntry(SettingsRemoteText.FILES_TITLE, SettingsRemoteText.FILES_DESC,
            SettingsResources.REMOTE_FILES_ICON, SettingsAccent.GREEN, '', () => {
              this.onOpenRemoteFilesDirectory()
            })
        }
        this.overviewEntry(SettingsShellText.HELP_TITLE, SettingsShellText.HELP_DESC,
          SettingsResources.PROJECT_HELP_ICON, SettingsAccent.PURPLE, '', () => {
            this.pageName = SettingsRoute.PROJECT_HELP
            this.remoteControlTargetSection = ''
          })
      }
      .alignItems(HorizontalAlign.Start)
      .width('100%')
    }
    .alignItems(HorizontalAlign.Start)
    .justifyContent(FlexAlign.Start)
    .width('100%')
    .padding(22)
  }

  @Builder
  private buildSettingsContent() {
    if (this.pageName === SettingsRoute.BASIC) {
      BasicSettingsPage({
        onModeChange: (mode: SettingsAppearanceMode) => {
          this.appearanceMode = mode
        },
        onBack: () => {
          this.pageName = SettingsRoute.SETTINGS
        }
      })
    } else if (this.pageName === SettingsRoute.REMOTE_CONTROL && this.remoteControlServerAvailable) {
      RemoteControlSettingsPage({
        isDark: this.isDarkMode(),
        layoutMode: this.layoutMode,
        targetSection: this.remoteControlTargetSection,
        localIpAddress: this.localIpAddress,
        remoteAccessCode: this.remoteAccessCode,
        remoteAccessCodeGateEnabled: this.remoteAccessCodeGateEnabled,
        screenRecordingPermissionGranted: this.screenRecordingPermissionGranted,
        inputInjectionPermissionGranted: this.inputInjectionPermissionGranted,
        xrdpServerRunning: this.xrdpServerRunning,
        xrdpServerState: this.xrdpServerState,
        xrdpServerPort: this.xrdpServerPort,
        xrdpServerMessage: this.xrdpServerMessage,
        xrdpServerBusy: this.xrdpServerBusy,
        screenRecordingPermissionBusy: this.screenRecordingPermissionBusy,
        inputInjectionPermissionBusy: this.inputInjectionPermissionBusy,
        onRemoteAccessCodeGateChange: this.onRemoteAccessCodeGateChange,
        onRemoteAccessCodeRegenerate: this.onRemoteAccessCodeRegenerate,
        onRequestScreenRecordingPermission: this.onRequestScreenRecordingPermission,
        onRefreshScreenRecordingPermission: this.onRefreshScreenRecordingPermission,
        onRequestInputInjectionPermission: this.onRequestInputInjectionPermission,
        onRefreshInputInjectionPermission: this.onRefreshInputInjectionPermission,
        onRefreshXrdpServerStatus: this.onRefreshXrdpServerStatus,
        onStartXrdpServer: this.onStartXrdpServer,
        onRefreshNetwork: (): void => {
          this.refreshLocalIpAddress()
        },
        onOpenRemoteFilesDirectory: this.onOpenRemoteFilesDirectory,
        onBack: () => {
          this.pageName = SettingsRoute.SETTINGS
          this.remoteControlTargetSection = ''
        }
      })
    } else if (this.pageName === SettingsRoute.PROJECT_HELP) {
      ProjectHelpPage({
        isDark: this.isDarkMode(),
        layoutMode: this.layoutMode,
        remoteControlServerAvailable: this.remoteControlServerAvailable,
        xrdpServerRunning: this.xrdpServerRunning,
        onOpenRemoteControlSettings: (): void => {
          if (this.remoteControlServerAvailable) {
            this.pageName = SettingsRoute.REMOTE_CONTROL
            this.remoteControlTargetSection = SettingsRemoteControlSection.SERVER
          }
        },
        onBack: () => {
          this.pageName = SettingsRoute.SETTINGS
        }
      })
    } else {
      Scroll() {
        this.buildOverviewPage()
      }
      .width('100%')
      .height('100%')
    }
  }

  @Builder
  private buildContentPane() {
    Column() {
      this.buildSettingsContent()
    }
    .layoutWeight(1)
    .height('100%')
    .alignItems(HorizontalAlign.Start)
    .justifyContent(FlexAlign.Start)
    .backgroundColor(SettingsTheme.pageBackground(this.isDarkMode()))
    .borderRadius(16)
  }

  @Builder
  private buildCompactLayout() {
    Column() {
      if (this.pageName === SettingsRoute.SETTINGS) {
        this.buildCompactTopBar()
      }
      this.buildContentPane()
    }
    .width('100%')
    .height('100%')
    .backgroundColor(SettingsTheme.pageBackground(this.isDarkMode()))
  }

  @Builder
  private buildExpandedLayout() {
    Row({ space: 16 }) {
      this.buildDesktopNav()
      this.buildContentPane()
    }
    .width('100%')
    .height('100%')
    .padding(16)
    .backgroundColor(SettingsTheme.pageBackground(this.isDarkMode()))
  }

  build() {
    Stack() {
      if (this.layoutMode === LayoutMode.COMPACT) {
        this.buildCompactLayout()
      } else {
        this.buildExpandedLayout()
      }
    }
    .width('100%')
    .height('100%')
  }
}
