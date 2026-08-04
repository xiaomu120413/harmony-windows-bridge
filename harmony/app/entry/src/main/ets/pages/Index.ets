import { common } from '@kit.AbilityKit'
import { uiObserver } from '@kit.ArkUI'
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
import { RdpLogger } from '../rdp/RdpLogger'
import { RdpPermissionCallbackDelegate, RdpPermissionManager } from '../rdp/RdpPermissions'
import { RemoteFilesDirectory } from '../rdp/RemoteFilesDirectory'
import {
  XrdpServerController,
  XrdpServerDisplayMessages,
  XrdpServerStartOptions,
  XrdpServerStatus
} from '../rdp/XrdpServerController'
import {
  WindowsConnectionProfile,
  WindowsConnectionSaveInput,
  WindowsConnectionSnapshot,
  WindowsConnectionStore
} from '../rdp/WindowsConnectionStore'
import { HomePage } from '../components/home/HomePage'
import { HomeText } from '../components/home/HomeText'
import { SettingsPage } from '../components/SettingsPage'
import { SettingsRoute, SettingsText } from '../components/settings/SettingsConstants'
import { SettingsAppearanceMode, SettingsStatusTone, SettingsTheme } from '../components/settings/SettingsUi'
import rdpNative, {
  NativeCommandResult
} from 'libentry.so'

interface ConnectionValidationResult {
  ok: boolean
  message: string
  host: string
  port: string
  username: string
  password: string
}

interface NativePermissionCallbackRegistration {
  label: string
  trigger: string
  onRequest: (callback: (requestId: string) => void) => NativeCommandResult
  ensurePermission: (trigger: string) => Promise<boolean>
  completeRequest: (requestId: number, granted: boolean) => NativeCommandResult
}

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
  // Temporarily pin TOFU certificate policy; restore policy selector when strict validation is needed.
  private certPolicy: string = 'tofu'
  @State private remoteAccessCode: string = '000000'
  @State private remoteAccessCodeGateEnabled: boolean = false
  @State private screenRecordingPermissionGranted: boolean = false
  @State private screenRecordingPermissionBusy: boolean = false
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
  private connected: boolean = false
  private callbacksRegistered: boolean = false
  private remoteAccessCodeInitialized: boolean = false
  private surfaceController: XComponentController = new XComponentController()
  private permissionManager: RdpPermissionManager | null = null
  private xrdpServerController: XrdpServerController | null = null
  private xrdpServerStartRequest: Promise<XrdpServerStatus> | null = null
  private windowsConnectionStore: WindowsConnectionStore | null = null
  private connectionProfilesLoaded: boolean = false
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
  }

  aboutToDisappear(): void {
    this.unregisterWindowLayoutObserver()
    this.releaseActiveModifiers()
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
        this.onScreenRecordingPermissionPromptOpen()
      })
    }
    return this.permissionManager
  }

  private getXrdpServerController(): XrdpServerController {
    if (this.xrdpServerController === null) {
      this.xrdpServerController = new XrdpServerController(
        this.xrdpServerDisplayMessages(),
        this.remoteControlServerAvailable()
      )
    }
    return this.xrdpServerController
  }

  private getWindowsConnectionStore(): WindowsConnectionStore | null {
    if (this.windowsConnectionStore !== null) {
      return this.windowsConnectionStore
    }

    const context = this.getUiAbilityContext()
    if (context === null) {
      return null
    }

    this.windowsConnectionStore = new WindowsConnectionStore(context)
    return this.windowsConnectionStore
  }

  private loadConnectionProfiles(): void {
    const store = this.getWindowsConnectionStore()
    if (store === null) {
      return
    }

    store.loadSnapshot()
      .then((snapshot: WindowsConnectionSnapshot) => {
        this.deferUiUpdate(() => {
          this.connectionProfilesLoaded = true
          this.applyConnectionSnapshot(snapshot, true, false)
        })
      })
      .catch((error: Error) => {
        RdpLogger.warn(`windows connection profiles load failed: ${JSON.stringify(error)}`)
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

    this.applyConnectionProfile(profile, '')
    if (!profile.rememberPassword) {
      return
    }

    const store = this.getWindowsConnectionStore()
    if (store === null) {
      return
    }
    store.loadPassword(profile)
      .then((savedPassword: string) => {
        this.deferUiUpdate(() => {
          if (this.selectedConnectionProfileId === profile.id) {
            this.password = savedPassword
          }
        })
      })
      .catch((error: Error) => {
        RdpLogger.warn(`windows connection profile password select failed: ${JSON.stringify(error)}`)
      })
  }

  private startNewConnectionProfile(): void {
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

    const store = this.getWindowsConnectionStore()
    if (store === null) {
      return
    }

    store.deleteProfile(this.selectedConnectionProfileId)
      .then((snapshot: WindowsConnectionSnapshot) => {
        this.deferUiUpdate(() => {
          this.applyConnectionSnapshot(snapshot, true, true)
        })
      })
      .catch((error: Error) => {
        RdpLogger.warn(`windows connection profile delete failed: ${JSON.stringify(error)}`)
      })
  }

  private setRememberConnectionPassword(remember: boolean): void {
    this.rememberConnectionPassword = remember
  }

  private saveConnectionProfile(input: WindowsConnectionSaveInput): void {
    const store = this.getWindowsConnectionStore()
    if (store === null) {
      return
    }

    store.saveProfile(input)
      .then((profile: WindowsConnectionProfile | null) => {
        if (profile === null) {
          return
        }
        this.deferUiUpdate(() => {
          this.selectedConnectionProfileId = profile.id
          this.rememberConnectionPassword = profile.rememberPassword
        })
        return store.loadSnapshot()
      })
      .then((snapshot: WindowsConnectionSnapshot | void) => {
        if (!snapshot) {
          return
        }
        this.deferUiUpdate(() => {
          this.connectionProfiles = snapshot.profiles
          this.selectedConnectionProfileId = snapshot.selectedProfileId
        })
      })
      .catch((error: Error) => {
        RdpLogger.warn(`windows connection profile save failed: ${JSON.stringify(error)}`)
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

    const store = this.getWindowsConnectionStore()
    if (store === null) {
      this.setConnectionFeedback(HomeText.PASSWORD_CLEAR_STORAGE_UNAVAILABLE, 'danger')
      return
    }

    store.clearPassword(this.selectedConnectionProfileId)
      .then((snapshot: WindowsConnectionSnapshot) => {
        this.deferUiUpdate(() => {
          this.applyConnectionSnapshot(snapshot, true, false)
          this.password = ''
          this.rememberConnectionPassword = false
          this.setConnectionFeedback(HomeText.PASSWORD_CLEARED_SAVED, 'ok')
        })
      })
      .catch((error: Error) => {
        this.setConnectionFeedback(HomeText.PASSWORD_CLEAR_FAILED, 'danger')
        RdpLogger.warn(`windows connection password clear failed: ${JSON.stringify(error)}`)
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
    const host = this.host.trim()
    const port = this.port.trim()
    const username = this.username.trim()
    const password = this.password
    const messages: string[] = []
    const hostMessage = this.validateHost(host)
    if (hostMessage.length > 0) {
      messages.push(hostMessage)
    }
    const portMessage = this.validatePort(port)
    if (portMessage.length > 0) {
      messages.push(portMessage)
    }
    if (username.length === 0) {
      messages.push(HomeText.USERNAME_REQUIRED)
    }
    if (password.trim().length === 0) {
      messages.push(HomeText.PASSWORD_REQUIRED)
    }
    if (messages.length > 0) {
      return this.connectionValidationError(messages.join(HomeText.VALIDATION_SEPARATOR), host, port, username, password)
    }
    return {
      ok: true,
      message: '',
      host: host,
      port: port,
      username: username,
      password: password
    }
  }

  private connectionValidationError(message: string, host: string, port: string, username: string,
    password: string): ConnectionValidationResult {
    return {
      ok: false,
      message: message,
      host: host,
      port: port,
      username: username,
      password: password
    }
  }

  private validateHost(host: string): string {
    if (host.length === 0) {
      return HomeText.HOST_REQUIRED
    }
    if (host.length > 253) {
      return HomeText.HOST_TOO_LONG
    }
    if (/\s/.test(host)) {
      return HomeText.HOST_NO_SPACES
    }
    if (host.indexOf('/') >= 0 || host.indexOf('\\') >= 0 || host.indexOf('://') >= 0) {
      return HomeText.HOST_NO_PROTOCOL_OR_PATH
    }
    if (host.indexOf(':') >= 0) {
      return this.isValidIpv6Host(host) ? '' : HomeText.HOST_FORMAT_INVALID
    }
    if (this.isIpv4Candidate(host)) {
      return this.isValidIpv4Host(host) ? '' : HomeText.HOST_IP_FORMAT_INVALID
    }
    return this.isValidDnsHost(host) ? '' : HomeText.HOST_FORMAT_INVALID
  }

  private validatePort(port: string): string {
    if (port.length === 0) {
      return HomeText.PORT_REQUIRED
    }
    if (!/^[0-9]+$/.test(port)) {
      return HomeText.PORT_DIGITS_ONLY
    }
    const portNumber = Number(port)
    if (portNumber < 1 || portNumber > 65535) {
      return HomeText.PORT_RANGE_INVALID
    }
    return ''
  }

  private isIpv4Candidate(host: string): boolean {
    return /^[0-9.]+$/.test(host) && host.indexOf('.') >= 0
  }

  private isValidIpv4Host(host: string): boolean {
    const parts = host.split('.')
    if (parts.length !== 4) {
      return false
    }
    for (const part of parts) {
      if (part.length === 0 || !/^[0-9]+$/.test(part)) {
        return false
      }
      const value = Number(part)
      if (value < 0 || value > 255 || Math.floor(value) !== value) {
        return false
      }
    }
    return true
  }

  private isValidIpv6Host(host: string): boolean {
    return /^[0-9A-Fa-f:]+$/.test(host) && host.indexOf(':') >= 0 && host.split(':').length >= 3
  }

  private isValidDnsHost(host: string): boolean {
    const labels = host.split('.')
    const labelPattern = /^[A-Za-z0-9]([A-Za-z0-9-]{0,61}[A-Za-z0-9])?$/
    for (const label of labels) {
      if (label.length === 0 || label.length > 63 || !labelPattern.test(label)) {
        return false
      }
    }
    return true
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
      rdpNative.onState((state: string) => {
        this.deferUiUpdate(() => {
          const wasConnected = this.isConnected()
          const nextConnected = this.isConnectedNativeState(state)
          if (!nextConnected && (state === 'Disconnected' || state === 'Failed' || state === 'Idle')) {
            this.setSessionVisible(false)
          }
          this.connected = nextConnected
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
      })
      rdpNative.onError((message: string) => {
        this.deferUiUpdate(() => {
          this.setSessionVisible(false)
          this.connected = false
          this.clearSessionNotice()
          this.pendingConnectionProfileSave = null
          this.lastConnectionErrorMessage = message
          this.setConnectionFeedback(this.connectionFailureMessage(message), 'danger')
          RdpLogger.error(`Native error: ${message}`)
        })
      })
      this.registerNativePermissionCallbacks()
      this.callbacksRegistered = true
    } catch (error) {
      RdpLogger.error(`Native callback registration failed: ${JSON.stringify(error)}`)
    }
  }

  private isConnectedNativeState(state: string): boolean {
    return state === 'Connected' || state === 'RemoteLoginWaiting' || state === 'RemoteDesktopReady'
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
    const registrations: NativePermissionCallbackRegistration[] = [
      {
        label: 'Remote audio capture requested microphone permission',
        trigger: 'remote audio capture',
        onRequest: (callback: (requestId: string) => void): NativeCommandResult =>
          rdpNative.onMicrophonePermissionRequest(callback),
        ensurePermission: (trigger: string): Promise<boolean> =>
          this.getPermissionManager().ensureMicrophonePermission(trigger),
        completeRequest: (requestId: number, granted: boolean): NativeCommandResult =>
          rdpNative.completeMicrophonePermissionRequest({ requestId: requestId, granted: granted })
      },
      {
        label: 'Remote camera redirection requested camera permission',
        trigger: 'remote camera redirection',
        onRequest: (callback: (requestId: string) => void): NativeCommandResult =>
          rdpNative.onCameraPermissionRequest(callback),
        ensurePermission: (trigger: string): Promise<boolean> =>
          this.getPermissionManager().ensureCameraPermission(trigger),
        completeRequest: (requestId: number, granted: boolean): NativeCommandResult =>
          rdpNative.completeCameraPermissionRequest({ requestId: requestId, granted: granted })
      },
      {
        label: 'Clipboard sync requested pasteboard permission',
        trigger: 'clipboard read',
        onRequest: (callback: (requestId: string) => void): NativeCommandResult =>
          rdpNative.onClipboardPermissionRequest(callback),
        ensurePermission: (trigger: string): Promise<boolean> =>
          this.getPermissionManager().ensurePasteboardReadPermission(trigger),
        completeRequest: (requestId: number, granted: boolean): NativeCommandResult =>
          rdpNative.completeClipboardPermissionRequest({ requestId: requestId, granted: granted })
      },
      {
        label: 'RDP location channel requested location permission',
        trigger: 'remote location redirection',
        onRequest: (callback: (requestId: string) => void): NativeCommandResult =>
          rdpNative.onLocationPermissionRequest(callback),
        ensurePermission: (trigger: string): Promise<boolean> =>
          this.getPermissionManager().ensureLocationPermission(trigger),
        completeRequest: (requestId: number, granted: boolean): NativeCommandResult =>
          rdpNative.completeLocationPermissionRequest({ requestId: requestId, granted: granted })
      }
    ]

    for (let i = 0; i < registrations.length; i++) {
      const registration = registrations[i]
      registration.onRequest((requestIdText: string) => {
        this.deferUiUpdate(() => {
          this.handleNativePermissionRequest(
            requestIdText,
            registration.label,
            registration.trigger,
            registration.ensurePermission,
            registration.completeRequest
          )
        })
      })
    }
  }

  private handleNativePermissionRequest(requestIdText: string, label: string, trigger: string,
    ensurePermission: (trigger: string) => Promise<boolean>,
    completeRequest: (requestId: number, granted: boolean) => NativeCommandResult): void {
    const requestId = Number(requestIdText)
    const safeRequestId = isNaN(requestId) ? 0 : requestId
    RdpLogger.info(`${label}: requestId=${requestIdText}`)
    ensurePermission(trigger)
      .then((granted: boolean) => {
        const result: NativeCommandResult = completeRequest(safeRequestId, granted)
        if (!granted || !result.ok) {
          RdpLogger.warn(`${result.message}: granted=${granted}`)
        }
      })
      .catch((error: Error) => {
        RdpLogger.error(`${label} failed: ${JSON.stringify(error)}`)
        const result: NativeCommandResult = completeRequest(safeRequestId, false)
        RdpLogger.warn(`${result.message}: granted=false`)
      })
  }

  private generateRemoteAccessCode(): string {
    const value = Math.floor(100000 + Math.random() * 900000)
    return `${value}`
  }

  private ensureRemoteAccessCode(): string {
    if (!this.remoteAccessCodeInitialized || this.remoteAccessCode === '000000') {
      this.remoteAccessCode = this.generateRemoteAccessCode()
      this.remoteAccessCodeInitialized = true
    }
    return this.remoteAccessCode
  }

  private hasScreenRecordingPermission(): boolean {
    if (!this.remoteControlServerAvailable()) {
      return false
    }
    return this.getPermissionManager().hasScreenRecordingPermission()
  }

  private refreshScreenRecordingPermissionState(): boolean {
    if (!this.remoteControlServerAvailable()) {
      this.screenRecordingPermissionGranted = false
      this.screenRecordingPermissionBusy = false
      return false
    }
    this.screenRecordingPermissionGranted = this.hasScreenRecordingPermission()
    return this.screenRecordingPermissionGranted
  }

  private clearXrdpServerBusy(reason: string): void {
    if (this.xrdpServerBusy) {
      RdpLogger.info(`xrdp busy cleared: ${reason}`)
    }
    this.xrdpServerBusy = false
  }

  private setScreenRecordingPermissionBusy(busy: boolean): void {
    this.screenRecordingPermissionBusy = busy
    if (busy) {
      this.xrdpServerBusy = true
    }
  }

  private onScreenRecordingPermissionPromptOpen(): void {
    RdpLogger.info('screen recording permission prompt opened')
    this.setScreenRecordingPermissionBusy(true)
  }

  private applyScreenRecordingPermissionResult(granted: boolean, clearBusy: boolean = false): boolean {
    const allowed = granted || this.hasScreenRecordingPermission()
    RdpLogger.info(`screen recording permission result applied: granted=${granted} allowed=${allowed}`)
    this.screenRecordingPermissionGranted = allowed
    if (clearBusy || !allowed) {
      this.clearXrdpServerBusy('screen recording permission not granted')
    }
    this.refreshXrdpServerDiagnostics()
    return allowed
  }

  private requestScreenRecordingPermissionFromSettings(): Promise<boolean> {
    if (!this.remoteControlServerAvailable()) {
      return Promise.resolve(false)
    }
    return this.getPermissionManager().requestScreenRecordingPermission('remote control settings')
      .then((granted: boolean) => {
        const allowed = this.applyScreenRecordingPermissionResult(granted, !granted)
        if (!allowed) {
          return false
        }
        return this.ensureXrdpServerStarted('screen recording permission granted')
          .then((_status: XrdpServerStatus) => {
            return this.applyScreenRecordingPermissionResult(true)
          })
      })
      .catch((error: Error) => {
        RdpLogger.error(`screen recording permission request from settings failed: ${JSON.stringify(error)}`)
        return this.applyScreenRecordingPermissionResult(false, true)
      })
      .finally(() => {
        this.setScreenRecordingPermissionBusy(false)
      })
  }

  private currentXrdpServerStatus(): XrdpServerStatus {
    return {
      running: this.xrdpServerRunning,
      state: this.xrdpServerState,
      port: this.xrdpServerPort,
      message: this.xrdpServerMessage
    }
  }

  private applyXrdpServerStatus(status: XrdpServerStatus): XrdpServerStatus {
    this.xrdpServerRunning = status.running
    this.xrdpServerState = status.state
    this.xrdpServerPort = status.port
    this.xrdpServerMessage = status.message
    return status
  }

  private refreshXrdpServerDiagnostics(): XrdpServerStatus {
    return this.applyXrdpServerStatus(this.getXrdpServerController().diagnostics())
  }

  private refreshRemoteControlReadiness(clearBusy: boolean = false): XrdpServerStatus {
    if (clearBusy) {
      this.clearXrdpServerBusy('remote control readiness refreshed')
    }
    this.refreshScreenRecordingPermissionState()
    return this.refreshXrdpServerDiagnostics()
  }

  private startXrdpServerFromSettings(): Promise<XrdpServerStatus> {
    return this.ensureXrdpServerStarted('remote control settings button', false, true)
  }

  private setRemoteAccessCodeGateFromSettings(enabled: boolean): string {
    this.setRemoteAccessCodeGateEnabled(enabled)
    return this.remoteAccessCode
  }

  private regenerateRemoteAccessCodeFromSettings(): string {
    this.remoteAccessCode = this.generateRemoteAccessCode()
    this.remoteAccessCodeInitialized = true
    if (this.remoteAccessCodeGateEnabled) {
      this.ensureXrdpServerStarted('access code regenerated', true)
    }
    return this.remoteAccessCode
  }

  private xrdpServerStartOptions(restartIfRunning: boolean = false): XrdpServerStartOptions {
    return {
      appFilesDir: this.getAppFilesDir(),
      accessCode: this.remoteAccessCodeGateEnabled ? this.ensureRemoteAccessCode() : '',
      accessCodeGateEnabled: this.remoteAccessCodeGateEnabled,
      restartIfRunning: restartIfRunning
    }
  }

  private ensureXrdpServerStarted(trigger: string, restartIfRunning: boolean = false,
    requestScreenRecordingImmediately: boolean = false): Promise<XrdpServerStatus> {
    if (!this.remoteControlServerAvailable()) {
      return Promise.resolve(this.refreshXrdpServerDiagnostics())
    }
    if (this.xrdpServerStartRequest !== null && !restartIfRunning) {
      if (requestScreenRecordingImmediately) {
        this.getPermissionManager().requestScreenRecordingPermission(`xrdp ${trigger}`)
          .then((granted: boolean) => {
            this.applyScreenRecordingPermissionResult(granted, !granted)
          })
          .catch((error: Error) => {
            RdpLogger.error(`xrdp immediate permission request failed: ${JSON.stringify(error)}`)
            this.applyScreenRecordingPermissionResult(false, true)
          })
          .finally(() => {
            this.setScreenRecordingPermissionBusy(false)
          })
      }
      return this.xrdpServerStartRequest
    }
    this.xrdpServerBusy = this.hasScreenRecordingPermission()
    const screenRecordingPermissionRequest = requestScreenRecordingImmediately ?
      this.getPermissionManager().requestScreenRecordingPermission(`xrdp ${trigger}`) :
      this.getPermissionManager().ensureScreenRecordingPermission(`xrdp ${trigger}`)
    this.xrdpServerStartRequest = screenRecordingPermissionRequest
      .then((granted: boolean) => {
        const allowed = this.applyScreenRecordingPermissionResult(granted, !granted)
        if (!allowed) {
          RdpLogger.warn(`xrdp start skipped: screen recording permission denied: ${trigger}`)
          return this.refreshRemoteControlReadiness(true)
        }
        return this.startXrdpServerAfterPermissions(trigger, restartIfRunning)
      })
      .catch((error: Error) => {
        RdpLogger.error(`xrdp permission check failed: ${JSON.stringify(error)}`)
        this.applyScreenRecordingPermissionResult(false, true)
        return this.refreshXrdpServerDiagnostics()
      })
      .finally(() => {
        this.xrdpServerStartRequest = null
        this.clearXrdpServerBusy('xrdp start request finished')
        this.setScreenRecordingPermissionBusy(false)
        this.refreshRemoteControlReadiness()
      })
    return this.xrdpServerStartRequest
  }

  private startXrdpServerAfterPermissions(trigger: string, restartIfRunning: boolean): XrdpServerStatus {
    const status = this.getXrdpServerController().start(this.xrdpServerStartOptions(restartIfRunning), trigger)
    return this.applyXrdpServerStatus(status)
  }

  private setRemoteAccessCodeGateEnabled(enabled: boolean): void {
    if (!this.remoteControlServerAvailable()) {
      return
    }
    if (this.remoteAccessCodeGateEnabled === enabled) {
      return
    }
    this.remoteAccessCodeGateEnabled = enabled
    if (enabled) {
      this.ensureRemoteAccessCode()
    }
    this.ensureXrdpServerStarted(enabled ? 'access code gate enabled' : 'access code gate disabled', true)
  }

  private connectNative(): void {
    const validation = this.validateConnectionForm()
    if (!validation.ok) {
      this.connected = false
      this.pendingConnectionProfileSave = null
      this.setConnectionFeedback(validation.message, 'warning')
      RdpLogger.warn(`Connection validation failed: ${validation.message}`)
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
    this.connected = false
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
      const result: NativeCommandResult = rdpNative.connect({
        host: host,
        port: port,
        username: username,
        password: password,
        certPolicy: policy,
        appFilesDir: appFilesDir
      })
      if (result.ok) {
        this.connected = this.isConnectedNativeState(result.state)
        this.setSessionVisible(true)
        if (this.connected) {
          this.lastConnectionErrorMessage = ''
          this.setConnectionFeedback(HomeText.CONNECTION_SUCCESS, 'ok')
          this.persistPendingConnectionProfile()
        }
      } else {
        RdpLogger.error(result.message)
        this.setSessionVisible(false)
        this.connected = false
        this.clearSessionNotice()
        this.pendingConnectionProfileSave = null
        this.lastConnectionErrorMessage = result.message
        this.setConnectionFeedback(this.connectionFailureMessage(result.message), 'danger')
      }
    } catch (error) {
      this.setSessionVisible(false)
      this.connected = false
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
      this.releaseActiveModifiers()
    }
    this.showSession = visible
    if (!visible) {
      this.clearSessionNotice()
    }
  }

  private isConnected(): boolean {
    return this.connected
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

  private releaseActiveModifiers(): void {
    if (!this.isConnected()) {
      return
    }

    try {
      const result: NativeCommandResult = rdpNative.releaseAllKeys()
      if (!result.ok) {
        RdpLogger.warn(result.message)
      }
    } catch (error) {
      RdpLogger.error(`Native release all keys failed: ${JSON.stringify(error)}`)
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

  @Builder
  private buildSessionPage() {
    Stack({ alignContent: Alignment.TopStart }) {
        XComponent({
          id: 'rdpSurface',
          type: XComponentType.SURFACE,
          libraryname: 'entry',
          controller: this.surfaceController
        })
          .width('100%')
          .height('100%')
          .onLoad(() => {
            this.deferUiUpdate(() => {
              this.focusRemoteSurface()
            })
          })
          .id(REMOTE_SURFACE_FOCUS_ID)
          .focusable(true)
          .focusOnTouch(true)
          .defaultFocus(true)
          .backgroundColor(Color.Black)
          .renderFit(RenderFit.CENTER)
        if (this.shouldShowSessionNotice()) {
          this.buildSessionNotice()
        }
    }
    .width('100%')
    .height('100%')
    .backgroundColor(Color.Black)
  }

  private shouldShowSessionNotice(): boolean {
    return this.sessionNoticeTitle.length > 0
  }

  @Builder
  private buildSessionNotice() {
    Row() {
      Column() {
        Text(this.sessionNoticeTitle)
          .fontSize(14)
          .fontWeight(FontWeight.Medium)
          .fontColor(Color.White)
          .maxLines(1)
          .textOverflow({ overflow: TextOverflow.Ellipsis })
        Text(this.sessionNoticeSubtitle)
          .fontSize(12)
          .fontColor('rgba(255,255,255,0.72)')
          .margin({ top: 4 })
          .maxLines(1)
          .textOverflow({ overflow: TextOverflow.Ellipsis })
      }
      .alignItems(HorizontalAlign.Start)
    }
    .padding({ left: 14, right: 14, top: 10, bottom: 10 })
    .margin({ left: 18, top: 18, right: 18 })
    .constraintSize({ maxWidth: 520 })
    .backgroundColor(this.remoteLoginWaiting ? 'rgba(84,64,22,0.88)' : 'rgba(18,22,30,0.82)')
    .border({
      width: 1,
      color: this.remoteLoginWaiting ? 'rgba(255,190,92,0.42)' : 'rgba(255,255,255,0.18)'
    })
    .borderRadius(8)
  }

  build() {
    Column() {
      if (this.showSession) {
        this.buildSessionPage()
      } else if (this.showSettings) {
        SettingsPage({
          layoutMode: this.layoutMode,
          initialPageName: this.settingsInitialPageName,
          initialRemoteControlSection: this.settingsRemoteControlSection,
          remoteAccessCode: this.remoteAccessCode,
          remoteAccessCodeGateEnabled: this.remoteAccessCodeGateEnabled,
          screenRecordingPermissionGranted: this.screenRecordingPermissionGranted,
          screenRecordingPermissionBusy: this.screenRecordingPermissionBusy,
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
          connectionFeedbackText: this.connectionFeedbackText,
          connectionFeedbackTone: this.connectionFeedbackTone,
          isDark: this.isAppDark(),
          xrdpServerBusy: this.xrdpServerBusy,
          xrdpServerState: this.xrdpServerState,
          xrdpServerRunning: this.xrdpServerRunning,
          xrdpServerPort: this.xrdpServerPort,
          screenRecordingPermissionGranted: this.screenRecordingPermissionGranted,
          screenRecordingPermissionBusy: this.screenRecordingPermissionBusy,
          remoteAccessCodeGateEnabled: this.remoteAccessCodeGateEnabled,
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
