import { BasicSettingsPage } from './settings/BasicSettingsPage'
import { LayoutMode } from '../adaptive/WindowLayoutPolicy'
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
  @Prop screenRecordingPermissionBusy: boolean = false
  @Prop remoteControlServerAvailable: boolean = true
  @State private pageName: string = SettingsRoute.SETTINGS
  @State private remoteControlTargetSection: string = ''
  @State private appearanceMode: SettingsAppearanceMode = SettingsTheme.getStoredAppearanceMode()
  @StorageLink('settingsSystemDark') private systemDark: boolean = false

  aboutToAppear(): void {
    this.appearanceMode = SettingsTheme.getStoredAppearanceMode()
    this.pageName = this.initialPageName.length > 0 ? this.initialPageName : SettingsRoute.SETTINGS
    if (!this.remoteControlServerAvailable && this.pageName === SettingsRoute.REMOTE_CONTROL) {
      this.pageName = SettingsRoute.SETTINGS
    }
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
    return SettingsText.remoteServerStatusTone(this.xrdpServerRunning, this.xrdpServerBusy, this.xrdpServerState)
  }

  private xrdpStatusText(): string {
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

      Column({ space: 8 }) {
        SettingsDesktopNavItem({
          title: SettingsText.SETTINGS_NAV_OVERVIEW,
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
          title: SettingsText.BASIC_SETTINGS_ENTRY_TITLE,
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
            title: SettingsText.REMOTE_CONTROL_ENTRY_TITLE,
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
          title: SettingsText.PROJECT_HELP_ENTRY_TITLE,
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

      Text(SettingsText.SETTINGS_TITLE)
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
      Text(SettingsText.SETTINGS_OVERVIEW_TITLE)
        .fontSize(24)
        .fontWeight(FontWeight.Bold)
        .fontColor(SettingsTheme.primaryText(this.isDarkMode()))
        .width('100%')
        .margin({ bottom: 6 })
      Text(this.remoteControlServerAvailable ? SettingsText.SETTINGS_OVERVIEW_SUBTITLE :
        SettingsText.SETTINGS_OVERVIEW_CLIENT_SUBTITLE)
        .fontSize(13)
        .fontColor(SettingsTheme.mutedText(this.isDarkMode()))
        .lineHeight(20)
        .width('100%')
        .margin({ bottom: 18 })

      if (this.remoteControlServerAvailable) {
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
      }

      Column() {
        this.overviewEntry(SettingsText.BASIC_SETTINGS_ENTRY_TITLE, SettingsText.BASIC_SETTINGS_ENTRY_DESC,
          SettingsResources.BASIC_ICON, SettingsAccent.CYAN, SettingsText.appearanceValue(this.appearanceMode),
          () => {
            this.pageName = SettingsRoute.BASIC
            this.remoteControlTargetSection = ''
          })
        if (this.remoteControlServerAvailable) {
          this.overviewEntry(SettingsText.REMOTE_CONTROL_ENTRY_TITLE, SettingsText.REMOTE_CONTROL_ENTRY_DESC,
            SettingsResources.REMOTE_CONTROL_ICON, SettingsAccent.BLUE, this.xrdpStatusText(), () => {
              this.pageName = SettingsRoute.REMOTE_CONTROL
              this.remoteControlTargetSection = ''
            })
        } else {
          this.overviewEntry(SettingsText.REMOTE_FILES_FEATURE_TITLE, SettingsText.REMOTE_FILES_FEATURE_DESC,
            SettingsResources.REMOTE_FILES_ICON, SettingsAccent.GREEN, '', () => {
              this.onOpenRemoteFilesDirectory()
            })
        }
        this.overviewEntry(SettingsText.PROJECT_HELP_ENTRY_TITLE, SettingsText.PROJECT_HELP_ENTRY_DESC,
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
        targetSection: this.remoteControlTargetSection,
        remoteAccessCode: this.remoteAccessCode,
        remoteAccessCodeGateEnabled: this.remoteAccessCodeGateEnabled,
        screenRecordingPermissionGranted: this.screenRecordingPermissionGranted,
        xrdpServerRunning: this.xrdpServerRunning,
        xrdpServerState: this.xrdpServerState,
        xrdpServerPort: this.xrdpServerPort,
        xrdpServerMessage: this.xrdpServerMessage,
        xrdpServerBusy: this.xrdpServerBusy,
        screenRecordingPermissionBusy: this.screenRecordingPermissionBusy,
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
