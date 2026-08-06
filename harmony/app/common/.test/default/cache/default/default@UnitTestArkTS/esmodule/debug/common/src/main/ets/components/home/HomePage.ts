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
let bjccovmshb1i6x = new BjcCov({ version: "bjc v1.0.0", versionCode: 10000, path: "common/src/main/ets/components/home/HomePage.ets", hash: "864cc7e5b7d22b135fad83ec117e39de8141b2e66ce132b70c3f3d71cd47f65e", lineCnt: 201, count: 0, projectPath: "", functions: { 0: { name: "anonymous_1", count: 0, regions: { 0: { startLoc: { line: 36, col: 17 }, endLoc: { line: 36, col: 40 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 0 }, 1: { name: "anonymous_2", count: 0, regions: { 0: { startLoc: { line: 38, col: 17 }, endLoc: { line: 38, col: 40 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 1 }, 2: { name: "anonymous_3", count: 0, regions: { 0: { startLoc: { line: 40, col: 21 }, endLoc: { line: 40, col: 44 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 2 }, 3: { name: "anonymous_4", count: 0, regions: { 0: { startLoc: { line: 42, col: 21 }, endLoc: { line: 42, col: 44 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 3 }, 4: { name: "anonymous_5", count: 0, regions: { 0: { startLoc: { line: 44, col: 20 }, endLoc: { line: 44, col: 47 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 4 }, 5: { name: "anonymous_6", count: 0, regions: { 0: { startLoc: { line: 46, col: 17 }, endLoc: { line: 46, col: 27 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 5 }, 6: { name: "anonymous_7", count: 0, regions: { 0: { startLoc: { line: 48, col: 20 }, endLoc: { line: 48, col: 30 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 6 }, 7: { name: "anonymous_8", count: 0, regions: { 0: { startLoc: { line: 50, col: 29 }, endLoc: { line: 50, col: 56 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 7 }, 8: { name: "anonymous_9", count: 0, regions: { 0: { startLoc: { line: 52, col: 20 }, endLoc: { line: 52, col: 30 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 8 }, 9: { name: "anonymous_10", count: 0, regions: { 0: { startLoc: { line: 54, col: 14 }, endLoc: { line: 54, col: 24 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 9 }, 10: { name: "anonymous_11", count: 0, regions: { 0: { startLoc: { line: 56, col: 19 }, endLoc: { line: 56, col: 57 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 10 }, 11: { name: "anonymous_12", count: 0, regions: { 0: { startLoc: { line: 36, col: 43 }, endLoc: { line: 37, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 11 }, 12: { name: "anonymous_13", count: 0, regions: { 0: { startLoc: { line: 38, col: 43 }, endLoc: { line: 39, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 12 }, 13: { name: "anonymous_14", count: 0, regions: { 0: { startLoc: { line: 40, col: 47 }, endLoc: { line: 41, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 13 }, 14: { name: "anonymous_15", count: 0, regions: { 0: { startLoc: { line: 42, col: 47 }, endLoc: { line: 43, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 14 }, 15: { name: "anonymous_16", count: 0, regions: { 0: { startLoc: { line: 44, col: 50 }, endLoc: { line: 45, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 15 }, 16: { name: "anonymous_17", count: 0, regions: { 0: { startLoc: { line: 46, col: 30 }, endLoc: { line: 47, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 16 }, 17: { name: "anonymous_18", count: 0, regions: { 0: { startLoc: { line: 48, col: 33 }, endLoc: { line: 49, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 17 }, 18: { name: "anonymous_19", count: 0, regions: { 0: { startLoc: { line: 50, col: 59 }, endLoc: { line: 51, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 18 }, 19: { name: "anonymous_20", count: 0, regions: { 0: { startLoc: { line: 52, col: 33 }, endLoc: { line: 53, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 19 }, 20: { name: "anonymous_21", count: 0, regions: { 0: { startLoc: { line: 54, col: 27 }, endLoc: { line: 55, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 20 }, 21: { name: "anonymous_22", count: 0, regions: { 0: { startLoc: { line: 56, col: 60 }, endLoc: { line: 57, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 21 }, 22: { name: "setInitiallyProvidedValue", count: 0, regions: { 0: { startLoc: { line: 15, col: 34 }, endLoc: { line: 34, col: 53 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 22 }, 23: { name: "updateStateVars", count: 0, regions: { 0: { startLoc: { line: 15, col: 9 }, endLoc: { line: 34, col: 37 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 23 }, 24: { name: "layoutMode", count: 0, regions: { 0: { startLoc: { line: 15, col: 9 }, endLoc: { line: 15, col: 19 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 24 }, 25: { name: "layoutMode", count: 0, regions: { 0: { startLoc: { line: 15, col: 9 }, endLoc: { line: 15, col: 31 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 25 }, 26: { name: "host", count: 0, regions: { 0: { startLoc: { line: 16, col: 9 }, endLoc: { line: 16, col: 13 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 26 }, 27: { name: "host", count: 0, regions: { 0: { startLoc: { line: 16, col: 9 }, endLoc: { line: 16, col: 21 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 27 }, 28: { name: "port", count: 0, regions: { 0: { startLoc: { line: 17, col: 9 }, endLoc: { line: 17, col: 13 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 28 }, 29: { name: "port", count: 0, regions: { 0: { startLoc: { line: 17, col: 9 }, endLoc: { line: 17, col: 21 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 29 }, 30: { name: "username", count: 0, regions: { 0: { startLoc: { line: 18, col: 9 }, endLoc: { line: 18, col: 17 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 30 }, 31: { name: "username", count: 0, regions: { 0: { startLoc: { line: 18, col: 9 }, endLoc: { line: 18, col: 25 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 31 }, 32: { name: "password", count: 0, regions: { 0: { startLoc: { line: 19, col: 9 }, endLoc: { line: 19, col: 17 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 32 }, 33: { name: "password", count: 0, regions: { 0: { startLoc: { line: 19, col: 9 }, endLoc: { line: 19, col: 25 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 33 }, 34: { name: "connectionProfiles", count: 0, regions: { 0: { startLoc: { line: 20, col: 9 }, endLoc: { line: 20, col: 27 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 34 }, 35: { name: "connectionProfiles", count: 0, regions: { 0: { startLoc: { line: 20, col: 9 }, endLoc: { line: 20, col: 55 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 35 }, 36: { name: "selectedConnectionProfileId", count: 0, regions: { 0: { startLoc: { line: 21, col: 9 }, endLoc: { line: 21, col: 36 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 36 }, 37: { name: "selectedConnectionProfileId", count: 0, regions: { 0: { startLoc: { line: 21, col: 9 }, endLoc: { line: 21, col: 44 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 37 }, 38: { name: "rememberPassword", count: 0, regions: { 0: { startLoc: { line: 22, col: 9 }, endLoc: { line: 22, col: 25 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 38 }, 39: { name: "rememberPassword", count: 0, regions: { 0: { startLoc: { line: 22, col: 9 }, endLoc: { line: 22, col: 34 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 39 }, 40: { name: "passwordLoading", count: 0, regions: { 0: { startLoc: { line: 23, col: 9 }, endLoc: { line: 23, col: 24 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 40 }, 41: { name: "passwordLoading", count: 0, regions: { 0: { startLoc: { line: 23, col: 9 }, endLoc: { line: 23, col: 33 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 41 }, 42: { name: "connectionFeedbackText", count: 0, regions: { 0: { startLoc: { line: 24, col: 9 }, endLoc: { line: 24, col: 31 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 42 }, 43: { name: "connectionFeedbackText", count: 0, regions: { 0: { startLoc: { line: 24, col: 9 }, endLoc: { line: 24, col: 39 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 43 }, 44: { name: "connectionFeedbackTone", count: 0, regions: { 0: { startLoc: { line: 25, col: 9 }, endLoc: { line: 25, col: 31 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 44 }, 45: { name: "connectionFeedbackTone", count: 0, regions: { 0: { startLoc: { line: 25, col: 9 }, endLoc: { line: 25, col: 51 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 45 }, 46: { name: "isDark", count: 0, regions: { 0: { startLoc: { line: 26, col: 9 }, endLoc: { line: 26, col: 15 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 46 }, 47: { name: "isDark", count: 0, regions: { 0: { startLoc: { line: 26, col: 9 }, endLoc: { line: 26, col: 24 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 47 }, 48: { name: "xrdpServerBusy", count: 0, regions: { 0: { startLoc: { line: 27, col: 9 }, endLoc: { line: 27, col: 23 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 48 }, 49: { name: "xrdpServerBusy", count: 0, regions: { 0: { startLoc: { line: 27, col: 9 }, endLoc: { line: 27, col: 32 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 49 }, 50: { name: "xrdpServerState", count: 0, regions: { 0: { startLoc: { line: 28, col: 9 }, endLoc: { line: 28, col: 24 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 50 }, 51: { name: "xrdpServerState", count: 0, regions: { 0: { startLoc: { line: 28, col: 9 }, endLoc: { line: 28, col: 32 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 51 }, 52: { name: "xrdpServerRunning", count: 0, regions: { 0: { startLoc: { line: 29, col: 9 }, endLoc: { line: 29, col: 26 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 52 }, 53: { name: "xrdpServerRunning", count: 0, regions: { 0: { startLoc: { line: 29, col: 9 }, endLoc: { line: 29, col: 35 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 53 }, 54: { name: "screenRecordingPermissionGranted", count: 0, regions: { 0: { startLoc: { line: 30, col: 9 }, endLoc: { line: 30, col: 41 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 54 }, 55: { name: "screenRecordingPermissionGranted", count: 0, regions: { 0: { startLoc: { line: 30, col: 9 }, endLoc: { line: 30, col: 50 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 55 }, 56: { name: "screenRecordingPermissionBusy", count: 0, regions: { 0: { startLoc: { line: 31, col: 9 }, endLoc: { line: 31, col: 38 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 56 }, 57: { name: "screenRecordingPermissionBusy", count: 0, regions: { 0: { startLoc: { line: 31, col: 9 }, endLoc: { line: 31, col: 47 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 57 }, 58: { name: "inputInjectionPermissionGranted", count: 0, regions: { 0: { startLoc: { line: 32, col: 9 }, endLoc: { line: 32, col: 40 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 58 }, 59: { name: "inputInjectionPermissionGranted", count: 0, regions: { 0: { startLoc: { line: 32, col: 9 }, endLoc: { line: 32, col: 49 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 59 }, 60: { name: "inputInjectionPermissionBusy", count: 0, regions: { 0: { startLoc: { line: 33, col: 9 }, endLoc: { line: 33, col: 37 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 60 }, 61: { name: "inputInjectionPermissionBusy", count: 0, regions: { 0: { startLoc: { line: 33, col: 9 }, endLoc: { line: 33, col: 46 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 61 }, 62: { name: "remoteControlServerAvailable", count: 0, regions: { 0: { startLoc: { line: 34, col: 9 }, endLoc: { line: 34, col: 37 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 62 }, 63: { name: "remoteControlServerAvailable", count: 0, regions: { 0: { startLoc: { line: 34, col: 9 }, endLoc: { line: 34, col: 46 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 63 }, 64: { name: "compactPage", count: 0, regions: { 0: { startLoc: { line: 35, col: 18 }, endLoc: { line: 35, col: 29 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 64 }, 65: { name: "compactPage", count: 0, regions: { 0: { startLoc: { line: 35, col: 18 }, endLoc: { line: 35, col: 46 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 65 }, 66: { name: "anonymous_23", count: 0, regions: { 0: { startLoc: { line: 36, col: 17 }, endLoc: { line: 36, col: 40 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 66 }, 67: { name: "anonymous_24", count: 0, regions: { 0: { startLoc: { line: 38, col: 17 }, endLoc: { line: 38, col: 40 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 67 }, 68: { name: "anonymous_25", count: 0, regions: { 0: { startLoc: { line: 40, col: 21 }, endLoc: { line: 40, col: 44 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 68 }, 69: { name: "anonymous_26", count: 0, regions: { 0: { startLoc: { line: 42, col: 21 }, endLoc: { line: 42, col: 44 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 69 }, 70: { name: "anonymous_27", count: 0, regions: { 0: { startLoc: { line: 44, col: 20 }, endLoc: { line: 44, col: 47 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 70 }, 71: { name: "anonymous_28", count: 0, regions: { 0: { startLoc: { line: 46, col: 17 }, endLoc: { line: 46, col: 27 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 71 }, 72: { name: "anonymous_29", count: 0, regions: { 0: { startLoc: { line: 48, col: 20 }, endLoc: { line: 48, col: 30 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 72 }, 73: { name: "anonymous_30", count: 0, regions: { 0: { startLoc: { line: 50, col: 29 }, endLoc: { line: 50, col: 56 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 73 }, 74: { name: "anonymous_31", count: 0, regions: { 0: { startLoc: { line: 52, col: 20 }, endLoc: { line: 52, col: 30 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 74 }, 75: { name: "anonymous_32", count: 0, regions: { 0: { startLoc: { line: 54, col: 14 }, endLoc: { line: 54, col: 24 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 75 }, 76: { name: "anonymous_33", count: 0, regions: { 0: { startLoc: { line: 56, col: 19 }, endLoc: { line: 56, col: 57 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 76 }, 77: { name: "selectProfile", count: 0, regions: { 0: { startLoc: { line: 59, col: 3 }, endLoc: { line: 62, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 60, col: 5 }, endLoc: { line: 62, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 77 }, 78: { name: "createProfile", count: 0, regions: { 0: { startLoc: { line: 64, col: 3 }, endLoc: { line: 67, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 65, col: 5 }, endLoc: { line: 67, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 78 }, 79: { name: "buildDeviceList", count: 0, regions: { 0: { startLoc: { line: 69, col: 3 }, endLoc: { line: 83, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 79 }, 80: { name: "anonymous_34", count: 0, regions: { 0: { startLoc: { line: 71, col: 5 }, endLoc: { line: 75, col: 26 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 80 }, 81: { name: "anonymous_35", count: 0, regions: { 0: { startLoc: { line: 76, col: 24 }, endLoc: { line: 78, col: 8 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 77, col: 9 }, endLoc: { line: 78, col: 8 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 81 }, 82: { name: "anonymous_36", count: 0, regions: { 0: { startLoc: { line: 79, col: 21 }, endLoc: { line: 81, col: 8 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 80, col: 9 }, endLoc: { line: 81, col: 8 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 82 }, 83: { name: "buildConnectionDetails", count: 0, regions: { 0: { startLoc: { line: 85, col: 3 }, endLoc: { line: 108, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 83 }, 84: { name: "anonymous_38", count: 0, regions: { 0: { startLoc: { line: 87, col: 5 }, endLoc: { line: 98, col: 26 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 84 }, 85: { name: "buildExpandedMain", count: 0, regions: { 0: { startLoc: { line: 110, col: 3 }, endLoc: { line: 125, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 85 }, 86: { name: "anonymous_40", count: 0, regions: { 0: { startLoc: { line: 112, col: 5 }, endLoc: { line: 124, col: 58 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 86 }, 87: { name: "anonymous_41", count: 0, regions: { 0: { startLoc: { line: 114, col: 7 }, endLoc: { line: 119, col: 31 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 87 }, 88: { name: "buildCompactMain", count: 0, regions: { 0: { startLoc: { line: 127, col: 3 }, endLoc: { line: 164, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 88 }, 89: { name: "anonymous_42", count: 0, regions: { 0: { startLoc: { line: 129, col: 5 }, endLoc: { line: 163, col: 58 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 89 }, 90: { name: "anonymous_43", count: 0, regions: { 0: { startLoc: { line: 130, col: 7 }, endLoc: { line: 158, col: 8 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 130, col: 43 }, endLoc: { line: 156, col: 8 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 156, col: 14 }, endLoc: { line: 158, col: 8 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 130, col: 11 }, endLoc: { line: 130, col: 41 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 90 }, 91: { name: "anonymous_44", count: 0, regions: { 0: { startLoc: { line: 131, col: 9 }, endLoc: { line: 150, col: 15 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 91 }, 92: { name: "anonymous_45", count: 0, regions: { 0: { startLoc: { line: 131, col: 9 }, endLoc: { line: 148, col: 30 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 92 }, 93: { name: "anonymous_46", count: 0, regions: { 0: { startLoc: { line: 132, col: 11 }, endLoc: { line: 133, col: 32 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 93 }, 94: { name: "anonymous_47", count: 0, regions: { 0: { startLoc: { line: 134, col: 21 }, endLoc: { line: 136, col: 14 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 135, col: 15 }, endLoc: { line: 136, col: 14 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 94 }, 95: { name: "anonymous_49", count: 0, regions: { 0: { startLoc: { line: 138, col: 11 }, endLoc: { line: 144, col: 62 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 95 }, 96: { name: "anonymous_50", count: 0, regions: { 0: { startLoc: { line: 150, col: 9 }, endLoc: { line: 155, col: 33 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 96 }, 97: { name: "anonymous_51", count: 0, regions: { 0: { startLoc: { line: 157, col: 9 }, endLoc: { line: 157, col: 31 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 97 }, 98: { name: "initialRender", count: 0, regions: { 0: { startLoc: { line: 166, col: 3 }, endLoc: { line: 199, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 98 }, 99: { name: "anonymous_52", count: 0, regions: { 0: { startLoc: { line: 167, col: 5 }, endLoc: { line: 198, col: 58 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 99 }, 100: { name: "anonymous_53", count: 0, regions: { 0: { startLoc: { line: 168, col: 7 }, endLoc: { line: 174, col: 72 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 100 }, 101: { name: "anonymous_55", count: 0, regions: { 0: { startLoc: { line: 177, col: 7 }, endLoc: { line: 181, col: 8 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 177, col: 51 }, endLoc: { line: 179, col: 8 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 179, col: 14 }, endLoc: { line: 181, col: 8 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 177, col: 11 }, endLoc: { line: 177, col: 49 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 101 }, 102: { name: "anonymous_56", count: 0, regions: { 0: { startLoc: { line: 178, col: 9 }, endLoc: { line: 178, col: 32 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 102 }, 103: { name: "anonymous_57", count: 0, regions: { 0: { startLoc: { line: 180, col: 9 }, endLoc: { line: 180, col: 33 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 103 }, 104: { name: "anonymous_58", count: 0, regions: { 0: { startLoc: { line: 182, col: 7 }, endLoc: { line: 192, col: 72 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 104 } }, exeLine: { 0: 1, 1: 2, 2: 3, 3: 4, 4: 5, 5: 6, 6: 7, 7: 8, 8: 9, 9: 11, 10: 14, 11: 15, 12: 16, 13: 17, 14: 18, 15: 19, 16: 20, 17: 21, 18: 22, 19: 23, 20: 24, 21: 25, 22: 26, 23: 27, 24: 28, 25: 29, 26: 30, 27: 31, 28: 32, 29: 33, 30: 34, 31: 35, 32: 36, 33: 38, 34: 40, 35: 42, 36: 44, 37: 46, 38: 48, 39: 50, 40: 52, 41: 54, 42: 56, 43: 59, 44: 60, 45: 61, 46: 64, 47: 65, 48: 66, 49: 70, 50: 71, 51: 72, 52: 73, 53: 74, 54: 75, 55: 76, 56: 77, 57: 79, 58: 80, 59: 86, 60: 87, 61: 88, 62: 89, 63: 90, 64: 91, 65: 92, 66: 93, 67: 94, 68: 95, 69: 96, 70: 97, 71: 98, 72: 99, 73: 100, 74: 101, 75: 102, 76: 103, 77: 104, 78: 105, 79: 106, 80: 111, 81: 112, 82: 113, 83: 114, 84: 115, 85: 117, 86: 118, 87: 119, 88: 121, 89: 122, 90: 123, 91: 124, 92: 128, 93: 129, 94: 130, 95: 131, 96: 132, 97: 133, 98: 134, 99: 135, 100: 138, 101: 139, 102: 140, 103: 141, 104: 142, 105: 143, 106: 144, 107: 146, 108: 147, 109: 148, 110: 150, 111: 151, 112: 153, 113: 154, 114: 155, 115: 156, 116: 157, 117: 160, 118: 161, 119: 162, 120: 163, 121: 166, 122: 167, 123: 168, 124: 169, 125: 170, 126: 171, 127: 172, 128: 173, 129: 174, 130: 175, 131: 177, 132: 178, 133: 179, 134: 180, 135: 182, 136: 183, 137: 184, 138: 185, 139: 186, 140: 187, 141: 188, 142: 189, 143: 190, 144: 191, 145: 192, 146: 193, 147: 196, 148: 197, 149: 198 } });
if (!("finalizeConstruction" in ViewPU.prototype)) {
    Reflect.set(ViewPU.prototype, "finalizeConstruction", () => { });
}
interface HomePage_Params {
    layoutMode?: LayoutMode;
    host?: string;
    port?: string;
    username?: string;
    password?: string;
    connectionProfiles?: WindowsConnectionProfile[];
    selectedConnectionProfileId?: string;
    rememberPassword?: boolean;
    passwordLoading?: boolean;
    connectionFeedbackText?: string;
    connectionFeedbackTone?: SettingsStatusTone;
    isDark?: boolean;
    xrdpServerBusy?: boolean;
    xrdpServerState?: string;
    xrdpServerRunning?: boolean;
    screenRecordingPermissionGranted?: boolean;
    screenRecordingPermissionBusy?: boolean;
    inputInjectionPermissionGranted?: boolean;
    inputInjectionPermissionBusy?: boolean;
    remoteControlServerAvailable?: boolean;
    compactPage?: HomeCompactPage;
    onHostChange?: (value: string) => void;
    onPortChange?: (value: string) => void;
    onUsernameChange?: (value: string) => void;
    onPasswordChange?: (value: string) => void;
    onProfileSelect?: (profileId: string) => void;
    onNewProfile?: () => void;
    onDeleteProfile?: () => void;
    onRememberPasswordChange?: (remember: boolean) => void;
    onClearPassword?: () => void;
    onConnect?: () => void;
    onOpenSettings?: (remoteControlSection: string) => void;
}
import { LayoutMode } from "@normalized:N&&&common/src/main/ets/adaptive/WindowLayoutPolicy&";
import { SettingsBackButton, SettingsTheme } from "@normalized:N&&&common/src/main/ets/components/settings/SettingsUi&";
import type { SettingsStatusTone } from "@normalized:N&&&common/src/main/ets/components/settings/SettingsUi&";
import type { WindowsConnectionProfile } from '../../rdp/WindowsConnectionStore';
import { HomeConnectionDetails } from "@normalized:N&&&common/src/main/ets/components/home/HomeConnectionDetails&";
import { HomeDeviceList } from "@normalized:N&&&common/src/main/ets/components/home/HomeDeviceList&";
import { HomeHeader } from "@normalized:N&&&common/src/main/ets/components/home/HomeHeader&";
import { HomeText } from "@normalized:N&&&common/src/main/ets/components/home/HomeText&";
import { HomeStatusFooter } from "@normalized:N&&&common/src/main/ets/components/home/HomeStatusFooter&";
import { HomeTheme } from "@normalized:N&&&common/src/main/ets/components/home/HomeTheme&";
type HomeCompactPage = 'devices' | 'details';
export class HomePage extends ViewPU {
    constructor(parent, params, __localStorage, elmtId = -1, paramsLambda = undefined, extraInfo) {
        super(parent, __localStorage, elmtId, extraInfo);
        if (typeof paramsLambda === "function") {
            this.paramsGenerator_ = paramsLambda;
        }
        this.__layoutMode = new SynchedPropertySimpleOneWayPU(params.layoutMode, this, "layoutMode");
        this.__host = new SynchedPropertySimpleTwoWayPU(params.host, this, "host");
        this.__port = new SynchedPropertySimpleTwoWayPU(params.port, this, "port");
        this.__username = new SynchedPropertySimpleTwoWayPU(params.username, this, "username");
        this.__password = new SynchedPropertySimpleTwoWayPU(params.password, this, "password");
        this.__connectionProfiles = new SynchedPropertyObjectOneWayPU(params.connectionProfiles, this, "connectionProfiles");
        this.__selectedConnectionProfileId = new SynchedPropertySimpleOneWayPU(params.selectedConnectionProfileId, this, "selectedConnectionProfileId");
        this.__rememberPassword = new SynchedPropertySimpleTwoWayPU(params.rememberPassword, this, "rememberPassword");
        this.__passwordLoading = new SynchedPropertySimpleOneWayPU(params.passwordLoading, this, "passwordLoading");
        this.__connectionFeedbackText = new SynchedPropertySimpleOneWayPU(params.connectionFeedbackText, this, "connectionFeedbackText");
        this.__connectionFeedbackTone = new SynchedPropertySimpleOneWayPU(params.connectionFeedbackTone, this, "connectionFeedbackTone");
        this.__isDark = new SynchedPropertySimpleOneWayPU(params.isDark, this, "isDark");
        this.__xrdpServerBusy = new SynchedPropertySimpleOneWayPU(params.xrdpServerBusy, this, "xrdpServerBusy");
        this.__xrdpServerState = new SynchedPropertySimpleOneWayPU(params.xrdpServerState, this, "xrdpServerState");
        this.__xrdpServerRunning = new SynchedPropertySimpleOneWayPU(params.xrdpServerRunning, this, "xrdpServerRunning");
        this.__screenRecordingPermissionGranted = new SynchedPropertySimpleOneWayPU(params.screenRecordingPermissionGranted, this, "screenRecordingPermissionGranted");
        this.__screenRecordingPermissionBusy = new SynchedPropertySimpleOneWayPU(params.screenRecordingPermissionBusy, this, "screenRecordingPermissionBusy");
        this.__inputInjectionPermissionGranted = new SynchedPropertySimpleOneWayPU(params.inputInjectionPermissionGranted, this, "inputInjectionPermissionGranted");
        this.__inputInjectionPermissionBusy = new SynchedPropertySimpleOneWayPU(params.inputInjectionPermissionBusy, this, "inputInjectionPermissionBusy");
        this.__remoteControlServerAvailable = new SynchedPropertySimpleOneWayPU(params.remoteControlServerAvailable, this, "remoteControlServerAvailable");
        this.__compactPage = new ObservedPropertySimplePU('devices', this, "compactPage");
        this.onHostChange = (_value: string) => {
            bjccovmshb1i6x.instrumentFunction(11);
        };
        this.onPortChange = (_value: string) => {
            bjccovmshb1i6x.instrumentFunction(12);
        };
        this.onUsernameChange = (_value: string) => {
            bjccovmshb1i6x.instrumentFunction(13);
        };
        this.onPasswordChange = (_value: string) => {
            bjccovmshb1i6x.instrumentFunction(14);
        };
        this.onProfileSelect = (_profileId: string) => {
            bjccovmshb1i6x.instrumentFunction(15);
        };
        this.onNewProfile = () => {
            bjccovmshb1i6x.instrumentFunction(16);
        };
        this.onDeleteProfile = () => {
            bjccovmshb1i6x.instrumentFunction(17);
        };
        this.onRememberPasswordChange = (_remember: boolean) => {
            bjccovmshb1i6x.instrumentFunction(18);
        };
        this.onClearPassword = () => {
            bjccovmshb1i6x.instrumentFunction(19);
        };
        this.onConnect = () => {
            bjccovmshb1i6x.instrumentFunction(20);
        };
        this.onOpenSettings = (_remoteControlSection: string) => {
            bjccovmshb1i6x.instrumentFunction(21);
        };
        this.setInitiallyProvidedValue(params);
        this.finalizeConstruction();
    }
    setInitiallyProvidedValue(params: HomePage_Params) {
        bjccovmshb1i6x.instrumentFunction(22);
        if (params.layoutMode === undefined) {
            this.__layoutMode.set(LayoutMode.COMPACT);
        }
        else {
        }
        if (params.connectionProfiles === undefined) {
            this.__connectionProfiles.set([]);
        }
        else {
        }
        if (params.selectedConnectionProfileId === undefined) {
            this.__selectedConnectionProfileId.set('');
        }
        else {
        }
        if (params.passwordLoading === undefined) {
            this.__passwordLoading.set(false);
        }
        else {
        }
        if (params.connectionFeedbackText === undefined) {
            this.__connectionFeedbackText.set('');
        }
        else {
        }
        if (params.connectionFeedbackTone === undefined) {
            this.__connectionFeedbackTone.set('neutral');
        }
        else {
        }
        if (params.isDark === undefined) {
            this.__isDark.set(false);
        }
        else {
        }
        if (params.xrdpServerBusy === undefined) {
            this.__xrdpServerBusy.set(false);
        }
        else {
        }
        if (params.xrdpServerState === undefined) {
            this.__xrdpServerState.set('Stopped');
        }
        else {
        }
        if (params.xrdpServerRunning === undefined) {
            this.__xrdpServerRunning.set(false);
        }
        else {
        }
        if (params.screenRecordingPermissionGranted === undefined) {
            this.__screenRecordingPermissionGranted.set(false);
        }
        else {
        }
        if (params.screenRecordingPermissionBusy === undefined) {
            this.__screenRecordingPermissionBusy.set(false);
        }
        else {
        }
        if (params.inputInjectionPermissionGranted === undefined) {
            this.__inputInjectionPermissionGranted.set(false);
        }
        else {
        }
        if (params.inputInjectionPermissionBusy === undefined) {
            this.__inputInjectionPermissionBusy.set(false);
        }
        else {
        }
        if (params.remoteControlServerAvailable === undefined) {
            this.__remoteControlServerAvailable.set(true);
        }
        else {
        }
        if (params.compactPage !== undefined) {
            this.compactPage = params.compactPage;
        }
        else {
        }
        if (params.onHostChange !== undefined) {
            this.onHostChange = params.onHostChange;
        }
        else {
        }
        if (params.onPortChange !== undefined) {
            this.onPortChange = params.onPortChange;
        }
        else {
        }
        if (params.onUsernameChange !== undefined) {
            this.onUsernameChange = params.onUsernameChange;
        }
        else {
        }
        if (params.onPasswordChange !== undefined) {
            this.onPasswordChange = params.onPasswordChange;
        }
        else {
        }
        if (params.onProfileSelect !== undefined) {
            this.onProfileSelect = params.onProfileSelect;
        }
        else {
        }
        if (params.onNewProfile !== undefined) {
            this.onNewProfile = params.onNewProfile;
        }
        else {
        }
        if (params.onDeleteProfile !== undefined) {
            this.onDeleteProfile = params.onDeleteProfile;
        }
        else {
        }
        if (params.onRememberPasswordChange !== undefined) {
            this.onRememberPasswordChange = params.onRememberPasswordChange;
        }
        else {
        }
        if (params.onClearPassword !== undefined) {
            this.onClearPassword = params.onClearPassword;
        }
        else {
        }
        if (params.onConnect !== undefined) {
            this.onConnect = params.onConnect;
        }
        else {
        }
        if (params.onOpenSettings !== undefined) {
            this.onOpenSettings = params.onOpenSettings;
        }
        else {
        }
    }
    updateStateVars(params: HomePage_Params) {
        bjccovmshb1i6x.instrumentFunction(23);
        this.__layoutMode.reset(params.layoutMode);
        this.__connectionProfiles.reset(params.connectionProfiles);
        this.__selectedConnectionProfileId.reset(params.selectedConnectionProfileId);
        this.__passwordLoading.reset(params.passwordLoading);
        this.__connectionFeedbackText.reset(params.connectionFeedbackText);
        this.__connectionFeedbackTone.reset(params.connectionFeedbackTone);
        this.__isDark.reset(params.isDark);
        this.__xrdpServerBusy.reset(params.xrdpServerBusy);
        this.__xrdpServerState.reset(params.xrdpServerState);
        this.__xrdpServerRunning.reset(params.xrdpServerRunning);
        this.__screenRecordingPermissionGranted.reset(params.screenRecordingPermissionGranted);
        this.__screenRecordingPermissionBusy.reset(params.screenRecordingPermissionBusy);
        this.__inputInjectionPermissionGranted.reset(params.inputInjectionPermissionGranted);
        this.__inputInjectionPermissionBusy.reset(params.inputInjectionPermissionBusy);
        this.__remoteControlServerAvailable.reset(params.remoteControlServerAvailable);
    }
    purgeVariableDependenciesOnElmtId(rmElmtId) {
        this.__layoutMode.purgeDependencyOnElmtId(rmElmtId);
        this.__host.purgeDependencyOnElmtId(rmElmtId);
        this.__port.purgeDependencyOnElmtId(rmElmtId);
        this.__username.purgeDependencyOnElmtId(rmElmtId);
        this.__password.purgeDependencyOnElmtId(rmElmtId);
        this.__connectionProfiles.purgeDependencyOnElmtId(rmElmtId);
        this.__selectedConnectionProfileId.purgeDependencyOnElmtId(rmElmtId);
        this.__rememberPassword.purgeDependencyOnElmtId(rmElmtId);
        this.__passwordLoading.purgeDependencyOnElmtId(rmElmtId);
        this.__connectionFeedbackText.purgeDependencyOnElmtId(rmElmtId);
        this.__connectionFeedbackTone.purgeDependencyOnElmtId(rmElmtId);
        this.__isDark.purgeDependencyOnElmtId(rmElmtId);
        this.__xrdpServerBusy.purgeDependencyOnElmtId(rmElmtId);
        this.__xrdpServerState.purgeDependencyOnElmtId(rmElmtId);
        this.__xrdpServerRunning.purgeDependencyOnElmtId(rmElmtId);
        this.__screenRecordingPermissionGranted.purgeDependencyOnElmtId(rmElmtId);
        this.__screenRecordingPermissionBusy.purgeDependencyOnElmtId(rmElmtId);
        this.__inputInjectionPermissionGranted.purgeDependencyOnElmtId(rmElmtId);
        this.__inputInjectionPermissionBusy.purgeDependencyOnElmtId(rmElmtId);
        this.__remoteControlServerAvailable.purgeDependencyOnElmtId(rmElmtId);
        this.__compactPage.purgeDependencyOnElmtId(rmElmtId);
    }
    aboutToBeDeleted() {
        this.__layoutMode.aboutToBeDeleted();
        this.__host.aboutToBeDeleted();
        this.__port.aboutToBeDeleted();
        this.__username.aboutToBeDeleted();
        this.__password.aboutToBeDeleted();
        this.__connectionProfiles.aboutToBeDeleted();
        this.__selectedConnectionProfileId.aboutToBeDeleted();
        this.__rememberPassword.aboutToBeDeleted();
        this.__passwordLoading.aboutToBeDeleted();
        this.__connectionFeedbackText.aboutToBeDeleted();
        this.__connectionFeedbackTone.aboutToBeDeleted();
        this.__isDark.aboutToBeDeleted();
        this.__xrdpServerBusy.aboutToBeDeleted();
        this.__xrdpServerState.aboutToBeDeleted();
        this.__xrdpServerRunning.aboutToBeDeleted();
        this.__screenRecordingPermissionGranted.aboutToBeDeleted();
        this.__screenRecordingPermissionBusy.aboutToBeDeleted();
        this.__inputInjectionPermissionGranted.aboutToBeDeleted();
        this.__inputInjectionPermissionBusy.aboutToBeDeleted();
        this.__remoteControlServerAvailable.aboutToBeDeleted();
        this.__compactPage.aboutToBeDeleted();
        SubscriberManager.Get().delete(this.id__());
        this.aboutToBeDeletedInternal();
    }
    private __layoutMode: SynchedPropertySimpleOneWayPU<LayoutMode>;
    get layoutMode() {
        bjccovmshb1i6x.instrumentFunction(24);
        return this.__layoutMode.get();
    }
    set layoutMode(newValue: LayoutMode) {
        bjccovmshb1i6x.instrumentFunction(25);
        this.__layoutMode.set(newValue);
    }
    private __host: SynchedPropertySimpleTwoWayPU<string>;
    get host() {
        bjccovmshb1i6x.instrumentFunction(26);
        return this.__host.get();
    }
    set host(newValue: string) {
        bjccovmshb1i6x.instrumentFunction(27);
        this.__host.set(newValue);
    }
    private __port: SynchedPropertySimpleTwoWayPU<string>;
    get port() {
        bjccovmshb1i6x.instrumentFunction(28);
        return this.__port.get();
    }
    set port(newValue: string) {
        bjccovmshb1i6x.instrumentFunction(29);
        this.__port.set(newValue);
    }
    private __username: SynchedPropertySimpleTwoWayPU<string>;
    get username() {
        bjccovmshb1i6x.instrumentFunction(30);
        return this.__username.get();
    }
    set username(newValue: string) {
        bjccovmshb1i6x.instrumentFunction(31);
        this.__username.set(newValue);
    }
    private __password: SynchedPropertySimpleTwoWayPU<string>;
    get password() {
        bjccovmshb1i6x.instrumentFunction(32);
        return this.__password.get();
    }
    set password(newValue: string) {
        bjccovmshb1i6x.instrumentFunction(33);
        this.__password.set(newValue);
    }
    private __connectionProfiles: SynchedPropertySimpleOneWayPU<WindowsConnectionProfile[]>;
    get connectionProfiles() {
        bjccovmshb1i6x.instrumentFunction(34);
        return this.__connectionProfiles.get();
    }
    set connectionProfiles(newValue: WindowsConnectionProfile[]) {
        bjccovmshb1i6x.instrumentFunction(35);
        this.__connectionProfiles.set(newValue);
    }
    private __selectedConnectionProfileId: SynchedPropertySimpleOneWayPU<string>;
    get selectedConnectionProfileId() {
        bjccovmshb1i6x.instrumentFunction(36);
        return this.__selectedConnectionProfileId.get();
    }
    set selectedConnectionProfileId(newValue: string) {
        bjccovmshb1i6x.instrumentFunction(37);
        this.__selectedConnectionProfileId.set(newValue);
    }
    private __rememberPassword: SynchedPropertySimpleTwoWayPU<boolean>;
    get rememberPassword() {
        bjccovmshb1i6x.instrumentFunction(38);
        return this.__rememberPassword.get();
    }
    set rememberPassword(newValue: boolean) {
        bjccovmshb1i6x.instrumentFunction(39);
        this.__rememberPassword.set(newValue);
    }
    private __passwordLoading: SynchedPropertySimpleOneWayPU<boolean>;
    get passwordLoading() {
        bjccovmshb1i6x.instrumentFunction(40);
        return this.__passwordLoading.get();
    }
    set passwordLoading(newValue: boolean) {
        bjccovmshb1i6x.instrumentFunction(41);
        this.__passwordLoading.set(newValue);
    }
    private __connectionFeedbackText: SynchedPropertySimpleOneWayPU<string>;
    get connectionFeedbackText() {
        bjccovmshb1i6x.instrumentFunction(42);
        return this.__connectionFeedbackText.get();
    }
    set connectionFeedbackText(newValue: string) {
        bjccovmshb1i6x.instrumentFunction(43);
        this.__connectionFeedbackText.set(newValue);
    }
    private __connectionFeedbackTone: SynchedPropertySimpleOneWayPU<SettingsStatusTone>;
    get connectionFeedbackTone() {
        bjccovmshb1i6x.instrumentFunction(44);
        return this.__connectionFeedbackTone.get();
    }
    set connectionFeedbackTone(newValue: SettingsStatusTone) {
        bjccovmshb1i6x.instrumentFunction(45);
        this.__connectionFeedbackTone.set(newValue);
    }
    private __isDark: SynchedPropertySimpleOneWayPU<boolean>;
    get isDark() {
        bjccovmshb1i6x.instrumentFunction(46);
        return this.__isDark.get();
    }
    set isDark(newValue: boolean) {
        bjccovmshb1i6x.instrumentFunction(47);
        this.__isDark.set(newValue);
    }
    private __xrdpServerBusy: SynchedPropertySimpleOneWayPU<boolean>;
    get xrdpServerBusy() {
        bjccovmshb1i6x.instrumentFunction(48);
        return this.__xrdpServerBusy.get();
    }
    set xrdpServerBusy(newValue: boolean) {
        bjccovmshb1i6x.instrumentFunction(49);
        this.__xrdpServerBusy.set(newValue);
    }
    private __xrdpServerState: SynchedPropertySimpleOneWayPU<string>;
    get xrdpServerState() {
        bjccovmshb1i6x.instrumentFunction(50);
        return this.__xrdpServerState.get();
    }
    set xrdpServerState(newValue: string) {
        bjccovmshb1i6x.instrumentFunction(51);
        this.__xrdpServerState.set(newValue);
    }
    private __xrdpServerRunning: SynchedPropertySimpleOneWayPU<boolean>;
    get xrdpServerRunning() {
        bjccovmshb1i6x.instrumentFunction(52);
        return this.__xrdpServerRunning.get();
    }
    set xrdpServerRunning(newValue: boolean) {
        bjccovmshb1i6x.instrumentFunction(53);
        this.__xrdpServerRunning.set(newValue);
    }
    private __screenRecordingPermissionGranted: SynchedPropertySimpleOneWayPU<boolean>;
    get screenRecordingPermissionGranted() {
        bjccovmshb1i6x.instrumentFunction(54);
        return this.__screenRecordingPermissionGranted.get();
    }
    set screenRecordingPermissionGranted(newValue: boolean) {
        bjccovmshb1i6x.instrumentFunction(55);
        this.__screenRecordingPermissionGranted.set(newValue);
    }
    private __screenRecordingPermissionBusy: SynchedPropertySimpleOneWayPU<boolean>;
    get screenRecordingPermissionBusy() {
        bjccovmshb1i6x.instrumentFunction(56);
        return this.__screenRecordingPermissionBusy.get();
    }
    set screenRecordingPermissionBusy(newValue: boolean) {
        bjccovmshb1i6x.instrumentFunction(57);
        this.__screenRecordingPermissionBusy.set(newValue);
    }
    private __inputInjectionPermissionGranted: SynchedPropertySimpleOneWayPU<boolean>;
    get inputInjectionPermissionGranted() {
        bjccovmshb1i6x.instrumentFunction(58);
        return this.__inputInjectionPermissionGranted.get();
    }
    set inputInjectionPermissionGranted(newValue: boolean) {
        bjccovmshb1i6x.instrumentFunction(59);
        this.__inputInjectionPermissionGranted.set(newValue);
    }
    private __inputInjectionPermissionBusy: SynchedPropertySimpleOneWayPU<boolean>;
    get inputInjectionPermissionBusy() {
        bjccovmshb1i6x.instrumentFunction(60);
        return this.__inputInjectionPermissionBusy.get();
    }
    set inputInjectionPermissionBusy(newValue: boolean) {
        bjccovmshb1i6x.instrumentFunction(61);
        this.__inputInjectionPermissionBusy.set(newValue);
    }
    private __remoteControlServerAvailable: SynchedPropertySimpleOneWayPU<boolean>;
    get remoteControlServerAvailable() {
        bjccovmshb1i6x.instrumentFunction(62);
        return this.__remoteControlServerAvailable.get();
    }
    set remoteControlServerAvailable(newValue: boolean) {
        bjccovmshb1i6x.instrumentFunction(63);
        this.__remoteControlServerAvailable.set(newValue);
    }
    private __compactPage: ObservedPropertySimplePU<HomeCompactPage>;
    get compactPage() {
        bjccovmshb1i6x.instrumentFunction(64);
        return this.__compactPage.get();
    }
    set compactPage(newValue: HomeCompactPage) {
        bjccovmshb1i6x.instrumentFunction(65);
        this.__compactPage.set(newValue);
    }
    private onHostChange: (value: string) => void;
    private onPortChange: (value: string) => void;
    private onUsernameChange: (value: string) => void;
    private onPasswordChange: (value: string) => void;
    private onProfileSelect: (profileId: string) => void;
    private onNewProfile: () => void;
    private onDeleteProfile: () => void;
    private onRememberPasswordChange: (remember: boolean) => void;
    private onClearPassword: () => void;
    private onConnect: () => void;
    private onOpenSettings: (remoteControlSection: string) => void;
    private selectProfile(profileId: string): void {
        bjccovmshb1i6x.instrumentFunction(77);
        bjccovmshb1i6x.instrumentRegion(77, 1);
        this.onProfileSelect(profileId);
        this.compactPage = 'details';
    }
    private createProfile(): void {
        bjccovmshb1i6x.instrumentFunction(78);
        bjccovmshb1i6x.instrumentRegion(78, 1);
        this.onNewProfile();
        this.compactPage = 'details';
    }
    private buildDeviceList(parent = null) {
        bjccovmshb1i6x.instrumentFunction(79);
        {
            this.observeComponentCreation2((elmtId, isInitialRender) => {
                bjccovmshb1i6x.instrumentFunction(80);
                if (isInitialRender) {
                    let componentCall = new HomeDeviceList(this, {
                        layoutMode: this.layoutMode,
                        connectionProfiles: this.connectionProfiles,
                        selectedConnectionProfileId: this.selectedConnectionProfileId,
                        isDark: this.isDark,
                        onProfileSelect: (profileId: string): void => {
                            bjccovmshb1i6x.instrumentFunction(81);
                            bjccovmshb1i6x.instrumentRegion(81, 1);
                            this.selectProfile(profileId);
                        },
                        onNewProfile: (): void => {
                            bjccovmshb1i6x.instrumentFunction(82);
                            bjccovmshb1i6x.instrumentRegion(82, 1);
                            this.createProfile();
                        }
                    }, undefined, elmtId, () => { }, { page: "common/src/main/ets/components/home/HomePage.ets", line: 71, col: 5 });
                    ViewPU.create(componentCall);
                    let paramsLambda = () => {
                        return {
                            layoutMode: this.layoutMode,
                            connectionProfiles: this.connectionProfiles,
                            selectedConnectionProfileId: this.selectedConnectionProfileId,
                            isDark: this.isDark,
                            onProfileSelect: (profileId: string): void => {
                                this.selectProfile(profileId);
                            },
                            onNewProfile: (): void => {
                                this.createProfile();
                            }
                        };
                    };
                    componentCall.paramsGenerator_ = paramsLambda;
                }
                else {
                    this.updateStateVarsOfChildByElmtId(elmtId, {
                        layoutMode: this.layoutMode,
                        connectionProfiles: this.connectionProfiles,
                        selectedConnectionProfileId: this.selectedConnectionProfileId,
                        isDark: this.isDark
                    });
                }
            }, { name: "HomeDeviceList" });
        }
    }
    private buildConnectionDetails(parent = null) {
        bjccovmshb1i6x.instrumentFunction(83);
        {
            this.observeComponentCreation2((elmtId, isInitialRender) => {
                bjccovmshb1i6x.instrumentFunction(84);
                if (isInitialRender) {
                    let componentCall = new HomeConnectionDetails(this, {
                        layoutMode: this.layoutMode,
                        host: this.__host,
                        port: this.__port,
                        username: this.__username,
                        password: this.__password,
                        rememberPassword: this.__rememberPassword,
                        passwordLoading: this.passwordLoading,
                        selectedConnectionProfileId: this.selectedConnectionProfileId,
                        connectionFeedbackText: this.connectionFeedbackText,
                        connectionFeedbackTone: this.connectionFeedbackTone,
                        isDark: this.isDark,
                        onHostChange: this.onHostChange,
                        onPortChange: this.onPortChange,
                        onUsernameChange: this.onUsernameChange,
                        onPasswordChange: this.onPasswordChange,
                        onDeleteProfile: this.onDeleteProfile,
                        onRememberPasswordChange: this.onRememberPasswordChange,
                        onClearPassword: this.onClearPassword,
                        onConnect: this.onConnect
                    }, undefined, elmtId, () => { }, { page: "common/src/main/ets/components/home/HomePage.ets", line: 87, col: 5 });
                    ViewPU.create(componentCall);
                    let paramsLambda = () => {
                        return {
                            layoutMode: this.layoutMode,
                            host: this.host,
                            port: this.port,
                            username: this.username,
                            password: this.password,
                            rememberPassword: this.rememberPassword,
                            passwordLoading: this.passwordLoading,
                            selectedConnectionProfileId: this.selectedConnectionProfileId,
                            connectionFeedbackText: this.connectionFeedbackText,
                            connectionFeedbackTone: this.connectionFeedbackTone,
                            isDark: this.isDark,
                            onHostChange: this.onHostChange,
                            onPortChange: this.onPortChange,
                            onUsernameChange: this.onUsernameChange,
                            onPasswordChange: this.onPasswordChange,
                            onDeleteProfile: this.onDeleteProfile,
                            onRememberPasswordChange: this.onRememberPasswordChange,
                            onClearPassword: this.onClearPassword,
                            onConnect: this.onConnect
                        };
                    };
                    componentCall.paramsGenerator_ = paramsLambda;
                }
                else {
                    this.updateStateVarsOfChildByElmtId(elmtId, {
                        layoutMode: this.layoutMode,
                        passwordLoading: this.passwordLoading,
                        selectedConnectionProfileId: this.selectedConnectionProfileId,
                        connectionFeedbackText: this.connectionFeedbackText,
                        connectionFeedbackTone: this.connectionFeedbackTone,
                        isDark: this.isDark
                    });
                }
            }, { name: "HomeConnectionDetails" });
        }
    }
    private buildExpandedMain(parent = null) {
        bjccovmshb1i6x.instrumentFunction(85);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i6x.instrumentFunction(86);
            Row.create({ space: 18 });
            Row.width('100%');
            Row.layoutWeight(1);
            Row.padding({ left: 30, right: 30, top: 22, bottom: 22 });
            Row.backgroundColor(HomeTheme.appBackground(this.isDark));
        }, Row);
        this.buildDeviceList.bind(this)();
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i6x.instrumentFunction(87);
            Scroll.create();
            Scroll.layoutWeight(1);
            Scroll.height('100%');
            Scroll.scrollBar(BarState.Auto);
        }, Scroll);
        this.buildConnectionDetails.bind(this)();
        Scroll.pop();
        Row.pop();
    }
    private buildCompactMain(parent = null) {
        bjccovmshb1i6x.instrumentFunction(88);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i6x.instrumentFunction(89);
            Column.create();
            Column.width('100%');
            Column.layoutWeight(1);
            Column.padding({ left: 12, right: 12, top: 12, bottom: 12 });
            Column.backgroundColor(HomeTheme.appBackground(this.isDark));
        }, Column);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i6x.instrumentFunction(90);
            If.create();
            if (this.compactPage === 'details') {
                bjccovmshb1i6x.instrumentBranch(90, 0, true);
                bjccovmshb1i6x.instrumentRegion(90, 1);
                this.ifElseBranchUpdateFunction(0, () => {
                    bjccovmshb1i6x.instrumentFunction(91);
                    this.observeComponentCreation2((elmtId, isInitialRender) => {
                        bjccovmshb1i6x.instrumentFunction(92);
                        Row.create({ space: 12 });
                        Row.width('100%');
                        Row.alignItems(VerticalAlign.Center);
                        Row.margin({ bottom: 8 });
                    }, Row);
                    {
                        this.observeComponentCreation2((elmtId, isInitialRender) => {
                            bjccovmshb1i6x.instrumentFunction(93);
                            if (isInitialRender) {
                                let componentCall = new SettingsBackButton(this, {
                                    isDark: this.isDark,
                                    onBack: (): void => {
                                        bjccovmshb1i6x.instrumentFunction(94);
                                        bjccovmshb1i6x.instrumentRegion(94, 1);
                                        this.compactPage = 'devices';
                                    }
                                }, undefined, elmtId, () => { }, { page: "common/src/main/ets/components/home/HomePage.ets", line: 132, col: 11 });
                                ViewPU.create(componentCall);
                                let paramsLambda = () => {
                                    return {
                                        isDark: this.isDark,
                                        onBack: (): void => {
                                            this.compactPage = 'devices';
                                        }
                                    };
                                };
                                componentCall.paramsGenerator_ = paramsLambda;
                            }
                            else {
                                this.updateStateVarsOfChildByElmtId(elmtId, {
                                    isDark: this.isDark
                                });
                            }
                        }, { name: "SettingsBackButton" });
                    }
                    this.observeComponentCreation2((elmtId, isInitialRender) => {
                        bjccovmshb1i6x.instrumentFunction(95);
                        Text.create(HomeText.CONNECTION_DETAILS_TITLE);
                        Text.fontSize(20);
                        Text.fontWeight(FontWeight.Bold);
                        Text.fontColor(SettingsTheme.primaryText(this.isDark));
                        Text.layoutWeight(1);
                        Text.maxLines(1);
                        Text.textOverflow({ overflow: TextOverflow.Ellipsis });
                    }, Text);
                    Text.pop();
                    Row.pop();
                    this.observeComponentCreation2((elmtId, isInitialRender) => {
                        bjccovmshb1i6x.instrumentFunction(96);
                        Scroll.create();
                        Scroll.width('100%');
                        Scroll.layoutWeight(1);
                        Scroll.scrollBar(BarState.Auto);
                    }, Scroll);
                    this.buildConnectionDetails.bind(this)();
                    Scroll.pop();
                });
            }
            else {
                bjccovmshb1i6x.instrumentBranch(90, 0, false);
                bjccovmshb1i6x.instrumentRegion(90, 2);
                this.ifElseBranchUpdateFunction(1, () => {
                    bjccovmshb1i6x.instrumentFunction(97);
                    this.buildDeviceList.bind(this)();
                });
            }
        }, If);
        If.pop();
        Column.pop();
    }
    initialRender() {
        bjccovmshb1i6x.instrumentFunction(98);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i6x.instrumentFunction(99);
            Column.create();
            Column.width('100%');
            Column.height('100%');
            Column.backgroundColor(HomeTheme.appBackground(this.isDark));
        }, Column);
        {
            this.observeComponentCreation2((elmtId, isInitialRender) => {
                bjccovmshb1i6x.instrumentFunction(100);
                if (isInitialRender) {
                    let componentCall = new HomeHeader(this, {
                        layoutMode: this.layoutMode,
                        isDark: this.isDark,
                        xrdpServerBusy: this.xrdpServerBusy,
                        xrdpServerState: this.xrdpServerState,
                        xrdpServerRunning: this.xrdpServerRunning,
                        remoteControlServerAvailable: this.remoteControlServerAvailable,
                        onOpenSettings: this.onOpenSettings
                    }, undefined, elmtId, () => { }, { page: "common/src/main/ets/components/home/HomePage.ets", line: 168, col: 7 });
                    ViewPU.create(componentCall);
                    let paramsLambda = () => {
                        return {
                            layoutMode: this.layoutMode,
                            isDark: this.isDark,
                            xrdpServerBusy: this.xrdpServerBusy,
                            xrdpServerState: this.xrdpServerState,
                            xrdpServerRunning: this.xrdpServerRunning,
                            remoteControlServerAvailable: this.remoteControlServerAvailable,
                            onOpenSettings: this.onOpenSettings
                        };
                    };
                    componentCall.paramsGenerator_ = paramsLambda;
                }
                else {
                    this.updateStateVarsOfChildByElmtId(elmtId, {
                        layoutMode: this.layoutMode,
                        isDark: this.isDark,
                        xrdpServerBusy: this.xrdpServerBusy,
                        xrdpServerState: this.xrdpServerState,
                        xrdpServerRunning: this.xrdpServerRunning,
                        remoteControlServerAvailable: this.remoteControlServerAvailable
                    });
                }
            }, { name: "HomeHeader" });
        }
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i6x.instrumentFunction(101);
            If.create();
            if (this.layoutMode === LayoutMode.COMPACT) {
                bjccovmshb1i6x.instrumentBranch(101, 0, true);
                bjccovmshb1i6x.instrumentRegion(101, 1);
                this.ifElseBranchUpdateFunction(0, () => {
                    bjccovmshb1i6x.instrumentFunction(102);
                    this.buildCompactMain.bind(this)();
                });
            }
            else {
                bjccovmshb1i6x.instrumentBranch(101, 0, false);
                bjccovmshb1i6x.instrumentRegion(101, 2);
                this.ifElseBranchUpdateFunction(1, () => {
                    bjccovmshb1i6x.instrumentFunction(103);
                    this.buildExpandedMain.bind(this)();
                });
            }
        }, If);
        If.pop();
        {
            this.observeComponentCreation2((elmtId, isInitialRender) => {
                bjccovmshb1i6x.instrumentFunction(104);
                if (isInitialRender) {
                    let componentCall = new HomeStatusFooter(this, {
                        layoutMode: this.layoutMode,
                        isDark: this.isDark,
                        xrdpServerBusy: this.xrdpServerBusy,
                        xrdpServerState: this.xrdpServerState,
                        xrdpServerRunning: this.xrdpServerRunning,
                        screenRecordingPermissionGranted: this.screenRecordingPermissionGranted,
                        screenRecordingPermissionBusy: this.screenRecordingPermissionBusy,
                        inputInjectionPermissionGranted: this.inputInjectionPermissionGranted,
                        inputInjectionPermissionBusy: this.inputInjectionPermissionBusy,
                        remoteControlServerAvailable: this.remoteControlServerAvailable,
                        onOpenSettings: this.onOpenSettings
                    }, undefined, elmtId, () => { }, { page: "common/src/main/ets/components/home/HomePage.ets", line: 182, col: 7 });
                    ViewPU.create(componentCall);
                    let paramsLambda = () => {
                        return {
                            layoutMode: this.layoutMode,
                            isDark: this.isDark,
                            xrdpServerBusy: this.xrdpServerBusy,
                            xrdpServerState: this.xrdpServerState,
                            xrdpServerRunning: this.xrdpServerRunning,
                            screenRecordingPermissionGranted: this.screenRecordingPermissionGranted,
                            screenRecordingPermissionBusy: this.screenRecordingPermissionBusy,
                            inputInjectionPermissionGranted: this.inputInjectionPermissionGranted,
                            inputInjectionPermissionBusy: this.inputInjectionPermissionBusy,
                            remoteControlServerAvailable: this.remoteControlServerAvailable,
                            onOpenSettings: this.onOpenSettings
                        };
                    };
                    componentCall.paramsGenerator_ = paramsLambda;
                }
                else {
                    this.updateStateVarsOfChildByElmtId(elmtId, {
                        layoutMode: this.layoutMode,
                        isDark: this.isDark,
                        xrdpServerBusy: this.xrdpServerBusy,
                        xrdpServerState: this.xrdpServerState,
                        xrdpServerRunning: this.xrdpServerRunning,
                        screenRecordingPermissionGranted: this.screenRecordingPermissionGranted,
                        screenRecordingPermissionBusy: this.screenRecordingPermissionBusy,
                        inputInjectionPermissionGranted: this.inputInjectionPermissionGranted,
                        inputInjectionPermissionBusy: this.inputInjectionPermissionBusy,
                        remoteControlServerAvailable: this.remoteControlServerAvailable
                    });
                }
            }, { name: "HomeStatusFooter" });
        }
        Column.pop();
    }
    rerender() {
        this.updateDirtyElements();
    }
}
