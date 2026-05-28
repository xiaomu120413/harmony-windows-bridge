import rdpNative, {
  NativeXrdpServerParams,
  NativeXrdpServerResult
} from 'libentry.so'
import { RdpLogger } from './RdpLogger'

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
}

export interface XrdpServerStartOptions {
  appFilesDir: string
  accessCode: string
  accessCodeGateEnabled: boolean
  restartIfRunning: boolean
}

export class XrdpServerController {
  private readonly messages: XrdpServerDisplayMessages

  constructor(messages: XrdpServerDisplayMessages) {
    this.messages = messages
  }

  diagnostics(): XrdpServerStatus {
    try {
      const result: NativeXrdpServerResult = rdpNative.getXrdpServerDiagnostics()
      return this.toStatus(result)
    } catch (error) {
      RdpLogger.error(`xrdp diagnostics failed: ${JSON.stringify(error)}`)
      return this.failedStatus()
    }
  }

  start(options: XrdpServerStartOptions, trigger: string): XrdpServerStatus {
    try {
      const result: NativeXrdpServerResult = rdpNative.ensureXrdpServerStarted(this.toNativeParams(options))
      return this.toStatus(result)
    } catch (error) {
      RdpLogger.error(`xrdp ensure start failed: ${JSON.stringify(error)}`)
      return this.diagnostics()
    }
  }

  private toNativeParams(options: XrdpServerStartOptions): NativeXrdpServerParams {
    return {
      appFilesDir: options.appFilesDir,
      accessCode: options.accessCode,
      accessCodeGateEnabled: options.accessCodeGateEnabled,
      restartIfRunning: options.restartIfRunning
    }
  }

  private toStatus(result: NativeXrdpServerResult): XrdpServerStatus {
    return {
      running: result.ok && result.state !== 'Stopped' && result.state !== 'Exited',
      state: result.state,
      port: result.port > 0 ? result.port : 3390,
      message: this.displayMessage(result)
    }
  }

  private displayMessage(result: NativeXrdpServerResult): string {
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
    if (result.state === 'Failed') {
      return this.messages.failed
    }
    return result.message
  }

  private failedStatus(): XrdpServerStatus {
    return {
      running: false,
      state: 'Failed',
      port: 3390,
      message: this.messages.failed
    }
  }
}
