import { RdpLogger } from './RdpLogger'
import { RdpClientController } from './RdpClientController'
import {
  ConnectionValidationIssue,
  ConnectionValidationResult,
  RdpConnectionValidator
} from './RdpConnectionValidator'
import {
  NativeCommandResult
} from './NativeRdpGateway'
import { WindowsConnectionSaveInput } from './WindowsConnectionStore'
import { SettingsStatusTone } from '../components/settings/SettingsTypes'

export interface RdpSessionStateSnapshot {
  sessionVisible: boolean
  remoteLoginWaiting: boolean
  sessionNoticeTitle: string
  sessionNoticeSubtitle: string
  connectionFeedbackText: string
  connectionFeedbackTone: SettingsStatusTone
}

export interface SessionStateMessages {
  sessionConnectingTitle: string
  sessionConnectingSubtitle: string
  sessionConnectedTitle: string
  sessionConnectedSubtitle: string
  sessionRemoteLoginWaitingTitle: string
  sessionRemoteLoginWaitingSubtitle: string
  sessionResolvingTitle: string
  sessionResolvingSubtitle: string
  sessionTcpConnectedTitle: string
  sessionTcpConnectedSubtitle: string
  sessionNegotiatingTitle: string
  sessionNegotiatingSubtitle: string
  sessionAuthenticatingTitle: string
  sessionAuthenticatingSubtitle: string
  connectionSuccess: string
  connectionConnecting: string
  connectionFailure: string
  connectionFailureCredentials: string
  connectionFailureNoSession: string
  connectionFailureWithDetail: (detail: string) => string
  validationMessage: (issues: ConnectionValidationIssue[]) => string
}

export interface SessionStateConnectInput {
  host: string
  port: string
  username: string
  password: string
  certPolicy: string
  selectedProfileId: string
  rememberPassword: boolean
}

export class RdpSessionStateCoordinator {
  private readonly rdpClientController: RdpClientController
  private readonly messages: SessionStateMessages
  private readonly appFilesDir: () => string
  private readonly onPersistProfile: (input: WindowsConnectionSaveInput) => void
  private readonly onChange: (snapshot: RdpSessionStateSnapshot) => void
  private readonly releaseInput: () => void
  private callbacksRegistered: boolean = false
  private pendingConnectionProfileSave: WindowsConnectionSaveInput | null = null
  private lastConnectionErrorMessage: string = ''
  private sessionVisible: boolean = false
  private remoteLoginWaiting: boolean = false
  private sessionNoticeTitle: string = ''
  private sessionNoticeSubtitle: string = ''
  private connectionFeedbackText: string = ''
  private connectionFeedbackTone: SettingsStatusTone = 'neutral'

  constructor(rdpClientController: RdpClientController, messages: SessionStateMessages,
    appFilesDir: () => string, onPersistProfile: (input: WindowsConnectionSaveInput) => void,
    onChange: (snapshot: RdpSessionStateSnapshot) => void,
    releaseInput: () => void = (): void => {}) {
    this.rdpClientController = rdpClientController
    this.messages = messages
    this.appFilesDir = appFilesDir
    this.onPersistProfile = onPersistProfile
    this.onChange = onChange
    this.releaseInput = releaseInput
  }

  snapshot(): RdpSessionStateSnapshot {
    return {
      sessionVisible: this.sessionVisible,
      remoteLoginWaiting: this.remoteLoginWaiting,
      sessionNoticeTitle: this.sessionNoticeTitle,
      sessionNoticeSubtitle: this.sessionNoticeSubtitle,
      connectionFeedbackText: this.connectionFeedbackText,
      connectionFeedbackTone: this.connectionFeedbackTone
    }
  }

  register(): NativeCommandResult {
    if (this.callbacksRegistered) {
      return {
        ok: true,
        state: this.rdpClientController.isConnected() ? 'Connected' : 'Idle',
        message: 'session state callbacks already registered'
      }
    }
    try {
      const callbackResult = this.rdpClientController.registerCallbacks({
        onState: (state: string, wasConnected: boolean): void => {
          this.handleState(state, wasConnected)
        },
        onError: (message: string, wasConnected: boolean): void => {
          this.handleError(message, wasConnected)
        }
      })
      if (!callbackResult.ok) {
        RdpLogger.error(`Native callback registration failed: ${callbackResult.message}`)
        return callbackResult
      }
      this.callbacksRegistered = true
      return callbackResult
    } catch (error) {
      RdpLogger.error(`Native callback registration failed: ${JSON.stringify(error)}`)
      return { ok: false, state: '', message: `registration failed: ${JSON.stringify(error)}` }
    }
  }

  handleState(state: string, wasConnected: boolean): void {
    const nextConnected = RdpClientController.isConnectedState(state)
    if (!nextConnected && (state === 'Disconnected' || state === 'Failed' || state === 'Idle')) {
      if (wasConnected) {
        this.releaseInput()
      }
      this.setSessionVisible(false)
    }
    this.updateSessionNoticeForNativeState(state)
    if (nextConnected && !wasConnected) {
      this.setSessionVisible(true)
      this.lastConnectionErrorMessage = ''
      this.setConnectionFeedback(this.messages.connectionSuccess, 'ok')
      this.persistPendingConnectionProfile()
    }
    if (state === 'Failed') {
      this.pendingConnectionProfileSave = null
      this.setConnectionFeedback(this.connectionFailureMessage(this.lastConnectionErrorMessage,
        this.messages.connectionFailureCredentials), 'danger')
      RdpLogger.error(`Native state: ${state}`)
    } else if ((state === 'Disconnected' || state === 'Idle') && this.pendingConnectionProfileSave !== null) {
      this.pendingConnectionProfileSave = null
      this.setConnectionFeedback(this.connectionFailureMessage(this.lastConnectionErrorMessage,
        this.messages.connectionFailureNoSession), 'danger')
    } else if (state === 'Connected' || state === 'Disconnected' ||
      state === 'RemoteLoginWaiting' || state === 'RemoteDesktopReady') {
      RdpLogger.info(`Native state: ${state}`)
    }
    this.publish()
  }

  handleError(message: string, wasConnected: boolean): void {
    if (wasConnected) {
      this.releaseInput()
    }
    this.setSessionVisible(false)
    this.clearSessionNotice()
    this.pendingConnectionProfileSave = null
    this.lastConnectionErrorMessage = message
    this.setConnectionFeedback(this.connectionFailureMessage(message), 'danger')
    RdpLogger.error(`Native error: ${message}`)
    this.publish()
  }

  connect(input: SessionStateConnectInput): void {
    const validation = RdpConnectionValidator.validate(input.host, input.port, input.username, input.password)
    if (!validation.ok) {
      const validationMessage = this.messages.validationMessage(validation.issues)
      this.rdpClientController.beginConnect()
      this.pendingConnectionProfileSave = null
      this.setConnectionFeedback(validationMessage, 'warning')
      RdpLogger.warn(`Connection validation failed: ${validationMessage}`)
      this.publish()
      return
    }

    const policy = input.certPolicy
    const host = validation.host
    const port = validation.port
    const username = validation.username
    const password = validation.password
    const appFilesDir = this.appFilesDir()
    this.pendingConnectionProfileSave = {
      profileId: input.selectedProfileId,
      host: host,
      port: port,
      username: username,
      password: password,
      certPolicy: policy,
      rememberPassword: input.rememberPassword
    }
    this.lastConnectionErrorMessage = ''
    this.rdpClientController.beginConnect()
    this.remoteLoginWaiting = false
    this.setConnectionFeedback(this.messages.connectionConnecting, 'info')
    this.sessionNoticeTitle = this.messages.sessionConnectingTitle
    this.sessionNoticeSubtitle = this.messages.sessionConnectingSubtitle
    this.setSessionVisible(true)
    this.queueNativeConnect(host, port, username, password, policy, appFilesDir)
    this.publish()
  }

  handleConnectResult(result: NativeCommandResult): void {
    if (result.ok) {
      this.setSessionVisible(true)
      if (this.rdpClientController.isConnected()) {
        this.lastConnectionErrorMessage = ''
        this.setConnectionFeedback(this.messages.connectionSuccess, 'ok')
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
    this.publish()
  }

  setConnectionFeedback(text: string, tone: SettingsStatusTone): void {
    this.connectionFeedbackText = text
    this.connectionFeedbackTone = tone
    this.publish()
  }

  clearConnectionFeedback(): void {
    this.connectionFeedbackText = ''
    this.connectionFeedbackTone = 'neutral'
    this.publish()
  }

  private queueNativeConnect(host: string, port: string, username: string, password: string,
    policy: string, appFilesDir: string): void {
    setTimeout((): void => {
      try {
        const result = this.rdpClientController.connect({
          host: host,
          port: port,
          username: username,
          password: password,
          certPolicy: policy,
          appFilesDir: appFilesDir
        })
        this.handleConnectResult(result)
      } catch (error) {
        this.handleConnectException(error as Error)
      }
    }, 0)
  }

  private handleConnectException(error: Error): void {
    this.setSessionVisible(false)
    this.clearSessionNotice()
    this.pendingConnectionProfileSave = null
    this.lastConnectionErrorMessage = this.errorMessage(error)
    this.setConnectionFeedback(this.connectionFailureMessage(this.lastConnectionErrorMessage,
      this.messages.connectionFailureCredentials), 'danger')
    RdpLogger.error(`Native connect failed: ${JSON.stringify(error)}`)
    this.publish()
  }

  private updateSessionNoticeForNativeState(state: string): void {
    if (state === 'RemoteLoginWaiting') {
      this.remoteLoginWaiting = true
      this.sessionNoticeTitle = this.messages.sessionRemoteLoginWaitingTitle
      this.sessionNoticeSubtitle = this.messages.sessionRemoteLoginWaitingSubtitle
      return
    }

    this.remoteLoginWaiting = false
    if (state === 'Connected') {
      this.sessionNoticeTitle = this.messages.sessionConnectedTitle
      this.sessionNoticeSubtitle = this.messages.sessionConnectedSubtitle
      return
    }

    if (state === 'RemoteDesktopReady' || state === 'Disconnected' || state === 'Failed' || state === 'Idle') {
      this.sessionNoticeTitle = ''
      this.sessionNoticeSubtitle = ''
      return
    }

    if (state === 'Resolving') {
      this.sessionNoticeTitle = this.messages.sessionResolvingTitle
      this.sessionNoticeSubtitle = this.messages.sessionResolvingSubtitle
    } else if (state === 'TCP connected') {
      this.sessionNoticeTitle = this.messages.sessionTcpConnectedTitle
      this.sessionNoticeSubtitle = this.messages.sessionTcpConnectedSubtitle
    } else if (state === 'Negotiating') {
      this.sessionNoticeTitle = this.messages.sessionNegotiatingTitle
      this.sessionNoticeSubtitle = this.messages.sessionNegotiatingSubtitle
    } else if (state === 'Authenticating') {
      this.sessionNoticeTitle = this.messages.sessionAuthenticatingTitle
      this.sessionNoticeSubtitle = this.messages.sessionAuthenticatingSubtitle
    }
  }

  private setSessionVisible(visible: boolean): void {
    this.sessionVisible = visible
    if (!visible) {
      this.clearSessionNotice()
    }
  }

  private clearSessionNotice(): void {
    this.remoteLoginWaiting = false
    this.sessionNoticeTitle = ''
    this.sessionNoticeSubtitle = ''
  }

  private persistPendingConnectionProfile(): void {
    if (this.pendingConnectionProfileSave === null) {
      return
    }
    const saveInput = this.pendingConnectionProfileSave
    this.pendingConnectionProfileSave = null
    this.onPersistProfile(saveInput)
  }

  private connectionFailureMessage(message: string, fallback: string = this.messages.connectionFailure): string {
    const detail = message.trim()
    if (detail.length === 0) {
      return fallback
    }
    return this.messages.connectionFailureWithDetail(detail)
  }

  private errorMessage(error: Error): string {
    if (error.message.length > 0) {
      return error.message
    }
    const serialized = JSON.stringify(error)
    return serialized.length > 0 && serialized !== '{}' ? serialized : ''
  }

  private publish(): void {
    this.onChange(this.snapshot())
  }
}
