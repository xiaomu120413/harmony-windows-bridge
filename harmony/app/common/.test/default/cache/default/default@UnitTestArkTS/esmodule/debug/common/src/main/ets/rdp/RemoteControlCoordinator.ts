import { RdpLogger } from "@normalized:N&&&common/src/main/ets/rdp/RdpLogger&";
import type { RdpPermissionManager } from './RdpPermissions';
import { XrdpServerController } from "@normalized:N&&&common/src/main/ets/rdp/XrdpServerController&";
import type { XrdpServerDisplayMessages, XrdpServerStartOptions, XrdpServerStatus } from "@normalized:N&&&common/src/main/ets/rdp/XrdpServerController&";
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
let bjccovmshb1i2f = new BjcCov({ version: "bjc v1.0.0", versionCode: 10000, path: "common/src/main/ets/rdp/RemoteControlCoordinator.ets", hash: "76342e6f420d4536c581018b628318fe775895935a13a205e251037b2a938129", lineCnt: 304, count: 0, projectPath: "", functions: { 0: { name: "anonymous_0", count: 0, regions: { 0: { startLoc: { line: 26, col: 39 }, endLoc: { line: 26, col: 65 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 0 }, 1: { name: "anonymous_1", count: 0, regions: { 0: { startLoc: { line: 27, col: 33 }, endLoc: { line: 27, col: 45 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 1 }, 2: { name: "anonymous_2", count: 0, regions: { 0: { startLoc: { line: 28, col: 30 }, endLoc: { line: 28, col: 71 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 2 }, 3: { name: "RemoteControlCoordinator.constructor", count: 0, regions: { 0: { startLoc: { line: 44, col: 3 }, endLoc: { line: 52, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 47, col: 5 }, endLoc: { line: 52, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 3 }, 4: { name: "anonymous_3", count: 0, regions: { 0: { startLoc: { line: 45, col: 24 }, endLoc: { line: 45, col: 50 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 4 }, 5: { name: "anonymous_4", count: 0, regions: { 0: { startLoc: { line: 45, col: 65 }, endLoc: { line: 45, col: 77 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 5 }, 6: { name: "anonymous_5", count: 0, regions: { 0: { startLoc: { line: 46, col: 15 }, endLoc: { line: 46, col: 56 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 6 }, 7: { name: "RemoteControlCoordinator.snapshot", count: 0, regions: { 0: { startLoc: { line: 54, col: 3 }, endLoc: { line: 68, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 55, col: 5 }, endLoc: { line: 68, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 7 }, 8: { name: "RemoteControlCoordinator.permissionPromptOpened", count: 0, regions: { 0: { startLoc: { line: 70, col: 3 }, endLoc: { line: 73, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 71, col: 5 }, endLoc: { line: 73, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 8 }, 9: { name: "RemoteControlCoordinator.refreshPermissionState", count: 0, regions: { 0: { startLoc: { line: 75, col: 3 }, endLoc: { line: 85, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 76, col: 26 }, endLoc: { line: 81, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 82, col: 5 }, endLoc: { line: 85, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 76, col: 9 }, endLoc: { line: 76, col: 24 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 9 }, 10: { name: "RemoteControlCoordinator.refreshInputInjectionPermissionState", count: 0, regions: { 0: { startLoc: { line: 87, col: 3 }, endLoc: { line: 97, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 88, col: 26 }, endLoc: { line: 93, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 94, col: 5 }, endLoc: { line: 97, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 88, col: 9 }, endLoc: { line: 88, col: 24 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 10 }, 11: { name: "RemoteControlCoordinator.requestInputInjectionPermissionFromSettings", count: 0, regions: { 0: { startLoc: { line: 99, col: 3 }, endLoc: { line: 121, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 100, col: 63 }, endLoc: { line: 102, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 103, col: 5 }, endLoc: { line: 121, col: 4 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 109, col: 9 }, endLoc: { line: 111, col: 8 }, count: 0, ignored: 0 }, 4: { startLoc: { line: 114, col: 9 }, endLoc: { line: 116, col: 8 }, count: 0, ignored: 0 }, 5: { startLoc: { line: 119, col: 9 }, endLoc: { line: 120, col: 8 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 100, col: 9 }, endLoc: { line: 100, col: 61 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 11 }, 12: { name: "anonymous_6", count: 0, regions: { 0: { startLoc: { line: 106, col: 13 }, endLoc: { line: 111, col: 8 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 12 }, 13: { name: "anonymous_7", count: 0, regions: { 0: { startLoc: { line: 112, col: 14 }, endLoc: { line: 116, col: 8 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 13 }, 14: { name: "anonymous_8", count: 0, regions: { 0: { startLoc: { line: 117, col: 16 }, endLoc: { line: 120, col: 8 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 14 }, 15: { name: "RemoteControlCoordinator.refreshDiagnostics", count: 0, regions: { 0: { startLoc: { line: 123, col: 3 }, endLoc: { line: 125, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 124, col: 5 }, endLoc: { line: 125, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 15 }, 16: { name: "RemoteControlCoordinator.refreshReadiness", count: 0, regions: { 0: { startLoc: { line: 127, col: 3 }, endLoc: { line: 134, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 128, col: 20 }, endLoc: { line: 130, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 131, col: 5 }, endLoc: { line: 134, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 128, col: 9 }, endLoc: { line: 128, col: 18 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 16 }, 17: { name: "RemoteControlCoordinator.requestPermissionFromSettings", count: 0, regions: { 0: { startLoc: { line: 136, col: 3 }, endLoc: { line: 154, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 137, col: 26 }, endLoc: { line: 139, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 140, col: 5 }, endLoc: { line: 154, col: 4 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 146, col: 9 }, endLoc: { line: 148, col: 8 }, count: 0, ignored: 0 }, 4: { startLoc: { line: 151, col: 9 }, endLoc: { line: 152, col: 8 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 137, col: 9 }, endLoc: { line: 137, col: 24 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 17 }, 18: { name: "anonymous_9", count: 0, regions: { 0: { startLoc: { line: 141, col: 13 }, endLoc: { line: 148, col: 8 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 143, col: 23 }, endLoc: { line: 145, col: 10 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 143, col: 13 }, endLoc: { line: 143, col: 21 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 18 }, 19: { name: "anonymous_10", count: 0, regions: { 0: { startLoc: { line: 147, col: 17 }, endLoc: { line: 147, col: 89 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 19 }, 20: { name: "anonymous_11", count: 0, regions: { 0: { startLoc: { line: 149, col: 14 }, endLoc: { line: 152, col: 8 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 20 }, 21: { name: "anonymous_12", count: 0, regions: { 0: { startLoc: { line: 153, col: 16 }, endLoc: { line: 153, col: 57 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 21 }, 22: { name: "RemoteControlCoordinator.startFromSettings", count: 0, regions: { 0: { startLoc: { line: 156, col: 3 }, endLoc: { line: 158, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 157, col: 5 }, endLoc: { line: 158, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 22 }, 23: { name: "RemoteControlCoordinator.setAccessCodeGate", count: 0, regions: { 0: { startLoc: { line: 160, col: 3 }, endLoc: { line: 171, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 161, col: 68 }, endLoc: { line: 163, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 165, col: 18 }, endLoc: { line: 167, col: 6 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 168, col: 5 }, endLoc: { line: 171, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 161, col: 9 }, endLoc: { line: 161, col: 66 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 165, col: 9 }, endLoc: { line: 165, col: 16 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 2: { startLoc: { line: 169, col: 24 }, endLoc: { line: 169, col: 90 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 23 }, 24: { name: "RemoteControlCoordinator.regenerateAccessCode", count: 0, regions: { 0: { startLoc: { line: 173, col: 3 }, endLoc: { line: 181, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 177, col: 37 }, endLoc: { line: 179, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 180, col: 5 }, endLoc: { line: 181, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 177, col: 9 }, endLoc: { line: 177, col: 35 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 24 }, 25: { name: "RemoteControlCoordinator.ensureStarted", count: 0, regions: { 0: { startLoc: { line: 183, col: 3 }, endLoc: { line: 220, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 185, col: 26 }, endLoc: { line: 187, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 188, col: 58 }, endLoc: { line: 193, col: 6 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 189, col: 41 }, endLoc: { line: 191, col: 8 }, count: 0, ignored: 0 }, 4: { startLoc: { line: 194, col: 5 }, endLoc: { line: 220, col: 4 }, count: 0, ignored: 0 }, 5: { startLoc: { line: 192, col: 7 }, endLoc: { line: 193, col: 6 }, count: 0, ignored: 0 }, 6: { startLoc: { line: 206, col: 9 }, endLoc: { line: 207, col: 8 }, count: 0, ignored: 0 }, 7: { startLoc: { line: 204, col: 11 }, endLoc: { line: 205, col: 10 }, count: 0, ignored: 0 }, 8: { startLoc: { line: 210, col: 9 }, endLoc: { line: 212, col: 8 }, count: 0, ignored: 0 }, 9: { startLoc: { line: 215, col: 9 }, endLoc: { line: 218, col: 8 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 185, col: 9 }, endLoc: { line: 185, col: 24 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 188, col: 9 }, endLoc: { line: 188, col: 56 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 2: { startLoc: { line: 189, col: 11 }, endLoc: { line: 189, col: 39 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 3: { startLoc: { line: 196, col: 31 }, endLoc: { line: 198, col: 82 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 25 }, 26: { name: "anonymous_13", count: 0, regions: { 0: { startLoc: { line: 200, col: 13 }, endLoc: { line: 207, col: 8 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 202, col: 23 }, endLoc: { line: 205, col: 10 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 202, col: 13 }, endLoc: { line: 202, col: 21 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 26 }, 27: { name: "anonymous_14", count: 0, regions: { 0: { startLoc: { line: 208, col: 14 }, endLoc: { line: 212, col: 8 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 27 }, 28: { name: "anonymous_15", count: 0, regions: { 0: { startLoc: { line: 213, col: 16 }, endLoc: { line: 218, col: 8 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 28 }, 29: { name: "RemoteControlCoordinator.requestPermissionForRunningStart", count: 0, regions: { 0: { startLoc: { line: 222, col: 3 }, endLoc: { line: 232, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 223, col: 5 }, endLoc: { line: 232, col: 4 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 229, col: 9 }, endLoc: { line: 230, col: 8 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 29 }, 30: { name: "anonymous_16", count: 0, regions: { 0: { startLoc: { line: 224, col: 13 }, endLoc: { line: 226, col: 8 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 225, col: 9 }, endLoc: { line: 226, col: 8 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 30 }, 31: { name: "anonymous_17", count: 0, regions: { 0: { startLoc: { line: 227, col: 14 }, endLoc: { line: 230, col: 8 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 31 }, 32: { name: "anonymous_18", count: 0, regions: { 0: { startLoc: { line: 231, col: 16 }, endLoc: { line: 231, col: 57 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 32 }, 33: { name: "RemoteControlCoordinator.hasPermission", count: 0, regions: { 0: { startLoc: { line: 234, col: 3 }, endLoc: { line: 236, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 235, col: 5 }, endLoc: { line: 236, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 33 }, 34: { name: "RemoteControlCoordinator.applyPermissionResult", count: 0, regions: { 0: { startLoc: { line: 238, col: 3 }, endLoc: { line: 247, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 242, col: 32 }, endLoc: { line: 244, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 245, col: 5 }, endLoc: { line: 247, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 242, col: 9 }, endLoc: { line: 242, col: 30 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 34 }, 35: { name: "RemoteControlCoordinator.startAfterPermissions", count: 0, regions: { 0: { startLoc: { line: 249, col: 3 }, endLoc: { line: 251, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 250, col: 5 }, endLoc: { line: 251, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 35 }, 36: { name: "RemoteControlCoordinator.startOptions", count: 0, regions: { 0: { startLoc: { line: 253, col: 3 }, endLoc: { line: 260, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 254, col: 5 }, endLoc: { line: 260, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 256, col: 19 }, endLoc: { line: 256, col: 76 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 36 }, 37: { name: "RemoteControlCoordinator.ensureAccessCode", count: 0, regions: { 0: { startLoc: { line: 262, col: 3 }, endLoc: { line: 269, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 263, col: 70 }, endLoc: { line: 267, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 268, col: 5 }, endLoc: { line: 269, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 263, col: 9 }, endLoc: { line: 263, col: 68 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 37 }, 38: { name: "RemoteControlCoordinator.applyStatus", count: 0, regions: { 0: { startLoc: { line: 271, col: 3 }, endLoc: { line: 278, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 272, col: 5 }, endLoc: { line: 278, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 38 }, 39: { name: "RemoteControlCoordinator.setPermissionBusy", count: 0, regions: { 0: { startLoc: { line: 280, col: 3 }, endLoc: { line: 286, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 282, col: 15 }, endLoc: { line: 284, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 285, col: 5 }, endLoc: { line: 286, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 282, col: 9 }, endLoc: { line: 282, col: 13 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 39 }, 40: { name: "RemoteControlCoordinator.clearServerBusy", count: 0, regions: { 0: { startLoc: { line: 288, col: 3 }, endLoc: { line: 294, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 289, col: 26 }, endLoc: { line: 291, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 292, col: 5 }, endLoc: { line: 294, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 289, col: 9 }, endLoc: { line: 289, col: 24 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 40 }, 41: { name: "RemoteControlCoordinator.publish", count: 0, regions: { 0: { startLoc: { line: 296, col: 3 }, endLoc: { line: 298, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 297, col: 5 }, endLoc: { line: 298, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 41 }, 42: { name: "RemoteControlCoordinator.generateAccessCode", count: 0, regions: { 0: { startLoc: { line: 300, col: 3 }, endLoc: { line: 302, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 301, col: 5 }, endLoc: { line: 302, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 42 } }, exeLine: { 0: 1, 1: 2, 2: 3, 3: 4, 4: 5, 5: 6, 6: 7, 7: 8, 8: 10, 9: 11, 10: 12, 11: 13, 12: 14, 13: 15, 14: 16, 15: 17, 16: 18, 17: 19, 18: 20, 19: 21, 20: 24, 21: 25, 22: 26, 23: 27, 24: 28, 25: 29, 26: 30, 27: 31, 28: 32, 29: 33, 30: 34, 31: 35, 32: 36, 33: 37, 34: 38, 35: 39, 36: 40, 37: 41, 38: 42, 39: 44, 40: 45, 41: 46, 42: 47, 43: 48, 44: 49, 45: 50, 46: 51, 47: 54, 48: 55, 49: 56, 50: 57, 51: 58, 52: 59, 53: 60, 54: 61, 55: 62, 56: 63, 57: 64, 58: 65, 59: 66, 60: 70, 61: 71, 62: 72, 63: 75, 64: 76, 65: 77, 66: 78, 67: 79, 68: 80, 69: 82, 70: 83, 71: 84, 72: 87, 73: 88, 74: 89, 75: 90, 76: 91, 77: 92, 78: 94, 79: 95, 80: 96, 81: 99, 82: 100, 83: 101, 84: 103, 85: 104, 86: 105, 87: 106, 88: 107, 89: 108, 90: 109, 91: 110, 92: 112, 93: 113, 94: 114, 95: 115, 96: 117, 97: 118, 98: 119, 99: 123, 100: 124, 101: 127, 102: 128, 103: 129, 104: 131, 105: 132, 106: 133, 107: 136, 108: 137, 109: 138, 110: 140, 111: 141, 112: 142, 113: 143, 114: 144, 115: 146, 116: 147, 117: 149, 118: 150, 119: 151, 120: 153, 121: 156, 122: 157, 123: 160, 124: 161, 125: 162, 126: 164, 127: 165, 128: 166, 129: 168, 130: 169, 131: 170, 132: 173, 133: 174, 134: 175, 135: 176, 136: 177, 137: 178, 138: 180, 139: 183, 140: 184, 141: 185, 142: 186, 143: 188, 144: 189, 145: 190, 146: 192, 147: 194, 148: 195, 149: 196, 150: 197, 151: 198, 152: 199, 153: 200, 154: 201, 155: 202, 156: 203, 157: 204, 158: 206, 159: 208, 160: 209, 161: 210, 162: 211, 163: 213, 164: 214, 165: 215, 166: 216, 167: 217, 168: 219, 169: 222, 170: 223, 171: 224, 172: 225, 173: 227, 174: 228, 175: 229, 176: 231, 177: 234, 178: 235, 179: 238, 180: 239, 181: 240, 182: 241, 183: 242, 184: 243, 185: 245, 186: 246, 187: 249, 188: 250, 189: 253, 190: 254, 191: 255, 192: 256, 193: 257, 194: 258, 195: 262, 196: 263, 197: 264, 198: 265, 199: 266, 200: 268, 201: 271, 202: 272, 203: 273, 204: 274, 205: 275, 206: 276, 207: 277, 208: 280, 209: 281, 210: 282, 211: 283, 212: 285, 213: 288, 214: 289, 215: 290, 216: 292, 217: 293, 218: 296, 219: 297, 220: 300, 221: 301 } });
export interface RemoteControlSnapshot {
    accessCode: string;
    accessCodeGateEnabled: boolean;
    screenRecordingPermissionGranted: boolean;
    screenRecordingPermissionBusy: boolean;
    inputInjectionPermissionGranted: boolean;
    inputInjectionPermissionBusy: boolean;
    serverRunning: boolean;
    serverState: string;
    serverPort: number;
    serverMessage: string;
    serverBusy: boolean;
}
export class RemoteControlCoordinator {
    private readonly available: boolean;
    private readonly permissionManager: () => RdpPermissionManager;
    private readonly appFilesDir: () => string;
    private readonly onChange: (snapshot: RemoteControlSnapshot) => void;
    private readonly serverController: XrdpServerController;
    private startRequest: Promise<XrdpServerStatus> | null = null;
    private accessCode: string = '000000';
    private accessCodeGateEnabled: boolean = false;
    private accessCodeInitialized: boolean = false;
    private permissionGranted: boolean = false;
    private permissionBusy: boolean = false;
    private inputInjectionPermissionGranted: boolean = false;
    private inputInjectionPermissionBusy: boolean = false;
    private serverRunning: boolean = false;
    private serverState: string = 'Stopped';
    private serverPort: number = 3390;
    private serverMessage: string = '';
    private serverBusy: boolean = false;
    constructor(available: boolean, messages: XrdpServerDisplayMessages, permissionManager: () => RdpPermissionManager, appFilesDir: () => string, onChange: (snapshot: RemoteControlSnapshot) => void) {
        bjccovmshb1i2f.instrumentFunction(3);
        bjccovmshb1i2f.instrumentRegion(3, 1);
        this.available = available;
        this.permissionManager = permissionManager;
        this.appFilesDir = appFilesDir;
        this.onChange = onChange;
        this.serverController = new XrdpServerController(messages, available);
    }
    snapshot(): RemoteControlSnapshot {
        bjccovmshb1i2f.instrumentFunction(7);
        bjccovmshb1i2f.instrumentRegion(7, 1);
        return {
            accessCode: this.accessCode,
            accessCodeGateEnabled: this.accessCodeGateEnabled,
            screenRecordingPermissionGranted: this.permissionGranted,
            screenRecordingPermissionBusy: this.permissionBusy,
            inputInjectionPermissionGranted: this.inputInjectionPermissionGranted,
            inputInjectionPermissionBusy: this.inputInjectionPermissionBusy,
            serverRunning: this.serverRunning,
            serverState: this.serverState,
            serverPort: this.serverPort,
            serverMessage: this.serverMessage,
            serverBusy: this.serverBusy
        };
    }
    permissionPromptOpened(): void {
        bjccovmshb1i2f.instrumentFunction(8);
        bjccovmshb1i2f.instrumentRegion(8, 1);
        RdpLogger.info('screen recording permission prompt opened');
        this.setPermissionBusy(true);
    }
    refreshPermissionState(): boolean {
        bjccovmshb1i2f.instrumentFunction(9);
        if (!this.available) {
            bjccovmshb1i2f.instrumentBranch(9, 0, true);
            bjccovmshb1i2f.instrumentRegion(9, 1);
            this.permissionGranted = false;
            this.permissionBusy = false;
            this.publish();
            return false;
        }
        else {
            bjccovmshb1i2f.instrumentBranch(9, 0, false);
        }
        bjccovmshb1i2f.instrumentRegion(9, 2);
        this.permissionGranted = this.hasPermission();
        this.publish();
        return this.permissionGranted;
    }
    refreshInputInjectionPermissionState(): boolean {
        bjccovmshb1i2f.instrumentFunction(10);
        if (!this.available) {
            bjccovmshb1i2f.instrumentBranch(10, 0, true);
            bjccovmshb1i2f.instrumentRegion(10, 1);
            this.inputInjectionPermissionGranted = false;
            this.inputInjectionPermissionBusy = false;
            this.publish();
            return false;
        }
        else {
            bjccovmshb1i2f.instrumentBranch(10, 0, false);
        }
        bjccovmshb1i2f.instrumentRegion(10, 2);
        this.inputInjectionPermissionGranted = this.permissionManager().hasControlDevicePermission();
        this.publish();
        return this.inputInjectionPermissionGranted;
    }
    requestInputInjectionPermissionFromSettings(): Promise<boolean> {
        bjccovmshb1i2f.instrumentFunction(11);
        if (!this.available || this.inputInjectionPermissionBusy) {
            bjccovmshb1i2f.instrumentBranch(11, 0, true);
            bjccovmshb1i2f.instrumentRegion(11, 1);
            return Promise.resolve(false);
        }
        else {
            bjccovmshb1i2f.instrumentBranch(11, 0, false);
        }
        bjccovmshb1i2f.instrumentRegion(11, 2);
        this.inputInjectionPermissionBusy = true;
        this.publish();
        return this.permissionManager().requestControlDevicePermission('remote control settings')
            .then((granted: boolean): boolean => {
            bjccovmshb1i2f.instrumentFunction(12);
            this.inputInjectionPermissionGranted = granted ||
                this.permissionManager().hasControlDevicePermission();
            bjccovmshb1i2f.instrumentRegion(11, 3);
            RdpLogger.info(`input injection permission result applied: allowed=${this.inputInjectionPermissionGranted}`);
            return this.inputInjectionPermissionGranted;
        })
            .catch((error: Error): boolean => {
            bjccovmshb1i2f.instrumentFunction(13);
            RdpLogger.error(`input injection permission request failed: ${JSON.stringify(error)}`);
            bjccovmshb1i2f.instrumentRegion(11, 4);
            this.inputInjectionPermissionGranted = this.permissionManager().hasControlDevicePermission();
            return this.inputInjectionPermissionGranted;
        })
            .finally((): void => {
            bjccovmshb1i2f.instrumentFunction(14);
            this.inputInjectionPermissionBusy = false;
            bjccovmshb1i2f.instrumentRegion(11, 5);
            this.publish();
        });
    }
    refreshDiagnostics(): XrdpServerStatus {
        bjccovmshb1i2f.instrumentFunction(15);
        bjccovmshb1i2f.instrumentRegion(15, 1);
        return this.applyStatus(this.serverController.diagnostics());
    }
    refreshReadiness(clearBusy: boolean = false): XrdpServerStatus {
        bjccovmshb1i2f.instrumentFunction(16);
        if (clearBusy) {
            bjccovmshb1i2f.instrumentBranch(16, 0, true);
            bjccovmshb1i2f.instrumentRegion(16, 1);
            this.clearServerBusy('remote control readiness refreshed');
        }
        else {
            bjccovmshb1i2f.instrumentBranch(16, 0, false);
        }
        bjccovmshb1i2f.instrumentRegion(16, 2);
        this.refreshPermissionState();
        this.refreshInputInjectionPermissionState();
        return this.refreshDiagnostics();
    }
    requestPermissionFromSettings(): Promise<boolean> {
        bjccovmshb1i2f.instrumentFunction(17);
        if (!this.available) {
            bjccovmshb1i2f.instrumentBranch(17, 0, true);
            bjccovmshb1i2f.instrumentRegion(17, 1);
            return Promise.resolve(false);
        }
        else {
            bjccovmshb1i2f.instrumentBranch(17, 0, false);
        }
        bjccovmshb1i2f.instrumentRegion(17, 2);
        return this.permissionManager().requestScreenRecordingPermission('remote control settings')
            .then((granted: boolean): Promise<boolean> | boolean => {
            bjccovmshb1i2f.instrumentFunction(18);
            const allowed = this.applyPermissionResult(granted, !granted);
            if (!allowed) {
                bjccovmshb1i2f.instrumentBranch(18, 0, true);
                bjccovmshb1i2f.instrumentRegion(18, 1);
                return false;
            }
            else {
                bjccovmshb1i2f.instrumentBranch(18, 0, false);
            }
            bjccovmshb1i2f.instrumentRegion(17, 3);
            return this.ensureStarted('screen recording permission granted')
                .then((_status: XrdpServerStatus): boolean => { bjccovmshb1i2f.instrumentFunction(19); return this.applyPermissionResult(true); });
        })
            .catch((error: Error): boolean => {
            bjccovmshb1i2f.instrumentFunction(20);
            RdpLogger.error(`screen recording permission request from settings failed: ${JSON.stringify(error)}`);
            bjccovmshb1i2f.instrumentRegion(17, 4);
            return this.applyPermissionResult(false, true);
        })
            .finally((): void => { bjccovmshb1i2f.instrumentFunction(21); return this.setPermissionBusy(false); });
    }
    startFromSettings(): Promise<XrdpServerStatus> {
        bjccovmshb1i2f.instrumentFunction(22);
        bjccovmshb1i2f.instrumentRegion(22, 1);
        return this.ensureStarted('remote control settings button', false, true);
    }
    setAccessCodeGate(enabled: boolean): string {
        bjccovmshb1i2f.instrumentFunction(23);
        if (!this.available || this.accessCodeGateEnabled === enabled) {
            bjccovmshb1i2f.instrumentBranch(23, 0, true);
            bjccovmshb1i2f.instrumentRegion(23, 1);
            return this.accessCode;
        }
        else {
            bjccovmshb1i2f.instrumentBranch(23, 0, false);
        }
        this.accessCodeGateEnabled = enabled;
        if (enabled) {
            bjccovmshb1i2f.instrumentBranch(23, 1, true);
            bjccovmshb1i2f.instrumentRegion(23, 2);
            this.ensureAccessCode();
        }
        else {
            bjccovmshb1i2f.instrumentBranch(23, 1, false);
        }
        bjccovmshb1i2f.instrumentRegion(23, 3);
        this.publish();
        this.ensureStarted(enabled ? (bjccovmshb1i2f.instrumentBranch(23, 2, true), 'access code gate enabled') : (bjccovmshb1i2f.instrumentBranch(23, 2, false), 'access code gate disabled'), true);
        return this.accessCode;
    }
    regenerateAccessCode(): string {
        bjccovmshb1i2f.instrumentFunction(24);
        this.accessCode = RemoteControlCoordinator.generateAccessCode();
        this.accessCodeInitialized = true;
        this.publish();
        if (this.accessCodeGateEnabled) {
            bjccovmshb1i2f.instrumentBranch(24, 0, true);
            bjccovmshb1i2f.instrumentRegion(24, 1);
            this.ensureStarted('access code regenerated', true);
        }
        else {
            bjccovmshb1i2f.instrumentBranch(24, 0, false);
        }
        bjccovmshb1i2f.instrumentRegion(24, 2);
        return this.accessCode;
    }
    ensureStarted(trigger: string, restartIfRunning: boolean = false, requestPermissionImmediately: boolean = false): Promise<XrdpServerStatus> {
        bjccovmshb1i2f.instrumentFunction(25);
        if (!this.available) {
            bjccovmshb1i2f.instrumentBranch(25, 0, true);
            bjccovmshb1i2f.instrumentRegion(25, 1);
            return Promise.resolve(this.refreshDiagnostics());
        }
        else {
            bjccovmshb1i2f.instrumentBranch(25, 0, false);
        }
        if (this.startRequest !== null && !restartIfRunning) {
            bjccovmshb1i2f.instrumentBranch(25, 1, true);
            bjccovmshb1i2f.instrumentRegion(25, 2);
            if (requestPermissionImmediately) {
                bjccovmshb1i2f.instrumentBranch(25, 2, true);
                bjccovmshb1i2f.instrumentRegion(25, 3);
                this.requestPermissionForRunningStart(trigger);
            }
            else {
                bjccovmshb1i2f.instrumentBranch(25, 2, false);
            }
            bjccovmshb1i2f.instrumentRegion(25, 5);
            return this.startRequest;
        }
        else {
            bjccovmshb1i2f.instrumentBranch(25, 1, false);
        }
        bjccovmshb1i2f.instrumentRegion(25, 4);
        this.serverBusy = this.hasPermission();
        this.publish();
        const permissionRequest = requestPermissionImmediately ? (bjccovmshb1i2f.instrumentBranch(25, 3, true), this.permissionManager().requestScreenRecordingPermission(`xrdp ${trigger}`)) : (bjccovmshb1i2f.instrumentBranch(25, 3, false), this.permissionManager().ensureScreenRecordingPermission(`xrdp ${trigger}`));
        this.startRequest = permissionRequest
            .then((granted: boolean): XrdpServerStatus => {
            bjccovmshb1i2f.instrumentFunction(26);
            const allowed = this.applyPermissionResult(granted, !granted);
            if (!allowed) {
                bjccovmshb1i2f.instrumentBranch(26, 0, true);
                bjccovmshb1i2f.instrumentRegion(26, 1);
                RdpLogger.warn(`xrdp start skipped: screen recording permission denied: ${trigger}`);
                bjccovmshb1i2f.instrumentRegion(25, 7);
                return this.refreshReadiness(true);
            }
            else {
                bjccovmshb1i2f.instrumentBranch(26, 0, false);
            }
            bjccovmshb1i2f.instrumentRegion(25, 6);
            return this.startAfterPermissions(trigger, restartIfRunning);
        })
            .catch((error: Error): XrdpServerStatus => {
            bjccovmshb1i2f.instrumentFunction(27);
            RdpLogger.error(`xrdp permission check failed: ${JSON.stringify(error)}`);
            bjccovmshb1i2f.instrumentRegion(25, 8);
            this.applyPermissionResult(false, true);
            return this.refreshDiagnostics();
        })
            .finally((): void => {
            bjccovmshb1i2f.instrumentFunction(28);
            this.startRequest = null;
            bjccovmshb1i2f.instrumentRegion(25, 9);
            this.clearServerBusy('xrdp start request finished');
            this.setPermissionBusy(false);
            this.refreshReadiness();
        });
        return this.startRequest;
    }
    private requestPermissionForRunningStart(trigger: string): void {
        bjccovmshb1i2f.instrumentFunction(29);
        bjccovmshb1i2f.instrumentRegion(29, 1);
        this.permissionManager().requestScreenRecordingPermission(`xrdp ${trigger}`)
            .then((granted: boolean): void => {
            bjccovmshb1i2f.instrumentFunction(30);
            bjccovmshb1i2f.instrumentRegion(30, 1);
            this.applyPermissionResult(granted, !granted);
        })
            .catch((error: Error): void => {
            bjccovmshb1i2f.instrumentFunction(31);
            RdpLogger.error(`xrdp immediate permission request failed: ${JSON.stringify(error)}`);
            bjccovmshb1i2f.instrumentRegion(29, 2);
            this.applyPermissionResult(false, true);
        })
            .finally((): void => { bjccovmshb1i2f.instrumentFunction(32); return this.setPermissionBusy(false); });
    }
    private hasPermission(): boolean {
        bjccovmshb1i2f.instrumentFunction(33);
        bjccovmshb1i2f.instrumentRegion(33, 1);
        return this.available && this.permissionManager().hasScreenRecordingPermission();
    }
    private applyPermissionResult(granted: boolean, clearBusy: boolean = false): boolean {
        bjccovmshb1i2f.instrumentFunction(34);
        const allowed = granted || this.hasPermission();
        RdpLogger.info(`screen recording permission result applied: granted=${granted} allowed=${allowed}`);
        this.permissionGranted = allowed;
        if (clearBusy || !allowed) {
            bjccovmshb1i2f.instrumentBranch(34, 0, true);
            bjccovmshb1i2f.instrumentRegion(34, 1);
            this.clearServerBusy('screen recording permission not granted');
        }
        else {
            bjccovmshb1i2f.instrumentBranch(34, 0, false);
        }
        bjccovmshb1i2f.instrumentRegion(34, 2);
        this.refreshDiagnostics();
        return allowed;
    }
    private startAfterPermissions(trigger: string, restartIfRunning: boolean): XrdpServerStatus {
        bjccovmshb1i2f.instrumentFunction(35);
        bjccovmshb1i2f.instrumentRegion(35, 1);
        return this.applyStatus(this.serverController.start(this.startOptions(restartIfRunning), trigger));
    }
    private startOptions(restartIfRunning: boolean): XrdpServerStartOptions {
        bjccovmshb1i2f.instrumentFunction(36);
        bjccovmshb1i2f.instrumentRegion(36, 1);
        return {
            appFilesDir: this.appFilesDir(),
            accessCode: this.accessCodeGateEnabled ? (bjccovmshb1i2f.instrumentBranch(36, 0, true), this.ensureAccessCode()) : (bjccovmshb1i2f.instrumentBranch(36, 0, false), ''),
            accessCodeGateEnabled: this.accessCodeGateEnabled,
            restartIfRunning: restartIfRunning
        };
    }
    private ensureAccessCode(): string {
        bjccovmshb1i2f.instrumentFunction(37);
        if (!this.accessCodeInitialized || this.accessCode === '000000') {
            bjccovmshb1i2f.instrumentBranch(37, 0, true);
            bjccovmshb1i2f.instrumentRegion(37, 1);
            this.accessCode = RemoteControlCoordinator.generateAccessCode();
            this.accessCodeInitialized = true;
            this.publish();
        }
        else {
            bjccovmshb1i2f.instrumentBranch(37, 0, false);
        }
        bjccovmshb1i2f.instrumentRegion(37, 2);
        return this.accessCode;
    }
    private applyStatus(status: XrdpServerStatus): XrdpServerStatus {
        bjccovmshb1i2f.instrumentFunction(38);
        bjccovmshb1i2f.instrumentRegion(38, 1);
        this.serverRunning = status.running;
        this.serverState = status.state;
        this.serverPort = status.port;
        this.serverMessage = status.message;
        this.publish();
        return status;
    }
    private setPermissionBusy(busy: boolean): void {
        bjccovmshb1i2f.instrumentFunction(39);
        this.permissionBusy = busy;
        if (busy) {
            bjccovmshb1i2f.instrumentBranch(39, 0, true);
            bjccovmshb1i2f.instrumentRegion(39, 1);
            this.serverBusy = true;
        }
        else {
            bjccovmshb1i2f.instrumentBranch(39, 0, false);
        }
        bjccovmshb1i2f.instrumentRegion(39, 2);
        this.publish();
    }
    private clearServerBusy(reason: string): void {
        bjccovmshb1i2f.instrumentFunction(40);
        if (this.serverBusy) {
            bjccovmshb1i2f.instrumentBranch(40, 0, true);
            bjccovmshb1i2f.instrumentRegion(40, 1);
            RdpLogger.info(`xrdp busy cleared: ${reason}`);
        }
        else {
            bjccovmshb1i2f.instrumentBranch(40, 0, false);
        }
        bjccovmshb1i2f.instrumentRegion(40, 2);
        this.serverBusy = false;
        this.publish();
    }
    private publish(): void {
        bjccovmshb1i2f.instrumentFunction(41);
        bjccovmshb1i2f.instrumentRegion(41, 1);
        this.onChange(this.snapshot());
    }
    private static generateAccessCode(): string {
        bjccovmshb1i2f.instrumentFunction(42);
        bjccovmshb1i2f.instrumentRegion(42, 1);
        return `${Math.floor(100000 + Math.random() * 900000)}`;
    }
}
