export interface RemoteControlProcessParams {
  appFilesDir: string
  accessCode: string
  accessCodeGateEnabled: boolean
  restartIfRunning: boolean
}

export interface RemoteControlProcessResult {
  ok: boolean
  state: string
  message: string
  pid: number
  port: number
  lastExitCode: number
}

export interface RemoteControlPort {
  readonly available: boolean
  start(params: RemoteControlProcessParams): RemoteControlProcessResult
  diagnostics(): RemoteControlProcessResult
  stop(reason: string): RemoteControlProcessResult
}

export class UnavailableRemoteControlPort implements RemoteControlPort {
  readonly available: boolean = false

  start(_params: RemoteControlProcessParams): RemoteControlProcessResult {
    return this.unavailableResult()
  }

  diagnostics(): RemoteControlProcessResult {
    return this.unavailableResult()
  }

  stop(_reason: string): RemoteControlProcessResult {
    return this.unavailableResult()
  }

  private unavailableResult(): RemoteControlProcessResult {
    return {
      ok: false,
      state: 'Unavailable',
      message: 'Remote control server is unavailable on this device',
      pid: 0,
      port: 3390,
      lastExitCode: 0
    }
  }
}
