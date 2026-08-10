import { common } from '@kit.AbilityKit'
import { SettingsRemoteText } from '../components/settings/SettingsConstants'
import { RdpDiagnosticsExporter } from './RdpDiagnosticsExporter'
import { RdpSessionDiagnosticSnapshot } from './RdpSessionLifecycleCoordinator'
import { RdpLogger } from './RdpLogger'
import { RdpPermissionRequestCoordinator } from './RdpPermissionRequestCoordinator'
import { RdpPermissionCallbackDelegate, RdpPermissionManager } from './RdpPermissions'
import {
  RemoteControlCoordinator,
  RemoteControlSnapshot,
  XrdpServerDisplayMessages,
  XrdpServerStatus
} from './RemoteControlCoordinator'
import { RemoteControlPort } from './RemoteControlPort'
import { RemoteFilesDirectory } from './RemoteFilesDirectory'

export class RemoteControlPageCoordinator {
  private readonly remoteControlPort: RemoteControlPort
  private readonly deviceAllowsServer: boolean
  private readonly contextProvider: () => common.UIAbilityContext | null
  private readonly defer: (task: () => void) => void
  private readonly onSnapshot: (snapshot: RemoteControlSnapshot) => void
  private permissionManager: RdpPermissionManager | null = null
  private permissionRequestCoordinator: RdpPermissionRequestCoordinator | null = null
  private coordinator: RemoteControlCoordinator | null = null

  constructor(remoteControlPort: RemoteControlPort, deviceAllowsServer: boolean,
    contextProvider: () => common.UIAbilityContext | null, defer: (task: () => void) => void,
    onSnapshot: (snapshot: RemoteControlSnapshot) => void) {
    this.remoteControlPort = remoteControlPort
    this.deviceAllowsServer = deviceAllowsServer
    this.contextProvider = contextProvider
    this.defer = defer
    this.onSnapshot = onSnapshot
  }

  available(): boolean {
    return this.deviceAllowsServer && this.remoteControlPort.available
  }

  private messages(): XrdpServerDisplayMessages {
    return {
      active: SettingsRemoteText.SERVER_MESSAGE_ACTIVE,
      listening: SettingsRemoteText.SERVER_MESSAGE_LISTENING,
      stopped: SettingsRemoteText.SERVER_MESSAGE_STOPPED,
      exited: SettingsRemoteText.SERVER_MESSAGE_EXITED,
      failed: SettingsRemoteText.SERVER_MESSAGE_FAILED,
      unavailable: SettingsRemoteText.SERVER_MESSAGE_UNAVAILABLE
    }
  }

  private getPermissionManager(): RdpPermissionManager {
    if (this.permissionManager === null) {
      const delegate = new RdpPermissionCallbackDelegate(this.contextProvider)
      this.permissionManager = new RdpPermissionManager(delegate, (): void => {
        this.getCoordinator().permissionPromptOpened()
      })
    }
    return this.permissionManager
  }

  private getPermissionRequestCoordinator(): RdpPermissionRequestCoordinator {
    if (this.permissionRequestCoordinator === null) {
      this.permissionRequestCoordinator = new RdpPermissionRequestCoordinator(this.getPermissionManager(), this.defer)
    }
    return this.permissionRequestCoordinator
  }

  private getCoordinator(): RemoteControlCoordinator {
    if (this.coordinator === null) {
      this.coordinator = new RemoteControlCoordinator(
        this.remoteControlPort,
        this.messages(),
        (): RdpPermissionManager => this.getPermissionManager(),
        (): string => this.appFilesDir(),
        (snapshot: RemoteControlSnapshot): void => {
          this.defer((): void => this.onSnapshot(snapshot))
        }
      )
      this.onSnapshot(this.coordinator.snapshot())
    }
    return this.coordinator
  }

  private appFilesDir(): string {
    const context = this.contextProvider()
    return context === null ? '' : context.filesDir
  }

  registerPermissionCallbacks(): void {
    this.getPermissionRequestCoordinator().register()
  }

  refreshScreenRecordingPermission(): boolean {
    return this.getCoordinator().refreshPermissionState()
  }

  requestScreenRecordingPermission(): Promise<boolean> {
    return this.getCoordinator().requestPermissionFromSettings()
  }

  refreshInputInjectionPermission(): boolean {
    return this.getCoordinator().refreshInputInjectionPermissionState()
  }

  requestInputInjectionPermission(): Promise<boolean> {
    return this.getCoordinator().requestInputInjectionPermissionFromSettings()
  }

  refreshDiagnostics(): XrdpServerStatus {
    return this.getCoordinator().refreshDiagnostics()
  }

  refreshReadiness(clearBusy: boolean = false): XrdpServerStatus {
    return this.getCoordinator().refreshReadiness(clearBusy)
  }

  startFromSettings(): Promise<XrdpServerStatus> {
    return this.getCoordinator().startFromSettings()
  }

  stopFromSettings(): Promise<XrdpServerStatus> {
    return this.getCoordinator().stopFromSettings()
  }

  ensureStarted(trigger: string, restartIfRunning: boolean = false,
    requestScreenRecordingImmediately: boolean = false): Promise<XrdpServerStatus> {
    return this.getCoordinator().ensureStarted(trigger, restartIfRunning, requestScreenRecordingImmediately)
  }

  setAccessCodeGate(enabled: boolean): string {
    return this.getCoordinator().setAccessCodeGate(enabled)
  }

  regenerateAccessCode(): string {
    return this.getCoordinator().regenerateAccessCode()
  }

  openRemoteFilesDirectory(): void {
    RemoteFilesDirectory.openSharedDirectory(this.contextProvider()).catch((error: Error): void => {
      RdpLogger.error(`remote files directory open failed: ${JSON.stringify(error)}`)
    })
  }

  exportDiagnostics(hostLength: number, usernameLength: number, port: string,
    session: RdpSessionDiagnosticSnapshot): Promise<string> {
    const xrdpStatus: XrdpServerStatus = this.available() ? this.refreshDiagnostics() : {
      running: false,
      state: 'Unavailable',
      port: 3390,
      message: SettingsRemoteText.SERVER_MESSAGE_UNAVAILABLE,
      pid: 0,
      lastExitCode: 0
    }
    return RdpDiagnosticsExporter.capture(this.contextProvider(), hostLength, usernameLength, port,
      this.available(), this.getPermissionManager(), xrdpStatus, session).then((result): string => result.message)
  }
}
