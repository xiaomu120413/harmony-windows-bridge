export interface NativeProbeResult {
  bridgeVersion: string;
  abi: string;
  freeRdpVersion: string;
  freeRdpLinked: boolean;
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

declare const rdpNative: {
  probe(): NativeProbeResult;
  connect(params: NativeConnectParams): NativeCommandResult;
  disconnect(): NativeCommandResult;
};

export default rdpNative;
