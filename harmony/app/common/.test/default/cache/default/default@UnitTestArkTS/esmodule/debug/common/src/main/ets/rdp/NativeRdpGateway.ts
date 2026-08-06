import type { NodeContent } from "@ohos:arkui.node";
import rdpNative from "@normalized:Y&&&libentry.so&";
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
let bjccovmshb1i07 = new BjcCov({ version: "bjc v1.0.0", versionCode: 10000, path: "common/src/main/ets/rdp/NativeRdpGateway.ets", hash: "d658c00705fd2183c3d59c4cf2fcdb4c5b06f3c7de4b8b34ecda31131aa12e90", lineCnt: 103, count: 0, projectPath: "", functions: { 0: { name: "NativeRdpGateway.connect", count: 0, regions: { 0: { startLoc: { line: 59, col: 3 }, endLoc: { line: 61, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 60, col: 5 }, endLoc: { line: 61, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 0 }, 1: { name: "NativeRdpGateway.ensureXrdpServerStarted", count: 0, regions: { 0: { startLoc: { line: 63, col: 3 }, endLoc: { line: 65, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 64, col: 5 }, endLoc: { line: 65, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 1 }, 2: { name: "NativeRdpGateway.getXrdpServerDiagnostics", count: 0, regions: { 0: { startLoc: { line: 67, col: 3 }, endLoc: { line: 69, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 68, col: 5 }, endLoc: { line: 69, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 2 }, 3: { name: "NativeRdpGateway.bindImeHostWindow", count: 0, regions: { 0: { startLoc: { line: 71, col: 3 }, endLoc: { line: 73, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 72, col: 5 }, endLoc: { line: 73, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 3 }, 4: { name: "NativeRdpGateway.attachXComponentContent", count: 0, regions: { 0: { startLoc: { line: 75, col: 3 }, endLoc: { line: 77, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 76, col: 5 }, endLoc: { line: 77, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 4 }, 5: { name: "NativeRdpGateway.detachXComponentContent", count: 0, regions: { 0: { startLoc: { line: 79, col: 3 }, endLoc: { line: 81, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 80, col: 5 }, endLoc: { line: 81, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 5 }, 6: { name: "NativeRdpGateway.releaseAllInput", count: 0, regions: { 0: { startLoc: { line: 83, col: 3 }, endLoc: { line: 85, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 84, col: 5 }, endLoc: { line: 85, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 6 }, 7: { name: "NativeRdpGateway.onState", count: 0, regions: { 0: { startLoc: { line: 87, col: 3 }, endLoc: { line: 89, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 88, col: 5 }, endLoc: { line: 89, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 7 }, 8: { name: "anonymous_0", count: 0, regions: { 0: { startLoc: { line: 87, col: 28 }, endLoc: { line: 87, col: 51 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 8 }, 9: { name: "NativeRdpGateway.onError", count: 0, regions: { 0: { startLoc: { line: 91, col: 3 }, endLoc: { line: 93, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 92, col: 5 }, endLoc: { line: 93, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 9 }, 10: { name: "anonymous_1", count: 0, regions: { 0: { startLoc: { line: 91, col: 28 }, endLoc: { line: 91, col: 53 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 10 }, 11: { name: "NativeRdpGateway.onPermissionRequest", count: 0, regions: { 0: { startLoc: { line: 95, col: 3 }, endLoc: { line: 97, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 96, col: 5 }, endLoc: { line: 97, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 11 }, 12: { name: "anonymous_2", count: 0, regions: { 0: { startLoc: { line: 95, col: 40 }, endLoc: { line: 95, col: 82 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 12 }, 13: { name: "NativeRdpGateway.completePermissionRequest", count: 0, regions: { 0: { startLoc: { line: 99, col: 3 }, endLoc: { line: 101, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 100, col: 5 }, endLoc: { line: 101, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 13 } }, exeLine: { 0: 1, 1: 2, 2: 4, 3: 5, 4: 6, 5: 7, 6: 8, 7: 9, 8: 10, 9: 11, 10: 12, 11: 15, 12: 16, 13: 17, 14: 18, 15: 21, 16: 22, 17: 23, 18: 24, 19: 25, 20: 28, 21: 29, 22: 30, 23: 31, 24: 32, 25: 33, 26: 34, 27: 35, 28: 36, 29: 37, 30: 38, 31: 39, 32: 40, 33: 41, 34: 42, 35: 45, 36: 47, 37: 48, 38: 49, 39: 52, 40: 53, 41: 54, 42: 55, 43: 58, 44: 59, 45: 60, 46: 63, 47: 64, 48: 67, 49: 68, 50: 71, 51: 72, 52: 75, 53: 76, 54: 79, 55: 80, 56: 83, 57: 84, 58: 87, 59: 88, 60: 91, 61: 92, 62: 95, 63: 96, 64: 99, 65: 100 } });
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
export class NativeRdpGateway {
    static connect(params: NativeConnectParams): NativeCommandResult {
        bjccovmshb1i07.instrumentFunction(0);
        bjccovmshb1i07.instrumentRegion(0, 1);
        return rdpNative.connect(params);
    }
    static ensureXrdpServerStarted(params?: NativeXrdpServerParams): NativeXrdpServerResult {
        bjccovmshb1i07.instrumentFunction(1);
        bjccovmshb1i07.instrumentRegion(1, 1);
        return rdpNative.ensureXrdpServerStarted(params);
    }
    static getXrdpServerDiagnostics(): NativeXrdpServerResult {
        bjccovmshb1i07.instrumentFunction(2);
        bjccovmshb1i07.instrumentRegion(2, 1);
        return rdpNative.getXrdpServerDiagnostics();
    }
    static bindImeHostWindow(windowId: number): NativeCommandResult {
        bjccovmshb1i07.instrumentFunction(3);
        bjccovmshb1i07.instrumentRegion(3, 1);
        return rdpNative.bindImeHostWindow(windowId);
    }
    static attachXComponentContent(nodeContent: NodeContent): NativeCommandResult {
        bjccovmshb1i07.instrumentFunction(4);
        bjccovmshb1i07.instrumentRegion(4, 1);
        return rdpNative.attachXComponentContent(nodeContent);
    }
    static detachXComponentContent(): NativeCommandResult {
        bjccovmshb1i07.instrumentFunction(5);
        bjccovmshb1i07.instrumentRegion(5, 1);
        return rdpNative.detachXComponentContent();
    }
    static releaseAllInput(): NativeCommandResult {
        bjccovmshb1i07.instrumentFunction(6);
        bjccovmshb1i07.instrumentRegion(6, 1);
        return rdpNative.releaseAllInput();
    }
    static onState(callback: (state: string) => void): NativeCommandResult {
        bjccovmshb1i07.instrumentFunction(7);
        bjccovmshb1i07.instrumentRegion(7, 1);
        return rdpNative.onState(callback);
    }
    static onError(callback: (message: string) => void): NativeCommandResult {
        bjccovmshb1i07.instrumentFunction(9);
        bjccovmshb1i07.instrumentRegion(9, 1);
        return rdpNative.onError(callback);
    }
    static onPermissionRequest(callback: (request: NativePermissionRequest) => void): NativeCommandResult {
        bjccovmshb1i07.instrumentFunction(11);
        bjccovmshb1i07.instrumentRegion(11, 1);
        return rdpNative.onPermissionRequest(callback);
    }
    static completePermissionRequest(result: NativePermissionResult): NativeCommandResult {
        bjccovmshb1i07.instrumentFunction(13);
        bjccovmshb1i07.instrumentRegion(13, 1);
        return rdpNative.completePermissionRequest(result);
    }
}
