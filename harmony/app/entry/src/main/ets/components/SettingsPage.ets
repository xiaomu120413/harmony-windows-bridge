import { BasicSettingsPage } from './settings/BasicSettingsPage'
import { ProjectHelpPage } from './settings/ProjectHelpPage'
import { RemoteControlSettingsPage } from './settings/RemoteControlSettingsPage'
import { XrdpServerStatus } from '../rdp/XrdpServerController'
import { SettingsRoute, SettingsText } from './settings/SettingsConstants'
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
  @Prop xrdpServerRunning: boolean = false
  @Prop xrdpServerState: string = 'Stopped'
  @Prop xrdpServerPort: number = 3390
  @Prop xrdpServerMessage: string = ''
  @Prop xrdpServerBusy: boolean = false
  @State private pageName: string = SettingsRoute.SETTINGS
  @State private remoteControlTargetSection: string = ''
  @State private appearanceMode: SettingsAppearanceMode = SettingsTheme.getStoredAppearanceMode()
  @StorageLink('settingsSystemDark') private systemDark: boolean = false

  aboutToAppear(): void {
    this.appearanceMode = SettingsTheme.getStoredAppearanceMode()
    this.pageName = this.initialPageName.length > 0 ? this.initialPageName : SettingsRoute.SETTINGS
    this.remoteControlTargetSection = this.pageName === SettingsRoute.REMOTE_CONTROL ?
      this.initialRemoteControlSection : ''
  }

  private isDarkMode(): boolean {
    if (this.appearanceMode === 'system') {
      return this.systemDark
    }
    return SettingsTheme.isDark(this.getUIContext(), this.appearanceMode)
  }

  private xrdpStatusTone(): SettingsStatusTone {
    if (this.xrdpServerBusy) {
      return 'info'
    }
    if (this.xrdpServerState === 'Exited' || this.xrdpServerState === 'Failed') {
      return 'danger'
    }
    if (this.xrdpServerRunning) {
      return 'ok'
    }
    return 'neutral'
  }

  private xrdpStatusText(): string {
    if (this.xrdpServerBusy) {
      return SettingsText.REMOTE_SERVER_BUSY
    }
    if (this.xrdpServerRunning) {
      return `${SettingsText.REMOTE_SERVER_RUNNING} · ${this.xrdpServerPort}`
    }
    return SettingsText.REMOTE_SERVER_STOPPED
  }

  private screenStatusTone(): SettingsStatusTone {
    return this.screenRecordingPermissionGranted ? 'ok' : 'warning'
  }

  private screenStatusText(): string {
    return this.screenRecordingPermissionGranted ? SettingsText.REMOTE_SCREEN_PERMISSION_GRANTED :
      SettingsText.REMOTE_SCREEN_PERMISSION_MISSING
  }

  private gateStatusTone(): SettingsStatusTone {
    return this.remoteAccessCodeGateEnabled ? 'ok' : 'neutral'
  }

  private gateStatusText(): string {
    return this.remoteAccessCodeGateEnabled ? SettingsText.REMOTE_ACCESS_GATE_ON : SettingsText.REMOTE_ACCESS_GATE_OFF
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
          Text(SettingsText.SETTINGS_TITLE)
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

      SettingsDesktopNavItem({
        title: SettingsText.SETTINGS_NAV_OVERVIEW,
        description: SettingsText.SETTINGS_CURRENT_STATE,
        iconResource: SettingsResources.DASHBOARD_ICON,
        accentName: SettingsAccent.BLUE,
        selected: this.pageName === SettingsRoute.SETTINGS,
        isDark: this.isDarkMode(),
        onPress: () => {
          this.pageName = SettingsRoute.SETTINGS
          this.remoteControlTargetSection = ''
        }
      })
      SettingsDesktopNavItem({
        title: SettingsText.BASIC_SETTINGS_ENTRY_TITLE,
        description: SettingsText.BASIC_SETTINGS_ENTRY_DESC,
        value: SettingsText.appearanceValue(this.appearanceMode),
        iconResource: SettingsResources.SETTINGS_ICON,
        accentName: SettingsAccent.CYAN,
        selected: this.pageName === SettingsRoute.BASIC,
        isDark: this.isDarkMode(),
        onPress: () => {
          this.pageName = SettingsRoute.BASIC
          this.remoteControlTargetSection = ''
        }
      })
      SettingsDesktopNavItem({
        title: SettingsText.REMOTE_CONTROL_ENTRY_TITLE,
        description: SettingsText.REMOTE_CONTROL_ENTRY_DESC,
        value: this.xrdpStatusText(),
        iconResource: SettingsResources.DASHBOARD_ICON,
        accentName: SettingsAccent.BLUE,
        selected: this.pageName === SettingsRoute.REMOTE_CONTROL,
        isDark: this.isDarkMode(),
        onPress: () => {
          this.pageName = SettingsRoute.REMOTE_CONTROL
          this.remoteControlTargetSection = ''
        }
      })
      SettingsDesktopNavItem({
        title: SettingsText.PROJECT_HELP_ENTRY_TITLE,
        description: SettingsText.PROJECT_HELP_ENTRY_DESC,
        iconResource: SettingsResources.HELP_ICON,
        accentName: SettingsAccent.PURPLE,
        selected: this.pageName === SettingsRoute.PROJECT_HELP,
        isDark: this.isDarkMode(),
        onPress: () => {
          this.pageName = SettingsRoute.PROJECT_HELP
          this.remoteControlTargetSection = ''
        }
      })
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
      Text(SettingsText.SETTINGS_OVERVIEW_TITLE)
        .fontSize(24)
        .fontWeight(FontWeight.Bold)
        .fontColor(SettingsTheme.primaryText(this.isDarkMode()))
        .width('100%')
        .margin({ bottom: 6 })
      Text(SettingsText.SETTINGS_OVERVIEW_SUBTITLE)
        .fontSize(13)
        .fontColor(SettingsTheme.mutedText(this.isDarkMode()))
        .lineHeight(20)
        .width('100%')
        .margin({ bottom: 18 })

      Column() {
        Row() {
          Text(SettingsText.SETTINGS_CURRENT_STATE)
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

        Row({ space: 24 }) {
          Column() {
            SettingsKeyValueRow({
              label: SettingsText.BASIC_APPEARANCE_SECTION,
              value: SettingsText.appearanceValue(this.appearanceMode),
              isDark: this.isDarkMode()
            })
            SettingsKeyValueRow({
              label: SettingsText.REMOTE_SERVER_STATUS_LABEL,
              value: this.xrdpStatusText(),
              tone: this.xrdpStatusTone(),
              isDark: this.isDarkMode()
            })
          }
          .layoutWeight(1)

          Column() {
            SettingsKeyValueRow({
              label: SettingsText.REMOTE_SCREEN_SECTION,
              value: this.screenStatusText(),
              tone: this.screenStatusTone(),
              isDark: this.isDarkMode()
            })
            SettingsKeyValueRow({
              label: SettingsText.REMOTE_ACCESS_SECTION,
              value: this.gateStatusText(),
              tone: this.gateStatusTone(),
              isDark: this.isDarkMode()
            })
          }
          .layoutWeight(1)
        }
        .width('100%')
      }
      .width('100%')
      .padding(18)
      .backgroundColor(SettingsTheme.cardBackground(this.isDarkMode()))
      .borderRadius(SettingsTheme.CARD_RADIUS)
      .border({
        width: 1,
        color: SettingsTheme.borderColor(this.isDarkMode())
      })
      .margin({ bottom: 18 })

      Column() {
        this.overviewEntry(SettingsText.BASIC_SETTINGS_ENTRY_TITLE, SettingsText.BASIC_SETTINGS_ENTRY_DESC,
          SettingsResources.SETTINGS_ICON, SettingsAccent.CYAN, SettingsText.appearanceValue(this.appearanceMode),
          () => {
            this.pageName = SettingsRoute.BASIC
            this.remoteControlTargetSection = ''
          })
        this.overviewEntry(SettingsText.REMOTE_CONTROL_ENTRY_TITLE, SettingsText.REMOTE_CONTROL_ENTRY_DESC,
          SettingsResources.DASHBOARD_ICON, SettingsAccent.BLUE, this.xrdpStatusText(), () => {
            this.pageName = SettingsRoute.REMOTE_CONTROL
            this.remoteControlTargetSection = ''
          })
        this.overviewEntry(SettingsText.PROJECT_HELP_ENTRY_TITLE, SettingsText.PROJECT_HELP_ENTRY_DESC,
          SettingsResources.HELP_ICON, SettingsAccent.PURPLE, '', () => {
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

  build() {
    Row({ space: 16 }) {
      this.buildDesktopNav()

      Column() {
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
            targetSection: this.remoteControlTargetSection,
            remoteAccessCode: this.remoteAccessCode,
            remoteAccessCodeGateEnabled: this.remoteAccessCodeGateEnabled,
            screenRecordingPermissionGranted: this.screenRecordingPermissionGranted,
            xrdpServerRunning: this.xrdpServerRunning,
            xrdpServerState: this.xrdpServerState,
            xrdpServerPort: this.xrdpServerPort,
            xrdpServerMessage: this.xrdpServerMessage,
            xrdpServerBusy: this.xrdpServerBusy,
            onRemoteAccessCodeGateChange: this.onRemoteAccessCodeGateChange,
            onRemoteAccessCodeRegenerate: this.onRemoteAccessCodeRegenerate,
            onRequestScreenRecordingPermission: this.onRequestScreenRecordingPermission,
            onRefreshScreenRecordingPermission: this.onRefreshScreenRecordingPermission,
            onRefreshXrdpServerStatus: this.onRefreshXrdpServerStatus,
            onStartXrdpServer: this.onStartXrdpServer,
            onOpenRemoteFilesDirectory: this.onOpenRemoteFilesDirectory,
            onBack: () => {
              this.pageName = SettingsRoute.SETTINGS
              this.remoteControlTargetSection = ''
            }
          })
        } else if (this.pageName === SettingsRoute.PROJECT_HELP) {
          ProjectHelpPage({
            isDark: this.isDarkMode(),
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
      .layoutWeight(1)
      .height('100%')
      .alignItems(HorizontalAlign.Start)
      .justifyContent(FlexAlign.Start)
      .backgroundColor(SettingsTheme.pageBackground(this.isDarkMode()))
      .borderRadius(16)
    }
    .width('100%')
    .height('100%')
    .padding(16)
    .backgroundColor(SettingsTheme.pageBackground(this.isDarkMode()))
  }
}
