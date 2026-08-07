import {
  SettingsPageHeader,
  SettingsSectionTitle,
  SettingsTheme
} from './SettingsUi'
import { LayoutMode } from '../../adaptive/WindowLayoutPolicy'
import { XrdpServerStatus } from '../../rdp/RemoteControlCoordinator'
import { SettingsNetworkText, SettingsRemoteControlSection, SettingsRemoteText, SettingsShellText } from './SettingsConstants'
import { SettingsAccent, SettingsResources } from './SettingsUi'
import {
  RemoteAccessCard,
  RemoteFilesCard,
  RemotePermissionCard,
  XrdpServerCard
} from './RemoteControlCards'

@Component
export struct RemoteControlSettingsPage {
  onBack: () => void = () => {
  }
  onRemoteAccessCodeGateChange: (enabled: boolean) => string = (_enabled: boolean) => {
    return ''
  }
  onRemoteAccessCodeRegenerate: () => string = () => {
    return ''
  }
  onRefreshNetwork: () => void = () => {
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
  @Prop isDark: boolean = false
  @Prop layoutMode: LayoutMode = LayoutMode.COMPACT
  @Prop targetSection: string = ''
  @Prop localIpAddress: string = SettingsNetworkText.LOADING
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
  @Prop passiveControlAvailable: boolean = true
  @State private localScreenRecordingGranted: boolean = false
  @State private localInputInjectionGranted: boolean = false
  @State private localGateEnabled: boolean = false
  @State private localAccessCode: string = '000000'
  @State private localXrdpServerRunning: boolean = false
  @State private localXrdpServerState: string = 'Stopped'
  @State private localXrdpServerPort: number = 3390
  @State private localXrdpServerMessage: string = ''
  @State private localXrdpServerBusy: boolean = false
  private screenPermissionRequestPending: boolean = false
  private inputPermissionRequestPending: boolean = false

  aboutToAppear(): void {
    this.localScreenRecordingGranted = this.screenRecordingPermissionGranted
    this.localInputInjectionGranted = this.inputInjectionPermissionGranted
    this.localGateEnabled = this.remoteAccessCodeGateEnabled
    this.localAccessCode = this.remoteAccessCode
    this.localXrdpServerRunning = this.xrdpServerRunning
    this.localXrdpServerState = this.xrdpServerState
    this.localXrdpServerPort = this.xrdpServerPort
    this.localXrdpServerMessage = this.xrdpServerMessage
    this.localXrdpServerBusy = false
    if (this.passiveControlAvailable) {
      this.refreshScreenRecordingState()
      this.refreshInputInjectionState()
      this.refreshXrdpServerStatus()
      this.onRefreshNetwork()
    }
  }

  private setGateEnabled(enabled: boolean): void {
    this.localAccessCode = this.onRemoteAccessCodeGateChange(enabled)
    this.localGateEnabled = enabled
  }

  private regenerateAccessCode(): void {
    this.localAccessCode = this.onRemoteAccessCodeRegenerate()
  }

  private applyXrdpServerStatus(status: XrdpServerStatus): void {
    this.localXrdpServerRunning = status.running
    this.localXrdpServerState = status.state.length > 0 ? status.state : 'Stopped'
    this.localXrdpServerPort = status.port > 0 ? status.port : 3390
    this.localXrdpServerMessage = status.message
  }

  private refreshXrdpServerStatus(): void {
    this.applyXrdpServerStatus(this.onRefreshXrdpServerStatus())
  }

  private startXrdpServer(): void {
    if (this.localXrdpServerBusy || this.xrdpServerBusy || this.screenRecordingPermissionBusy) {
      return
    }
    this.localXrdpServerBusy = true
    this.onStartXrdpServer()
      .then((status: XrdpServerStatus) => {
        this.applyXrdpServerStatus(status)
        this.localScreenRecordingGranted = this.localScreenRecordingGranted ||
          this.onRefreshScreenRecordingPermission()
      })
      .catch((_error: Error) => {
        this.refreshXrdpServerStatus()
      })
      .finally(() => {
        this.localXrdpServerBusy = false
      })
  }

  private refreshScreenRecordingState(): void {
    this.localScreenRecordingGranted = this.onRefreshScreenRecordingPermission()
  }

  private requestScreenRecordingPermission(): void {
    if (this.screenPermissionRequestPending || this.screenRecordingPermissionBusy) {
      return
    }
    this.screenPermissionRequestPending = true
    this.onRequestScreenRecordingPermission()
      .then((granted: boolean) => {
        const allowed = granted || this.onRefreshScreenRecordingPermission()
        this.localScreenRecordingGranted = allowed
        if (!allowed) {
          this.localXrdpServerBusy = false
        }
        this.refreshXrdpServerStatus()
      })
      .catch((_error: Error) => {
        this.localScreenRecordingGranted = this.onRefreshScreenRecordingPermission()
        if (!this.localScreenRecordingGranted) {
          this.localXrdpServerBusy = false
        }
      })
      .finally(() => {
        this.screenPermissionRequestPending = false
      })
  }

  private refreshInputInjectionState(): void {
    this.localInputInjectionGranted = this.onRefreshInputInjectionPermission()
  }

  private requestInputInjectionPermission(): void {
    if (this.inputPermissionRequestPending || this.inputInjectionPermissionBusy) {
      return
    }
    this.inputPermissionRequestPending = true
    this.onRequestInputInjectionPermission()
      .then((granted: boolean) => {
        this.localInputInjectionGranted = granted || this.onRefreshInputInjectionPermission()
      })
      .catch((_error: Error) => {
        this.localInputInjectionGranted = this.onRefreshInputInjectionPermission()
      })
      .finally(() => {
        this.inputPermissionRequestPending = false
      })
  }

  build() {
    Column() {
      SettingsPageHeader({
        title: SettingsShellText.REMOTE_TITLE,
        subtitle: this.passiveControlAvailable ? SettingsRemoteText.PAGE_SUBTITLE :
          SettingsRemoteText.PAGE_CONTROLLER_SUBTITLE,
        isDark: this.isDark,
        onBack: this.onBack
      })

      Scroll() {
        Column({ space: 14 }) {
          if (this.passiveControlAvailable) {
            SettingsSectionTitle({
              title: SettingsRemoteText.PASSIVE_SECTION,
              subtitle: SettingsRemoteText.PASSIVE_DESC,
              isDark: this.isDark
            })

            XrdpServerCard({
              isDark: this.isDark,
              layoutMode: this.layoutMode,
              targetSection: this.targetSection,
              localIpAddress: this.localIpAddress,
              running: this.localXrdpServerRunning,
              busy: this.localXrdpServerBusy || this.xrdpServerBusy || this.screenRecordingPermissionBusy,
              state: this.localXrdpServerState,
              port: this.localXrdpServerPort,
              message: this.localXrdpServerMessage,
              onRefresh: (): void => {
                this.refreshXrdpServerStatus()
                this.onRefreshNetwork()
              },
              onStart: (): void => {
                this.startXrdpServer()
              }
            })
            RemotePermissionCard({
              isDark: this.isDark,
              targetSection: this.targetSection,
              section: SettingsRemoteControlSection.SCREEN,
              granted: this.localScreenRecordingGranted,
              busy: this.screenRecordingPermissionBusy,
              onRequest: (): void => {
                this.requestScreenRecordingPermission()
              }
            })
            RemotePermissionCard({
              isDark: this.isDark,
              targetSection: this.targetSection,
              section: SettingsRemoteControlSection.INPUT,
              title: SettingsRemoteText.INPUT_PERMISSION_TITLE,
              description: SettingsRemoteText.INPUT_PERMISSION_DESC,
              icon: SettingsResources.REMOTE_ACCESS_ICON,
              accent: SettingsAccent.PURPLE,
              granted: this.localInputInjectionGranted,
              busy: this.inputInjectionPermissionBusy,
              onRequest: (): void => {
                this.requestInputInjectionPermission()
              }
            })
          }
          SettingsSectionTitle({
            title: SettingsRemoteText.ACTIVE_SECTION,
            subtitle: SettingsRemoteText.ACTIVE_DESC,
            isDark: this.isDark
          })
          RemoteFilesCard({
            isDark: this.isDark,
            targetSection: this.targetSection,
            onOpenRemoteFilesDirectory: this.onOpenRemoteFilesDirectory
          })
          RemoteAccessCard({
            isDark: this.isDark,
            targetSection: this.targetSection,
            gateEnabled: this.localGateEnabled,
            accessCode: this.localAccessCode,
            onGateChange: (enabled: boolean): void => {
              this.setGateEnabled(enabled)
            },
            onRegenerate: (): void => {
              this.regenerateAccessCode()
            }
          })
        }
        .alignItems(HorizontalAlign.Start)
        .padding({ left: 20, right: 20, bottom: 24 })
        .width('100%')
      }
      .layoutWeight(1)
    }
    .width('100%')
    .height('100%')
    .backgroundColor(SettingsTheme.pageBackground(this.isDark))
  }
}
