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

export interface NativeXrdpServerParams {
  appFilesDir?: string;
  accessCode?: string;
  accessCodeGateEnabled?: boolean;
  restartIfRunning?: boolean;
}

export interface NativeXrdpServerResult extends NativeCommandResult {
  libraryPath: string;
  runtimeRoot: string;
  configPath: string;
  modulePath: string;
  logPath: string;
  activeMstscSession: boolean;
  port: number;
  rdpecamDeviceName: string;
  rdpecamFormat: number;
  rdpecamWidth: number;
  rdpecamHeight: number;
  rdpecamSampleCount: number;
  rdpecamBytes: number;
  rdpecamErrors: number;
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
  ensureXrdpServerStarted(params?: NativeXrdpServerParams): NativeXrdpServerResult;
  getXrdpServerDiagnostics(): NativeXrdpServerResult;
  bindImeHostWindow(windowId: number): NativeCommandResult;
  attachXComponentContent(nodeContent: NodeContent): NativeCommandResult;
  detachXComponentContent(): NativeCommandResult;
  releaseAllInput(): NativeCommandResult;
  onState(callback: (state: string) => void): NativeCommandResult;
  onError(callback: (message: string) => void): NativeCommandResult;
  onPermissionRequest(callback: (request: NativePermissionRequest) => void): NativeCommandResult;
  completePermissionRequest(result: NativePermissionResult): NativeCommandResult;
};

export default rdpNative;
