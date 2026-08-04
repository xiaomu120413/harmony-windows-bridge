import { LayoutMode } from '../../adaptive/WindowLayoutPolicy'
import { SettingsBackButton, SettingsStatusTone, SettingsTheme } from '../settings/SettingsUi'
import { WindowsConnectionProfile } from '../../rdp/WindowsConnectionStore'
import { HomeConnectionDetails } from './HomeConnectionDetails'
import { HomeDeviceList } from './HomeDeviceList'
import { HomeHeader } from './HomeHeader'
import { HomeStatusFooter } from './HomeStatusFooter'
import { HomeTheme } from './HomeTheme'

type HomeCompactPage = 'devices' | 'details'

@Component
export struct HomePage {
  @Prop layoutMode: LayoutMode = LayoutMode.COMPACT
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
  @Prop screenRecordingPermissionBusy: boolean = false
  @Prop remoteAccessCodeGateEnabled: boolean = false
  @State private compactPage: HomeCompactPage = 'devices'
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

  private selectProfile(profileId: string): void {
    this.onProfileSelect(profileId)
    this.compactPage = 'details'
  }

  private createProfile(): void {
    this.onNewProfile()
    this.compactPage = 'details'
  }

  @Builder
  private buildDeviceList() {
    HomeDeviceList({
      layoutMode: this.layoutMode,
      connectionProfiles: this.connectionProfiles,
      selectedConnectionProfileId: this.selectedConnectionProfileId,
      isDark: this.isDark,
      onProfileSelect: (profileId: string): void => {
        this.selectProfile(profileId)
      },
      onNewProfile: (): void => {
        this.createProfile()
      }
    })
  }

  @Builder
  private buildConnectionDetails() {
    HomeConnectionDetails({
      layoutMode: this.layoutMode,
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

  @Builder
  private buildExpandedMain() {
    Row({ space: 18 }) {
      this.buildDeviceList()
      this.buildConnectionDetails()
    }
    .width('100%')
    .layoutWeight(1)
    .padding({ left: 30, right: 30, top: 22, bottom: 22 })
    .backgroundColor(HomeTheme.appBackground(this.isDark))
  }

  @Builder
  private buildCompactMain() {
    Column() {
      if (this.compactPage === 'details') {
        Row({ space: 12 }) {
          SettingsBackButton({
            isDark: this.isDark,
            onBack: (): void => {
              this.compactPage = 'devices'
            }
          })
          Text(HomeText.CONNECTION_DETAILS_TITLE)
            .fontSize(20)
            .fontWeight(FontWeight.Bold)
            .fontColor(SettingsTheme.primaryText(this.isDark))
            .layoutWeight(1)
            .maxLines(1)
            .textOverflow({ overflow: TextOverflow.Ellipsis })
        }
        .width('100%')
        .alignItems(VerticalAlign.Center)
        .margin({ bottom: 8 })

        Scroll() {
          this.buildConnectionDetails()
        }
        .width('100%')
        .layoutWeight(1)
        .scrollBar(BarState.Auto)
      } else {
        this.buildDeviceList()
      }
    }
    .width('100%')
    .layoutWeight(1)
    .padding({ left: 12, right: 12, top: 12, bottom: 12 })
    .backgroundColor(HomeTheme.appBackground(this.isDark))
  }

  build() {
    Column() {
      HomeHeader({
        layoutMode: this.layoutMode,
        isDark: this.isDark,
        xrdpServerBusy: this.xrdpServerBusy,
        xrdpServerState: this.xrdpServerState,
        xrdpServerRunning: this.xrdpServerRunning,
        onOpenSettings: this.onOpenSettings
      })
      if (this.layoutMode === LayoutMode.COMPACT) {
        this.buildCompactMain()
      } else {
        this.buildExpandedMain()
      }
      HomeStatusFooter({
        layoutMode: this.layoutMode,
        isDark: this.isDark,
        xrdpServerBusy: this.xrdpServerBusy,
        xrdpServerState: this.xrdpServerState,
        xrdpServerRunning: this.xrdpServerRunning,
        xrdpServerPort: this.xrdpServerPort,
        screenRecordingPermissionGranted: this.screenRecordingPermissionGranted,
        screenRecordingPermissionBusy: this.screenRecordingPermissionBusy,
        remoteAccessCodeGateEnabled: this.remoteAccessCodeGateEnabled,
        onOpenSettings: this.onOpenSettings
      })
    }
    .width('100%')
    .height('100%')
    .backgroundColor(HomeTheme.appBackground(this.isDark))
  }
}
