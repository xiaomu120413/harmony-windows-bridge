import { NativeRdpGateway } from "@normalized:N&&&common/src/main/ets/rdp/NativeRdpGateway&";
import type { NativeCommandResult, NativeConnectParams } from "@normalized:N&&&common/src/main/ets/rdp/NativeRdpGateway&";
class BjcCov {
    coverage: {
        [key: string]: string | number;
    };
    constructor(covData: object) {
        const gcv = "__coverage__";
        let coverage = globalThis[gcv] || (globalThis[gcv] = {});
        if (!coverage[covData.path] && true) {
            coverage[covData.path] = covData;
        }
        this.coverage = coverage[covData.path];
    }
    instrumentFunction(func: number) {
        this.coverage.functions[func].count++;
        this.coverage.functions[func].regions[0].count++;
    }
    instrumentRegion(func: number, region: number) {
        this.coverage.functions[func].regions[region].count++;
    }
    instrumentReturn(func: number, retIdx: number) {
        this.coverage.functions[func].returnes[retIdx].count++;
    }
    instrumentBranch(func: number, branch: number, trueOrFalse: boolean) {
        if (trueOrFalse) {
            this.coverage.functions[func].branches[branch].trueCount++;
            for (let r of Object.values(this.coverage.functions[func].branches[branch].group)) {
                if (r !== branch) {
                    this.coverage.functions[func].branches[r as number].falseCount++;
                }
            }
        }
        else {
            this.coverage.functions[func].branches[branch].falseCount++;
        }
    }
}
let bjccovmshb1i0f = new BjcCov({ version: "bjc v1.0.0", versionCode: 10000, path: "common/src/main/ets/rdp/RdpClientController.ets", hash: "c03337bea7d8b42ee0a224a3c388f15df9f9d2b06e2989e5e22c5792d8070df0", lineCnt: 69, count: 0, projectPath: "", functions: { 0: { name: "anonymous_0", count: 0, regions: { 0: { startLoc: { line: 8, col: 12 }, endLoc: { line: 8, col: 58 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 0 }, 1: { name: "anonymous_1", count: 0, regions: { 0: { startLoc: { line: 9, col: 12 }, endLoc: { line: 9, col: 60 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 1 }, 2: { name: "RdpClientController.registerCallbacks", count: 0, regions: { 0: { startLoc: { line: 16, col: 3 }, endLoc: { line: 45, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 17, col: 35 }, endLoc: { line: 23, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 30, col: 26 }, endLoc: { line: 32, col: 6 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 39, col: 26 }, endLoc: { line: 41, col: 6 }, count: 0, ignored: 0 }, 4: { startLoc: { line: 43, col: 5 }, endLoc: { line: 45, col: 4 }, count: 0, ignored: 0 }, 5: { startLoc: { line: 27, col: 7 }, endLoc: { line: 29, col: 6 }, count: 0, ignored: 0 }, 6: { startLoc: { line: 36, col: 7 }, endLoc: { line: 38, col: 6 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 17, col: 9 }, endLoc: { line: 17, col: 33 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 20, col: 16 }, endLoc: { line: 20, col: 53 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 2: { startLoc: { line: 30, col: 9 }, endLoc: { line: 30, col: 24 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 3: { startLoc: { line: 39, col: 9 }, endLoc: { line: 39, col: 24 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 2 }, 3: { name: "anonymous_2", count: 0, regions: { 0: { startLoc: { line: 25, col: 50 }, endLoc: { line: 29, col: 6 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 3 }, 4: { name: "anonymous_3", count: 0, regions: { 0: { startLoc: { line: 34, col: 50 }, endLoc: { line: 38, col: 6 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 4 }, 5: { name: "RdpClientController.connect", count: 0, regions: { 0: { startLoc: { line: 47, col: 3 }, endLoc: { line: 51, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 48, col: 5 }, endLoc: { line: 51, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 5 }, 6: { name: "RdpClientController.beginConnect", count: 0, regions: { 0: { startLoc: { line: 53, col: 3 }, endLoc: { line: 55, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 54, col: 5 }, endLoc: { line: 55, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 6 }, 7: { name: "RdpClientController.releaseAllInput", count: 0, regions: { 0: { startLoc: { line: 57, col: 3 }, endLoc: { line: 59, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 58, col: 5 }, endLoc: { line: 59, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 7 }, 8: { name: "RdpClientController.isConnected", count: 0, regions: { 0: { startLoc: { line: 61, col: 3 }, endLoc: { line: 63, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 62, col: 5 }, endLoc: { line: 63, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 8 }, 9: { name: "RdpClientController.isConnectedState", count: 0, regions: { 0: { startLoc: { line: 65, col: 3 }, endLoc: { line: 67, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 66, col: 5 }, endLoc: { line: 67, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 9 } }, exeLine: { 0: 1, 1: 2, 2: 3, 3: 4, 4: 5, 5: 7, 6: 8, 7: 9, 8: 12, 9: 13, 10: 14, 11: 16, 12: 17, 13: 18, 14: 19, 15: 20, 16: 21, 17: 25, 18: 26, 19: 27, 20: 28, 21: 30, 22: 31, 23: 34, 24: 35, 25: 36, 26: 37, 27: 39, 28: 40, 29: 43, 30: 44, 31: 47, 32: 48, 33: 49, 34: 50, 35: 53, 36: 54, 37: 57, 38: 58, 39: 61, 40: 62, 41: 65, 42: 66 } });
export interface RdpClientCallbacks {
    onState: (state: string, wasConnected: boolean) => void;
    onError: (message: string, wasConnected: boolean) => void;
}
export class RdpClientController {
    private callbacksRegistered: boolean = false;
    private connected: boolean = false;
    registerCallbacks(callbacks: RdpClientCallbacks): NativeCommandResult {
        bjccovmshb1i0f.instrumentFunction(2);
        if (this.callbacksRegistered) {
            bjccovmshb1i0f.instrumentBranch(2, 0, true);
            bjccovmshb1i0f.instrumentRegion(2, 1);
            return {
                ok: true,
                state: this.connected ? (bjccovmshb1i0f.instrumentBranch(2, 1, true), 'Connected') : (bjccovmshb1i0f.instrumentBranch(2, 1, false), 'Idle'),
                message: 'RDP client callbacks already registered'
            };
        }
        else {
            bjccovmshb1i0f.instrumentBranch(2, 0, false);
        }
        const stateResult = NativeRdpGateway.onState((state: string): void => {
            bjccovmshb1i0f.instrumentFunction(3);
            const wasConnected = this.connected;
            bjccovmshb1i0f.instrumentRegion(2, 5);
            this.connected = RdpClientController.isConnectedState(state);
            callbacks.onState(state, wasConnected);
        });
        if (!stateResult.ok) {
            bjccovmshb1i0f.instrumentBranch(2, 2, true);
            bjccovmshb1i0f.instrumentRegion(2, 2);
            return stateResult;
        }
        else {
            bjccovmshb1i0f.instrumentBranch(2, 2, false);
        }
        const errorResult = NativeRdpGateway.onError((message: string): void => {
            bjccovmshb1i0f.instrumentFunction(4);
            const wasConnected = this.connected;
            bjccovmshb1i0f.instrumentRegion(2, 6);
            this.connected = false;
            callbacks.onError(message, wasConnected);
        });
        if (!errorResult.ok) {
            bjccovmshb1i0f.instrumentBranch(2, 3, true);
            bjccovmshb1i0f.instrumentRegion(2, 3);
            return errorResult;
        }
        else {
            bjccovmshb1i0f.instrumentBranch(2, 3, false);
        }
        bjccovmshb1i0f.instrumentRegion(2, 4);
        this.callbacksRegistered = true;
        return errorResult;
    }
    connect(params: NativeConnectParams): NativeCommandResult {
        bjccovmshb1i0f.instrumentFunction(5);
        bjccovmshb1i0f.instrumentRegion(5, 1);
        const result = NativeRdpGateway.connect(params);
        this.connected = result.ok && RdpClientController.isConnectedState(result.state);
        return result;
    }
    beginConnect(): void {
        bjccovmshb1i0f.instrumentFunction(6);
        bjccovmshb1i0f.instrumentRegion(6, 1);
        this.connected = false;
    }
    releaseAllInput(): NativeCommandResult {
        bjccovmshb1i0f.instrumentFunction(7);
        bjccovmshb1i0f.instrumentRegion(7, 1);
        return NativeRdpGateway.releaseAllInput();
    }
    isConnected(): boolean {
        bjccovmshb1i0f.instrumentFunction(8);
        bjccovmshb1i0f.instrumentRegion(8, 1);
        return this.connected;
    }
    static isConnectedState(state: string): boolean {
        bjccovmshb1i0f.instrumentFunction(9);
        bjccovmshb1i0f.instrumentRegion(9, 1);
        return state === 'Connected' || state === 'RemoteLoginWaiting' || state === 'RemoteDesktopReady';
    }
}
