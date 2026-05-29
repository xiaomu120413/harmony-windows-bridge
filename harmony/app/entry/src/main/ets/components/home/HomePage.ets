import { SettingsStatusTone } from '../settings/SettingsUi'
import { WindowsConnectionProfile } from '../../rdp/WindowsConnectionStore'
import { HomeConnectionDetails } from './HomeConnectionDetails'
import { HomeDeviceList } from './HomeDeviceList'
import { HomeHeader } from './HomeHeader'
import { HomeStatusFooter } from './HomeStatusFooter'
import { HomeTheme } from './HomeTheme'

@Component
export struct HomePage {
  @Link host: string
  @Link port: string
  @Link username: string
  @Link password: string
  @Prop connectionProfiles: WindowsConnectionProfile[] = []
  @Prop selectedConnectionProfileId: string = ''
  @Link rememberPassword: boolean
  @Prop connectionFeedbackText: string = ''
  @Prop connectionFeedbackTone: SettingsStatusTone = 'neutral'
  @Prop isDark: boolean = false
  @Prop xrdpServerBusy: boolean = false
  @Prop xrdpServerState: string = 'Stopped'
  @Prop xrdpServerRunning: boolean = false
  @Prop xrdpServerPort: number = 3390
  @Prop screenRecordingPermissionGranted: boolean = false
  @Prop remoteAccessCodeGateEnabled: boolean = false
  onHostChange: (value: string) => void = (_value: string) => {
  }
  onPortChange: (value: string) => void = (_value: string) => {
  }
  onUsernameChange: (value: string) => void = (_value: string) => {
  }
  onPasswordChange: (value: string) => void = (_value: string) => {
  }
  onProfileSelect: (profileId: string) => void = (_profileId: string) => {
  }
  onNewProfile: () => void = () => {
  }
  onDeleteProfile: () => void = () => {
  }
  onRememberPasswordChange: (remember: boolean) => void = (_remember: boolean) => {
  }
  onClearPassword: () => void = () => {
  }
  onConnect: () => void = () => {
  }
  onOpenSettings: (remoteControlSection: string) => void = (_remoteControlSection: string) => {
  }

  @Builder
  private buildMain() {
    Row({ space: 18 }) {
      HomeDeviceList({
        connectionProfiles: this.connectionProfiles,
        selectedConnectionProfileId: this.selectedConnectionProfileId,
        isDark: this.isDark,
        onProfileSelect: this.onProfileSelect,
        onNewProfile: this.onNewProfile
      })

      HomeConnectionDetails({
        host: $host,
        port: $port,
        username: $username,
        password: $password,
        rememberPassword: $rememberPassword,
        selectedConnectionProfileId: this.selectedConnectionProfileId,
        connectionFeedbackText: this.connectionFeedbackText,
        connectionFeedbackTone: this.connectionFeedbackTone,
        isDark: this.isDark,
        onHostChange: this.onHostChange,
        onPortChange: this.onPortChange,
        onUsernameChange: this.onUsernameChange,
        onPasswordChange: this.onPasswordChange,
        onDeleteProfile: this.onDeleteProfile,
        onRememberPasswordChange: this.onRememberPasswordChange,
        onClearPassword: this.onClearPassword,
        onConnect: this.onConnect
      })
    }
    .width('100%')
    .layoutWeight(1)
    .padding({ left: 30, right: 30, top: 22, bottom: 22 })
    .backgroundColor(HomeTheme.appBackground(this.isDark))
  }

  build() {
    Column() {
      HomeHeader({
        isDark: this.isDark,
        xrdpServerBusy: this.xrdpServerBusy,
        xrdpServerRunning: this.xrdpServerRunning,
        screenRecordingPermissionGranted: this.screenRecordingPermissionGranted,
        onOpenSettings: this.onOpenSettings
      })
      this.buildMain()
      HomeStatusFooter({
        isDark: this.isDark,
        xrdpServerBusy: this.xrdpServerBusy,
        xrdpServerState: this.xrdpServerState,
        xrdpServerRunning: this.xrdpServerRunning,
        xrdpServerPort: this.xrdpServerPort,
        screenRecordingPermissionGranted: this.screenRecordingPermissionGranted,
        remoteAccessCodeGateEnabled: this.remoteAccessCodeGateEnabled,
        onOpenSettings: this.onOpenSettings
      })
    }
    .width('100%')
    .height('100%')
    .backgroundColor(HomeTheme.appBackground(this.isDark))
  }
}
