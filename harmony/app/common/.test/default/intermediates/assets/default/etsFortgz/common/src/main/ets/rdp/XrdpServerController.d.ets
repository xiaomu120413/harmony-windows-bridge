export interface XrdpServerStatus {
    running: boolean;
    state: string;
    port: number;
    message: string;
}
export interface XrdpServerDisplayMessages {
    active: string;
    listening: string;
    stopped: string;
    exited: string;
    failed: string;
    unavailable: string;
}
export interface XrdpServerStartOptions {
    appFilesDir: string;
    accessCode: string;
    accessCodeGateEnabled: boolean;
    restartIfRunning: boolean;
}
export declare class XrdpServerController {
    private readonly messages;
    private readonly available;
    constructor(messages: XrdpServerDisplayMessages, available?: boolean);
    diagnostics(): XrdpServerStatus;
    start(options: XrdpServerStartOptions, trigger: string): XrdpServerStatus;
    private toNativeParams;
    private toStatus;
    private displayMessage;
    private failedStatus;
    private unavailableStatus;
}
