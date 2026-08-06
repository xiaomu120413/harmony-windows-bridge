export interface NativeXrdpControlParams {
  appFilesDir: string;
  accessCode: string;
  accessCodeGateEnabled: boolean;
  restartIfRunning: boolean;
}

export interface NativeXrdpControlResult {
  ok: boolean;
  state: string;
  message: string;
  pid: number;
  port: number;
  lastExitCode: number;
}

declare const xrdpControl: {
  start(params: NativeXrdpControlParams): NativeXrdpControlResult;
  diagnostics(): NativeXrdpControlResult;
  stop(reason: string): NativeXrdpControlResult;
};

export default xrdpControl;
