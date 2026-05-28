import { BasicSettingsPage } from './settings/BasicSettingsPage'
import { ProjectHelpPage } from './settings/ProjectHelpPage'
import { RemoteControlSettingsPage } from './settings/RemoteControlSettingsPage'
import { XrdpServerStatus } from '../rdp/XrdpServerController'
import { SettingsRoute, SettingsText } from './settings/SettingsConstants'
import {
  SettingsAccent,
  SettingsAppearanceMode,
  SettingsListItem,
  SettingsPageHeader,
  SettingsResources,
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
  @Prop remoteAccessCode: string = '000000'
  @Prop remoteAccessCodeGateEnabled: boolean = false
  @Prop screenRecordingPermissionGranted: boolean = false
  @Prop xrdpServerRunning: boolean = false
  @Prop xrdpServerState: string = 'Stopped'
  @Prop xrdpServerPort: number = 3390
  @Prop xrdpServerMessage: string = ''
  @Prop xrdpServerBusy: boolean = false
  @State private pageName: string = SettingsRoute.SETTINGS
  @State private appearanceMode: SettingsAppearanceMode = SettingsTheme.getStoredAppearanceMode()
  @StorageLink('settingsSystemDark') private systemDark: boolean = false

  aboutToAppear(): void {
    this.appearanceMode = SettingsTheme.getStoredAppearanceMode()
  }

  private isDarkMode(): boolean {
    if (this.appearanceMode === 'system') {
      return this.systemDark
    }
    return SettingsTheme.isDark(this.getUIContext(), this.appearanceMode)
  }

  @Builder
  private buildSettingsPage() {
    Column() {
      SettingsPageHeader({
        title: SettingsText.SETTINGS_TITLE,
        subtitle: SettingsText.SETTINGS_SUBTITLE,
        isDark: this.isDarkMode(),
        onBack: () => {
          this.onClose()
        }
      })

      Scroll() {
        Column() {
          SettingsListItem({
            title: SettingsText.BASIC_SETTINGS_ENTRY_TITLE,
            description: SettingsText.BASIC_SETTINGS_ENTRY_DESC,
            iconResource: SettingsResources.SETTINGS_ICON,
            accentName: SettingsAccent.CYAN,
            value: SettingsText.appearanceValue(this.appearanceMode),
            isDark: this.isDarkMode(),
            onPress: () => {
              this.pageName = SettingsRoute.BASIC
            }
          })
          SettingsListItem({
            title: SettingsText.REMOTE_CONTROL_ENTRY_TITLE,
            description: SettingsText.REMOTE_CONTROL_ENTRY_DESC,
            iconResource: SettingsResources.DASHBOARD_ICON,
            accentName: SettingsAccent.BLUE,
            value: this.xrdpServerBusy ? SettingsText.REMOTE_SERVER_BUSY :
              (this.xrdpServerRunning ? SettingsText.REMOTE_SERVER_RUNNING : SettingsText.REMOTE_SERVER_STOPPED),
            isDark: this.isDarkMode(),
            onPress: () => {
              this.pageName = SettingsRoute.REMOTE_CONTROL
            }
          })
          SettingsListItem({
            title: SettingsText.PROJECT_HELP_ENTRY_TITLE,
            description: SettingsText.PROJECT_HELP_ENTRY_DESC,
            iconResource: SettingsResources.HELP_ICON,
            accentName: SettingsAccent.PURPLE,
            isDark: this.isDarkMode(),
            onPress: () => {
              this.pageName = SettingsRoute.PROJECT_HELP
            }
          })
        }
        .padding({ left: 20, right: 20, bottom: 20 })
        .width('100%')
      }
      .layoutWeight(1)
    }
    .width('100%')
    .height('100%')
    .backgroundColor(SettingsTheme.pageBackground(this.isDarkMode()))
  }

  build() {
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
        this.buildSettingsPage()
      }
    }
    .width('100%')
    .height('100%')
  }
}
