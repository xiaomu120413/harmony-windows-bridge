import { socket } from '@kit.NetworkKit'

export type RdpEndpointProbeState = 'Reachable' | 'Unreachable'

export interface RdpEndpointProbeResult {
  state: RdpEndpointProbeState
  message: string
  elapsedMs: number
}

const DEFAULT_PROBE_TIMEOUT_MS: number = 3000

export class RdpEndpointProbe {
  static async probe(host: string, port: number,
    timeoutMs: number = DEFAULT_PROBE_TIMEOUT_MS): Promise<RdpEndpointProbeResult> {
    const startedAt = Date.now()
    const tcpSocket = socket.constructTCPSocketInstance()
    try {
      const address = RdpEndpointProbe.normalizeAddress(host)
      await tcpSocket.connect({
        address: {
          address: address,
          family: address.indexOf(':') >= 0 ? 2 : 1,
          port: port
        },
        timeout: timeoutMs
      })
      return {
        state: 'Reachable',
        message: '远程主机端口可达',
        elapsedMs: Date.now() - startedAt
      }
    } catch (error) {
      return {
        state: 'Unreachable',
        message: RdpEndpointProbe.failureMessage(error as Error),
        elapsedMs: Date.now() - startedAt
      }
    } finally {
      try {
        await tcpSocket.close()
      } catch (_error) {
      }
    }
  }

  private static normalizeAddress(host: string): string {
    const trimmed = host.trim()
    if (trimmed.length > 2 && trimmed.startsWith('[') && trimmed.endsWith(']')) {
      return trimmed.substring(1, trimmed.length - 1)
    }
    return trimmed
  }

  private static failureMessage(error: Error): string {
    const detail = error.message.trim().toLowerCase()
    if (detail.indexOf('timed out') >= 0 || detail.indexOf('timeout') >= 0) {
      return '远程主机端口连接超时，请检查网络、防火墙和远程桌面服务'
    }
    if (detail.indexOf('refused') >= 0) {
      return '远程主机拒绝连接，请确认远程桌面服务和端口'
    }
    if (detail.indexOf('resolve') >= 0 || detail.indexOf('lookup') >= 0 || detail.indexOf('name') >= 0) {
      return '无法解析远程主机名，请检查 Host'
    }
    return '无法访问远程主机端口，请检查网络、防火墙和远程桌面服务'
  }
}
