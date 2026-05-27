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
  logs: string[];
}

export interface NativeXrdpServerParams {
  appFilesDir?: string;
  runtimeRoot?: string;
  hnpRoot?: string;
  libraryPath?: string;
  libDir?: string;
  modulePath?: string;
  configPath?: string;
  sharePath?: string;
  accessCode?: string;
  accessCodeGateEnabled?: boolean;
  restartIfRunning?: boolean;
  port?: number | string;
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

export interface NativeXrdpDiagnosticsResult {
  ok: boolean;
  running: boolean;
  activeMstscSession: boolean;
  port: number;
  sessionWidth: number;
  sessionHeight: number;
  sessionBpp: number;
  backendEventCount: number;
  inputEventCount: number;
  state: string;
  message: string;
  lastBackendEvent: string;
  lastDisconnectReason: string;
  libraryPath: string;
  backendLibraryPath: string;
  runtimeRoot: string;
  configPath: string;
  modulePath: string;
  sharePath: string;
  logPath: string;
  logs: string[];
}

declare const rdpNative: {
  connect(params: NativeConnectParams): NativeCommandResult;
  ensureXrdpServerStarted(params?: NativeXrdpServerParams): NativeXrdpServerResult;
  getXrdpServerDiagnostics(): NativeXrdpDiagnosticsResult;
  releaseAllKeys(): NativeCommandResult;
  onState(callback: (state: string) => void): NativeCommandResult;
  onError(callback: (message: string) => void): NativeCommandResult;
  onMicrophonePermissionRequest(callback: (requestId: string) => void): NativeCommandResult;
  onClipboardPermissionRequest(callback: (requestId: string) => void): NativeCommandResult;
  onLocationPermissionRequest(callback: (requestId: string) => void): NativeCommandResult;
  completeClipboardPermissionRequest(result: Object): NativeCommandResult;
  completeMicrophonePermissionRequest(result: Object): NativeCommandResult;
  completeLocationPermissionRequest(result: Object): NativeCommandResult;
};

export default rdpNative;
