import {
  NativeCommandResult,
  NativeConnectParams,
  NativeRdpGateway
} from './NativeRdpGateway'

export interface RdpClientCallbacks {
  onState: (state: string, wasConnected: boolean) => void
  onError: (message: string, wasConnected: boolean) => void
}

export class RdpClientController {
  private callbacksRegistered: boolean = false
  private connected: boolean = false

  registerCallbacks(callbacks: RdpClientCallbacks): NativeCommandResult {
    if (this.callbacksRegistered) {
      return {
        ok: true,
        state: this.connected ? 'Connected' : 'Idle',
        message: 'RDP client callbacks already registered'
      }
    }

    const stateResult = NativeRdpGateway.onState((state: string): void => {
      const wasConnected = this.connected
      this.connected = RdpClientController.isConnectedState(state)
      callbacks.onState(state, wasConnected)
    })
    if (!stateResult.ok) {
      return stateResult
    }

    const errorResult = NativeRdpGateway.onError((message: string): void => {
      const wasConnected = this.connected
      this.connected = false
      callbacks.onError(message, wasConnected)
    })
    if (!errorResult.ok) {
      return errorResult
    }

    this.callbacksRegistered = true
    return errorResult
  }

  connect(params: NativeConnectParams): NativeCommandResult {
    const result = NativeRdpGateway.connect(params)
    this.connected = result.ok && RdpClientController.isConnectedState(result.state)
    return result
  }

  beginConnect(): void {
    this.connected = false
  }

  releaseAllInput(): NativeCommandResult {
    return NativeRdpGateway.releaseAllInput()
  }

  isConnected(): boolean {
    return this.connected
  }

  static isConnectedState(state: string): boolean {
    return state === 'Connected' || state === 'RemoteLoginWaiting' || state === 'RemoteDesktopReady'
  }
}
