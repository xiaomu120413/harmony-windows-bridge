import { NativeRdpGateway } from "@normalized:N&&&common/src/main/ets/rdp/NativeRdpGateway&";
import type { NativeXrdpServerParams, NativeXrdpServerResult } from "@normalized:N&&&common/src/main/ets/rdp/NativeRdpGateway&";
import { RdpLogger } from "@normalized:N&&&common/src/main/ets/rdp/RdpLogger&";
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
let bjccovmshb1i3w = new BjcCov({ version: "bjc v1.0.0", versionCode: 10000, path: "common/src/main/ets/rdp/XrdpServerController.ets", hash: "4522a523178f2e5e7ea6ac8bc206f662f4f9a8477c97e3a28d86c6d7f080c4ef", lineCnt: 122, count: 0, projectPath: "", functions: { 0: { name: "XrdpServerController.constructor", count: 0, regions: { 0: { startLoc: { line: 35, col: 3 }, endLoc: { line: 38, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 36, col: 5 }, endLoc: { line: 38, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 0 }, 1: { name: "XrdpServerController.diagnostics", count: 0, regions: { 0: { startLoc: { line: 40, col: 3 }, endLoc: { line: 51, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 41, col: 26 }, endLoc: { line: 43, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 44, col: 9 }, endLoc: { line: 47, col: 6 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 47, col: 7 }, endLoc: { line: 50, col: 6 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 41, col: 9 }, endLoc: { line: 41, col: 24 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 1 }, 2: { name: "XrdpServerController.start", count: 0, regions: { 0: { startLoc: { line: 53, col: 3 }, endLoc: { line: 65, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 54, col: 26 }, endLoc: { line: 57, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 58, col: 9 }, endLoc: { line: 61, col: 6 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 61, col: 7 }, endLoc: { line: 64, col: 6 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 54, col: 9 }, endLoc: { line: 54, col: 24 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 2 }, 3: { name: "XrdpServerController.toNativeParams", count: 0, regions: { 0: { startLoc: { line: 67, col: 3 }, endLoc: { line: 74, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 68, col: 5 }, endLoc: { line: 74, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 3 }, 4: { name: "XrdpServerController.toStatus", count: 0, regions: { 0: { startLoc: { line: 76, col: 3 }, endLoc: { line: 83, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 77, col: 5 }, endLoc: { line: 83, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 80, col: 13 }, endLoc: { line: 80, col: 49 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 4 }, 5: { name: "XrdpServerController.displayMessage", count: 0, regions: { 0: { startLoc: { line: 85, col: 3 }, endLoc: { line: 102, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 86, col: 43 }, endLoc: { line: 88, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 89, col: 39 }, endLoc: { line: 91, col: 6 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 92, col: 37 }, endLoc: { line: 94, col: 6 }, count: 0, ignored: 0 }, 4: { startLoc: { line: 95, col: 36 }, endLoc: { line: 97, col: 6 }, count: 0, ignored: 0 }, 5: { startLoc: { line: 98, col: 36 }, endLoc: { line: 100, col: 6 }, count: 0, ignored: 0 }, 6: { startLoc: { line: 101, col: 5 }, endLoc: { line: 102, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 86, col: 9 }, endLoc: { line: 86, col: 41 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 89, col: 9 }, endLoc: { line: 89, col: 37 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 2: { startLoc: { line: 92, col: 9 }, endLoc: { line: 92, col: 35 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 3: { startLoc: { line: 95, col: 9 }, endLoc: { line: 95, col: 34 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 4: { startLoc: { line: 98, col: 9 }, endLoc: { line: 98, col: 34 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 5 }, 6: { name: "XrdpServerController.failedStatus", count: 0, regions: { 0: { startLoc: { line: 104, col: 3 }, endLoc: { line: 111, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 105, col: 5 }, endLoc: { line: 111, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 6 }, 7: { name: "XrdpServerController.unavailableStatus", count: 0, regions: { 0: { startLoc: { line: 113, col: 3 }, endLoc: { line: 120, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 114, col: 5 }, endLoc: { line: 120, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 7 } }, exeLine: { 0: 1, 1: 2, 2: 3, 3: 4, 4: 5, 5: 6, 6: 8, 7: 9, 8: 10, 9: 11, 10: 12, 11: 15, 12: 16, 13: 17, 14: 18, 15: 19, 16: 20, 17: 21, 18: 24, 19: 25, 20: 26, 21: 27, 22: 28, 23: 31, 24: 32, 25: 33, 26: 35, 27: 36, 28: 37, 29: 40, 30: 41, 31: 42, 32: 44, 33: 45, 34: 46, 35: 47, 36: 48, 37: 49, 38: 53, 39: 54, 40: 55, 41: 56, 42: 58, 43: 59, 44: 60, 45: 61, 46: 62, 47: 63, 48: 67, 49: 68, 50: 69, 51: 70, 52: 71, 53: 72, 54: 76, 55: 77, 56: 78, 57: 79, 58: 80, 59: 81, 60: 85, 61: 86, 62: 87, 63: 89, 64: 90, 65: 92, 66: 93, 67: 95, 68: 96, 69: 98, 70: 99, 71: 101, 72: 104, 73: 105, 74: 106, 75: 107, 76: 108, 77: 109, 78: 113, 79: 114, 80: 115, 81: 116, 82: 117, 83: 118 } });
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
export class XrdpServerController {
    private readonly messages: XrdpServerDisplayMessages;
    private readonly available: boolean;
    constructor(messages: XrdpServerDisplayMessages, available: boolean = true) {
        bjccovmshb1i3w.instrumentFunction(0);
        bjccovmshb1i3w.instrumentRegion(0, 1);
        this.messages = messages;
        this.available = available;
    }
    diagnostics(): XrdpServerStatus {
        bjccovmshb1i3w.instrumentFunction(1);
        if (!this.available) {
            bjccovmshb1i3w.instrumentBranch(1, 0, true);
            bjccovmshb1i3w.instrumentRegion(1, 1);
            return this.unavailableStatus();
        }
        else {
            bjccovmshb1i3w.instrumentBranch(1, 0, false);
        }
        try {
            bjccovmshb1i3w.instrumentRegion(1, 2);
            const result: NativeXrdpServerResult = NativeRdpGateway.getXrdpServerDiagnostics();
            return this.toStatus(result);
        }
        catch (error) {
            bjccovmshb1i3w.instrumentRegion(1, 3);
            RdpLogger.error(`xrdp diagnostics failed: ${JSON.stringify(error)}`);
            return this.failedStatus();
        }
    }
    start(options: XrdpServerStartOptions, trigger: string): XrdpServerStatus {
        bjccovmshb1i3w.instrumentFunction(2);
        if (!this.available) {
            bjccovmshb1i3w.instrumentBranch(2, 0, true);
            bjccovmshb1i3w.instrumentRegion(2, 1);
            RdpLogger.info(`xrdp start blocked by device capability: ${trigger}`);
            return this.unavailableStatus();
        }
        else {
            bjccovmshb1i3w.instrumentBranch(2, 0, false);
        }
        try {
            bjccovmshb1i3w.instrumentRegion(2, 2);
            const result: NativeXrdpServerResult = NativeRdpGateway.ensureXrdpServerStarted(this.toNativeParams(options));
            return this.toStatus(result);
        }
        catch (error) {
            bjccovmshb1i3w.instrumentRegion(2, 3);
            RdpLogger.error(`xrdp ensure start failed: ${JSON.stringify(error)}`);
            return this.diagnostics();
        }
    }
    private toNativeParams(options: XrdpServerStartOptions): NativeXrdpServerParams {
        bjccovmshb1i3w.instrumentFunction(3);
        bjccovmshb1i3w.instrumentRegion(3, 1);
        return {
            appFilesDir: options.appFilesDir,
            accessCode: options.accessCode,
            accessCodeGateEnabled: options.accessCodeGateEnabled,
            restartIfRunning: options.restartIfRunning
        };
    }
    private toStatus(result: NativeXrdpServerResult): XrdpServerStatus {
        bjccovmshb1i3w.instrumentFunction(4);
        bjccovmshb1i3w.instrumentRegion(4, 1);
        return {
            running: result.ok && result.state !== 'Stopped' && result.state !== 'Exited',
            state: result.state,
            port: result.port > 0 ? (bjccovmshb1i3w.instrumentBranch(4, 0, true), result.port) : (bjccovmshb1i3w.instrumentBranch(4, 0, false), 3390),
            message: this.displayMessage(result)
        };
    }
    private displayMessage(result: NativeXrdpServerResult): string {
        bjccovmshb1i3w.instrumentFunction(5);
        if (result.state === 'ActiveSession') {
            bjccovmshb1i3w.instrumentBranch(5, 0, true);
            bjccovmshb1i3w.instrumentRegion(5, 1);
            return this.messages.active;
        }
        else {
            bjccovmshb1i3w.instrumentBranch(5, 0, false);
        }
        if (result.state === 'Listening') {
            bjccovmshb1i3w.instrumentBranch(5, 1, true);
            bjccovmshb1i3w.instrumentRegion(5, 2);
            return this.messages.listening;
        }
        else {
            bjccovmshb1i3w.instrumentBranch(5, 1, false);
        }
        if (result.state === 'Stopped') {
            bjccovmshb1i3w.instrumentBranch(5, 2, true);
            bjccovmshb1i3w.instrumentRegion(5, 3);
            return this.messages.stopped;
        }
        else {
            bjccovmshb1i3w.instrumentBranch(5, 2, false);
        }
        if (result.state === 'Exited') {
            bjccovmshb1i3w.instrumentBranch(5, 3, true);
            bjccovmshb1i3w.instrumentRegion(5, 4);
            return this.messages.exited;
        }
        else {
            bjccovmshb1i3w.instrumentBranch(5, 3, false);
        }
        if (result.state === 'Failed') {
            bjccovmshb1i3w.instrumentBranch(5, 4, true);
            bjccovmshb1i3w.instrumentRegion(5, 5);
            return this.messages.failed;
        }
        else {
            bjccovmshb1i3w.instrumentBranch(5, 4, false);
        }
        bjccovmshb1i3w.instrumentRegion(5, 6);
        return result.message;
    }
    private failedStatus(): XrdpServerStatus {
        bjccovmshb1i3w.instrumentFunction(6);
        bjccovmshb1i3w.instrumentRegion(6, 1);
        return {
            running: false,
            state: 'Failed',
            port: 3390,
            message: this.messages.failed
        };
    }
    private unavailableStatus(): XrdpServerStatus {
        bjccovmshb1i3w.instrumentFunction(7);
        bjccovmshb1i3w.instrumentRegion(7, 1);
        return {
            running: false,
            state: 'Unavailable',
            port: 3390,
            message: this.messages.unavailable
        };
    }
}
