import { NodeContent } from '@kit.ArkUI'
import rdpNative from 'librdpclient.so'

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

  static getDiagnostics(): string {
    return rdpNative.getDiagnostics()
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
