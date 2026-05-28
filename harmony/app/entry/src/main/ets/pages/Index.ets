import { common } from '@kit.AbilityKit'
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
  WindowsConnectionSnapshot,
  WindowsConnectionStore
} from '../rdp/WindowsConnectionStore'
import { HomePage } from '../components/home/HomePage'
import { SettingsPage } from '../components/SettingsPage'
import { SettingsRoute, SettingsText } from '../components/settings/SettingsConstants'
import { SettingsAppearanceMode, SettingsTheme } from '../components/settings/SettingsUi'
import rdpNative, {
  NativeCommandResult
} from 'libentry.so'

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
  @StorageLink('settingsSystemDark') private systemDark: boolean = false
  @StorageLink('settingsAppearanceMode') private appearanceMode: string = 'system'
  private connected: boolean = false
  private callbacksRegistered: boolean = false
  private xrdpServerStartRequested: boolean = false
  private remoteAccessCodeInitialized: boolean = false
  private surfaceController: XComponentController = new XComponentController()
  private permissionManager: RdpPermissionManager | null = null
  private xrdpServerController: XrdpServerController | null = null
  private xrdpServerStartRequest: Promise<XrdpServerStatus> | null = null
  private windowsConnectionStore: WindowsConnectionStore | null = null
  private connectionProfilesLoaded: boolean = false
  private connectionProfileSaveTimer: number | null = null

  private deferUiUpdate(task: () => void): void {
    setTimeout((): void => {
      task()
    }, 0)
  }

  aboutToAppear(): void {
    this.registerNativeCallbacks()
    this.loadConnectionProfiles()
    this.refreshScreenRecordingPermissionState()
    this.refreshXrdpServerDiagnostics()
    this.ensureXrdpServerStarted('page appear')
  }

  onPageShow(): void {
    if (!this.connectionProfilesLoaded) {
      this.loadConnectionProfiles()
    }
    this.refreshScreenRecordingPermissionState()
    this.refreshXrdpServerDiagnostics()
  }

  onPageHide(): void {
    this.flushConnectionProfileSave()
  }

  aboutToDisappear(): void {
    this.flushConnectionProfileSave()
    this.releaseActiveModifiers()
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

  private getPermissionManager(): RdpPermissionManager {
    if (this.permissionManager === null) {
      const delegate = new RdpPermissionCallbackDelegate(
        (): common.UIAbilityContext | null => this.getUiAbilityContext()
      )
      this.permissionManager = new RdpPermissionManager(delegate)
    }
    return this.permissionManager
  }

  private getXrdpServerController(): XrdpServerController {
    if (this.xrdpServerController === null) {
      this.xrdpServerController = new XrdpServerController(
        this.xrdpServerDisplayMessages()
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
    this.clearConnectionProfileSaveTimer()
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
    this.clearConnectionProfileSaveTimer()
    this.selectedConnectionProfileId = ''
    this.host = ''
    this.port = '3389'
    this.username = ''
    this.password = ''
    this.rememberConnectionPassword = false
    this.certPolicy = 'tofu'
  }

  private deleteSelectedConnectionProfile(): void {
    this.clearConnectionProfileSaveTimer()
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
    this.scheduleConnectionProfileSave()
  }

  private scheduleConnectionProfileSave(): void {
    if (this.host.trim().length === 0) {
      return
    }
    this.clearConnectionProfileSaveTimer()
    this.connectionProfileSaveTimer = setTimeout((): void => {
      this.connectionProfileSaveTimer = null
      this.saveCurrentConnectionProfile()
    }, 450)
  }

  private clearConnectionProfileSaveTimer(): void {
    if (this.connectionProfileSaveTimer !== null) {
      clearTimeout(this.connectionProfileSaveTimer)
      this.connectionProfileSaveTimer = null
    }
  }

  private flushConnectionProfileSave(): void {
    if (this.host.trim().length === 0) {
      this.clearConnectionProfileSaveTimer()
      return
    }
    this.clearConnectionProfileSaveTimer()
    this.saveCurrentConnectionProfile()
  }

  private saveCurrentConnectionProfile(): void {
    const store = this.getWindowsConnectionStore()
    if (store === null) {
      return
    }

    store.saveProfile({
      profileId: this.selectedConnectionProfileId,
      host: this.host,
      port: this.port,
      username: this.username,
      password: this.password,
      certPolicy: this.certPolicy,
      rememberPassword: this.rememberConnectionPassword
    })
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

  private xrdpServerDisplayMessages(): XrdpServerDisplayMessages {
    return {
      active: SettingsText.REMOTE_SERVER_MESSAGE_ACTIVE,
      listening: SettingsText.REMOTE_SERVER_MESSAGE_LISTENING,
      stopped: SettingsText.REMOTE_SERVER_MESSAGE_STOPPED,
      exited: SettingsText.REMOTE_SERVER_MESSAGE_EXITED,
      failed: SettingsText.REMOTE_SERVER_MESSAGE_FAILED
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
          }
          if (state === 'Failed') {
            RdpLogger.error(`Native state: ${state}`)
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
          RdpLogger.error(`Native error: ${message}`)
        })
      })
      rdpNative.onMicrophonePermissionRequest((requestIdText: string) => {
        this.deferUiUpdate(() => {
          this.handleNativePermissionRequest(
            requestIdText,
            'Remote audio capture requested microphone permission',
            'remote audio capture',
            (trigger: string): Promise<boolean> => this.getPermissionManager().ensureMicrophonePermission(trigger),
            (requestId: number, granted: boolean): NativeCommandResult =>
              rdpNative.completeMicrophonePermissionRequest({ requestId: requestId, granted: granted })
          )
        })
      })
      rdpNative.onClipboardPermissionRequest((requestIdText: string) => {
        this.deferUiUpdate(() => {
          this.handleNativePermissionRequest(
            requestIdText,
            'Clipboard sync requested pasteboard permission',
            'clipboard read',
            (trigger: string): Promise<boolean> => this.getPermissionManager().ensurePasteboardReadPermission(trigger),
            (requestId: number, granted: boolean): NativeCommandResult =>
              rdpNative.completeClipboardPermissionRequest({ requestId: requestId, granted: granted })
          )
        })
      })
      rdpNative.onLocationPermissionRequest((requestIdText: string) => {
        this.deferUiUpdate(() => {
          this.handleNativePermissionRequest(
            requestIdText,
            'RDP location channel requested location permission',
            'remote location redirection',
            (trigger: string): Promise<boolean> => this.getPermissionManager().ensureLocationPermission(trigger),
            (requestId: number, granted: boolean): NativeCommandResult =>
              rdpNative.completeLocationPermissionRequest({ requestId: requestId, granted: granted })
          )
        })
      })
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
      this.sessionNoticeTitle = '已连接，等待远端桌面'
      this.sessionNoticeSubtitle = '远端桌面登录阶段暂未收到新画面'
      return
    }

    this.remoteLoginWaiting = false
    if (state === 'Connected') {
      this.sessionNoticeTitle = '已连接，正在打开远端桌面'
      this.sessionNoticeSubtitle = '等待远端桌面首帧'
      return
    }

    if (state === 'RemoteDesktopReady' || state === 'Disconnected' || state === 'Failed' || state === 'Idle') {
      this.sessionNoticeTitle = ''
      this.sessionNoticeSubtitle = ''
      return
    }

    if (state === 'Resolving') {
      this.sessionNoticeTitle = '正在解析主机'
      this.sessionNoticeSubtitle = '准备建立 RDP 连接'
    } else if (state === 'TCP connected') {
      this.sessionNoticeTitle = '已连通主机'
      this.sessionNoticeSubtitle = '正在协商远程桌面会话'
    } else if (state === 'Negotiating') {
      this.sessionNoticeTitle = '正在协商远程桌面'
      this.sessionNoticeSubtitle = '等待服务端返回图形能力'
    } else if (state === 'Authenticating') {
      this.sessionNoticeTitle = '正在验证登录信息'
      this.sessionNoticeSubtitle = '即将进入远端桌面'
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
    this.refreshScreenRecordingPermissionState()
    this.refreshXrdpServerDiagnostics()
    this.settingsRemoteControlSection = remoteControlSection
    this.settingsInitialPageName = remoteControlSection.length > 0 ? SettingsRoute.REMOTE_CONTROL : SettingsRoute.SETTINGS
    this.showSettings = true
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
    return this.getPermissionManager().hasScreenRecordingPermission()
  }

  private refreshScreenRecordingPermissionState(): boolean {
    this.screenRecordingPermissionGranted = this.hasScreenRecordingPermission()
    return this.screenRecordingPermissionGranted
  }

  private requestScreenRecordingPermissionFromSettings(): Promise<boolean> {
    return this.getPermissionManager().requestScreenRecordingPermission('remote control settings')
      .then((granted: boolean) => {
        const allowed = granted || this.refreshScreenRecordingPermissionState()
        this.screenRecordingPermissionGranted = allowed
        if (!allowed) {
          return false
        }
        return this.ensureXrdpServerStarted('screen recording permission granted')
          .then((_status: XrdpServerStatus) => {
            return this.refreshScreenRecordingPermissionState()
          })
      })
      .catch((error: Error) => {
        RdpLogger.error(`screen recording permission request from settings failed: ${JSON.stringify(error)}`)
        return this.refreshScreenRecordingPermissionState()
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
    if (this.xrdpServerStartRequest !== null && !restartIfRunning) {
      if (requestScreenRecordingImmediately) {
        this.getPermissionManager().requestScreenRecordingPermission(`xrdp ${trigger}`)
          .catch((error: Error) => {
            RdpLogger.error(`xrdp immediate permission request failed: ${JSON.stringify(error)}`)
          })
      }
      return this.xrdpServerStartRequest
    }
    if (this.xrdpServerStartRequested && !restartIfRunning) {
      return Promise.resolve(this.currentXrdpServerStatus())
    }
    this.xrdpServerStartRequested = true
    this.xrdpServerBusy = true
    const screenRecordingPermissionRequest = requestScreenRecordingImmediately ?
      this.getPermissionManager().requestScreenRecordingPermission(`xrdp ${trigger}`) :
      this.getPermissionManager().ensureScreenRecordingPermission(`xrdp ${trigger}`)
    this.xrdpServerStartRequest = screenRecordingPermissionRequest
      .then((granted: boolean) => {
        const allowed = granted || this.refreshScreenRecordingPermissionState()
        this.screenRecordingPermissionGranted = allowed
        if (!allowed) {
          RdpLogger.warn(`xrdp start skipped: screen recording permission denied: ${trigger}`)
          return this.refreshXrdpServerDiagnostics()
        }
        return this.startXrdpServerAfterPermissions(trigger, restartIfRunning)
      })
      .catch((error: Error) => {
        RdpLogger.error(`xrdp permission check failed: ${JSON.stringify(error)}`)
        return this.refreshXrdpServerDiagnostics()
      })
      .finally(() => {
        this.xrdpServerStartRequested = false
        this.xrdpServerBusy = false
        this.xrdpServerStartRequest = null
      })
    return this.xrdpServerStartRequest
  }

  private startXrdpServerAfterPermissions(trigger: string, restartIfRunning: boolean): XrdpServerStatus {
    const status = this.getXrdpServerController().start(this.xrdpServerStartOptions(restartIfRunning), trigger)
    return this.applyXrdpServerStatus(status)
  }

  private setRemoteAccessCodeGateEnabled(enabled: boolean): void {
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
    this.flushConnectionProfileSave()
    if (!this.host.trim() || !this.username.trim() || !this.password.trim()) {
      this.connected = false
      RdpLogger.warn('Missing host, username, or password')
      return
    }

    const policy = this.certPolicy
    const host = this.host
    const port = this.port
    const username = this.username
    const password = this.password
    const appFilesDir = this.getAppFilesDir()
    this.connected = false
    this.remoteLoginWaiting = false
    this.sessionNoticeTitle = '正在连接远程桌面'
    this.sessionNoticeSubtitle = '正在建立 RDP 会话'
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
        this.connected = result.state === 'Connected'
        this.setSessionVisible(true)
      } else {
        RdpLogger.error(result.message)
        this.setSessionVisible(false)
        this.connected = false
        this.clearSessionNotice()
      }
    } catch (error) {
      this.setSessionVisible(false)
      this.connected = false
      this.clearSessionNotice()
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
          initialPageName: this.settingsInitialPageName,
          initialRemoteControlSection: this.settingsRemoteControlSection,
          remoteAccessCode: this.remoteAccessCode,
          remoteAccessCodeGateEnabled: this.remoteAccessCodeGateEnabled,
          screenRecordingPermissionGranted: this.screenRecordingPermissionGranted,
          xrdpServerRunning: this.xrdpServerRunning,
          xrdpServerState: this.xrdpServerState,
          xrdpServerPort: this.xrdpServerPort,
          xrdpServerMessage: this.xrdpServerMessage,
          xrdpServerBusy: this.xrdpServerBusy,
          onRemoteAccessCodeGateChange: (enabled: boolean): string => this.setRemoteAccessCodeGateFromSettings(enabled),
          onRemoteAccessCodeRegenerate: (): string => this.regenerateRemoteAccessCodeFromSettings(),
          onRequestScreenRecordingPermission: (): Promise<boolean> => this.requestScreenRecordingPermissionFromSettings(),
          onRefreshScreenRecordingPermission: (): boolean => this.refreshScreenRecordingPermissionState(),
          onRefreshXrdpServerStatus: (): XrdpServerStatus => this.refreshXrdpServerDiagnostics(),
          onStartXrdpServer: (): Promise<XrdpServerStatus> => this.startXrdpServerFromSettings(),
          onOpenRemoteFilesDirectory: (): void => this.openRemoteFilesDirectoryFromSettings(),
          onClose: () => {
            this.showSettings = false
            this.settingsInitialPageName = SettingsRoute.SETTINGS
            this.settingsRemoteControlSection = ''
          }
        })
      } else {
        HomePage({
          host: $host,
          port: $port,
          username: $username,
          password: $password,
          connectionProfiles: this.connectionProfiles,
          selectedConnectionProfileId: this.selectedConnectionProfileId,
          rememberPassword: $rememberConnectionPassword,
          isDark: this.isAppDark(),
          xrdpServerBusy: this.xrdpServerBusy,
          xrdpServerState: this.xrdpServerState,
          xrdpServerRunning: this.xrdpServerRunning,
          xrdpServerPort: this.xrdpServerPort,
          screenRecordingPermissionGranted: this.screenRecordingPermissionGranted,
          remoteAccessCodeGateEnabled: this.remoteAccessCodeGateEnabled,
          onHostChange: (value: string): void => {
            this.host = value
            this.scheduleConnectionProfileSave()
          },
          onPortChange: (value: string): void => {
            this.port = value
            this.scheduleConnectionProfileSave()
          },
          onUsernameChange: (value: string): void => {
            this.username = value
            this.scheduleConnectionProfileSave()
          },
          onPasswordChange: (value: string): void => {
            this.password = value
            this.scheduleConnectionProfileSave()
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
