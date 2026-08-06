import { RdpLogger } from './RdpLogger'
import { RdpPermissionManager } from './RdpPermissions'
import {
  RemoteControlPort,
  RemoteControlProcessParams,
  RemoteControlProcessResult
} from './RemoteControlPort'

export interface XrdpServerStatus {
  running: boolean
  state: string
  port: number
  message: string
}

export interface XrdpServerDisplayMessages {
  active: string
  listening: string
  stopped: string
  exited: string
  failed: string
  unavailable: string
}

export interface RemoteControlSnapshot {
  accessCode: string
  accessCodeGateEnabled: boolean
  screenRecordingPermissionGranted: boolean
  screenRecordingPermissionBusy: boolean
  inputInjectionPermissionGranted: boolean
  inputInjectionPermissionBusy: boolean
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
  private readonly remoteControlPort: RemoteControlPort
  private readonly messages: XrdpServerDisplayMessages
  private startRequest: Promise<XrdpServerStatus> | null = null
  private accessCode: string = '000000'
  private accessCodeGateEnabled: boolean = false
  private accessCodeInitialized: boolean = false
  private permissionGranted: boolean = false
  private permissionBusy: boolean = false
  private inputInjectionPermissionGranted: boolean = false
  private inputInjectionPermissionBusy: boolean = false
  private serverRunning: boolean = false
  private serverState: string = 'Stopped'
  private serverPort: number = 3390
  private serverMessage: string = ''
  private serverBusy: boolean = false

  constructor(remoteControlPort: RemoteControlPort, messages: XrdpServerDisplayMessages,
    permissionManager: () => RdpPermissionManager, appFilesDir: () => string,
    onChange: (snapshot: RemoteControlSnapshot) => void) {
    this.available = remoteControlPort.available
    this.remoteControlPort = remoteControlPort
    this.messages = messages
    this.permissionManager = permissionManager
    this.appFilesDir = appFilesDir
    this.onChange = onChange
  }

  snapshot(): RemoteControlSnapshot {
    return {
      accessCode: this.accessCode,
      accessCodeGateEnabled: this.accessCodeGateEnabled,
      screenRecordingPermissionGranted: this.permissionGranted,
      screenRecordingPermissionBusy: this.permissionBusy,
      inputInjectionPermissionGranted: this.inputInjectionPermissionGranted,
      inputInjectionPermissionBusy: this.inputInjectionPermissionBusy,
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

  refreshInputInjectionPermissionState(): boolean {
    if (!this.available) {
      this.inputInjectionPermissionGranted = false
      this.inputInjectionPermissionBusy = false
      this.publish()
      return false
    }
    this.inputInjectionPermissionGranted = this.permissionManager().hasControlDevicePermission()
    this.publish()
    return this.inputInjectionPermissionGranted
  }

  requestInputInjectionPermissionFromSettings(): Promise<boolean> {
    if (!this.available || this.inputInjectionPermissionBusy) {
      return Promise.resolve(false)
    }
    this.inputInjectionPermissionBusy = true
    this.publish()
    return this.permissionManager().requestControlDevicePermission('remote control settings')
      .then((granted: boolean): boolean => {
        this.inputInjectionPermissionGranted = granted ||
          this.permissionManager().hasControlDevicePermission()
        RdpLogger.info(`input injection permission result applied: allowed=${this.inputInjectionPermissionGranted}`)
        return this.inputInjectionPermissionGranted
      })
      .catch((error: Error): boolean => {
        RdpLogger.error(`input injection permission request failed: ${JSON.stringify(error)}`)
        this.inputInjectionPermissionGranted = this.permissionManager().hasControlDevicePermission()
        return this.inputInjectionPermissionGranted
      })
      .finally((): void => {
        this.inputInjectionPermissionBusy = false
        this.publish()
      })
  }

  refreshDiagnostics(): XrdpServerStatus {
    return this.applyStatus(this.toStatus(this.remoteControlPort.diagnostics()))
  }

  refreshReadiness(clearBusy: boolean = false): XrdpServerStatus {
    if (clearBusy) {
      this.clearServerBusy('remote control readiness refreshed')
    }
    this.refreshPermissionState()
    this.refreshInputInjectionPermissionState()
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
    RdpLogger.info(`xrdp process start requested: ${trigger}`)
    return this.applyStatus(this.toStatus(this.remoteControlPort.start(this.startOptions(restartIfRunning))))
  }

  private startOptions(restartIfRunning: boolean): RemoteControlProcessParams {
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

  private toStatus(result: RemoteControlProcessResult): XrdpServerStatus {
    return {
      running: result.ok && result.state !== 'Stopped' && result.state !== 'Exited',
      state: result.state,
      port: result.port > 0 ? result.port : 3390,
      message: this.displayMessage(result)
    }
  }

  private displayMessage(result: RemoteControlProcessResult): string {
    if (result.state === 'ActiveSession') {
      return this.messages.active
    }
    if (result.state === 'Listening') {
      return this.messages.listening
    }
    if (result.state === 'Stopped') {
      return this.messages.stopped
    }
    if (result.state === 'Exited') {
      return this.messages.exited
    }
    if (result.state === 'Unavailable') {
      return this.messages.unavailable
    }
    if (result.state === 'Failed') {
      return this.messages.failed
    }
    return result.message
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
