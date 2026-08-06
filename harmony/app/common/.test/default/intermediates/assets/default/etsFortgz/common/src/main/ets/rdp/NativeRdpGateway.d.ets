import { NodeContent } from "@ohos.arkui.node";
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
export declare class NativeRdpGateway {
    static connect(params: NativeConnectParams): NativeCommandResult;
    static ensureXrdpServerStarted(params?: NativeXrdpServerParams): NativeXrdpServerResult;
    static getXrdpServerDiagnostics(): NativeXrdpServerResult;
    static bindImeHostWindow(windowId: number): NativeCommandResult;
    static attachXComponentContent(nodeContent: NodeContent): NativeCommandResult;
    static detachXComponentContent(): NativeCommandResult;
    static releaseAllInput(): NativeCommandResult;
    static onState(callback: (state: string) => void): NativeCommandResult;
    static onError(callback: (message: string) => void): NativeCommandResult;
    static onPermissionRequest(callback: (request: NativePermissionRequest) => void): NativeCommandResult;
    static completePermissionRequest(result: NativePermissionResult): NativeCommandResult;
}
