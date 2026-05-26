export interface NativeConnectParams {
  host: string;
  port: string;
  username: string;
  password: string;
  certPolicy: string;
  appFilesDir: string;
}

export interface NativeCommandResult {
  ok: boolean;
  state: string;
  message: string;
  logs: string[];
}

declare const rdpNative: {
  connect(params: NativeConnectParams): NativeCommandResult;
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
