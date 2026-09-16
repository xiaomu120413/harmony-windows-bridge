import { common } from '@kit.AbilityKit'
import { NodeContent, uiObserver } from '@kit.ArkUI'
import {
  LayoutMode,
  layoutModeForWidthBreakpoint
} from '../adaptive/WindowLayoutPolicy'
import {
  currentDeviceCapabilities,
  DeviceCapabilitySnapshot,
  hasRemoteControlServer
} from '../capability/DeviceCapabilityPolicy'
import {
  REMOTE_SURFACE_FOCUS_ID
} from '../rdp/RdpConstants'
import { RdpClientController } from '../rdp/RdpClientController'
import { ConnectionValidationIssue } from '../rdp/RdpConnectionValidator'
import { RdpLogger } from '../rdp/RdpLogger'
import {
  RdpSessionStateCoordinator,
  RdpSessionStateSnapshot,
  SessionStateMessages
} from '../rdp/RdpSessionStateCoordinator'
import { RdpPermissionRequestCoordinator } from '../rdp/RdpPermissionRequestCoordinator'
import { RdpPermissionCallbackDelegate, RdpPermissionManager } from '../rdp/RdpPermissions'
import {
  RemoteControlCoordinator,
  RemoteControlSnapshot,
  XrdpServerDisplayMessages,
  XrdpServerStatus
} from '../rdp/RemoteControlCoordinator'
import {
  RemoteControlPort,
  UnavailableRemoteControlPort
} from '../rdp/RemoteControlPort'
import { RemoteFilesDirectory } from '../rdp/RemoteFilesDirectory'
import { RdpSurfaceContentHost } from '../rdp/RdpSurfaceContentHost'
import {
  WindowsConnectionProfile,
  WindowsConnectionSaveInput,
  WindowsConnectionSnapshot
} from '../rdp/WindowsConnectionStore'
import { WindowsConnectionProfileCoordinator } from '../rdp/WindowsConnectionProfileCoordinator'
import { HomeConnectionValidation } from '../components/home/HomeConnectionValidation'
import { HomePage } from '../components/home/HomePage'
import { HomeText } from '../components/home/HomeText'
import { RdpSessionPage } from '../components/session/RdpSessionPage'
import { SettingsPage } from '../components/SettingsPage'
import {
  SettingsRemoteControlSection,
  SettingsRemoteText,
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
  private readonly rdpSurfaceContentHost: RdpSurfaceContentHost = new RdpSurfaceContentHost()
  private permissionManager: RdpPermissionManager | null = null
  private permissionRequestCoordinator: RdpPermissionRequestCoordinator | null = null
  private remoteControlCoordinator: RemoteControlCoordinator | null = null
  private sessionStateCoordinator: RdpSessionStateCoordinator | null = null
  private connectionProfileCoordinator: WindowsConnectionProfileCoordinator | null = null
  private connectionProfilesLoaded: boolean = false
  private connectionProfileSelectionGeneration: number = 0
  private windowLayoutObserverRegistered: boolean = false
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
    this.registerNativeCallbacks()
    this.loadConnectionProfiles()
    if (this.remoteControlServerAvailable()) {
      this.refreshScreenRecordingPermissionState()
      this.refreshInputInjectionPermissionState()
      this.refreshXrdpServerDiagnostics()
      this.ensureXrdpServerStarted('page appear')
    }
  }

  onPageShow(): void {
    if (!this.connectionProfilesLoaded) {
      this.loadConnectionProfiles()
    }
    if (this.remoteControlServerAvailable()) {
      this.refreshRemoteControlReadiness(this.xrdpServerBusy && !this.screenRecordingPermissionBusy)
    }
  }

  onPageHide(): void {
    this.releaseActiveInput()
  }

  aboutToDisappear(): void {
    this.rdpClientController.disconnect()
    this.unregisterWindowLayoutObserver()
    this.releaseActiveInput()
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
    return hasRemoteControlServer(this.deviceCapabilities) && this.remoteControlPort.available
  }

  private getPermissionManager(): RdpPermissionManager {
    if (this.permissionManager === null) {
      const delegate = new RdpPermissionCallbackDelegate(
        (): common.UIAbilityContext | null => this.getUiAbilityContext()
      )
      this.permissionManager = new RdpPermissionManager(delegate, (): void => {
        this.getRemoteControlCoordinator().permissionPromptOpened()
      })
    }
    return this.permissionManager
  }

  private getPermissionRequestCoordinator(): RdpPermissionRequestCoordinator {
    if (this.permissionRequestCoordinator === null) {
      this.permissionRequestCoordinator = new RdpPermissionRequestCoordinator(
        this.getPermissionManager(),
        (task: () => void): void => this.deferUiUpdate(task)
      )
    }
    return this.permissionRequestCoordinator
  }

  private getRemoteControlCoordinator(): RemoteControlCoordinator {
    if (this.remoteControlCoordinator === null) {
      this.remoteControlCoordinator = new RemoteControlCoordinator(
        this.remoteControlPort,
        this.xrdpServerDisplayMessages(),
        (): RdpPermissionManager => this.getPermissionManager(),
        (): string => this.getAppFilesDir(),
        (snapshot: RemoteControlSnapshot): void => {
          this.deferUiUpdate((): void => this.applyRemoteControlSnapshot(snapshot))
        }
      )
      this.applyRemoteControlSnapshot(this.remoteControlCoordinator.snapshot())
    }
    return this.remoteControlCoordinator
  }

  private sessionStateMessages(): SessionStateMessages {
    return {
      sessionConnectingTitle: HomeText.SESSION_CONNECTING_TITLE,
      sessionConnectingSubtitle: HomeText.SESSION_CONNECTING_SUBTITLE,
      sessionConnectedTitle: HomeText.SESSION_CONNECTED_TITLE,
      sessionConnectedSubtitle: HomeText.SESSION_CONNECTED_SUBTITLE,
      sessionRemoteLoginWaitingTitle: HomeText.SESSION_REMOTE_LOGIN_WAITING_TITLE,
      sessionRemoteLoginWaitingSubtitle: HomeText.SESSION_REMOTE_LOGIN_WAITING_SUBTITLE,
      sessionResolvingTitle: HomeText.SESSION_RESOLVING_TITLE,
      sessionResolvingSubtitle: HomeText.SESSION_RESOLVING_SUBTITLE,
      sessionTcpConnectedTitle: HomeText.SESSION_TCP_CONNECTED_TITLE,
      sessionTcpConnectedSubtitle: HomeText.SESSION_TCP_CONNECTED_SUBTITLE,
      sessionNegotiatingTitle: HomeText.SESSION_NEGOTIATING_TITLE,
      sessionNegotiatingSubtitle: HomeText.SESSION_NEGOTIATING_SUBTITLE,
      sessionAuthenticatingTitle: HomeText.SESSION_AUTHENTICATING_TITLE,
      sessionAuthenticatingSubtitle: HomeText.SESSION_AUTHENTICATING_SUBTITLE,
      connectionSuccess: HomeText.CONNECTION_SUCCESS,
      connectionConnecting: HomeText.CONNECTION_CONNECTING,
      connectionFailure: HomeText.CONNECTION_FAILURE,
      connectionFailureCredentials: HomeText.CONNECTION_FAILURE_CREDENTIALS,
      connectionFailureNoSession: HomeText.CONNECTION_FAILURE_NO_SESSION,
      connectionFailureWithDetail: (detail: string): string => HomeText.connectionFailureWithDetail(detail),
      validationMessage: (issues: ConnectionValidationIssue[]): string => HomeConnectionValidation.message(issues)
    }
  }

  private getSessionStateCoordinator(): RdpSessionStateCoordinator {
    if (this.sessionStateCoordinator === null) {
      this.sessionStateCoordinator = new RdpSessionStateCoordinator(
        this.rdpClientController,
        this.sessionStateMessages(),
        (): string => this.getAppFilesDir(),
        (input: WindowsConnectionSaveInput): void => this.saveConnectionProfile(input),
        (snapshot: RdpSessionStateSnapshot): void => {
          this.deferUiUpdate((): void => this.applySessionStateSnapshot(snapshot))
        },
        (): void => this.releaseActiveInput()
      )
      this.applySessionStateSnapshot(this.sessionStateCoordinator.snapshot())
    }
    return this.sessionStateCoordinator
  }

  private applySessionStateSnapshot(snapshot: RdpSessionStateSnapshot): void {
    const wasVisible = this.showSession
    this.showSession = snapshot.sessionVisible
    this.remoteLoginWaiting = snapshot.remoteLoginWaiting
    this.sessionNoticeTitle = snapshot.sessionNoticeTitle
    this.sessionNoticeSubtitle = snapshot.sessionNoticeSubtitle
    this.connectionFeedbackText = snapshot.connectionFeedbackText
    this.connectionFeedbackTone = snapshot.connectionFeedbackTone
    if (snapshot.sessionVisible && !wasVisible) {
      this.deferUiUpdate((): void => this.focusRemoteSurface())
    }
    if (!snapshot.sessionVisible && wasVisible && this.rdpClientController.isConnected()) {
      this.releaseActiveInput()
    }
  }

  private getConnectionProfileCoordinator(): WindowsConnectionProfileCoordinator | null {
    if (this.connectionProfileCoordinator !== null) {
      return this.connectionProfileCoordinator
    }

    const context = this.getUiAbilityContext()
    if (context === null) {
      return null
    }

    this.connectionProfileCoordinator = new WindowsConnectionProfileCoordinator(context)
    return this.connectionProfileCoordinator
  }

  private loadConnectionProfiles(): void {
    const coordinator = this.getConnectionProfileCoordinator()
    if (coordinator === null) {
      return
    }

    coordinator.load()
      .then((snapshot: WindowsConnectionSnapshot | null) => {
        if (snapshot === null) {
          return
        }
        this.deferUiUpdate(() => {
          this.connectionProfilesLoaded = true
          this.applyConnectionSnapshot(snapshot, true, false)
        })
      })
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
        this.startNewConnectionProfile()
      }
      return
    }

    this.applyConnectionProfile(selectedProfile, snapshot.selectedPassword)
  }

  private findConnectionProfile(profileId: string): WindowsConnectionProfile | null {
    return this.findConnectionProfileIn(this.connectionProfiles, profileId)
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
    if (profileId.length === 0) {
      this.startNewConnectionProfile()
      return
    }

    const profile = this.findConnectionProfile(profileId)
    if (profile === null) {
      return
    }

    const selectionGeneration = ++this.connectionProfileSelectionGeneration
    if (!profile.rememberPassword) {
      this.connectionProfilePasswordLoading = false
      this.applyConnectionProfile(profile, '')
      return
    }

    const coordinator = this.getConnectionProfileCoordinator()
    if (coordinator === null) {
      this.connectionProfilePasswordLoading = false
      this.applyConnectionProfile(profile, '')
      return
    }
    this.connectionProfilePasswordLoading = true
    coordinator.loadPassword(profile)
      .then((savedPassword: string | null) => {
        if (savedPassword === null) {
          this.deferUiUpdate(() => {
            if (this.connectionProfileSelectionGeneration === selectionGeneration) {
              this.connectionProfilePasswordLoading = false
              this.applyConnectionProfile(profile, '')
            }
          })
          return
        }
        this.deferUiUpdate(() => {
          if (this.connectionProfileSelectionGeneration === selectionGeneration) {
            this.applyConnectionProfile(profile, savedPassword)
            this.connectionProfilePasswordLoading = false
          }
        })
      })
  }

  private startNewConnectionProfile(): void {
    this.connectionProfileSelectionGeneration++
    this.connectionProfilePasswordLoading = false
    this.selectedConnectionProfileId = ''
    this.host = ''
    this.port = '3389'
    this.username = ''
    this.password = ''
    this.rememberConnectionPassword = false
    this.certPolicy = 'tofu'
    this.getSessionStateCoordinator().clearConnectionFeedback()
  }

  private deleteSelectedConnectionProfile(): void {
    if (this.selectedConnectionProfileId.length === 0) {
      this.startNewConnectionProfile()
      return
    }

    const coordinator = this.getConnectionProfileCoordinator()
    if (coordinator === null) {
      return
    }

    coordinator.deleteProfile(this.selectedConnectionProfileId)
      .then((snapshot: WindowsConnectionSnapshot | null) => {
        if (snapshot === null) {
          return
        }
        this.deferUiUpdate(() => {
          this.applyConnectionSnapshot(snapshot, true, true)
        })
      })
  }

  private setRememberConnectionPassword(remember: boolean): void {
    this.rememberConnectionPassword = remember
  }

  private saveConnectionProfile(input: WindowsConnectionSaveInput): void {
    const coordinator = this.getConnectionProfileCoordinator()
    if (coordinator === null) {
      return
    }

    coordinator.saveProfile(input)
      .then((snapshot: WindowsConnectionSnapshot | null) => {
        if (snapshot === null) {
          return
        }
        this.deferUiUpdate(() => {
          this.connectionProfiles = snapshot.profiles
          this.selectedConnectionProfileId = snapshot.selectedProfileId
          const selected = this.findConnectionProfileIn(snapshot.profiles, snapshot.selectedProfileId)
          if (selected !== null) {
            this.rememberConnectionPassword = selected.rememberPassword
          }
        })
      })
  }

  private clearSelectedConnectionPassword(): void {
    this.password = ''
    this.rememberConnectionPassword = false
    if (this.selectedConnectionProfileId.length === 0) {
      this.getSessionStateCoordinator().setConnectionFeedback(HomeText.PASSWORD_CLEARED_CURRENT, 'neutral')
      return
    }

    const coordinator = this.getConnectionProfileCoordinator()
    if (coordinator === null) {
      this.getSessionStateCoordinator().setConnectionFeedback(HomeText.PASSWORD_CLEAR_STORAGE_UNAVAILABLE, 'danger')
      return
    }

    coordinator.clearPassword(this.selectedConnectionProfileId)
      .then((snapshot: WindowsConnectionSnapshot | null) => {
        if (snapshot === null) {
          this.deferUiUpdate(() => {
            this.getSessionStateCoordinator().setConnectionFeedback(HomeText.PASSWORD_CLEAR_FAILED, 'danger')
          })
          return
        }
        this.deferUiUpdate(() => {
          this.applyConnectionSnapshot(snapshot, true, false)
          this.password = ''
          this.rememberConnectionPassword = false
          this.getSessionStateCoordinator().setConnectionFeedback(HomeText.PASSWORD_CLEARED_SAVED, 'ok')
        })
      })
  }

  private xrdpServerDisplayMessages(): XrdpServerDisplayMessages {
    return {
      active: SettingsRemoteText.SERVER_MESSAGE_ACTIVE,
      listening: SettingsRemoteText.SERVER_MESSAGE_LISTENING,
      stopped: SettingsRemoteText.SERVER_MESSAGE_STOPPED,
      exited: SettingsRemoteText.SERVER_MESSAGE_EXITED,
      failed: SettingsRemoteText.SERVER_MESSAGE_FAILED,
      unavailable: SettingsRemoteText.SERVER_MESSAGE_UNAVAILABLE
    }
  }

  private registerNativeCallbacks(): void {
    const result = this.getSessionStateCoordinator().register()
    if (!result.ok) {
      return
    }
    this.registerNativePermissionCallbacks()
  }

  private getAppFilesDir(): string {
    const context = this.getUiAbilityContext()
    if (context === null) {
      return ''
    }
    return context.filesDir
  }

  private openRemoteFilesDirectoryFromSettings(): void {
    RemoteFilesDirectory.openSharedDirectory(
      this.getUiAbilityContext()
    )
      .catch((error: Error) => {
        RdpLogger.error(`remote files directory open failed: ${JSON.stringify(error)}`)
      })
  }

  private openSettingsFromHome(remoteControlSection: string): void {
    const passiveControlAvailable = this.remoteControlServerAvailable()
    const activeControlSection = remoteControlSection === SettingsRemoteControlSection.FILES ||
      remoteControlSection === SettingsRemoteControlSection.ACCESS
    if (passiveControlAvailable) {
      this.refreshScreenRecordingPermissionState()
      this.refreshInputInjectionPermissionState()
      this.refreshXrdpServerDiagnostics()
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

  private registerNativePermissionCallbacks(): void {
    this.getPermissionRequestCoordinator().register()
  }

  private refreshScreenRecordingPermissionState(): boolean {
    return this.getRemoteControlCoordinator().refreshPermissionState()
  }

  private requestScreenRecordingPermissionFromSettings(): Promise<boolean> {
    return this.getRemoteControlCoordinator().requestPermissionFromSettings()
  }

  private refreshInputInjectionPermissionState(): boolean {
    return this.getRemoteControlCoordinator().refreshInputInjectionPermissionState()
  }

  private requestInputInjectionPermissionFromSettings(): Promise<boolean> {
    return this.getRemoteControlCoordinator().requestInputInjectionPermissionFromSettings()
  }

  private refreshXrdpServerDiagnostics(): XrdpServerStatus {
    return this.getRemoteControlCoordinator().refreshDiagnostics()
  }

  private refreshRemoteControlReadiness(clearBusy: boolean = false): XrdpServerStatus {
    return this.getRemoteControlCoordinator().refreshReadiness(clearBusy)
  }

  private startXrdpServerFromSettings(): Promise<XrdpServerStatus> {
    return this.getRemoteControlCoordinator().startFromSettings()
  }

  private setRemoteAccessCodeGateFromSettings(enabled: boolean): string {
    return this.getRemoteControlCoordinator().setAccessCodeGate(enabled)
  }

  private regenerateRemoteAccessCodeFromSettings(): string {
    return this.getRemoteControlCoordinator().regenerateAccessCode()
  }

  private ensureXrdpServerStarted(trigger: string, restartIfRunning: boolean = false,
    requestScreenRecordingImmediately: boolean = false): Promise<XrdpServerStatus> {
    return this.getRemoteControlCoordinator().ensureStarted(
      trigger, restartIfRunning, requestScreenRecordingImmediately)
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
    this.getSessionStateCoordinator().connect({
      host: this.host,
      port: this.port,
      username: this.username,
      password: this.password,
      certPolicy: this.certPolicy,
      selectedProfileId: this.selectedConnectionProfileId,
      rememberPassword: this.rememberConnectionPassword
    })
  }

  private focusRemoteSurface(): void {
    if (!this.rdpClientController.isConnected()) {
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
          getDisplayState: RdpClientController.getDisplaySettings,
          setResolution: RdpClientController.setDisplayResolution,
          refreshDisplay: RdpClientController.refreshDisplay,
          releaseInput: () => { this.rdpClientController.releaseAllInput() },
          onDisconnect: (): void => { this.rdpClientController.disconnect() },
          isDark: this.isAppDark(),
          noticeTitle: this.sessionNoticeTitle,
          noticeSubtitle: this.sessionNoticeSubtitle,
          remoteLoginWaiting: this.remoteLoginWaiting,
          onAttachContent: (content: NodeContent): boolean => this.rdpSurfaceContentHost.attach(content),
          onDetachContent: (): void => { this.rdpSurfaceContentHost.detach() },
          onSurfaceLoad: (): void => { this.deferUiUpdate(() => { this.focusRemoteSurface() }) }
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
          onRemoteAccessCodeGateChange: (enabled: boolean): string => this.setRemoteAccessCodeGateFromSettings(enabled),
          onRemoteAccessCodeRegenerate: (): string => this.regenerateRemoteAccessCodeFromSettings(),
          onRequestScreenRecordingPermission: (): Promise<boolean> => this.requestScreenRecordingPermissionFromSettings(),
          onRefreshScreenRecordingPermission: (): boolean => this.refreshScreenRecordingPermissionState(),
          onRequestInputInjectionPermission: (): Promise<boolean> =>
            this.requestInputInjectionPermissionFromSettings(),
          onRefreshInputInjectionPermission: (): boolean => this.refreshInputInjectionPermissionState(),
          onRefreshXrdpServerStatus: (): XrdpServerStatus => this.refreshXrdpServerDiagnostics(),
          onStartXrdpServer: (): Promise<XrdpServerStatus> => this.startXrdpServerFromSettings(),
          onOpenRemoteFilesDirectory: (): void => this.openRemoteFilesDirectoryFromSettings(),
          onClose: () => {
            if (this.remoteControlServerAvailable()) {
              this.refreshRemoteControlReadiness(this.xrdpServerBusy && !this.screenRecordingPermissionBusy)
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
            this.host = value
            this.getSessionStateCoordinator().clearConnectionFeedback()
          },
          onPortChange: (value: string): void => {
            this.port = value
            this.getSessionStateCoordinator().clearConnectionFeedback()
          },
          onUsernameChange: (value: string): void => {
            this.username = value
            this.getSessionStateCoordinator().clearConnectionFeedback()
          },
          onPasswordChange: (value: string): void => {
            this.password = value
            this.getSessionStateCoordinator().clearConnectionFeedback()
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
