import { NodeContent } from '@kit.ArkUI';

export interface NativeConnectParams {
  host: string;
  port: string;
  username: string;
  password: string;
  resolution?: string;
  certPolicy: string;
  graphicsMode?: string;
  appFilesDir?: string;
}

export interface NativeCommandResult {
  ok: boolean;
  state: string;
  message: string;
}

export type NativePermissionType = 'microphone' | 'camera' | 'clipboard' | 'location';

export interface NativePermissionRequest {
  type: NativePermissionType;
  requestId: string;
}

export interface NativePermissionResult {
  type: NativePermissionType;
  requestId: number;
  granted: boolean;
}

declare const rdpNative: {
  connect(params: NativeConnectParams): NativeCommandResult;
  bindImeHostWindow(windowId: number): NativeCommandResult;
  attachXComponentContent(nodeContent: NodeContent): NativeCommandResult;
  detachXComponentContent(): NativeCommandResult;
  releaseAllInput(): NativeCommandResult;
  getDiagnostics(): string;
  onState(callback: (state: string) => void): NativeCommandResult;
  onError(callback: (message: string) => void): NativeCommandResult;
  onPermissionRequest(callback: (request: NativePermissionRequest) => void): NativeCommandResult;
  completePermissionRequest(result: NativePermissionResult): NativeCommandResult;
};

export default rdpNative;
