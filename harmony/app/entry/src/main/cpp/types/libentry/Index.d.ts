export interface NativeProbeResult {
  bridgeVersion: string;
  abi: string;
  freeRdpVersion: string;
  winprVersion: string;
  opensslVersion: string;
  freeRdpLinked: boolean;
  surfaceRegistered: boolean;
  surfaceReady: boolean;
  surfaceId: string;
  surfaceWidth: number;
  surfaceHeight: number;
  surfaceCreatedCount: number;
  surfaceChangedCount: number;
  surfaceDestroyedCount: number;
  surfaceTouchCount: number;
  surfacePaintCount: number;
  surfaceLastPaintMessage: string;
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
}

export interface NativeUnicodeInput {
  code: number;
  down: boolean;
}

declare const rdpNative: {
  probe(): NativeProbeResult;
  connect(params: NativeConnectParams): NativeCommandResult;
  disconnect(): NativeCommandResult;
  paintTestPattern(): NativeCommandResult;
  sendPointer(input: NativePointerInput): NativeCommandResult;
  sendKey(input: NativeKeyInput): NativeCommandResult;
  sendUnicode(input: NativeUnicodeInput): NativeCommandResult;
  onState(callback: (state: string) => void): NativeCommandResult;
  onLog(callback: (line: string) => void): NativeCommandResult;
  onError(callback: (message: string) => void): NativeCommandResult;
};

export default rdpNative;
