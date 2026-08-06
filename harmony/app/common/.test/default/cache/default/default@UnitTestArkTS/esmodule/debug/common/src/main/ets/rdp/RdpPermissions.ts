import abilityAccessCtrl from "@ohos:abilityAccessCtrl";
import bundleManager from "@ohos:bundle.bundleManager";
import type common from "@ohos:app.ability.common";
import type { Permissions } from "@ohos:abilityAccessCtrl";
import { APPROXIMATE_LOCATION_PERMISSION, CAMERA_PERMISSION, CONTROL_DEVICE_PERMISSION, LOCATION_PERMISSION, MICROPHONE_PERMISSION, READ_PASTEBOARD_PERMISSION, SCREEN_RECORDING_PERMISSION } from "@normalized:N&&&common/src/main/ets/rdp/RdpConstants&";
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
let bjccovmshb1i1d = new BjcCov({ version: "bjc v1.0.0", versionCode: 10000, path: "common/src/main/ets/rdp/RdpPermissions.ets", hash: "1e45aab627a2dfd7f56f2624516d5b68ac579c7c01800d234cd77f50918f8dba", lineCnt: 351, count: 0, projectPath: "", functions: { 0: { name: "getContext", count: 0, regions: { 0: { startLoc: { line: 14, col: 3 }, endLoc: { line: 14, col: 47 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 0 }, 1: { name: "anonymous_0", count: 0, regions: { 0: { startLoc: { line: 18, col: 40 }, endLoc: { line: 18, col: 76 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 1 }, 2: { name: "RdpPermissionCallbackDelegate.constructor", count: 0, regions: { 0: { startLoc: { line: 20, col: 3 }, endLoc: { line: 22, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 21, col: 5 }, endLoc: { line: 22, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 2 }, 3: { name: "anonymous_1", count: 0, regions: { 0: { startLoc: { line: 20, col: 27 }, endLoc: { line: 20, col: 63 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 3 }, 4: { name: "RdpPermissionCallbackDelegate.getContext", count: 0, regions: { 0: { startLoc: { line: 24, col: 3 }, endLoc: { line: 26, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 25, col: 5 }, endLoc: { line: 26, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 4 }, 5: { name: "anonymous_2", count: 0, regions: { 0: { startLoc: { line: 34, col: 31 }, endLoc: { line: 34, col: 44 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 5 }, 6: { name: "anonymous_3", count: 0, regions: { 0: { startLoc: { line: 35, col: 33 }, endLoc: { line: 35, col: 87 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 6 }, 7: { name: "anonymous_4", count: 0, regions: { 0: { startLoc: { line: 36, col: 34 }, endLoc: { line: 36, col: 60 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 7 }, 8: { name: "anonymous_5", count: 0, regions: { 0: { startLoc: { line: 37, col: 39 }, endLoc: { line: 37, col: 49 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 8 }, 9: { name: "RdpPermissionManager.constructor", count: 0, regions: { 0: { startLoc: { line: 55, col: 3 }, endLoc: { line: 59, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 57, col: 5 }, endLoc: { line: 59, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 9 }, 10: { name: "anonymous_6", count: 0, regions: { 0: { startLoc: { line: 55, col: 120 }, endLoc: { line: 56, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 10 }, 11: { name: "RdpPermissionManager.ensurePasteboardReadPermission", count: 0, regions: { 0: { startLoc: { line: 61, col: 3 }, endLoc: { line: 72, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 62, col: 5 }, endLoc: { line: 72, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 11 }, 12: { name: "anonymous_7", count: 0, regions: { 0: { startLoc: { line: 66, col: 7 }, endLoc: { line: 66, col: 82 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 12 }, 13: { name: "anonymous_8", count: 0, regions: { 0: { startLoc: { line: 67, col: 7 }, endLoc: { line: 69, col: 87 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 13 }, 14: { name: "anonymous_9", count: 0, regions: { 0: { startLoc: { line: 69, col: 11 }, endLoc: { line: 69, col: 86 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 14 }, 15: { name: "RdpPermissionManager.ensureMicrophonePermission", count: 0, regions: { 0: { startLoc: { line: 74, col: 3 }, endLoc: { line: 85, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 75, col: 5 }, endLoc: { line: 85, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 15 }, 16: { name: "anonymous_10", count: 0, regions: { 0: { startLoc: { line: 79, col: 7 }, endLoc: { line: 79, col: 77 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 16 }, 17: { name: "anonymous_11", count: 0, regions: { 0: { startLoc: { line: 80, col: 7 }, endLoc: { line: 82, col: 82 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 17 }, 18: { name: "anonymous_12", count: 0, regions: { 0: { startLoc: { line: 82, col: 11 }, endLoc: { line: 82, col: 81 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 18 }, 19: { name: "RdpPermissionManager.ensureCameraPermission", count: 0, regions: { 0: { startLoc: { line: 87, col: 3 }, endLoc: { line: 98, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 88, col: 5 }, endLoc: { line: 98, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 19 }, 20: { name: "anonymous_13", count: 0, regions: { 0: { startLoc: { line: 92, col: 7 }, endLoc: { line: 92, col: 69 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 20 }, 21: { name: "anonymous_14", count: 0, regions: { 0: { startLoc: { line: 93, col: 7 }, endLoc: { line: 95, col: 74 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 21 }, 22: { name: "anonymous_15", count: 0, regions: { 0: { startLoc: { line: 95, col: 11 }, endLoc: { line: 95, col: 73 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 22 }, 23: { name: "RdpPermissionManager.ensureLocationPermission", count: 0, regions: { 0: { startLoc: { line: 100, col: 3 }, endLoc: { line: 112, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 101, col: 5 }, endLoc: { line: 112, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 23 }, 24: { name: "anonymous_16", count: 0, regions: { 0: { startLoc: { line: 106, col: 7 }, endLoc: { line: 106, col: 50 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 24 }, 25: { name: "anonymous_17", count: 0, regions: { 0: { startLoc: { line: 107, col: 7 }, endLoc: { line: 109, col: 55 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 25 }, 26: { name: "anonymous_18", count: 0, regions: { 0: { startLoc: { line: 109, col: 11 }, endLoc: { line: 109, col: 54 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 26 }, 27: { name: "RdpPermissionManager.ensureScreenRecordingPermission", count: 0, regions: { 0: { startLoc: { line: 114, col: 3 }, endLoc: { line: 124, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 115, col: 5 }, endLoc: { line: 124, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 27 }, 28: { name: "anonymous_19", count: 0, regions: { 0: { startLoc: { line: 119, col: 7 }, endLoc: { line: 119, col: 57 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 28 }, 29: { name: "anonymous_20", count: 0, regions: { 0: { startLoc: { line: 120, col: 7 }, endLoc: { line: 121, col: 71 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 29 }, 30: { name: "RdpPermissionManager.requestScreenRecordingPermission", count: 0, regions: { 0: { startLoc: { line: 126, col: 3 }, endLoc: { line: 144, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 127, col: 46 }, endLoc: { line: 129, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 131, col: 64 }, endLoc: { line: 134, col: 6 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 135, col: 5 }, endLoc: { line: 144, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 127, col: 9 }, endLoc: { line: 127, col: 44 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 130, col: 9 }, endLoc: { line: 131, col: 62 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 30 }, 31: { name: "anonymous_21", count: 0, regions: { 0: { startLoc: { line: 139, col: 7 }, endLoc: { line: 139, col: 57 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 31 }, 32: { name: "anonymous_22", count: 0, regions: { 0: { startLoc: { line: 140, col: 7 }, endLoc: { line: 141, col: 61 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 32 }, 33: { name: "RdpPermissionManager.hasScreenRecordingPermission", count: 0, regions: { 0: { startLoc: { line: 146, col: 3 }, endLoc: { line: 148, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 147, col: 5 }, endLoc: { line: 148, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 33 }, 34: { name: "RdpPermissionManager.requestControlDevicePermission", count: 0, regions: { 0: { startLoc: { line: 150, col: 3 }, endLoc: { line: 160, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 151, col: 5 }, endLoc: { line: 160, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 34 }, 35: { name: "anonymous_23", count: 0, regions: { 0: { startLoc: { line: 155, col: 7 }, endLoc: { line: 155, col: 55 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 35 }, 36: { name: "anonymous_24", count: 0, regions: { 0: { startLoc: { line: 156, col: 7 }, endLoc: { line: 157, col: 92 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 36 }, 37: { name: "RdpPermissionManager.hasControlDevicePermission", count: 0, regions: { 0: { startLoc: { line: 162, col: 3 }, endLoc: { line: 164, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 163, col: 5 }, endLoc: { line: 164, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 37 }, 38: { name: "RdpPermissionManager.ensurePermission", count: 0, regions: { 0: { startLoc: { line: 166, col: 3 }, endLoc: { line: 191, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 168, col: 22 }, endLoc: { line: 170, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 171, col: 33 }, endLoc: { line: 173, col: 6 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 174, col: 41 }, endLoc: { line: 177, col: 6 }, count: 0, ignored: 0 }, 4: { startLoc: { line: 180, col: 27 }, endLoc: { line: 183, col: 6 }, count: 0, ignored: 0 }, 5: { startLoc: { line: 185, col: 5 }, endLoc: { line: 191, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 168, col: 9 }, endLoc: { line: 168, col: 20 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 171, col: 9 }, endLoc: { line: 171, col: 31 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 2: { startLoc: { line: 174, col: 9 }, endLoc: { line: 174, col: 39 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 3: { startLoc: { line: 180, col: 9 }, endLoc: { line: 180, col: 25 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 38 }, 39: { name: "anonymous_25", count: 0, regions: { 0: { startLoc: { line: 187, col: 16 }, endLoc: { line: 189, col: 8 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 188, col: 9 }, endLoc: { line: 189, col: 8 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 39 }, 40: { name: "RdpPermissionManager.getSelfAccessTokenId", count: 0, regions: { 0: { startLoc: { line: 193, col: 3 }, endLoc: { line: 198, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 194, col: 5 }, endLoc: { line: 198, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 40 }, 41: { name: "RdpPermissionManager.hasPermission", count: 0, regions: { 0: { startLoc: { line: 200, col: 3 }, endLoc: { line: 209, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 201, col: 9 }, endLoc: { line: 205, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 205, col: 7 }, endLoc: { line: 208, col: 6 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 41 }, 42: { name: "RdpPermissionManager.openPermissionOnSetting", count: 0, regions: { 0: { startLoc: { line: 211, col: 3 }, endLoc: { line: 231, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 213, col: 5 }, endLoc: { line: 231, col: 4 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 225, col: 9 }, endLoc: { line: 226, col: 8 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 229, col: 9 }, endLoc: { line: 230, col: 8 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 42 }, 43: { name: "anonymous_26", count: 0, regions: { 0: { startLoc: { line: 215, col: 13 }, endLoc: { line: 226, col: 8 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 219, col: 77 }, endLoc: { line: 221, col: 10 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 222, col: 23 }, endLoc: { line: 224, col: 10 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 219, col: 13 }, endLoc: { line: 219, col: 75 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 222, col: 13 }, endLoc: { line: 222, col: 21 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 43 }, 44: { name: "anonymous_27", count: 0, regions: { 0: { startLoc: { line: 227, col: 14 }, endLoc: { line: 230, col: 8 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 44 }, 45: { name: "RdpPermissionManager.pollPermissionOnSettingResult", count: 0, regions: { 0: { startLoc: { line: 233, col: 3 }, endLoc: { line: 256, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 234, col: 5 }, endLoc: { line: 256, col: 4 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 252, col: 7 }, endLoc: { line: 255, col: 6 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 248, col: 9 }, endLoc: { line: 251, col: 8 }, count: 0, ignored: 0 }, 4: { startLoc: { line: 240, col: 11 }, endLoc: { line: 242, col: 10 }, count: 0, ignored: 0 }, 5: { startLoc: { line: 245, col: 11 }, endLoc: { line: 247, col: 10 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 45 }, 46: { name: "anonymous_28", count: 0, regions: { 0: { startLoc: { line: 235, col: 33 }, endLoc: { line: 255, col: 6 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 46 }, 47: { name: "anonymous_29", count: 0, regions: { 0: { startLoc: { line: 236, col: 20 }, endLoc: { line: 251, col: 8 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 238, col: 22 }, endLoc: { line: 242, col: 10 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 243, col: 76 }, endLoc: { line: 247, col: 10 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 238, col: 13 }, endLoc: { line: 238, col: 20 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 243, col: 13 }, endLoc: { line: 243, col: 74 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 47 }, 48: { name: "anonymous_30", count: 0, regions: { 0: { startLoc: { line: 248, col: 20 }, endLoc: { line: 250, col: 10 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 249, col: 11 }, endLoc: { line: 250, col: 10 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 48 }, 49: { name: "anonymous_31", count: 0, regions: { 0: { startLoc: { line: 252, col: 18 }, endLoc: { line: 254, col: 8 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 253, col: 9 }, endLoc: { line: 254, col: 8 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 49 }, 50: { name: "RdpPermissionManager.openScreenRecordingPermissionOnSetting", count: 0, regions: { 0: { startLoc: { line: 258, col: 3 }, endLoc: { line: 261, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 259, col: 5 }, endLoc: { line: 261, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 50 }, 51: { name: "RdpPermissionManager.openScreenRecordingPermissionOnSettingAfterDelay", count: 0, regions: { 0: { startLoc: { line: 263, col: 3 }, endLoc: { line: 286, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 264, col: 5 }, endLoc: { line: 286, col: 4 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 266, col: 7 }, endLoc: { line: 285, col: 6 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 276, col: 9 }, endLoc: { line: 284, col: 8 }, count: 0, ignored: 0 }, 4: { startLoc: { line: 274, col: 11 }, endLoc: { line: 275, col: 10 }, count: 0, ignored: 0 }, 5: { startLoc: { line: 282, col: 13 }, endLoc: { line: 283, col: 12 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 51 }, 52: { name: "anonymous_32", count: 0, regions: { 0: { startLoc: { line: 264, col: 33 }, endLoc: { line: 285, col: 6 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 52 }, 53: { name: "anonymous_33", count: 0, regions: { 0: { startLoc: { line: 266, col: 67 }, endLoc: { line: 284, col: 8 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 269, col: 38 }, endLoc: { line: 271, col: 10 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 272, col: 50 }, endLoc: { line: 275, col: 10 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 269, col: 13 }, endLoc: { line: 269, col: 36 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 272, col: 13 }, endLoc: { line: 272, col: 48 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 53 }, 54: { name: "anonymous_34", count: 0, regions: { 0: { startLoc: { line: 277, col: 17 }, endLoc: { line: 279, col: 12 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 278, col: 13 }, endLoc: { line: 279, col: 12 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 54 }, 55: { name: "anonymous_35", count: 0, regions: { 0: { startLoc: { line: 280, col: 18 }, endLoc: { line: 283, col: 12 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 55 }, 56: { name: "RdpPermissionManager.openPendingScreenRecordingPermissionPrompt", count: 0, regions: { 0: { startLoc: { line: 288, col: 3 }, endLoc: { line: 310, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 290, col: 34 }, endLoc: { line: 292, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 295, col: 27 }, endLoc: { line: 299, col: 6 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 301, col: 5 }, endLoc: { line: 310, col: 4 }, count: 0, ignored: 0 }, 4: { startLoc: { line: 308, col: 9 }, endLoc: { line: 309, col: 8 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 290, col: 9 }, endLoc: { line: 290, col: 32 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 295, col: 9 }, endLoc: { line: 295, col: 25 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 56 }, 57: { name: "anonymous_36", count: 0, regions: { 0: { startLoc: { line: 303, col: 13 }, endLoc: { line: 305, col: 8 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 304, col: 9 }, endLoc: { line: 305, col: 8 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 57 }, 58: { name: "anonymous_37", count: 0, regions: { 0: { startLoc: { line: 306, col: 14 }, endLoc: { line: 309, col: 8 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 58 }, 59: { name: "RdpPermissionManager.clearScreenRecordingPermissionPromptDelay", count: 0, regions: { 0: { startLoc: { line: 312, col: 3 }, endLoc: { line: 318, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 313, col: 66 }, endLoc: { line: 315, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 316, col: 5 }, endLoc: { line: 318, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 313, col: 9 }, endLoc: { line: 313, col: 64 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 59 }, 60: { name: "RdpPermissionManager.requestRuntimePermissions", count: 0, regions: { 0: { startLoc: { line: 320, col: 3 }, endLoc: { line: 344, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 322, col: 5 }, endLoc: { line: 344, col: 4 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 338, col: 9 }, endLoc: { line: 339, col: 8 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 330, col: 15 }, endLoc: { line: 331, col: 14 }, count: 0, ignored: 0 }, 4: { startLoc: { line: 342, col: 9 }, endLoc: { line: 343, col: 8 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 60 }, 61: { name: "anonymous_38", count: 0, regions: { 0: { startLoc: { line: 324, col: 13 }, endLoc: { line: 339, col: 8 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 326, col: 48 }, endLoc: { line: 333, col: 10 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 327, col: 11 }, endLoc: { line: 332, col: 12 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 328, col: 93 }, endLoc: { line: 331, col: 14 }, count: 0, ignored: 0 }, 4: { startLoc: { line: 335, col: 23 }, endLoc: { line: 337, col: 10 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 326, col: 13 }, endLoc: { line: 326, col: 46 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 328, col: 17 }, endLoc: { line: 328, col: 91 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 2: { startLoc: { line: 335, col: 13 }, endLoc: { line: 335, col: 21 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 61 }, 62: { name: "anonymous_39", count: 0, regions: { 0: { startLoc: { line: 340, col: 14 }, endLoc: { line: 343, col: 8 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 62 }, 63: { name: "RdpPermissionManager.hasLocationPermission", count: 0, regions: { 0: { startLoc: { line: 346, col: 3 }, endLoc: { line: 349, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 347, col: 5 }, endLoc: { line: 349, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 63 } }, exeLine: { 0: 1, 1: 2, 2: 3, 3: 4, 4: 5, 5: 6, 6: 7, 7: 8, 8: 9, 9: 10, 10: 11, 11: 13, 12: 14, 13: 17, 14: 18, 15: 20, 16: 21, 17: 24, 18: 25, 19: 29, 20: 30, 21: 31, 22: 34, 23: 35, 24: 36, 25: 37, 26: 39, 27: 40, 28: 41, 29: 43, 30: 44, 31: 45, 32: 46, 33: 47, 34: 48, 35: 49, 36: 50, 37: 51, 38: 52, 39: 53, 40: 55, 41: 56, 42: 57, 43: 58, 44: 61, 45: 62, 46: 63, 47: 64, 48: 65, 49: 66, 50: 67, 51: 68, 52: 69, 53: 70, 54: 74, 55: 75, 56: 76, 57: 77, 58: 78, 59: 79, 60: 80, 61: 81, 62: 82, 63: 83, 64: 87, 65: 88, 66: 89, 67: 90, 68: 91, 69: 92, 70: 93, 71: 94, 72: 95, 73: 96, 74: 100, 75: 101, 76: 102, 77: 103, 78: 104, 79: 105, 80: 106, 81: 107, 82: 108, 83: 109, 84: 110, 85: 114, 86: 115, 87: 116, 88: 117, 89: 118, 90: 119, 91: 120, 92: 121, 93: 122, 94: 126, 95: 127, 96: 128, 97: 130, 98: 131, 99: 132, 100: 133, 101: 135, 102: 136, 103: 137, 104: 138, 105: 139, 106: 140, 107: 141, 108: 142, 109: 146, 110: 147, 111: 150, 112: 151, 113: 152, 114: 153, 115: 154, 116: 155, 117: 156, 118: 157, 119: 158, 120: 162, 121: 163, 122: 166, 123: 167, 124: 168, 125: 169, 126: 171, 127: 172, 128: 174, 129: 175, 130: 176, 131: 179, 132: 180, 133: 181, 134: 182, 135: 185, 136: 186, 137: 187, 138: 188, 139: 190, 140: 193, 141: 194, 142: 195, 143: 197, 144: 200, 145: 201, 146: 202, 147: 203, 148: 204, 149: 205, 150: 206, 151: 207, 152: 211, 153: 212, 154: 213, 155: 214, 156: 215, 157: 216, 158: 217, 159: 218, 160: 219, 161: 220, 162: 222, 163: 223, 164: 225, 165: 227, 166: 228, 167: 229, 168: 233, 169: 234, 170: 235, 171: 236, 172: 237, 173: 238, 174: 239, 175: 240, 176: 241, 177: 243, 178: 244, 179: 245, 180: 246, 181: 248, 182: 249, 183: 250, 184: 252, 185: 253, 186: 254, 187: 258, 188: 259, 189: 260, 190: 263, 191: 264, 192: 265, 193: 266, 194: 267, 195: 268, 196: 269, 197: 270, 198: 272, 199: 273, 200: 274, 201: 276, 202: 277, 203: 278, 204: 280, 205: 281, 206: 282, 207: 284, 208: 288, 209: 289, 210: 290, 211: 291, 212: 293, 213: 294, 214: 295, 215: 296, 216: 297, 217: 298, 218: 301, 219: 302, 220: 303, 221: 304, 222: 306, 223: 307, 224: 308, 225: 312, 226: 313, 227: 314, 228: 316, 229: 317, 230: 320, 231: 321, 232: 322, 233: 323, 234: 324, 235: 325, 236: 326, 237: 327, 238: 328, 239: 329, 240: 330, 241: 334, 242: 335, 243: 336, 244: 338, 245: 340, 246: 341, 247: 342, 248: 346, 249: 347, 250: 348 } });
export interface RdpPermissionDelegate {
    getContext(): common.UIAbilityContext | null;
}
export class RdpPermissionCallbackDelegate implements RdpPermissionDelegate {
    private readonly getContextCallback: () => common.UIAbilityContext | null;
    constructor(getContext: () => common.UIAbilityContext | null) {
        bjccovmshb1i1d.instrumentFunction(2);
        bjccovmshb1i1d.instrumentRegion(2, 1);
        this.getContextCallback = getContext;
    }
    getContext(): common.UIAbilityContext | null {
        bjccovmshb1i1d.instrumentFunction(4);
        bjccovmshb1i1d.instrumentRegion(4, 1);
        return this.getContextCallback();
    }
}
interface PermissionRequestState {
    request: Promise<boolean> | null;
    requested: boolean;
}
type PermissionGrantChecker = () => boolean;
type PermissionRequestFactory = (context: common.UIAbilityContext) => Promise<boolean>;
type PermissionRequestResolver = (granted: boolean) => void;
type PermissionPromptOpenedCallback = () => void;
const SCREEN_RECORDING_PERMISSION_PROMPT_DELAY_MS: number = 3000;
const SETTINGS_PERMISSION_POLL_INTERVAL_MS: number = 1000;
const SETTINGS_PERMISSION_POLL_TIMEOUT_MS: number = 60000;
export class RdpPermissionManager {
    private delegate: RdpPermissionDelegate;
    private readonly onScreenRecordingPermissionPromptOpen: PermissionPromptOpenedCallback;
    private pasteboardPermission: PermissionRequestState = { request: null, requested: false };
    private microphonePermission: PermissionRequestState = { request: null, requested: false };
    private cameraPermission: PermissionRequestState = { request: null, requested: false };
    private locationPermission: PermissionRequestState = { request: null, requested: false };
    private screenRecordingPermission: PermissionRequestState = { request: null, requested: false };
    private controlDevicePermission: PermissionRequestState = { request: null, requested: false };
    private screenRecordingPermissionPromptDelayTimer: number | null = null;
    private screenRecordingPermissionPromptDelayResolve: PermissionRequestResolver | null = null;
    constructor(delegate: RdpPermissionDelegate, onScreenRecordingPermissionPromptOpen: PermissionPromptOpenedCallback = () => {
        bjccovmshb1i1d.instrumentFunction(10);
    }) {
        bjccovmshb1i1d.instrumentFunction(9);
        bjccovmshb1i1d.instrumentRegion(9, 1);
        this.delegate = delegate;
        this.onScreenRecordingPermissionPromptOpen = onScreenRecordingPermissionPromptOpen;
    }
    ensurePasteboardReadPermission(trigger: string): Promise<boolean> {
        bjccovmshb1i1d.instrumentFunction(11);
        bjccovmshb1i1d.instrumentRegion(11, 1);
        return this.ensurePermission(this.pasteboardPermission, 'Pasteboard read', trigger, (): boolean => { bjccovmshb1i1d.instrumentFunction(12); return this.hasPermission(READ_PASTEBOARD_PERMISSION, 'Pasteboard'); }, (context: common.UIAbilityContext): Promise<boolean> => { bjccovmshb1i1d.instrumentFunction(13); return this.requestRuntimePermissions(context, 'Pasteboard read', [READ_PASTEBOARD_PERMISSION], (): boolean => { bjccovmshb1i1d.instrumentFunction(14); return this.hasPermission(READ_PASTEBOARD_PERMISSION, 'Pasteboard'); }); }, true);
    }
    ensureMicrophonePermission(trigger: string): Promise<boolean> {
        bjccovmshb1i1d.instrumentFunction(15);
        bjccovmshb1i1d.instrumentRegion(15, 1);
        return this.ensurePermission(this.microphonePermission, 'Microphone', trigger, (): boolean => { bjccovmshb1i1d.instrumentFunction(16); return this.hasPermission(MICROPHONE_PERMISSION, 'Microphone'); }, (context: common.UIAbilityContext): Promise<boolean> => { bjccovmshb1i1d.instrumentFunction(17); return this.requestRuntimePermissions(context, 'Microphone', [MICROPHONE_PERMISSION], (): boolean => { bjccovmshb1i1d.instrumentFunction(18); return this.hasPermission(MICROPHONE_PERMISSION, 'Microphone'); }); }, false);
    }
    ensureCameraPermission(trigger: string): Promise<boolean> {
        bjccovmshb1i1d.instrumentFunction(19);
        bjccovmshb1i1d.instrumentRegion(19, 1);
        return this.ensurePermission(this.cameraPermission, 'Camera', trigger, (): boolean => { bjccovmshb1i1d.instrumentFunction(20); return this.hasPermission(CAMERA_PERMISSION, 'Camera'); }, (context: common.UIAbilityContext): Promise<boolean> => { bjccovmshb1i1d.instrumentFunction(21); return this.requestRuntimePermissions(context, 'Camera', [CAMERA_PERMISSION], (): boolean => { bjccovmshb1i1d.instrumentFunction(22); return this.hasPermission(CAMERA_PERMISSION, 'Camera'); }); }, false);
    }
    ensureLocationPermission(trigger: string): Promise<boolean> {
        bjccovmshb1i1d.instrumentFunction(23);
        bjccovmshb1i1d.instrumentRegion(23, 1);
        const permissions: Permissions[] = [APPROXIMATE_LOCATION_PERMISSION, LOCATION_PERMISSION];
        return this.ensurePermission(this.locationPermission, 'Location', trigger, (): boolean => { bjccovmshb1i1d.instrumentFunction(24); return this.hasLocationPermission(); }, (context: common.UIAbilityContext): Promise<boolean> => { bjccovmshb1i1d.instrumentFunction(25); return this.requestRuntimePermissions(context, 'Location', permissions, (): boolean => { bjccovmshb1i1d.instrumentFunction(26); return this.hasLocationPermission(); }); }, false);
    }
    ensureScreenRecordingPermission(trigger: string): Promise<boolean> {
        bjccovmshb1i1d.instrumentFunction(27);
        bjccovmshb1i1d.instrumentRegion(27, 1);
        return this.ensurePermission(this.screenRecordingPermission, 'Screen recording', trigger, (): boolean => { bjccovmshb1i1d.instrumentFunction(28); return this.hasScreenRecordingPermission(); }, (context: common.UIAbilityContext): Promise<boolean> => { bjccovmshb1i1d.instrumentFunction(29); return this.openScreenRecordingPermissionOnSettingAfterDelay(context); }, true);
    }
    requestScreenRecordingPermission(trigger: string): Promise<boolean> {
        bjccovmshb1i1d.instrumentFunction(30);
        if (this.hasScreenRecordingPermission()) {
            bjccovmshb1i1d.instrumentBranch(30, 0, true);
            bjccovmshb1i1d.instrumentRegion(30, 1);
            return Promise.resolve(true);
        }
        else {
            bjccovmshb1i1d.instrumentBranch(30, 0, false);
        }
        if (this.screenRecordingPermission.request !== null &&
            this.screenRecordingPermissionPromptDelayTimer !== null) {
            bjccovmshb1i1d.instrumentBranch(30, 1, true);
            bjccovmshb1i1d.instrumentRegion(30, 2);
            this.openPendingScreenRecordingPermissionPrompt(trigger);
            return this.screenRecordingPermission.request;
        }
        else {
            bjccovmshb1i1d.instrumentBranch(30, 1, false);
        }
        bjccovmshb1i1d.instrumentRegion(30, 3);
        return this.ensurePermission(this.screenRecordingPermission, 'Screen recording', trigger, (): boolean => { bjccovmshb1i1d.instrumentFunction(31); return this.hasScreenRecordingPermission(); }, (context: common.UIAbilityContext): Promise<boolean> => { bjccovmshb1i1d.instrumentFunction(32); return this.openScreenRecordingPermissionOnSetting(context); }, false);
    }
    hasScreenRecordingPermission(): boolean {
        bjccovmshb1i1d.instrumentFunction(33);
        bjccovmshb1i1d.instrumentRegion(33, 1);
        return this.hasPermission(SCREEN_RECORDING_PERMISSION, 'Screen recording');
    }
    requestControlDevicePermission(trigger: string): Promise<boolean> {
        bjccovmshb1i1d.instrumentFunction(34);
        bjccovmshb1i1d.instrumentRegion(34, 1);
        return this.ensurePermission(this.controlDevicePermission, 'Input injection', trigger, (): boolean => { bjccovmshb1i1d.instrumentFunction(35); return this.hasControlDevicePermission(); }, (context: common.UIAbilityContext): Promise<boolean> => { bjccovmshb1i1d.instrumentFunction(36); return this.openPermissionOnSetting(context, CONTROL_DEVICE_PERMISSION, 'Input injection'); }, false);
    }
    hasControlDevicePermission(): boolean {
        bjccovmshb1i1d.instrumentFunction(37);
        bjccovmshb1i1d.instrumentRegion(37, 1);
        return this.hasPermission(CONTROL_DEVICE_PERMISSION, 'Input injection');
    }
    private ensurePermission(state: PermissionRequestState, label: string, trigger: string, isGranted: PermissionGrantChecker, request: PermissionRequestFactory, requestOnce: boolean): Promise<boolean> {
        bjccovmshb1i1d.instrumentFunction(38);
        if (isGranted()) {
            bjccovmshb1i1d.instrumentBranch(38, 0, true);
            bjccovmshb1i1d.instrumentRegion(38, 1);
            return Promise.resolve(true);
        }
        else {
            bjccovmshb1i1d.instrumentBranch(38, 0, false);
        }
        if (state.request !== null) {
            bjccovmshb1i1d.instrumentBranch(38, 1, true);
            bjccovmshb1i1d.instrumentRegion(38, 2);
            return state.request;
        }
        else {
            bjccovmshb1i1d.instrumentBranch(38, 1, false);
        }
        if (requestOnce && state.requested) {
            bjccovmshb1i1d.instrumentBranch(38, 2, true);
            bjccovmshb1i1d.instrumentRegion(38, 3);
            RdpLogger.warn(`${label} permission is not granted: ${trigger}`);
            return Promise.resolve(false);
        }
        else {
            bjccovmshb1i1d.instrumentBranch(38, 2, false);
        }
        const context = this.delegate.getContext();
        if (context === null) {
            bjccovmshb1i1d.instrumentBranch(38, 3, true);
            bjccovmshb1i1d.instrumentRegion(38, 4);
            RdpLogger.warn(`${label} permission request skipped: no UIAbilityContext`);
            return Promise.resolve(false);
        }
        else {
            bjccovmshb1i1d.instrumentBranch(38, 3, false);
        }
        bjccovmshb1i1d.instrumentRegion(38, 5);
        state.requested = true;
        state.request = request(context)
            .finally(() => {
            bjccovmshb1i1d.instrumentFunction(39);
            bjccovmshb1i1d.instrumentRegion(39, 1);
            state.request = null;
        });
        return state.request;
    }
    private getSelfAccessTokenId(): number {
        bjccovmshb1i1d.instrumentFunction(40);
        bjccovmshb1i1d.instrumentRegion(40, 1);
        const bundleInfo = bundleManager.getBundleInfoForSelfSync(bundleManager.BundleFlag.GET_BUNDLE_INFO_WITH_APPLICATION);
        return bundleInfo.appInfo.accessTokenId;
    }
    private hasPermission(permission: Permissions, label: string): boolean {
        bjccovmshb1i1d.instrumentFunction(41);
        try {
            bjccovmshb1i1d.instrumentRegion(41, 1);
            const atManager = abilityAccessCtrl.createAtManager();
            const status = atManager.checkAccessTokenSync(this.getSelfAccessTokenId(), permission);
            return status === abilityAccessCtrl.GrantStatus.PERMISSION_GRANTED;
        }
        catch (error) {
            bjccovmshb1i1d.instrumentRegion(41, 2);
            RdpLogger.error(`${label} permission check failed: ${JSON.stringify(error)}`);
            return false;
        }
    }
    private openPermissionOnSetting(context: common.UIAbilityContext, permission: Permissions, label: string): Promise<boolean> {
        bjccovmshb1i1d.instrumentFunction(42);
        bjccovmshb1i1d.instrumentRegion(42, 1);
        const atManager = abilityAccessCtrl.createAtManager();
        return atManager.openPermissionOnSetting(context, permission)
            .then((result: abilityAccessCtrl.SelectedResult) => {
            bjccovmshb1i1d.instrumentFunction(43);
            const granted = result === abilityAccessCtrl.SelectedResult.GRANTED ||
                this.hasPermission(permission, label);
            RdpLogger.info(`${label} setting result: result=${result} granted=${granted}`);
            if (result === abilityAccessCtrl.SelectedResult.OPENED && !granted) {
                bjccovmshb1i1d.instrumentBranch(43, 0, true);
                bjccovmshb1i1d.instrumentRegion(43, 1);
                return this.pollPermissionOnSettingResult(permission, label);
            }
            else {
                bjccovmshb1i1d.instrumentBranch(43, 0, false);
            }
            if (!granted) {
                bjccovmshb1i1d.instrumentBranch(43, 1, true);
                bjccovmshb1i1d.instrumentRegion(43, 2);
                RdpLogger.warn(`${label} setting result: granted=${granted} result=${result}`);
            }
            else {
                bjccovmshb1i1d.instrumentBranch(43, 1, false);
            }
            bjccovmshb1i1d.instrumentRegion(42, 2);
            return granted;
        })
            .catch((error: Error) => {
            bjccovmshb1i1d.instrumentFunction(44);
            RdpLogger.error(`${label} setting request failed: ${JSON.stringify(error)}`);
            bjccovmshb1i1d.instrumentRegion(42, 3);
            return false;
        });
    }
    private pollPermissionOnSettingResult(permission: Permissions, label: string): Promise<boolean> {
        bjccovmshb1i1d.instrumentFunction(45);
        bjccovmshb1i1d.instrumentRegion(45, 1);
        const startedAt = Date.now();
        return new Promise<boolean>((resolve) => {
            bjccovmshb1i1d.instrumentFunction(46);
            const poll = (): void => {
                bjccovmshb1i1d.instrumentFunction(47);
                const granted = this.hasPermission(permission, label);
                if (granted) {
                    bjccovmshb1i1d.instrumentBranch(47, 0, true);
                    bjccovmshb1i1d.instrumentRegion(47, 1);
                    RdpLogger.info(`${label} setting poll result: granted=true`);
                    bjccovmshb1i1d.instrumentRegion(45, 4);
                    resolve(true);
                    return;
                }
                else {
                    bjccovmshb1i1d.instrumentBranch(47, 0, false);
                }
                if (Date.now() - startedAt >= SETTINGS_PERMISSION_POLL_TIMEOUT_MS) {
                    bjccovmshb1i1d.instrumentBranch(47, 1, true);
                    bjccovmshb1i1d.instrumentRegion(47, 2);
                    RdpLogger.warn(`${label} setting poll timed out: granted=false`);
                    bjccovmshb1i1d.instrumentRegion(45, 5);
                    resolve(false);
                    return;
                }
                else {
                    bjccovmshb1i1d.instrumentBranch(47, 1, false);
                }
                bjccovmshb1i1d.instrumentRegion(45, 3);
                setTimeout((): void => {
                    bjccovmshb1i1d.instrumentFunction(48);
                    bjccovmshb1i1d.instrumentRegion(48, 1);
                    poll();
                }, SETTINGS_PERMISSION_POLL_INTERVAL_MS);
            };
            bjccovmshb1i1d.instrumentRegion(45, 2);
            setTimeout((): void => {
                bjccovmshb1i1d.instrumentFunction(49);
                bjccovmshb1i1d.instrumentRegion(49, 1);
                poll();
            }, SETTINGS_PERMISSION_POLL_INTERVAL_MS);
        });
    }
    private openScreenRecordingPermissionOnSetting(context: common.UIAbilityContext): Promise<boolean> {
        bjccovmshb1i1d.instrumentFunction(50);
        bjccovmshb1i1d.instrumentRegion(50, 1);
        this.onScreenRecordingPermissionPromptOpen();
        return this.openPermissionOnSetting(context, SCREEN_RECORDING_PERMISSION, 'Screen recording');
    }
    private openScreenRecordingPermissionOnSettingAfterDelay(context: common.UIAbilityContext): Promise<boolean> {
        bjccovmshb1i1d.instrumentFunction(51);
        bjccovmshb1i1d.instrumentRegion(51, 1);
        return new Promise<boolean>((resolve) => {
            bjccovmshb1i1d.instrumentFunction(52);
            this.screenRecordingPermissionPromptDelayResolve = resolve;
            bjccovmshb1i1d.instrumentRegion(51, 2);
            this.screenRecordingPermissionPromptDelayTimer = setTimeout((): void => {
                bjccovmshb1i1d.instrumentFunction(53);
                const delayedResolve = this.screenRecordingPermissionPromptDelayResolve;
                this.clearScreenRecordingPermissionPromptDelay();
                if (delayedResolve === null) {
                    bjccovmshb1i1d.instrumentBranch(53, 0, true);
                    bjccovmshb1i1d.instrumentRegion(53, 1);
                    return;
                }
                else {
                    bjccovmshb1i1d.instrumentBranch(53, 0, false);
                }
                if (this.hasScreenRecordingPermission()) {
                    bjccovmshb1i1d.instrumentBranch(53, 1, true);
                    bjccovmshb1i1d.instrumentRegion(53, 2);
                    delayedResolve(true);
                    bjccovmshb1i1d.instrumentRegion(51, 4);
                    return;
                }
                else {
                    bjccovmshb1i1d.instrumentBranch(53, 1, false);
                }
                bjccovmshb1i1d.instrumentRegion(51, 3);
                this.openScreenRecordingPermissionOnSetting(context)
                    .then((granted: boolean) => {
                    bjccovmshb1i1d.instrumentFunction(54);
                    bjccovmshb1i1d.instrumentRegion(54, 1);
                    delayedResolve(granted);
                })
                    .catch((error: Error) => {
                    bjccovmshb1i1d.instrumentFunction(55);
                    RdpLogger.error(`Screen recording setting request failed: ${JSON.stringify(error)}`);
                    bjccovmshb1i1d.instrumentRegion(51, 5);
                    delayedResolve(false);
                });
            }, SCREEN_RECORDING_PERMISSION_PROMPT_DELAY_MS);
        });
    }
    private openPendingScreenRecordingPermissionPrompt(trigger: string): void {
        bjccovmshb1i1d.instrumentFunction(56);
        const delayedResolve = this.screenRecordingPermissionPromptDelayResolve;
        if (delayedResolve === null) {
            bjccovmshb1i1d.instrumentBranch(56, 0, true);
            bjccovmshb1i1d.instrumentRegion(56, 1);
            return;
        }
        else {
            bjccovmshb1i1d.instrumentBranch(56, 0, false);
        }
        const context = this.delegate.getContext();
        this.clearScreenRecordingPermissionPromptDelay();
        if (context === null) {
            bjccovmshb1i1d.instrumentBranch(56, 1, true);
            bjccovmshb1i1d.instrumentRegion(56, 2);
            RdpLogger.warn(`Screen recording permission request skipped: no UIAbilityContext`);
            delayedResolve(false);
            return;
        }
        else {
            bjccovmshb1i1d.instrumentBranch(56, 1, false);
        }
        bjccovmshb1i1d.instrumentRegion(56, 3);
        RdpLogger.info(`Screen recording permission requested immediately: ${trigger}`);
        this.openScreenRecordingPermissionOnSetting(context)
            .then((granted: boolean) => {
            bjccovmshb1i1d.instrumentFunction(57);
            bjccovmshb1i1d.instrumentRegion(57, 1);
            delayedResolve(granted);
        })
            .catch((error: Error) => {
            bjccovmshb1i1d.instrumentFunction(58);
            RdpLogger.error(`Screen recording setting request failed: ${JSON.stringify(error)}`);
            bjccovmshb1i1d.instrumentRegion(56, 4);
            delayedResolve(false);
        });
    }
    private clearScreenRecordingPermissionPromptDelay(): void {
        bjccovmshb1i1d.instrumentFunction(59);
        if (this.screenRecordingPermissionPromptDelayTimer !== null) {
            bjccovmshb1i1d.instrumentBranch(59, 0, true);
            bjccovmshb1i1d.instrumentRegion(59, 1);
            clearTimeout(this.screenRecordingPermissionPromptDelayTimer);
        }
        else {
            bjccovmshb1i1d.instrumentBranch(59, 0, false);
        }
        bjccovmshb1i1d.instrumentRegion(59, 2);
        this.screenRecordingPermissionPromptDelayTimer = null;
        this.screenRecordingPermissionPromptDelayResolve = null;
    }
    private requestRuntimePermissions(context: common.UIAbilityContext, label: string, permissions: Permissions[], isGranted: PermissionGrantChecker): Promise<boolean> {
        bjccovmshb1i1d.instrumentFunction(60);
        bjccovmshb1i1d.instrumentRegion(60, 1);
        const atManager = abilityAccessCtrl.createAtManager();
        return atManager.requestPermissionsFromUser(context, permissions)
            .then((result) => {
            bjccovmshb1i1d.instrumentFunction(61);
            let granted = false;
            if (Array.isArray(result.authResults)) {
                bjccovmshb1i1d.instrumentBranch(61, 0, true);
                bjccovmshb1i1d.instrumentRegion(61, 1);
                for (let i = 0; i < result.authResults.length; i++) {
                    bjccovmshb1i1d.instrumentRegion(61, 2);
                    if (result.authResults[i] === abilityAccessCtrl.GrantStatus.PERMISSION_GRANTED) {
                        bjccovmshb1i1d.instrumentBranch(61, 1, true);
                        bjccovmshb1i1d.instrumentRegion(61, 3);
                        granted = true;
                        bjccovmshb1i1d.instrumentRegion(60, 3);
                        break;
                    }
                    else {
                        bjccovmshb1i1d.instrumentBranch(61, 1, false);
                    }
                }
            }
            else {
                bjccovmshb1i1d.instrumentBranch(61, 0, false);
            }
            granted = granted || isGranted();
            if (!granted) {
                bjccovmshb1i1d.instrumentBranch(61, 2, true);
                bjccovmshb1i1d.instrumentRegion(61, 4);
                RdpLogger.warn(`${label} permission request result: granted=${granted}`);
            }
            else {
                bjccovmshb1i1d.instrumentBranch(61, 2, false);
            }
            bjccovmshb1i1d.instrumentRegion(60, 2);
            return granted;
        })
            .catch((error: Error) => {
            bjccovmshb1i1d.instrumentFunction(62);
            RdpLogger.error(`${label} permission request failed: ${JSON.stringify(error)}`);
            bjccovmshb1i1d.instrumentRegion(60, 4);
            return false;
        });
    }
    private hasLocationPermission(): boolean {
        bjccovmshb1i1d.instrumentFunction(63);
        bjccovmshb1i1d.instrumentRegion(63, 1);
        return this.hasPermission(APPROXIMATE_LOCATION_PERMISSION, 'Approximate location') ||
            this.hasPermission(LOCATION_PERMISSION, 'Location');
    }
}
