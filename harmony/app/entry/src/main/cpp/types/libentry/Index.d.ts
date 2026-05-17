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
  surfaceTouchCount: number;
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
  resolution: string;
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

export interface NativePointerInput {
  flags: number;
  x: number;
  y: number;
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

export interface NativeSurfaceLayoutInput {
  width: number;
  height: number;
}

declare const rdpNative: {
  probe(): NativeProbeResult;
  connect(params: NativeConnectParams): NativeCommandResult;
  disconnect(): NativeCommandResult;
  sendPointer(input: NativePointerInput): NativeCommandResult;
  sendKey(input: NativeKeyInput): NativeCommandResult;
  sendPlatformKey(input: NativePlatformKeyInput): NativeCommandResult;
  sendUnicode(input: NativeUnicodeInput): NativeCommandResult;
  notifySurfaceLayout(input: NativeSurfaceLayoutInput): NativeCommandResult;
  onState(callback: (state: string) => void): NativeCommandResult;
  onLog(callback: (line: string) => void): NativeCommandResult;
  onError(callback: (message: string) => void): NativeCommandResult;
};

export default rdpNative;
