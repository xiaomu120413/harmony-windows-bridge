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
}

declare const rdpNative: {
  connect(params: NativeConnectParams): NativeCommandResult;
  ensureXrdpServerStarted(params?: NativeXrdpServerParams): NativeXrdpServerResult;
  getXrdpServerDiagnostics(): NativeXrdpServerResult;
  releaseAllKeys(): NativeCommandResult;
  onState(callback: (state: string) => void): NativeCommandResult;
  onError(callback: (message: string) => void): NativeCommandResult;
  onMicrophonePermissionRequest(callback: (requestId: string) => void): NativeCommandResult;
  onCameraPermissionRequest(callback: (requestId: string) => void): NativeCommandResult;
  onClipboardPermissionRequest(callback: (requestId: string) => void): NativeCommandResult;
  onLocationPermissionRequest(callback: (requestId: string) => void): NativeCommandResult;
  completeClipboardPermissionRequest(result: Object): NativeCommandResult;
  completeMicrophonePermissionRequest(result: Object): NativeCommandResult;
  completeCameraPermissionRequest(result: Object): NativeCommandResult;
  completeLocationPermissionRequest(result: Object): NativeCommandResult;
};

export default rdpNative;
