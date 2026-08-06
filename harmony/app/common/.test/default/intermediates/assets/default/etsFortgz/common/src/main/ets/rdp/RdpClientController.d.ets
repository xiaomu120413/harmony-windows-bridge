import { NativeCommandResult, NativeConnectParams } from './NativeRdpGateway';
export interface RdpClientCallbacks {
    onState: (state: string, wasConnected: boolean) => void;
    onError: (message: string, wasConnected: boolean) => void;
}
export declare class RdpClientController {
    private callbacksRegistered;
    private connected;
    registerCallbacks(callbacks: RdpClientCallbacks): NativeCommandResult;
    connect(params: NativeConnectParams): NativeCommandResult;
    beginConnect(): void;
    releaseAllInput(): NativeCommandResult;
    isConnected(): boolean;
    static isConnectedState(state: string): boolean;
}
