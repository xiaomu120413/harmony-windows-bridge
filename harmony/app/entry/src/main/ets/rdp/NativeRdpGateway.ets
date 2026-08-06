import { NodeContent } from '@kit.ArkUI'
import rdpNative from 'libentry.so'

export interface NativeConnectParams {
  host: string
  port: string
  username: string
  password: string
  resolution?: string
  certPolicy: string
  graphicsMode?: string
  appFilesDir?: string
}

export interface NativeCommandResult {
  ok: boolean
  state: string
  message: string
}

export interface NativeXrdpServerParams {
  appFilesDir?: string
  accessCode?: string
  accessCodeGateEnabled?: boolean
  restartIfRunning?: boolean
}

export interface NativeXrdpServerResult extends NativeCommandResult {
  libraryPath: string
  runtimeRoot: string
  configPath: string
  modulePath: string
  logPath: string
  activeMstscSession: boolean
  port: number
  rdpecamDeviceName: string
  rdpecamFormat: number
  rdpecamWidth: number
  rdpecamHeight: number
  rdpecamSampleCount: number
  rdpecamBytes: number
  rdpecamErrors: number
}

export type NativePermissionType = 'microphone' | 'camera' | 'clipboard' | 'location'

export interface NativePermissionRequest {
  type: NativePermissionType
  requestId: string
}

export interface NativePermissionResult {
  type: NativePermissionType
  requestId: number
  granted: boolean
}

export class NativeRdpGateway {
  static connect(params: NativeConnectParams): NativeCommandResult {
    return rdpNative.connect(params)
  }

  static ensureXrdpServerStarted(params?: NativeXrdpServerParams): NativeXrdpServerResult {
    return rdpNative.ensureXrdpServerStarted(params)
  }

  static getXrdpServerDiagnostics(): NativeXrdpServerResult {
    return rdpNative.getXrdpServerDiagnostics()
  }

  static bindImeHostWindow(windowId: number): NativeCommandResult {
    return rdpNative.bindImeHostWindow(windowId)
  }

  static attachXComponentContent(nodeContent: NodeContent): NativeCommandResult {
    return rdpNative.attachXComponentContent(nodeContent)
  }

  static detachXComponentContent(): NativeCommandResult {
    return rdpNative.detachXComponentContent()
  }

  static releaseAllInput(): NativeCommandResult {
    return rdpNative.releaseAllInput()
  }

  static onState(callback: (state: string) => void): NativeCommandResult {
    return rdpNative.onState(callback)
  }

  static onError(callback: (message: string) => void): NativeCommandResult {
    return rdpNative.onError(callback)
  }

  static onPermissionRequest(callback: (request: NativePermissionRequest) => void): NativeCommandResult {
    return rdpNative.onPermissionRequest(callback)
  }

  static completePermissionRequest(result: NativePermissionResult): NativeCommandResult {
    return rdpNative.completePermissionRequest(result)
  }
}
