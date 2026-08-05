import { RdpLogger } from './RdpLogger'
import { RdpPermissionManager } from './RdpPermissions'
import {
  XrdpServerController,
  XrdpServerDisplayMessages,
  XrdpServerStartOptions,
  XrdpServerStatus
} from './XrdpServerController'

export interface RemoteControlSnapshot {
  accessCode: string
  accessCodeGateEnabled: boolean
  screenRecordingPermissionGranted: boolean
  screenRecordingPermissionBusy: boolean
  serverRunning: boolean
  serverState: string
  serverPort: number
  serverMessage: string
  serverBusy: boolean
}

export class RemoteControlCoordinator {
  private readonly available: boolean
  private readonly permissionManager: () => RdpPermissionManager
  private readonly appFilesDir: () => string
  private readonly onChange: (snapshot: RemoteControlSnapshot) => void
  private readonly serverController: XrdpServerController
  private startRequest: Promise<XrdpServerStatus> | null = null
  private accessCode: string = '000000'
  private accessCodeGateEnabled: boolean = false
  private accessCodeInitialized: boolean = false
  private permissionGranted: boolean = false
  private permissionBusy: boolean = false
  private serverRunning: boolean = false
  private serverState: string = 'Stopped'
  private serverPort: number = 3390
  private serverMessage: string = ''
  private serverBusy: boolean = false

  constructor(available: boolean, messages: XrdpServerDisplayMessages,
    permissionManager: () => RdpPermissionManager, appFilesDir: () => string,
    onChange: (snapshot: RemoteControlSnapshot) => void) {
    this.available = available
    this.permissionManager = permissionManager
    this.appFilesDir = appFilesDir
    this.onChange = onChange
    this.serverController = new XrdpServerController(messages, available)
  }

  snapshot(): RemoteControlSnapshot {
    return {
      accessCode: this.accessCode,
      accessCodeGateEnabled: this.accessCodeGateEnabled,
      screenRecordingPermissionGranted: this.permissionGranted,
      screenRecordingPermissionBusy: this.permissionBusy,
      serverRunning: this.serverRunning,
      serverState: this.serverState,
      serverPort: this.serverPort,
      serverMessage: this.serverMessage,
      serverBusy: this.serverBusy
    }
  }

  permissionPromptOpened(): void {
    RdpLogger.info('screen recording permission prompt opened')
    this.setPermissionBusy(true)
  }

  refreshPermissionState(): boolean {
    if (!this.available) {
      this.permissionGranted = false
      this.permissionBusy = false
      this.publish()
      return false
    }
    this.permissionGranted = this.hasPermission()
    this.publish()
    return this.permissionGranted
  }

  refreshDiagnostics(): XrdpServerStatus {
    return this.applyStatus(this.serverController.diagnostics())
  }

  refreshReadiness(clearBusy: boolean = false): XrdpServerStatus {
    if (clearBusy) {
      this.clearServerBusy('remote control readiness refreshed')
    }
    this.refreshPermissionState()
    return this.refreshDiagnostics()
  }

  requestPermissionFromSettings(): Promise<boolean> {
    if (!this.available) {
      return Promise.resolve(false)
    }
    return this.permissionManager().requestScreenRecordingPermission('remote control settings')
      .then((granted: boolean): Promise<boolean> | boolean => {
        const allowed = this.applyPermissionResult(granted, !granted)
        if (!allowed) {
          return false
        }
        return this.ensureStarted('screen recording permission granted')
          .then((_status: XrdpServerStatus): boolean => this.applyPermissionResult(true))
      })
      .catch((error: Error): boolean => {
        RdpLogger.error(`screen recording permission request from settings failed: ${JSON.stringify(error)}`)
        return this.applyPermissionResult(false, true)
      })
      .finally((): void => this.setPermissionBusy(false))
  }

  startFromSettings(): Promise<XrdpServerStatus> {
    return this.ensureStarted('remote control settings button', false, true)
  }

  setAccessCodeGate(enabled: boolean): string {
    if (!this.available || this.accessCodeGateEnabled === enabled) {
      return this.accessCode
    }
    this.accessCodeGateEnabled = enabled
    if (enabled) {
      this.ensureAccessCode()
    }
    this.publish()
    this.ensureStarted(enabled ? 'access code gate enabled' : 'access code gate disabled', true)
    return this.accessCode
  }

  regenerateAccessCode(): string {
    this.accessCode = RemoteControlCoordinator.generateAccessCode()
    this.accessCodeInitialized = true
    this.publish()
    if (this.accessCodeGateEnabled) {
      this.ensureStarted('access code regenerated', true)
    }
    return this.accessCode
  }

  ensureStarted(trigger: string, restartIfRunning: boolean = false,
    requestPermissionImmediately: boolean = false): Promise<XrdpServerStatus> {
    if (!this.available) {
      return Promise.resolve(this.refreshDiagnostics())
    }
    if (this.startRequest !== null && !restartIfRunning) {
      if (requestPermissionImmediately) {
        this.requestPermissionForRunningStart(trigger)
      }
      return this.startRequest
    }
    this.serverBusy = this.hasPermission()
    this.publish()
    const permissionRequest = requestPermissionImmediately ?
      this.permissionManager().requestScreenRecordingPermission(`xrdp ${trigger}`) :
      this.permissionManager().ensureScreenRecordingPermission(`xrdp ${trigger}`)
    this.startRequest = permissionRequest
      .then((granted: boolean): XrdpServerStatus => {
        const allowed = this.applyPermissionResult(granted, !granted)
        if (!allowed) {
          RdpLogger.warn(`xrdp start skipped: screen recording permission denied: ${trigger}`)
          return this.refreshReadiness(true)
        }
        return this.startAfterPermissions(trigger, restartIfRunning)
      })
      .catch((error: Error): XrdpServerStatus => {
        RdpLogger.error(`xrdp permission check failed: ${JSON.stringify(error)}`)
        this.applyPermissionResult(false, true)
        return this.refreshDiagnostics()
      })
      .finally((): void => {
        this.startRequest = null
        this.clearServerBusy('xrdp start request finished')
        this.setPermissionBusy(false)
        this.refreshReadiness()
      })
    return this.startRequest
  }

  private requestPermissionForRunningStart(trigger: string): void {
    this.permissionManager().requestScreenRecordingPermission(`xrdp ${trigger}`)
      .then((granted: boolean): void => {
        this.applyPermissionResult(granted, !granted)
      })
      .catch((error: Error): void => {
        RdpLogger.error(`xrdp immediate permission request failed: ${JSON.stringify(error)}`)
        this.applyPermissionResult(false, true)
      })
      .finally((): void => this.setPermissionBusy(false))
  }

  private hasPermission(): boolean {
    return this.available && this.permissionManager().hasScreenRecordingPermission()
  }

  private applyPermissionResult(granted: boolean, clearBusy: boolean = false): boolean {
    const allowed = granted || this.hasPermission()
    RdpLogger.info(`screen recording permission result applied: granted=${granted} allowed=${allowed}`)
    this.permissionGranted = allowed
    if (clearBusy || !allowed) {
      this.clearServerBusy('screen recording permission not granted')
    }
    this.refreshDiagnostics()
    return allowed
  }

  private startAfterPermissions(trigger: string, restartIfRunning: boolean): XrdpServerStatus {
    return this.applyStatus(this.serverController.start(this.startOptions(restartIfRunning), trigger))
  }

  private startOptions(restartIfRunning: boolean): XrdpServerStartOptions {
    return {
      appFilesDir: this.appFilesDir(),
      accessCode: this.accessCodeGateEnabled ? this.ensureAccessCode() : '',
      accessCodeGateEnabled: this.accessCodeGateEnabled,
      restartIfRunning: restartIfRunning
    }
  }

  private ensureAccessCode(): string {
    if (!this.accessCodeInitialized || this.accessCode === '000000') {
      this.accessCode = RemoteControlCoordinator.generateAccessCode()
      this.accessCodeInitialized = true
      this.publish()
    }
    return this.accessCode
  }

  private applyStatus(status: XrdpServerStatus): XrdpServerStatus {
    this.serverRunning = status.running
    this.serverState = status.state
    this.serverPort = status.port
    this.serverMessage = status.message
    this.publish()
    return status
  }

  private setPermissionBusy(busy: boolean): void {
    this.permissionBusy = busy
    if (busy) {
      this.serverBusy = true
    }
    this.publish()
  }

  private clearServerBusy(reason: string): void {
    if (this.serverBusy) {
      RdpLogger.info(`xrdp busy cleared: ${reason}`)
    }
    this.serverBusy = false
    this.publish()
  }

  private publish(): void {
    this.onChange(this.snapshot())
  }

  private static generateAccessCode(): string {
    return `${Math.floor(100000 + Math.random() * 900000)}`
  }
}
