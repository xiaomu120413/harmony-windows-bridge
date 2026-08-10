import { connection } from '@kit.NetworkKit'
import { BasicSettingsPage } from './settings/BasicSettingsPage'
import { LayoutMode } from '../adaptive/WindowLayoutPolicy'
import { ProjectHelpPage } from './settings/ProjectHelpPage'
import { RemoteControlSettingsPage } from './settings/RemoteControlSettingsPage'
import { SettingsOverviewPage } from './settings/SettingsOverviewPage'
import { XrdpServerStatus } from '../rdp/RemoteControlCoordinator'
import {
  SettingsNetworkText,
  SettingsRemoteControlSection,
  SettingsRemoteText,
  SettingsRoute,
  SettingsShellText
} from './settings/SettingsConstants'
import {
  SettingsAccent,
  SettingsAppearanceMode,
  SettingsBackButton,
  SettingsDesktopNavItem,
  SettingsResources,
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
  onStopXrdpServer: () => Promise<XrdpServerStatus> = () => {
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
  onExportDiagnostics: () => Promise<string> = () => {
    return Promise.resolve('')
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
  @StorageLink('settingsSystemDark') private systemDark: boolean = false

  aboutToAppear(): void {
    this.appearanceMode = SettingsTheme.getStoredAppearanceMode()
    this.pageName = this.initialPageName.length > 0 ? this.initialPageName : SettingsRoute.SETTINGS
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
    } else if (this.pageName === SettingsRoute.REMOTE_CONTROL) {
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
        passiveControlAvailable: this.remoteControlServerAvailable,
        onRemoteAccessCodeGateChange: this.onRemoteAccessCodeGateChange,
        onRemoteAccessCodeRegenerate: this.onRemoteAccessCodeRegenerate,
        onRequestScreenRecordingPermission: this.onRequestScreenRecordingPermission,
        onRefreshScreenRecordingPermission: this.onRefreshScreenRecordingPermission,
        onRequestInputInjectionPermission: this.onRequestInputInjectionPermission,
        onRefreshInputInjectionPermission: this.onRefreshInputInjectionPermission,
        onRefreshXrdpServerStatus: this.onRefreshXrdpServerStatus,
        onStartXrdpServer: this.onStartXrdpServer,
        onStopXrdpServer: this.onStopXrdpServer,
        onRefreshNetwork: (): void => {
          this.refreshLocalIpAddress()
        },
        onOpenRemoteFilesDirectory: this.onOpenRemoteFilesDirectory,
        onExportDiagnostics: this.onExportDiagnostics,
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
        SettingsOverviewPage({
          layoutMode: this.layoutMode,
          isDark: this.isDarkMode(),
          appearanceMode: this.appearanceMode,
          localIpAddress: this.localIpAddress,
          remoteControlServerAvailable: this.remoteControlServerAvailable,
          screenRecordingPermissionGranted: this.screenRecordingPermissionGranted,
          screenRecordingPermissionBusy: this.screenRecordingPermissionBusy,
          inputInjectionPermissionGranted: this.inputInjectionPermissionGranted,
          inputInjectionPermissionBusy: this.inputInjectionPermissionBusy,
          xrdpServerRunning: this.xrdpServerRunning,
          xrdpServerState: this.xrdpServerState,
          xrdpServerPort: this.xrdpServerPort,
          xrdpServerBusy: this.xrdpServerBusy,
          onOpenBasic: (): void => {
            this.pageName = SettingsRoute.BASIC
            this.remoteControlTargetSection = ''
          },
          onOpenRemoteControl: (): void => {
            this.pageName = SettingsRoute.REMOTE_CONTROL
            this.remoteControlTargetSection = ''
          },
          onOpenHelp: (): void => {
            this.pageName = SettingsRoute.PROJECT_HELP
            this.remoteControlTargetSection = ''
          }
        })
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
