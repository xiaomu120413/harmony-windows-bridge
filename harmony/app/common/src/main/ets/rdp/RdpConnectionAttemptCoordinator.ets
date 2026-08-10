import { NativeCommandResult, NativeConnectParams } from './NativeRdpGateway'
import { RdpClientController } from './RdpClientController'
import { RdpEndpointProbe, RdpEndpointProbeResult } from './RdpEndpointProbe'

export interface RdpConnectionAttemptOutcome {
  probe: RdpEndpointProbeResult
  nativeResult: NativeCommandResult | null
  errorMessage: string
}

export class RdpConnectionAttemptCoordinator {
  private readonly client: RdpClientController
  private generation: number = 0

  constructor(client: RdpClientController) {
    this.client = client
  }

  cancel(): void {
    this.generation++
  }

  begin(params: NativeConnectParams, onReachable: () => void): Promise<RdpConnectionAttemptOutcome | null> {
    const generation = ++this.generation
    this.client.beginConnect()
    return RdpEndpointProbe.probe(params.host, Number(params.port))
      .then((probe: RdpEndpointProbeResult): Promise<RdpConnectionAttemptOutcome | null> |
        RdpConnectionAttemptOutcome | null => {
        if (generation !== this.generation) {
          return null
        }
        if (probe.state !== 'Reachable') {
          return { probe: probe, nativeResult: null, errorMessage: '' }
        }
        onReachable()
        return new Promise<RdpConnectionAttemptOutcome | null>((resolve): void => {
          setTimeout((): void => {
            if (generation !== this.generation) {
              resolve(null)
              return
            }
            try {
              resolve({ probe: probe, nativeResult: this.client.connect(params), errorMessage: '' })
            } catch (error) {
              resolve({ probe: probe, nativeResult: null,
                errorMessage: RdpConnectionAttemptCoordinator.errorMessage(error as Error) })
            }
          }, 0)
        })
      })
  }

  static errorMessage(error: Error): string {
    if (error.message.length > 0) {
      return error.message
    }
    const serialized = JSON.stringify(error)
    return serialized.length > 0 && serialized !== '{}' ? serialized : ''
  }
}
