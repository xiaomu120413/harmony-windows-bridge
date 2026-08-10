import { HomeText } from '../components/home/HomeText'
import { SettingsStatusTone } from '../components/settings/SettingsUi'
import { RdpClientController } from './RdpClientController'
import {
  RdpConnectionAttemptCoordinator,
  RdpConnectionAttemptOutcome
} from './RdpConnectionAttemptCoordinator'
import { NativeConnectParams } from './NativeRdpGateway'
import { RdpLogger } from './RdpLogger'
import { RdpSessionNotice, RdpSessionPresentation } from './RdpSessionPresentation'
import { WindowsConnectionSaveInput } from './WindowsConnectionStore'

export interface RdpSessionLifecycleDelegate {
  defer: (task: () => void) => void
  setVisible: (visible: boolean) => void
  setNotice: (notice: RdpSessionNotice) => void
  setFeedback: (text: string, tone: SettingsStatusTone) => void
  releaseInput: () => void
  focusSurface: () => void
  persistProfile: (input: WindowsConnectionSaveInput) => void
  registerPermissionCallbacks: () => void
}

export type RdpDiagnosticErrorCategory = 'none' | 'network' | 'credentials' | 'certificate' | 'session' | 'unknown'

export interface RdpSessionDiagnosticSnapshot {
  stage: string
  endpointProbeState: string
  endpointProbeElapsedMs: number
  lastErrorCategory: RdpDiagnosticErrorCategory
}

export class RdpSessionLifecycleCoordinator {
  private readonly client: RdpClientController
  private readonly attempt: RdpConnectionAttemptCoordinator
  private readonly delegate: RdpSessionLifecycleDelegate
  private callbacksRegistered: boolean = false
  private pendingProfile: WindowsConnectionSaveInput | null = null
  private lastError: string = ''
  private currentStage: string = 'Idle'
  private endpointProbeState: string = 'NotRun'
  private endpointProbeElapsedMs: number = 0
  private lastErrorCategory: RdpDiagnosticErrorCategory = 'none'

  constructor(client: RdpClientController, delegate: RdpSessionLifecycleDelegate) {
    this.client = client
    this.attempt = new RdpConnectionAttemptCoordinator(client)
    this.delegate = delegate
  }

  cancel(): void {
    this.attempt.cancel()
  }

  diagnostics(): RdpSessionDiagnosticSnapshot {
    return {
      stage: this.currentStage,
      endpointProbeState: this.endpointProbeState,
      endpointProbeElapsedMs: this.endpointProbeElapsedMs,
      lastErrorCategory: this.lastErrorCategory
    }
  }

  private classifyError(message: string): RdpDiagnosticErrorCategory {
    const value = message.toLowerCase()
    if (value.length === 0) {
      return 'none'
    }
    if (value.includes('credential') || value.includes('auth') || value.includes('logon') ||
      value.includes('password') || value.includes('username')) {
      return 'credentials'
    }
    if (value.includes('certificate') || value.includes('tls') || value.includes('cert')) {
      return 'certificate'
    }
    if (value.includes('network') || value.includes('connect') || value.includes('timeout') ||
      value.includes('refused') || value.includes('resolve') || value.includes('socket') ||
      value.includes('host') || value.includes('port')) {
      return 'network'
    }
    if (value.includes('session') || value.includes('disconnect') || value.includes('channel')) {
      return 'session'
    }
    return 'unknown'
  }

  private clearNotice(): void {
    this.delegate.setNotice(RdpSessionPresentation.notice('Idle'))
  }

  private persistPendingProfile(): void {
    if (this.pendingProfile === null) {
      return
    }
    const profile = this.pendingProfile
    this.pendingProfile = null
    this.delegate.persistProfile(profile)
  }

  registerCallbacks(): void {
    if (this.callbacksRegistered) {
      return
    }
    try {
      const result = this.client.registerCallbacks({
        onState: (state: string, wasConnected: boolean): void => {
          this.delegate.defer((): void => this.onNativeState(state, wasConnected))
        },
        onError: (message: string, wasConnected: boolean): void => {
          this.delegate.defer((): void => this.onNativeError(message, wasConnected))
        }
      })
      if (!result.ok) {
        RdpLogger.error(`Native callback registration failed: ${result.message}`)
        return
      }
      this.delegate.registerPermissionCallbacks()
      this.callbacksRegistered = true
    } catch (error) {
      RdpLogger.error(`Native callback registration failed: ${JSON.stringify(error)}`)
    }
  }

  private onNativeState(state: string, wasConnected: boolean): void {
    this.currentStage = state
    const connected = RdpClientController.isConnectedState(state)
    if (!connected && (state === 'Disconnected' || state === 'Failed' || state === 'Idle')) {
      if (wasConnected) {
        this.delegate.releaseInput()
      }
      this.delegate.setVisible(false)
    }
    this.delegate.setNotice(RdpSessionPresentation.notice(state))
    if (connected && !wasConnected) {
      this.delegate.setVisible(true)
      this.delegate.focusSurface()
      this.lastError = ''
      this.lastErrorCategory = 'none'
      this.delegate.setFeedback(HomeText.CONNECTION_SUCCESS, 'ok')
      this.persistPendingProfile()
    }
    if (state === 'Failed') {
      this.pendingProfile = null
      this.delegate.setFeedback(RdpSessionPresentation.failureMessage(this.lastError,
        HomeText.CONNECTION_FAILURE_CREDENTIALS), 'danger')
      RdpLogger.error(`Native state: ${state}`)
    } else if ((state === 'Disconnected' || state === 'Idle') && this.pendingProfile !== null) {
      this.pendingProfile = null
      this.delegate.setFeedback(RdpSessionPresentation.failureMessage(this.lastError,
        HomeText.CONNECTION_FAILURE_NO_SESSION), 'danger')
    } else if (state === 'Connected' || state === 'Disconnected' ||
      state === 'RemoteLoginWaiting' || state === 'RemoteDesktopReady') {
      RdpLogger.info(`Native state: ${state}`)
    }
  }

  private onNativeError(message: string, wasConnected: boolean): void {
    if (wasConnected) {
      this.delegate.releaseInput()
    }
    this.delegate.setVisible(false)
    this.clearNotice()
    this.pendingProfile = null
    this.lastError = message
    this.lastErrorCategory = this.classifyError(message)
    this.delegate.setFeedback(RdpSessionPresentation.failureMessage(message, HomeText.CONNECTION_FAILURE), 'danger')
    RdpLogger.error(`Native error: ${message}`)
  }

  connect(input: NativeConnectParams, profile: WindowsConnectionSaveInput): void {
    this.pendingProfile = profile
    this.lastError = ''
    this.lastErrorCategory = 'none'
    this.currentStage = 'EndpointChecking'
    this.endpointProbeState = 'Checking'
    this.endpointProbeElapsedMs = 0
    this.delegate.setFeedback(HomeText.CONNECTION_CHECKING, 'info')
    this.attempt.begin(input, (): void => {
      this.currentStage = 'Connecting'
      this.delegate.setFeedback(HomeText.CONNECTION_PORT_REACHABLE, 'info')
      this.delegate.setNotice(RdpSessionPresentation.notice('Connecting'))
      this.delegate.setVisible(true)
    }).then((outcome: RdpConnectionAttemptOutcome | null): void => this.applyAttempt(outcome))
  }

  rejectValidation(message: string): void {
    this.client.beginConnect()
    this.pendingProfile = null
    this.currentStage = 'ValidationRejected'
    this.lastErrorCategory = this.classifyError(message)
    this.delegate.setFeedback(message, 'warning')
    RdpLogger.warn(`Connection validation failed: ${message}`)
  }

  private applyAttempt(outcome: RdpConnectionAttemptOutcome | null): void {
    if (outcome === null) {
      return
    }
    if (outcome.probe.state !== 'Reachable') {
      this.currentStage = 'EndpointFailed'
      this.endpointProbeState = outcome.probe.state
      this.endpointProbeElapsedMs = outcome.probe.elapsedMs
      this.pendingProfile = null
      this.lastError = outcome.probe.message
      this.lastErrorCategory = 'network'
      this.delegate.setFeedback(outcome.probe.message, 'danger')
      RdpLogger.warn(`RDP endpoint probe failed: elapsedMs=${outcome.probe.elapsedMs}`)
      return
    }
    this.endpointProbeState = outcome.probe.state
    this.endpointProbeElapsedMs = outcome.probe.elapsedMs
    RdpLogger.info(`RDP endpoint probe succeeded: elapsedMs=${outcome.probe.elapsedMs}`)
    if (outcome.nativeResult !== null && outcome.nativeResult.ok) {
      this.delegate.setVisible(true)
      if (this.client.isConnected()) {
        this.lastError = ''
        this.lastErrorCategory = 'none'
        this.delegate.setFeedback(HomeText.CONNECTION_SUCCESS, 'ok')
        this.persistPendingProfile()
      }
      return
    }
    this.delegate.setVisible(false)
    this.clearNotice()
    this.pendingProfile = null
    this.lastError = outcome.nativeResult === null ? outcome.errorMessage : outcome.nativeResult.message
    this.lastErrorCategory = this.classifyError(this.lastError)
    this.delegate.setFeedback(RdpSessionPresentation.failureMessage(this.lastError,
      HomeText.CONNECTION_FAILURE_CREDENTIALS), 'danger')
    RdpLogger.error(`Native connect failed: ${this.lastError}`)
  }
}
