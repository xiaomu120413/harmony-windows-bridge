export interface NativeProbeResult {
  bridgeVersion: string;
  abi: string;
  freeRdpVersion: string;
  winprVersion: string;
  opensslVersion: string;
  featureSummary: string;
  audioStats: string;
  renderStats: string;
  graphicsStats: string;
  inputDispatchMode: string;
  freeRdpLinked: boolean;
  surfaceRegistered: boolean;
  surfaceReady: boolean;
  surfaceId: string;
  surfaceWidth: number;
  surfaceHeight: number;
  surfaceViewportX: number;
  surfaceViewportY: number;
  surfaceViewportWidth: number;
  surfaceViewportHeight: number;
  surfaceCreatedCount: number;
  surfaceChangedCount: number;
  surfaceDestroyedCount: number;
  surfacePaintCount: number;
  surfaceLastPaintMessage: string;
  sessionConnected: boolean;
  desktopWidth: number;
  desktopHeight: number;
  inputQueueDepth: number;
  inputQueuedCount: number;
  inputSentCount: number;
  inputDroppedCount: number;
  probeJson: string;
  probeError: string;
  logs: string[];
}

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
  port?: number | string;
}

export interface NativeXrdpServerResult extends NativeCommandResult {
  libraryPath: string;
  runtimeRoot: string;
  configPath: string;
  modulePath: string;
}

export interface NativeXrdpFrameParams extends NativeXrdpServerParams {
  width?: number | string;
  height?: number | string;
}

export interface NativePointerEventInput {
  action: string;
  button?: string;
  buttons?: number;
  x: number;
  y: number;
  delta?: number;
  allowClamp?: boolean;
}

export interface NativeKeyInput {
  scancode: number;
  down: boolean;
  repeat?: boolean;
}

export interface NativePlatformKeyInput {
  keyCode: number;
  down: boolean;
  repeat?: boolean;
  ctrl?: boolean;
  shift?: boolean;
  alt?: boolean;
  meta?: boolean;
}

export interface NativeUnicodeInput {
  code: number;
  down: boolean;
}

export interface NativeTextInput {
  text: string;
}

export interface NativeSurfaceLayoutInput {
  width: number;
  height: number;
}

declare const rdpNative: {
  probe(): NativeProbeResult;
  connect(params: Object): NativeCommandResult;
  disconnect(): NativeCommandResult;
  probeXrdpServer(params?: NativeXrdpServerParams): NativeXrdpServerResult;
  startXrdpServer(params?: NativeXrdpServerParams): NativeXrdpServerResult;
  stopXrdpServer(): NativeXrdpServerResult;
  pushXrdpTestFrame(params?: NativeXrdpFrameParams): NativeXrdpServerResult;
  sendPointerEvent(input: Object): NativeCommandResult;
  sendKey(input: Object): NativeCommandResult;
  sendPlatformKey(input: Object): NativeCommandResult;
  sendUnicode(input: Object): NativeCommandResult;
  sendText(input: Object): NativeCommandResult;
  releaseAllKeys(): NativeCommandResult;
  notifySurfaceLayout(input: Object): NativeCommandResult;
  onState(callback: (state: string) => void): NativeCommandResult;
  onLog(callback: (line: string) => void): NativeCommandResult;
  onError(callback: (message: string) => void): NativeCommandResult;
  onMicrophonePermissionRequest(callback: (requestId: string) => void): NativeCommandResult;
  completeMicrophonePermissionRequest(result: Object): NativeCommandResult;
};

export default rdpNative;
