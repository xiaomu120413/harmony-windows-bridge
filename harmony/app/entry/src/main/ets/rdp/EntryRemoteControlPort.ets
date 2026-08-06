import {
  RemoteControlPort,
  RemoteControlProcessParams,
  RemoteControlProcessResult,
  RdpLogger
} from 'common'
import xrdpControl from 'libxrdpcontrol.so'

export class EntryRemoteControlPort implements RemoteControlPort {
  readonly available: boolean = true

  start(params: RemoteControlProcessParams): RemoteControlProcessResult {
    const result = xrdpControl.start(params)
    RdpLogger.info(`xrdp control start state=${result.state} pid=${result.pid}`)
    return result
  }

  diagnostics(): RemoteControlProcessResult {
    return xrdpControl.diagnostics()
  }

  stop(reason: string): RemoteControlProcessResult {
    const result = xrdpControl.stop(reason)
    RdpLogger.info(`xrdp control stop state=${result.state} pid=${result.pid}`)
    return result
  }
}

export const entryRemoteControlPort: EntryRemoteControlPort = new EntryRemoteControlPort()
