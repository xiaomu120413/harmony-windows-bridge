import { common } from '@kit.AbilityKit'
import { NodeContent, uiObserver } from '@kit.ArkUI'
import {
  LayoutMode,
  layoutModeForWidthBreakpoint
} from '../adaptive/WindowLayoutPolicy'
import { currentDeviceCapabilities, DeviceCapabilitySnapshot, hasRemoteControlServer } from '../capability/DeviceCapabilityPolicy'
import {
  REMOTE_SURFACE_FOCUS_ID
} from '../rdp/RdpConstants'
import { RdpClientController } from '../rdp/RdpClientController'
import {
  ConnectionValidationResult,
  RdpConnectionValidator
} from '../rdp/RdpConnectionValidator'
import { RdpLogger } from '../rdp/RdpLogger'
import {
  RemoteControlSnapshot,
  XrdpServerStatus
} from '../rdp/RemoteControlCoordinator'
import { RemoteControlPageCoordinator } from '../rdp/RemoteControlPageCoordinator'
import {
  RemoteControlPort,
  UnavailableRemoteControlPort
} from '../rdp/RemoteControlPort'
import { RdpSurfaceContentHost } from '../rdp/RdpSurfaceContentHost'
import {
  WindowsConnectionProfile,
  WindowsConnectionSaveInput,
  WindowsConnectionSnapshot
} from '../rdp/WindowsConnectionStore'
import { WindowsConnectionPageCoordinator } from '../rdp/WindowsConnectionPageCoordinator'
import { HomeConnectionValidation } from '../components/home/HomeConnectionValidation'
import { HomePage } from '../components/home/HomePage'
import { HomeText } from '../components/home/HomeText'
import { RdpSessionPage } from '../components/session/RdpSessionPage'
import { RdpSessionLifecycleCoordinator } from '../rdp/RdpSessionLifecycleCoordinator'
import { SettingsPage } from '../components/SettingsPage'
import {
  SettingsRemoteControlSection,
  SettingsRoute
} from '../components/settings/SettingsConstants'
import { SettingsAppearanceMode, SettingsStatusTone, SettingsTheme } from '../components/settings/SettingsUi'
@Component
export struct MuHubApp {
  remoteControlPort: RemoteControlPort = new UnavailableRemoteControlPort()
  @State private host: string = ''
  @State private port: string = '3389'
  @State private username: string = ''
  @State private password: string = ''
  @State private connectionProfiles: WindowsConnectionProfile[] = []
  @State private selectedConnectionProfileId: string = ''
  @State private rememberConnectionPassword: boolean = false
  @State private connectionProfilePasswordLoading: boolean = false
  // Temporarily pin TOFU certificate policy; restore policy selector when strict validation is needed.
  private certPolicy: string = 'tofu'
  @State private remoteAccessCode: string = '000000'
  @State private remoteAccessCodeGateEnabled: boolean = false
  @State private screenRecordingPermissionGranted: boolean = false
  @State private screenRecordingPermissionBusy: boolean = false
  @State private inputInjectionPermissionGranted: boolean = false
  @State private inputInjectionPermissionBusy: boolean = false
  @State private xrdpServerRunning: boolean = false
  @State private xrdpServerState: string = 'Stopped'
  @State private xrdpServerPort: number = 3390
  @State private xrdpServerMessage: string = ''
  @State private xrdpServerBusy: boolean = false
  @State private showSession: boolean = false
  @State private showSettings: boolean = false
  @State private settingsInitialPageName: string = SettingsRoute.SETTINGS
  @State private settingsRemoteControlSection: string = ''
  @State private remoteLoginWaiting: boolean = false
  @State private sessionNoticeTitle: string = ''
  @State private sessionNoticeSubtitle: string = ''
  @State private connectionFeedbackText: string = ''
  @State private connectionFeedbackTone: SettingsStatusTone = 'neutral'
  @State private layoutMode: LayoutMode = LayoutMode.COMPACT
  private readonly deviceCapabilities: DeviceCapabilitySnapshot = currentDeviceCapabilities()
  @StorageLink('settingsSystemDark') private systemDark: boolean = false
  @StorageLink('settingsAppearanceMode') private appearanceMode: string = 'system'
  private readonly rdpClientController: RdpClientController = new RdpClientController()
  private readonly sessionLifecycleCoordinator: RdpSessionLifecycleCoordinator =
    new RdpSessionLifecycleCoordinator(this.rdpClientController, {
      defer: (task: () => void): void => this.deferUiUpdate(task),
      setVisible: (visible: boolean): void => {
        this.showSession = visible
      },
      setNotice: (notice): void => {
        this.remoteLoginWaiting = notice.remoteLoginWaiting
        this.sessionNoticeTitle = notice.title
        this.sessionNoticeSubtitle = notice.subtitle
      },
      setFeedback: (text: string, tone: SettingsStatusTone): void => this.setConnectionFeedback(text, tone),
      releaseInput: (): void => this.releaseActiveInput(),
      focusSurface: (): void => this.focusRemoteSurface(),
      persistProfile: (input: WindowsConnectionSaveInput): void => this.saveConnectionProfile(input),
      registerPermissionCallbacks: (): void => this.remoteControlCoordinator().registerPermissionCallbacks()
    })
  private readonly rdpSurfaceContentHost: RdpSurfaceContentHost = new RdpSurfaceContentHost()
  private remoteControlPageCoordinator: RemoteControlPageCoordinator | null = null
  private readonly connectionPageCoordinator: WindowsConnectionPageCoordinator =
    new WindowsConnectionPageCoordinator({
      context: (): common.UIAbilityContext | null => this.getUiAbilityContext(),
      defer: (task: () => void): void => this.deferUiUpdate(task),
      cancelEndpointProbe: (): void => this.cancelPendingEndpointProbe(),
      applySnapshot: (snapshot: WindowsConnectionSnapshot, applySelected: boolean, clearWhenEmpty: boolean): void =>
        this.applyConnectionSnapshot(snapshot, applySelected, clearWhenEmpty),
      applyProfile: (profile: WindowsConnectionProfile, password: string): void =>
        this.applyConnectionProfile(profile, password),
      startNewProfile: (): void => this.resetConnectionForm(),
      setPasswordLoading: (loading: boolean): void => {
        this.connectionProfilePasswordLoading = loading
      },
      setFeedback: (text: string, tone: SettingsStatusTone): void => this.setConnectionFeedback(text, tone)
    })
  private windowLayoutObserverRegistered: boolean = false
  private xrdpLifecycleTimer: number = -1
  private windowSizeLayoutBreakpointCallback = (info: uiObserver.WindowSizeLayoutBreakpointInfo): void => {
    this.layoutMode = layoutModeForWidthBreakpoint(info.widthBreakpoint)
  }

  private deferUiUpdate(task: () => void): void {
    setTimeout((): void => {
      task()
    }, 0)
  }

  aboutToAppear(): void {
    this.registerWindowLayoutObserver()
    this.sessionLifecycleCoordinator.registerCallbacks()
    this.loadConnectionProfiles()
    if (this.remoteControlServerAvailable()) {
      this.remoteControlCoordinator().refreshScreenRecordingPermission()
      this.remoteControlCoordinator().refreshInputInjectionPermission()
      this.remoteControlCoordinator().refreshDiagnostics()
      this.remoteControlCoordinator().ensureStarted('page appear')
      this.startXrdpLifecyclePolling()
    }
  }

  onPageShow(): void {
    if (!this.connectionPageCoordinator.isLoaded()) {
      this.loadConnectionProfiles()
    }
    if (this.remoteControlServerAvailable()) {
      this.remoteControlCoordinator().refreshReadiness(this.xrdpServerBusy && !this.screenRecordingPermissionBusy)
      this.startXrdpLifecyclePolling()
    }
  }

  onPageHide(): void {
    this.stopXrdpLifecyclePolling()
    this.releaseActiveInput()
  }

  aboutToDisappear(): void {
    this.unregisterWindowLayoutObserver()
    this.stopXrdpLifecyclePolling()
    this.cancelPendingEndpointProbe()
    this.releaseActiveInput()
  }

  private cancelPendingEndpointProbe(): void {
    this.sessionLifecycleCoordinator.cancel()
  }

  private startXrdpLifecyclePolling(): void {
    if (!this.remoteControlServerAvailable() || this.xrdpLifecycleTimer >= 0) {
      return
    }
    this.xrdpLifecycleTimer = setInterval((): void => {
      if (!this.xrdpServerBusy) {
        this.remoteControlCoordinator().refreshDiagnostics()
      }
    }, 2000)
  }

  private stopXrdpLifecyclePolling(): void {
    if (this.xrdpLifecycleTimer < 0) {
      return
    }
    clearInterval(this.xrdpLifecycleTimer)
    this.xrdpLifecycleTimer = -1
  }

  private registerWindowLayoutObserver(): void {
    try {
      const uiContext = this.getUIContext()
      this.layoutMode = layoutModeForWidthBreakpoint(uiContext.getWindowWidthBreakpoint())
      if (this.windowLayoutObserverRegistered) {
        return
      }
      uiContext.getUIObserver().on('windowSizeLayoutBreakpointChange', this.windowSizeLayoutBreakpointCallback)
      this.windowLayoutObserverRegistered = true
    } catch (error) {
      RdpLogger.warn(`Window layout observer registration failed: ${JSON.stringify(error)}`)
    }
  }

  private unregisterWindowLayoutObserver(): void {
    if (!this.windowLayoutObserverRegistered) {
      return
    }
    try {
      this.getUIContext().getUIObserver().off('windowSizeLayoutBreakpointChange',
        this.windowSizeLayoutBreakpointCallback)
      this.windowLayoutObserverRegistered = false
    } catch (error) {
      RdpLogger.warn(`Window layout observer removal failed: ${JSON.stringify(error)}`)
    }
  }

  private getUiAbilityContext(): common.UIAbilityContext | null {
    try {
      const hostContext = this.getUIContext().getHostContext()
      if (!hostContext) {
        return null
      }
      return hostContext as common.UIAbilityContext
    } catch (error) {
      RdpLogger.warn(`UIAbilityContext unavailable: ${JSON.stringify(error)}`)
      return null
    }
  }

  private remoteControlServerAvailable(): boolean {
    return this.remoteControlCoordinator().available()
  }

  private remoteControlCoordinator(): RemoteControlPageCoordinator {
    if (this.remoteControlPageCoordinator === null) {
      this.remoteControlPageCoordinator = new RemoteControlPageCoordinator(
        this.remoteControlPort,
        hasRemoteControlServer(this.deviceCapabilities),
        (): common.UIAbilityContext | null => this.getUiAbilityContext(),
        (task: () => void): void => this.deferUiUpdate(task),
        (snapshot: RemoteControlSnapshot): void => this.applyRemoteControlSnapshot(snapshot)
      )
    }
    return this.remoteControlPageCoordinator
  }

  private loadConnectionProfiles(): void {
    this.connectionPageCoordinator.load()
  }

  private applyConnectionSnapshot(snapshot: WindowsConnectionSnapshot, applySelectedProfile: boolean,
    clearWhenEmpty: boolean): void {
    this.connectionProfiles = snapshot.profiles
    this.selectedConnectionProfileId = snapshot.selectedProfileId

    if (!applySelectedProfile) {
      return
    }

    const selectedProfile = this.findConnectionProfileIn(snapshot.profiles, snapshot.selectedProfileId)
    if (selectedProfile === null) {
      this.rememberConnectionPassword = false
      if (clearWhenEmpty) {
        this.resetConnectionForm()
      }
      return
    }

    this.applyConnectionProfile(selectedProfile, snapshot.selectedPassword)
  }

  private findConnectionProfileIn(profiles: WindowsConnectionProfile[], profileId: string): WindowsConnectionProfile | null {
    for (const profile of profiles) {
      if (profile.id === profileId) {
        return profile
      }
    }
    return null
  }

  private applyConnectionProfile(profile: WindowsConnectionProfile, password: string): void {
    this.selectedConnectionProfileId = profile.id
    this.host = profile.host
    this.port = profile.port
    this.username = profile.username
    this.password = password
    this.rememberConnectionPassword = profile.rememberPassword
    if (profile.certPolicy.length > 0) {
      this.certPolicy = profile.certPolicy
    }
  }

  private selectConnectionProfile(profileId: string): void {
    this.connectionPageCoordinator.select(profileId)
  }

  private startNewConnectionProfile(): void {
    this.connectionPageCoordinator.startNew()
  }

  private resetConnectionForm(): void {
    this.selectedConnectionProfileId = ''
    this.host = ''
    this.port = '3389'
    this.username = ''
    this.password = ''
    this.rememberConnectionPassword = false
    this.certPolicy = 'tofu'
    this.clearConnectionFeedback()
  }

  private deleteSelectedConnectionProfile(): void {
    this.connectionPageCoordinator.deleteSelected(this.selectedConnectionProfileId)
  }

  private setRememberConnectionPassword(remember: boolean): void {
    this.rememberConnectionPassword = remember
  }

  private saveConnectionProfile(input: WindowsConnectionSaveInput): void {
    this.connectionPageCoordinator.save(input)
  }

  private clearSelectedConnectionPassword(): void {
    this.password = ''
    this.rememberConnectionPassword = false
    this.connectionPageCoordinator.clearPassword(this.selectedConnectionProfileId)
  }

  private clearConnectionFeedback(): void {
    this.connectionFeedbackText = ''
    this.connectionFeedbackTone = 'neutral'
  }

  private setConnectionFeedback(text: string, tone: SettingsStatusTone): void {
    this.connectionFeedbackText = text
    this.connectionFeedbackTone = tone
  }

  private validateConnectionForm(): ConnectionValidationResult {
    return RdpConnectionValidator.validate(this.host, this.port, this.username, this.password)
  }

  private getAppFilesDir(): string {
    const context = this.getUiAbilityContext()
    if (context === null) {
      return ''
    }
    return context.filesDir
  }

  private openSettingsFromHome(remoteControlSection: string): void {
    const passiveControlAvailable = this.remoteControlServerAvailable()
    const activeControlSection = remoteControlSection === SettingsRemoteControlSection.FILES ||
      remoteControlSection === SettingsRemoteControlSection.ACCESS
    if (passiveControlAvailable) {
      this.remoteControlCoordinator().refreshScreenRecordingPermission()
      this.remoteControlCoordinator().refreshInputInjectionPermission()
      this.remoteControlCoordinator().refreshDiagnostics()
    }
    if (passiveControlAvailable || activeControlSection) {
      this.settingsRemoteControlSection = remoteControlSection
      this.settingsInitialPageName = remoteControlSection.length > 0 ? SettingsRoute.REMOTE_CONTROL :
        SettingsRoute.SETTINGS
    } else {
      this.settingsRemoteControlSection = ''
      this.settingsInitialPageName = SettingsRoute.SETTINGS
    }
    this.showSettings = true
  }

  private applyRemoteControlSnapshot(snapshot: RemoteControlSnapshot): void {
    this.remoteAccessCode = snapshot.accessCode
    this.remoteAccessCodeGateEnabled = snapshot.accessCodeGateEnabled
    this.screenRecordingPermissionGranted = snapshot.screenRecordingPermissionGranted
    this.screenRecordingPermissionBusy = snapshot.screenRecordingPermissionBusy
    this.inputInjectionPermissionGranted = snapshot.inputInjectionPermissionGranted
    this.inputInjectionPermissionBusy = snapshot.inputInjectionPermissionBusy
    this.xrdpServerRunning = snapshot.serverRunning
    this.xrdpServerState = snapshot.serverState
    this.xrdpServerPort = snapshot.serverPort
    this.xrdpServerMessage = snapshot.serverMessage
    this.xrdpServerBusy = snapshot.serverBusy
  }

  private connectNative(): void {
    const validation = this.validateConnectionForm()
    if (!validation.ok) {
      const validationMessage = HomeConnectionValidation.message(validation.issues)
      this.sessionLifecycleCoordinator.rejectValidation(validationMessage)
      return
    }

    const policy = this.certPolicy
    const host = validation.host
    const port = validation.port
    const username = validation.username
    const password = validation.password
    const appFilesDir = this.getAppFilesDir()
    this.host = host
    this.port = port
    this.username = username
    const profileInput: WindowsConnectionSaveInput = {
      profileId: this.selectedConnectionProfileId,
      host: host,
      port: port,
      username: username,
      password: password,
      certPolicy: policy,
      rememberPassword: this.rememberConnectionPassword
    }
    this.remoteLoginWaiting = false
    this.sessionLifecycleCoordinator.connect({
      host: host,
      port: port,
      username: username,
      password: password,
      certPolicy: policy,
      appFilesDir: appFilesDir
    }, profileInput)
  }

  private isConnected(): boolean {
    return this.rdpClientController.isConnected()
  }

  private focusRemoteSurface(): void {
    if (!this.isConnected()) {
      return
    }
    this.deferUiUpdate(() => {
      const focused = focusControl.requestFocus(REMOTE_SURFACE_FOCUS_ID)
      if (!focused) {
        RdpLogger.warn('Remote surface focus failed')
      }
    })
  }

  private releaseActiveInput(): void {
    try {
      const result = this.rdpClientController.releaseAllInput()
      if (!result.ok) {
        RdpLogger.warn(result.message)
      }
    } catch (error) {
      RdpLogger.error(`Native release all input failed: ${JSON.stringify(error)}`)
    }
  }

  private getAppearanceMode(): SettingsAppearanceMode {
    return SettingsTheme.normalizeAppearanceMode(this.appearanceMode)
  }

  private isAppDark(): boolean {
    const mode = this.getAppearanceMode()
    if (mode === 'dark') {
      return true
    }
    if (mode === 'light') {
      return false
    }
    return this.systemDark
  }

  build() {
    Column() {
      if (this.showSession) {
        RdpSessionPage({
          noticeTitle: this.sessionNoticeTitle,
          noticeSubtitle: this.sessionNoticeSubtitle,
          remoteLoginWaiting: this.remoteLoginWaiting,
          onAttachContent: (content: NodeContent): boolean => this.rdpSurfaceContentHost.attach(content),
          onDetachContent: (): void => {
            this.rdpSurfaceContentHost.detach()
          },
          onSurfaceLoad: (): void => {
            this.deferUiUpdate(() => {
              this.focusRemoteSurface()
            })
          }
        })
      } else if (this.showSettings) {
        SettingsPage({
          layoutMode: this.layoutMode,
          initialPageName: this.settingsInitialPageName,
          initialRemoteControlSection: this.settingsRemoteControlSection,
          remoteAccessCode: this.remoteAccessCode,
          remoteAccessCodeGateEnabled: this.remoteAccessCodeGateEnabled,
          screenRecordingPermissionGranted: this.screenRecordingPermissionGranted,
          screenRecordingPermissionBusy: this.screenRecordingPermissionBusy,
          inputInjectionPermissionGranted: this.inputInjectionPermissionGranted,
          inputInjectionPermissionBusy: this.inputInjectionPermissionBusy,
          xrdpServerRunning: this.xrdpServerRunning,
          xrdpServerState: this.xrdpServerState,
          xrdpServerPort: this.xrdpServerPort,
          xrdpServerMessage: this.xrdpServerMessage,
          xrdpServerBusy: this.xrdpServerBusy,
          remoteControlServerAvailable: this.remoteControlServerAvailable(),
          onRemoteAccessCodeGateChange: (enabled: boolean): string =>
            this.remoteControlCoordinator().setAccessCodeGate(enabled),
          onRemoteAccessCodeRegenerate: (): string => this.remoteControlCoordinator().regenerateAccessCode(),
          onRequestScreenRecordingPermission: (): Promise<boolean> =>
            this.remoteControlCoordinator().requestScreenRecordingPermission(),
          onRefreshScreenRecordingPermission: (): boolean =>
            this.remoteControlCoordinator().refreshScreenRecordingPermission(),
          onRequestInputInjectionPermission: (): Promise<boolean> =>
            this.remoteControlCoordinator().requestInputInjectionPermission(),
          onRefreshInputInjectionPermission: (): boolean =>
            this.remoteControlCoordinator().refreshInputInjectionPermission(),
          onRefreshXrdpServerStatus: (): XrdpServerStatus => this.remoteControlCoordinator().refreshDiagnostics(),
          onStartXrdpServer: (): Promise<XrdpServerStatus> => this.remoteControlCoordinator().startFromSettings(),
          onStopXrdpServer: (): Promise<XrdpServerStatus> => this.remoteControlCoordinator().stopFromSettings(),
          onOpenRemoteFilesDirectory: (): void => this.remoteControlCoordinator().openRemoteFilesDirectory(),
          onExportDiagnostics: (): Promise<string> =>
            this.remoteControlCoordinator().exportDiagnostics(this.host.length, this.username.length, this.port,
              this.sessionLifecycleCoordinator.diagnostics()),
          onClose: () => {
            if (this.remoteControlServerAvailable()) {
              this.remoteControlCoordinator().refreshReadiness(
                this.xrdpServerBusy && !this.screenRecordingPermissionBusy)
            }
            this.showSettings = false
            this.settingsInitialPageName = SettingsRoute.SETTINGS
            this.settingsRemoteControlSection = ''
          }
        })
      } else {
        HomePage({
          layoutMode: this.layoutMode,
          host: $host,
          port: $port,
          username: $username,
          password: $password,
          connectionProfiles: this.connectionProfiles,
          selectedConnectionProfileId: this.selectedConnectionProfileId,
          rememberPassword: $rememberConnectionPassword,
          passwordLoading: this.connectionProfilePasswordLoading,
          connectionFeedbackText: this.connectionFeedbackText,
          connectionFeedbackTone: this.connectionFeedbackTone,
          isDark: this.isAppDark(),
          xrdpServerBusy: this.xrdpServerBusy,
          xrdpServerState: this.xrdpServerState,
          xrdpServerRunning: this.xrdpServerRunning,
          screenRecordingPermissionGranted: this.screenRecordingPermissionGranted,
          screenRecordingPermissionBusy: this.screenRecordingPermissionBusy,
          inputInjectionPermissionGranted: this.inputInjectionPermissionGranted,
          inputInjectionPermissionBusy: this.inputInjectionPermissionBusy,
          remoteControlServerAvailable: this.remoteControlServerAvailable(),
          onHostChange: (value: string): void => {
            this.cancelPendingEndpointProbe()
            this.host = value
            this.clearConnectionFeedback()
          },
          onPortChange: (value: string): void => {
            this.cancelPendingEndpointProbe()
            this.port = value
            this.clearConnectionFeedback()
          },
          onUsernameChange: (value: string): void => {
            this.username = value
            this.clearConnectionFeedback()
          },
          onPasswordChange: (value: string): void => {
            this.password = value
            this.clearConnectionFeedback()
          },
          onProfileSelect: (profileId: string): void => {
            this.selectConnectionProfile(profileId)
          },
          onNewProfile: (): void => {
            this.startNewConnectionProfile()
          },
          onDeleteProfile: (): void => {
            this.deleteSelectedConnectionProfile()
          },
          onRememberPasswordChange: (remember: boolean): void => {
            this.setRememberConnectionPassword(remember)
          },
          onClearPassword: (): void => {
            this.clearSelectedConnectionPassword()
          },
          onConnect: (): void => {
            this.connectNative()
          },
          onOpenSettings: (remoteControlSection: string): void => {
            this.openSettingsFromHome(remoteControlSection)
          }
        })
      }
    }
    .width('100%')
    .height('100%')
    .backgroundColor(SettingsTheme.pageBackground(this.isAppDark()))
  }
}
