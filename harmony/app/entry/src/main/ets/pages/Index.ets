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
import {
  ConnectionValidationResult,
  RdpConnectionValidator
} from '../rdp/RdpConnectionValidator'
import { RdpLogger } from '../rdp/RdpLogger'
import { RdpPermissionRequestCoordinator } from '../rdp/RdpPermissionRequestCoordinator'
import { RdpPermissionCallbackDelegate, RdpPermissionManager } from '../rdp/RdpPermissions'
import { RemoteControlCoordinator, RemoteControlSnapshot } from '../rdp/RemoteControlCoordinator'
import { RemoteFilesDirectory } from '../rdp/RemoteFilesDirectory'
import { RdpSurfaceContentHost } from '../rdp/RdpSurfaceContentHost'
import {
  XrdpServerDisplayMessages,
  XrdpServerStatus
} from '../rdp/XrdpServerController'
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
import { SettingsRoute, SettingsText } from '../components/settings/SettingsConstants'
import { SettingsAppearanceMode, SettingsStatusTone, SettingsTheme } from '../components/settings/SettingsUi'
@Entry
@Component
struct Index {
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
  private callbacksRegistered: boolean = false
  private readonly rdpClientController: RdpClientController = new RdpClientController()
  private readonly rdpSurfaceContentHost: RdpSurfaceContentHost = new RdpSurfaceContentHost()
  private permissionManager: RdpPermissionManager | null = null
  private permissionRequestCoordinator: RdpPermissionRequestCoordinator | null = null
  private remoteControlCoordinator: RemoteControlCoordinator | null = null
  private connectionProfileCoordinator: WindowsConnectionProfileCoordinator | null = null
  private connectionProfilesLoaded: boolean = false
  private connectionProfileSelectionGeneration: number = 0
  private pendingConnectionProfileSave: WindowsConnectionSaveInput | null = null
  private lastConnectionErrorMessage: string = ''
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
    return hasRemoteControlServer(this.deviceCapabilities)
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
        this.remoteControlServerAvailable(),
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
    this.clearConnectionFeedback()
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

  private persistPendingConnectionProfile(): void {
    if (this.pendingConnectionProfileSave === null) {
      return
    }
    const saveInput = this.pendingConnectionProfileSave
    this.pendingConnectionProfileSave = null
    this.saveConnectionProfile(saveInput)
  }

  private clearSelectedConnectionPassword(): void {
    this.password = ''
    this.rememberConnectionPassword = false
    if (this.selectedConnectionProfileId.length === 0) {
      this.setConnectionFeedback(HomeText.PASSWORD_CLEARED_CURRENT, 'neutral')
      return
    }

    const coordinator = this.getConnectionProfileCoordinator()
    if (coordinator === null) {
      this.setConnectionFeedback(HomeText.PASSWORD_CLEAR_STORAGE_UNAVAILABLE, 'danger')
      return
    }

    coordinator.clearPassword(this.selectedConnectionProfileId)
      .then((snapshot: WindowsConnectionSnapshot | null) => {
        if (snapshot === null) {
          this.deferUiUpdate(() => {
            this.setConnectionFeedback(HomeText.PASSWORD_CLEAR_FAILED, 'danger')
          })
          return
        }
        this.deferUiUpdate(() => {
          this.applyConnectionSnapshot(snapshot, true, false)
          this.password = ''
          this.rememberConnectionPassword = false
          this.setConnectionFeedback(HomeText.PASSWORD_CLEARED_SAVED, 'ok')
        })
      })
  }

  private clearConnectionFeedback(): void {
    this.connectionFeedbackText = ''
    this.connectionFeedbackTone = 'neutral'
  }

  private setConnectionFeedback(text: string, tone: SettingsStatusTone): void {
    this.connectionFeedbackText = text
    this.connectionFeedbackTone = tone
  }

  private connectionFailureMessage(message: string, fallback: string = HomeText.CONNECTION_FAILURE): string {
    const detail = message.trim()
    if (detail.length === 0) {
      return fallback
    }
    return HomeText.connectionFailureWithDetail(detail)
  }

  private errorMessage(error: Error): string {
    if (error.message.length > 0) {
      return error.message
    }
    const serialized = JSON.stringify(error)
    return serialized.length > 0 && serialized !== '{}' ? serialized : ''
  }

  private validateConnectionForm(): ConnectionValidationResult {
    return RdpConnectionValidator.validate(this.host, this.port, this.username, this.password)
  }

  private xrdpServerDisplayMessages(): XrdpServerDisplayMessages {
    return {
      active: SettingsText.REMOTE_SERVER_MESSAGE_ACTIVE,
      listening: SettingsText.REMOTE_SERVER_MESSAGE_LISTENING,
      stopped: SettingsText.REMOTE_SERVER_MESSAGE_STOPPED,
      exited: SettingsText.REMOTE_SERVER_MESSAGE_EXITED,
      failed: SettingsText.REMOTE_SERVER_MESSAGE_FAILED,
      unavailable: SettingsText.REMOTE_SERVER_MESSAGE_UNAVAILABLE
    }
  }

  private registerNativeCallbacks(): void {
    if (this.callbacksRegistered) {
      return
    }

    try {
      const callbackResult = this.rdpClientController.registerCallbacks({
        onState: (state: string, wasConnected: boolean): void => {
          this.deferUiUpdate(() => {
            const nextConnected = this.isConnectedNativeState(state)
            if (!nextConnected && (state === 'Disconnected' || state === 'Failed' || state === 'Idle')) {
              if (wasConnected) {
                this.releaseActiveInput()
              }
              this.setSessionVisible(false)
            }
            this.updateSessionNoticeForNativeState(state)
            if (nextConnected && !wasConnected) {
              this.setSessionVisible(true)
              this.focusRemoteSurface()
              this.lastConnectionErrorMessage = ''
              this.setConnectionFeedback(HomeText.CONNECTION_SUCCESS, 'ok')
              this.persistPendingConnectionProfile()
            }
            if (state === 'Failed') {
              this.pendingConnectionProfileSave = null
              this.setConnectionFeedback(this.connectionFailureMessage(this.lastConnectionErrorMessage,
                HomeText.CONNECTION_FAILURE_CREDENTIALS), 'danger')
              RdpLogger.error(`Native state: ${state}`)
            } else if ((state === 'Disconnected' || state === 'Idle') && this.pendingConnectionProfileSave !== null) {
              this.pendingConnectionProfileSave = null
              this.setConnectionFeedback(this.connectionFailureMessage(this.lastConnectionErrorMessage,
                HomeText.CONNECTION_FAILURE_NO_SESSION), 'danger')
            } else if (state === 'Connected' || state === 'Disconnected' ||
              state === 'RemoteLoginWaiting' || state === 'RemoteDesktopReady') {
              RdpLogger.info(`Native state: ${state}`)
            }
          })
        },
        onError: (message: string, wasConnected: boolean): void => {
          this.deferUiUpdate(() => {
            if (wasConnected) {
              this.releaseActiveInput()
            }
            this.setSessionVisible(false)
            this.clearSessionNotice()
            this.pendingConnectionProfileSave = null
            this.lastConnectionErrorMessage = message
            this.setConnectionFeedback(this.connectionFailureMessage(message), 'danger')
            RdpLogger.error(`Native error: ${message}`)
          })
        }
      })
      if (!callbackResult.ok) {
        RdpLogger.error(`Native callback registration failed: ${callbackResult.message}`)
        return
      }
      this.registerNativePermissionCallbacks()
      this.callbacksRegistered = true
    } catch (error) {
      RdpLogger.error(`Native callback registration failed: ${JSON.stringify(error)}`)
    }
  }

  private isConnectedNativeState(state: string): boolean {
    return RdpClientController.isConnectedState(state)
  }

  private clearSessionNotice(): void {
    this.remoteLoginWaiting = false
    this.sessionNoticeTitle = ''
    this.sessionNoticeSubtitle = ''
  }

  private updateSessionNoticeForNativeState(state: string): void {
    if (state === 'RemoteLoginWaiting') {
      this.remoteLoginWaiting = true
      this.sessionNoticeTitle = HomeText.SESSION_REMOTE_LOGIN_WAITING_TITLE
      this.sessionNoticeSubtitle = HomeText.SESSION_REMOTE_LOGIN_WAITING_SUBTITLE
      return
    }

    this.remoteLoginWaiting = false
    if (state === 'Connected') {
      this.sessionNoticeTitle = HomeText.SESSION_CONNECTED_TITLE
      this.sessionNoticeSubtitle = HomeText.SESSION_CONNECTED_SUBTITLE
      return
    }

    if (state === 'RemoteDesktopReady' || state === 'Disconnected' || state === 'Failed' || state === 'Idle') {
      this.sessionNoticeTitle = ''
      this.sessionNoticeSubtitle = ''
      return
    }

    if (state === 'Resolving') {
      this.sessionNoticeTitle = HomeText.SESSION_RESOLVING_TITLE
      this.sessionNoticeSubtitle = HomeText.SESSION_RESOLVING_SUBTITLE
    } else if (state === 'TCP connected') {
      this.sessionNoticeTitle = HomeText.SESSION_TCP_CONNECTED_TITLE
      this.sessionNoticeSubtitle = HomeText.SESSION_TCP_CONNECTED_SUBTITLE
    } else if (state === 'Negotiating') {
      this.sessionNoticeTitle = HomeText.SESSION_NEGOTIATING_TITLE
      this.sessionNoticeSubtitle = HomeText.SESSION_NEGOTIATING_SUBTITLE
    } else if (state === 'Authenticating') {
      this.sessionNoticeTitle = HomeText.SESSION_AUTHENTICATING_TITLE
      this.sessionNoticeSubtitle = HomeText.SESSION_AUTHENTICATING_SUBTITLE
    }
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
    if (this.remoteControlServerAvailable()) {
      this.refreshScreenRecordingPermissionState()
      this.refreshInputInjectionPermissionState()
      this.refreshXrdpServerDiagnostics()
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
    const validation = this.validateConnectionForm()
    if (!validation.ok) {
      const validationMessage = HomeConnectionValidation.message(validation.issues)
      this.rdpClientController.beginConnect()
      this.pendingConnectionProfileSave = null
      this.setConnectionFeedback(validationMessage, 'warning')
      RdpLogger.warn(`Connection validation failed: ${validationMessage}`)
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
    this.pendingConnectionProfileSave = {
      profileId: this.selectedConnectionProfileId,
      host: host,
      port: port,
      username: username,
      password: password,
      certPolicy: policy,
      rememberPassword: this.rememberConnectionPassword
    }
    this.lastConnectionErrorMessage = ''
    this.rdpClientController.beginConnect()
    this.remoteLoginWaiting = false
    this.setConnectionFeedback(HomeText.CONNECTION_CONNECTING, 'info')
    this.sessionNoticeTitle = HomeText.SESSION_CONNECTING_TITLE
    this.sessionNoticeSubtitle = HomeText.SESSION_CONNECTING_SUBTITLE
    this.setSessionVisible(true)
    this.queueNativeConnect(host, port, username, password, policy, appFilesDir)
  }

  private queueNativeConnect(host: string, port: string, username: string, password: string, policy: string,
    appFilesDir: string): void {
    setTimeout((): void => {
      this.startNativeConnect(host, port, username, password, policy, appFilesDir)
    }, 0)
  }

  private startNativeConnect(host: string, port: string, username: string, password: string,
    policy: string, appFilesDir: string): void {
    try {
      const result = this.rdpClientController.connect({
        host: host,
        port: port,
        username: username,
        password: password,
        certPolicy: policy,
        appFilesDir: appFilesDir
      })
      if (result.ok) {
        this.setSessionVisible(true)
        if (this.isConnected()) {
          this.lastConnectionErrorMessage = ''
          this.setConnectionFeedback(HomeText.CONNECTION_SUCCESS, 'ok')
          this.persistPendingConnectionProfile()
        }
      } else {
        RdpLogger.error(result.message)
        this.setSessionVisible(false)
        this.clearSessionNotice()
        this.pendingConnectionProfileSave = null
        this.lastConnectionErrorMessage = result.message
        this.setConnectionFeedback(this.connectionFailureMessage(result.message), 'danger')
      }
    } catch (error) {
      this.setSessionVisible(false)
      this.clearSessionNotice()
      this.pendingConnectionProfileSave = null
      this.lastConnectionErrorMessage = this.errorMessage(error as Error)
      this.setConnectionFeedback(this.connectionFailureMessage(this.lastConnectionErrorMessage,
        HomeText.CONNECTION_FAILURE_CREDENTIALS), 'danger')
      RdpLogger.error(`Native connect failed: ${JSON.stringify(error)}`)
    }
  }

  private setSessionVisible(visible: boolean): void {
    if (!visible && this.showSession && this.isConnected()) {
      this.releaseActiveInput()
    }
    this.showSession = visible
    if (!visible) {
      this.clearSessionNotice()
    }
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
            this.clearConnectionFeedback()
          },
          onPortChange: (value: string): void => {
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
