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
let bjccovmshb1iaq = new BjcCov({ version: "bjc v1.0.0", versionCode: 10000, path: "common/src/main/ets/components/settings/RemoteControlSettingsPage.ets", hash: "2ba2e29dd5995123bb34e5d4632ba65d884d25648b7bea8a5f205aa1a58b853b", lineCnt: 277, count: 0, projectPath: "", functions: { 0: { name: "anonymous_1", count: 0, regions: { 0: { startLoc: { line: 18, col: 11 }, endLoc: { line: 18, col: 21 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 0 }, 1: { name: "anonymous_2", count: 0, regions: { 0: { startLoc: { line: 20, col: 33 }, endLoc: { line: 20, col: 61 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 1 }, 2: { name: "anonymous_3", count: 0, regions: { 0: { startLoc: { line: 23, col: 33 }, endLoc: { line: 23, col: 45 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 2 }, 3: { name: "anonymous_4", count: 0, regions: { 0: { startLoc: { line: 26, col: 39 }, endLoc: { line: 26, col: 61 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 3 }, 4: { name: "anonymous_5", count: 0, regions: { 0: { startLoc: { line: 29, col: 39 }, endLoc: { line: 29, col: 52 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 4 }, 5: { name: "anonymous_6", count: 0, regions: { 0: { startLoc: { line: 32, col: 38 }, endLoc: { line: 32, col: 60 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 5 }, 6: { name: "anonymous_7", count: 0, regions: { 0: { startLoc: { line: 35, col: 38 }, endLoc: { line: 35, col: 51 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 6 }, 7: { name: "anonymous_8", count: 0, regions: { 0: { startLoc: { line: 38, col: 30 }, endLoc: { line: 38, col: 52 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 7 }, 8: { name: "anonymous_9", count: 0, regions: { 0: { startLoc: { line: 46, col: 22 }, endLoc: { line: 46, col: 53 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 8 }, 9: { name: "anonymous_10", count: 0, regions: { 0: { startLoc: { line: 55, col: 31 }, endLoc: { line: 55, col: 41 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 9 }, 10: { name: "anonymous_11", count: 0, regions: { 0: { startLoc: { line: 18, col: 24 }, endLoc: { line: 19, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 10 }, 11: { name: "anonymous_12", count: 0, regions: { 0: { startLoc: { line: 20, col: 64 }, endLoc: { line: 22, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 21, col: 5 }, endLoc: { line: 22, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 11 }, 12: { name: "anonymous_13", count: 0, regions: { 0: { startLoc: { line: 23, col: 48 }, endLoc: { line: 25, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 24, col: 5 }, endLoc: { line: 25, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 12 }, 13: { name: "anonymous_14", count: 0, regions: { 0: { startLoc: { line: 26, col: 64 }, endLoc: { line: 28, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 27, col: 5 }, endLoc: { line: 28, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 13 }, 14: { name: "anonymous_15", count: 0, regions: { 0: { startLoc: { line: 29, col: 55 }, endLoc: { line: 31, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 30, col: 5 }, endLoc: { line: 31, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 14 }, 15: { name: "anonymous_16", count: 0, regions: { 0: { startLoc: { line: 32, col: 63 }, endLoc: { line: 34, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 33, col: 5 }, endLoc: { line: 34, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 15 }, 16: { name: "anonymous_17", count: 0, regions: { 0: { startLoc: { line: 35, col: 54 }, endLoc: { line: 37, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 36, col: 5 }, endLoc: { line: 37, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 16 }, 17: { name: "anonymous_18", count: 0, regions: { 0: { startLoc: { line: 38, col: 55 }, endLoc: { line: 45, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 39, col: 5 }, endLoc: { line: 45, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 17 }, 18: { name: "anonymous_19", count: 0, regions: { 0: { startLoc: { line: 46, col: 56 }, endLoc: { line: 54, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 47, col: 5 }, endLoc: { line: 54, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 18 }, 19: { name: "anonymous_20", count: 0, regions: { 0: { startLoc: { line: 55, col: 44 }, endLoc: { line: 56, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 19 }, 20: { name: "setInitiallyProvidedValue", count: 0, regions: { 0: { startLoc: { line: 57, col: 27 }, endLoc: { line: 69, col: 54 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 20 }, 21: { name: "updateStateVars", count: 0, regions: { 0: { startLoc: { line: 57, col: 9 }, endLoc: { line: 69, col: 37 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 21 }, 22: { name: "anonymous_21", count: 0, regions: { 0: { startLoc: { line: 18, col: 11 }, endLoc: { line: 18, col: 21 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 22 }, 23: { name: "anonymous_22", count: 0, regions: { 0: { startLoc: { line: 20, col: 33 }, endLoc: { line: 20, col: 61 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 23 }, 24: { name: "anonymous_23", count: 0, regions: { 0: { startLoc: { line: 23, col: 33 }, endLoc: { line: 23, col: 45 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 24 }, 25: { name: "anonymous_24", count: 0, regions: { 0: { startLoc: { line: 26, col: 39 }, endLoc: { line: 26, col: 61 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 25 }, 26: { name: "anonymous_25", count: 0, regions: { 0: { startLoc: { line: 29, col: 39 }, endLoc: { line: 29, col: 52 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 26 }, 27: { name: "anonymous_26", count: 0, regions: { 0: { startLoc: { line: 32, col: 38 }, endLoc: { line: 32, col: 60 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 27 }, 28: { name: "anonymous_27", count: 0, regions: { 0: { startLoc: { line: 35, col: 38 }, endLoc: { line: 35, col: 51 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 28 }, 29: { name: "anonymous_28", count: 0, regions: { 0: { startLoc: { line: 38, col: 30 }, endLoc: { line: 38, col: 52 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 29 }, 30: { name: "anonymous_29", count: 0, regions: { 0: { startLoc: { line: 46, col: 22 }, endLoc: { line: 46, col: 53 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 30 }, 31: { name: "anonymous_30", count: 0, regions: { 0: { startLoc: { line: 55, col: 31 }, endLoc: { line: 55, col: 41 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 31 }, 32: { name: "isDark", count: 0, regions: { 0: { startLoc: { line: 57, col: 9 }, endLoc: { line: 57, col: 15 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 32 }, 33: { name: "isDark", count: 0, regions: { 0: { startLoc: { line: 57, col: 9 }, endLoc: { line: 57, col: 24 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 33 }, 34: { name: "targetSection", count: 0, regions: { 0: { startLoc: { line: 58, col: 9 }, endLoc: { line: 58, col: 22 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 34 }, 35: { name: "targetSection", count: 0, regions: { 0: { startLoc: { line: 58, col: 9 }, endLoc: { line: 58, col: 30 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 35 }, 36: { name: "remoteAccessCode", count: 0, regions: { 0: { startLoc: { line: 59, col: 9 }, endLoc: { line: 59, col: 25 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 36 }, 37: { name: "remoteAccessCode", count: 0, regions: { 0: { startLoc: { line: 59, col: 9 }, endLoc: { line: 59, col: 33 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 37 }, 38: { name: "remoteAccessCodeGateEnabled", count: 0, regions: { 0: { startLoc: { line: 60, col: 9 }, endLoc: { line: 60, col: 36 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 38 }, 39: { name: "remoteAccessCodeGateEnabled", count: 0, regions: { 0: { startLoc: { line: 60, col: 9 }, endLoc: { line: 60, col: 45 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 39 }, 40: { name: "screenRecordingPermissionGranted", count: 0, regions: { 0: { startLoc: { line: 61, col: 9 }, endLoc: { line: 61, col: 41 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 40 }, 41: { name: "screenRecordingPermissionGranted", count: 0, regions: { 0: { startLoc: { line: 61, col: 9 }, endLoc: { line: 61, col: 50 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 41 }, 42: { name: "inputInjectionPermissionGranted", count: 0, regions: { 0: { startLoc: { line: 62, col: 9 }, endLoc: { line: 62, col: 40 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 42 }, 43: { name: "inputInjectionPermissionGranted", count: 0, regions: { 0: { startLoc: { line: 62, col: 9 }, endLoc: { line: 62, col: 49 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 43 }, 44: { name: "xrdpServerRunning", count: 0, regions: { 0: { startLoc: { line: 63, col: 9 }, endLoc: { line: 63, col: 26 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 44 }, 45: { name: "xrdpServerRunning", count: 0, regions: { 0: { startLoc: { line: 63, col: 9 }, endLoc: { line: 63, col: 35 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 45 }, 46: { name: "xrdpServerState", count: 0, regions: { 0: { startLoc: { line: 64, col: 9 }, endLoc: { line: 64, col: 24 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 46 }, 47: { name: "xrdpServerState", count: 0, regions: { 0: { startLoc: { line: 64, col: 9 }, endLoc: { line: 64, col: 32 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 47 }, 48: { name: "xrdpServerPort", count: 0, regions: { 0: { startLoc: { line: 65, col: 9 }, endLoc: { line: 65, col: 23 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 48 }, 49: { name: "xrdpServerPort", count: 0, regions: { 0: { startLoc: { line: 65, col: 9 }, endLoc: { line: 65, col: 31 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 49 }, 50: { name: "xrdpServerMessage", count: 0, regions: { 0: { startLoc: { line: 66, col: 9 }, endLoc: { line: 66, col: 26 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 50 }, 51: { name: "xrdpServerMessage", count: 0, regions: { 0: { startLoc: { line: 66, col: 9 }, endLoc: { line: 66, col: 34 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 51 }, 52: { name: "xrdpServerBusy", count: 0, regions: { 0: { startLoc: { line: 67, col: 9 }, endLoc: { line: 67, col: 23 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 52 }, 53: { name: "xrdpServerBusy", count: 0, regions: { 0: { startLoc: { line: 67, col: 9 }, endLoc: { line: 67, col: 32 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 53 }, 54: { name: "screenRecordingPermissionBusy", count: 0, regions: { 0: { startLoc: { line: 68, col: 9 }, endLoc: { line: 68, col: 38 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 54 }, 55: { name: "screenRecordingPermissionBusy", count: 0, regions: { 0: { startLoc: { line: 68, col: 9 }, endLoc: { line: 68, col: 47 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 55 }, 56: { name: "inputInjectionPermissionBusy", count: 0, regions: { 0: { startLoc: { line: 69, col: 9 }, endLoc: { line: 69, col: 37 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 56 }, 57: { name: "inputInjectionPermissionBusy", count: 0, regions: { 0: { startLoc: { line: 69, col: 9 }, endLoc: { line: 69, col: 46 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 57 }, 58: { name: "localAccessCode", count: 0, regions: { 0: { startLoc: { line: 70, col: 18 }, endLoc: { line: 70, col: 33 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 58 }, 59: { name: "localAccessCode", count: 0, regions: { 0: { startLoc: { line: 70, col: 18 }, endLoc: { line: 70, col: 41 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 59 }, 60: { name: "localGateEnabled", count: 0, regions: { 0: { startLoc: { line: 71, col: 18 }, endLoc: { line: 71, col: 34 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 60 }, 61: { name: "localGateEnabled", count: 0, regions: { 0: { startLoc: { line: 71, col: 18 }, endLoc: { line: 71, col: 43 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 61 }, 62: { name: "localScreenRecordingGranted", count: 0, regions: { 0: { startLoc: { line: 72, col: 18 }, endLoc: { line: 72, col: 45 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 62 }, 63: { name: "localScreenRecordingGranted", count: 0, regions: { 0: { startLoc: { line: 72, col: 18 }, endLoc: { line: 72, col: 54 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 63 }, 64: { name: "localInputInjectionGranted", count: 0, regions: { 0: { startLoc: { line: 73, col: 18 }, endLoc: { line: 73, col: 44 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 64 }, 65: { name: "localInputInjectionGranted", count: 0, regions: { 0: { startLoc: { line: 73, col: 18 }, endLoc: { line: 73, col: 53 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 65 }, 66: { name: "localXrdpServerRunning", count: 0, regions: { 0: { startLoc: { line: 74, col: 18 }, endLoc: { line: 74, col: 40 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 66 }, 67: { name: "localXrdpServerRunning", count: 0, regions: { 0: { startLoc: { line: 74, col: 18 }, endLoc: { line: 74, col: 49 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 67 }, 68: { name: "localXrdpServerState", count: 0, regions: { 0: { startLoc: { line: 75, col: 18 }, endLoc: { line: 75, col: 38 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 68 }, 69: { name: "localXrdpServerState", count: 0, regions: { 0: { startLoc: { line: 75, col: 18 }, endLoc: { line: 75, col: 46 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 69 }, 70: { name: "localXrdpServerPort", count: 0, regions: { 0: { startLoc: { line: 76, col: 18 }, endLoc: { line: 76, col: 37 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 70 }, 71: { name: "localXrdpServerPort", count: 0, regions: { 0: { startLoc: { line: 76, col: 18 }, endLoc: { line: 76, col: 45 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 71 }, 72: { name: "localXrdpServerMessage", count: 0, regions: { 0: { startLoc: { line: 77, col: 18 }, endLoc: { line: 77, col: 40 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 72 }, 73: { name: "localXrdpServerMessage", count: 0, regions: { 0: { startLoc: { line: 77, col: 18 }, endLoc: { line: 77, col: 48 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 73 }, 74: { name: "localXrdpServerBusy", count: 0, regions: { 0: { startLoc: { line: 78, col: 18 }, endLoc: { line: 78, col: 37 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 74 }, 75: { name: "localXrdpServerBusy", count: 0, regions: { 0: { startLoc: { line: 78, col: 18 }, endLoc: { line: 78, col: 46 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 75 }, 76: { name: "aboutToAppear", count: 0, regions: { 0: { startLoc: { line: 82, col: 3 }, endLoc: { line: 95, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 83, col: 5 }, endLoc: { line: 95, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 76 }, 77: { name: "setGateEnabled", count: 0, regions: { 0: { startLoc: { line: 97, col: 3 }, endLoc: { line: 103, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 100, col: 30 }, endLoc: { line: 102, col: 6 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 100, col: 9 }, endLoc: { line: 100, col: 28 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 77 }, 78: { name: "regenerateAccessCode", count: 0, regions: { 0: { startLoc: { line: 105, col: 3 }, endLoc: { line: 110, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 107, col: 30 }, endLoc: { line: 109, col: 6 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 107, col: 9 }, endLoc: { line: 107, col: 28 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 78 }, 79: { name: "applyXrdpServerStatus", count: 0, regions: { 0: { startLoc: { line: 112, col: 3 }, endLoc: { line: 117, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 113, col: 5 }, endLoc: { line: 117, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 114, col: 33 }, endLoc: { line: 114, col: 83 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 115, col: 32 }, endLoc: { line: 115, col: 68 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 79 }, 80: { name: "refreshXrdpServerStatus", count: 0, regions: { 0: { startLoc: { line: 119, col: 3 }, endLoc: { line: 121, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 120, col: 5 }, endLoc: { line: 121, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 80 }, 81: { name: "startXrdpServer", count: 0, regions: { 0: { startLoc: { line: 123, col: 3 }, endLoc: { line: 140, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 124, col: 96 }, endLoc: { line: 126, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 127, col: 5 }, endLoc: { line: 140, col: 4 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 131, col: 9 }, endLoc: { line: 133, col: 8 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 124, col: 9 }, endLoc: { line: 124, col: 94 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 81 }, 82: { name: "anonymous_31", count: 0, regions: { 0: { startLoc: { line: 129, col: 13 }, endLoc: { line: 133, col: 8 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 82 }, 83: { name: "anonymous_32", count: 0, regions: { 0: { startLoc: { line: 134, col: 14 }, endLoc: { line: 136, col: 8 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 135, col: 9 }, endLoc: { line: 136, col: 8 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 83 }, 84: { name: "anonymous_33", count: 0, regions: { 0: { startLoc: { line: 137, col: 16 }, endLoc: { line: 139, col: 8 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 138, col: 9 }, endLoc: { line: 139, col: 8 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 84 }, 85: { name: "refreshScreenRecordingState", count: 0, regions: { 0: { startLoc: { line: 142, col: 3 }, endLoc: { line: 144, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 143, col: 5 }, endLoc: { line: 144, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 85 }, 86: { name: "requestScreenRecordingPermission", count: 0, regions: { 0: { startLoc: { line: 146, col: 3 }, endLoc: { line: 169, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 147, col: 84 }, endLoc: { line: 149, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 150, col: 5 }, endLoc: { line: 169, col: 4 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 158, col: 9 }, endLoc: { line: 159, col: 8 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 147, col: 9 }, endLoc: { line: 147, col: 82 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 86 }, 87: { name: "anonymous_34", count: 0, regions: { 0: { startLoc: { line: 152, col: 13 }, endLoc: { line: 159, col: 8 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 155, col: 23 }, endLoc: { line: 157, col: 10 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 155, col: 13 }, endLoc: { line: 155, col: 21 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 87 }, 88: { name: "anonymous_35", count: 0, regions: { 0: { startLoc: { line: 160, col: 14 }, endLoc: { line: 165, col: 8 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 162, col: 48 }, endLoc: { line: 164, col: 10 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 162, col: 13 }, endLoc: { line: 162, col: 46 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 88 }, 89: { name: "anonymous_36", count: 0, regions: { 0: { startLoc: { line: 166, col: 16 }, endLoc: { line: 168, col: 8 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 167, col: 9 }, endLoc: { line: 168, col: 8 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 89 }, 90: { name: "refreshInputInjectionState", count: 0, regions: { 0: { startLoc: { line: 171, col: 3 }, endLoc: { line: 173, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 172, col: 5 }, endLoc: { line: 173, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 90 }, 91: { name: "requestInputInjectionPermission", count: 0, regions: { 0: { startLoc: { line: 175, col: 3 }, endLoc: { line: 190, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 176, col: 82 }, endLoc: { line: 178, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 179, col: 5 }, endLoc: { line: 190, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 176, col: 9 }, endLoc: { line: 176, col: 80 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 91 }, 92: { name: "anonymous_37", count: 0, regions: { 0: { startLoc: { line: 181, col: 13 }, endLoc: { line: 183, col: 8 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 182, col: 9 }, endLoc: { line: 183, col: 8 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 92 }, 93: { name: "anonymous_38", count: 0, regions: { 0: { startLoc: { line: 184, col: 14 }, endLoc: { line: 186, col: 8 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 185, col: 9 }, endLoc: { line: 186, col: 8 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 93 }, 94: { name: "anonymous_39", count: 0, regions: { 0: { startLoc: { line: 187, col: 16 }, endLoc: { line: 189, col: 8 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 188, col: 9 }, endLoc: { line: 189, col: 8 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 94 }, 95: { name: "initialRender", count: 0, regions: { 0: { startLoc: { line: 192, col: 3 }, endLoc: { line: 275, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 95 }, 96: { name: "anonymous_40", count: 0, regions: { 0: { startLoc: { line: 193, col: 5 }, endLoc: { line: 274, col: 63 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 96 }, 97: { name: "anonymous_41", count: 0, regions: { 0: { startLoc: { line: 194, col: 7 }, endLoc: { line: 197, col: 28 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 97 }, 98: { name: "anonymous_43", count: 0, regions: { 0: { startLoc: { line: 201, col: 7 }, endLoc: { line: 270, col: 22 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 98 }, 99: { name: "anonymous_44", count: 0, regions: { 0: { startLoc: { line: 202, col: 9 }, endLoc: { line: 268, col: 22 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 99 }, 100: { name: "anonymous_45", count: 0, regions: { 0: { startLoc: { line: 203, col: 11 }, endLoc: { line: 206, col: 32 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 100 }, 101: { name: "anonymous_47", count: 0, regions: { 0: { startLoc: { line: 209, col: 11 }, endLoc: { line: 216, col: 49 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 101 }, 102: { name: "anonymous_48", count: 0, regions: { 0: { startLoc: { line: 217, col: 24 }, endLoc: { line: 219, col: 14 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 218, col: 15 }, endLoc: { line: 219, col: 14 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 102 }, 103: { name: "anonymous_49", count: 0, regions: { 0: { startLoc: { line: 220, col: 22 }, endLoc: { line: 222, col: 14 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 221, col: 15 }, endLoc: { line: 222, col: 14 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 103 }, 104: { name: "anonymous_51", count: 0, regions: { 0: { startLoc: { line: 224, col: 11 }, endLoc: { line: 229, col: 53 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 104 }, 105: { name: "anonymous_52", count: 0, regions: { 0: { startLoc: { line: 230, col: 24 }, endLoc: { line: 232, col: 14 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 231, col: 15 }, endLoc: { line: 232, col: 14 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 105 }, 106: { name: "anonymous_54", count: 0, regions: { 0: { startLoc: { line: 234, col: 11 }, endLoc: { line: 243, col: 52 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 106 }, 107: { name: "anonymous_55", count: 0, regions: { 0: { startLoc: { line: 244, col: 24 }, endLoc: { line: 246, col: 14 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 245, col: 15 }, endLoc: { line: 246, col: 14 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 107 }, 108: { name: "anonymous_57", count: 0, regions: { 0: { startLoc: { line: 248, col: 11 }, endLoc: { line: 252, col: 45 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 108 }, 109: { name: "anonymous_58", count: 0, regions: { 0: { startLoc: { line: 253, col: 27 }, endLoc: { line: 255, col: 14 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 254, col: 15 }, endLoc: { line: 255, col: 14 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 109 }, 110: { name: "anonymous_59", count: 0, regions: { 0: { startLoc: { line: 256, col: 27 }, endLoc: { line: 258, col: 14 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 257, col: 15 }, endLoc: { line: 258, col: 14 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 110 }, 111: { name: "anonymous_61", count: 0, regions: { 0: { startLoc: { line: 260, col: 11 }, endLoc: { line: 262, col: 46 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 111 } }, exeLine: { 0: 1, 1: 2, 2: 3, 3: 4, 4: 5, 5: 6, 6: 7, 7: 8, 8: 9, 9: 10, 10: 11, 11: 12, 12: 13, 13: 14, 14: 17, 15: 18, 16: 20, 17: 21, 18: 23, 19: 24, 20: 26, 21: 27, 22: 29, 23: 30, 24: 32, 25: 33, 26: 35, 27: 36, 28: 38, 29: 39, 30: 40, 31: 41, 32: 42, 33: 43, 34: 46, 35: 47, 36: 48, 37: 49, 38: 50, 39: 51, 40: 53, 41: 55, 42: 57, 43: 58, 44: 59, 45: 60, 46: 61, 47: 62, 48: 63, 49: 64, 50: 65, 51: 66, 52: 67, 53: 68, 54: 69, 55: 70, 56: 71, 57: 72, 58: 73, 59: 74, 60: 75, 61: 76, 62: 77, 63: 78, 64: 79, 65: 80, 66: 82, 67: 83, 68: 84, 69: 85, 70: 86, 71: 87, 72: 88, 73: 89, 74: 90, 75: 91, 76: 92, 77: 93, 78: 94, 79: 97, 80: 98, 81: 99, 82: 100, 83: 101, 84: 105, 85: 106, 86: 107, 87: 108, 88: 112, 89: 113, 90: 114, 91: 115, 92: 116, 93: 119, 94: 120, 95: 123, 96: 124, 97: 125, 98: 127, 99: 128, 100: 129, 101: 130, 102: 131, 103: 132, 104: 134, 105: 135, 106: 137, 107: 138, 108: 142, 109: 143, 110: 146, 111: 147, 112: 148, 113: 150, 114: 151, 115: 152, 116: 153, 117: 154, 118: 155, 119: 156, 120: 158, 121: 160, 122: 161, 123: 162, 124: 163, 125: 166, 126: 167, 127: 171, 128: 172, 129: 175, 130: 176, 131: 177, 132: 179, 133: 180, 134: 181, 135: 182, 136: 184, 137: 185, 138: 187, 139: 188, 140: 192, 141: 193, 142: 194, 143: 195, 144: 196, 145: 197, 146: 198, 147: 201, 148: 202, 149: 203, 150: 204, 151: 205, 152: 206, 153: 209, 154: 210, 155: 211, 156: 212, 157: 213, 158: 214, 159: 215, 160: 216, 161: 217, 162: 218, 163: 220, 164: 221, 165: 224, 166: 225, 167: 226, 168: 227, 169: 228, 170: 229, 171: 230, 172: 231, 173: 234, 174: 235, 175: 236, 176: 237, 177: 238, 178: 239, 179: 240, 180: 241, 181: 242, 182: 243, 183: 244, 184: 245, 185: 248, 186: 249, 187: 250, 188: 251, 189: 252, 190: 253, 191: 254, 192: 256, 193: 257, 194: 260, 195: 261, 196: 262, 197: 263, 198: 266, 199: 267, 200: 268, 201: 270, 202: 272, 203: 273, 204: 274 } });
if (!("finalizeConstruction" in ViewPU.prototype)) {
    Reflect.set(ViewPU.prototype, "finalizeConstruction", () => { });
}
interface RemoteControlSettingsPage_Params {
    onBack?: () => void;
    onRemoteAccessCodeGateChange?: (enabled: boolean) => string;
    onRemoteAccessCodeRegenerate?: () => string;
    onRequestScreenRecordingPermission?: () => Promise<boolean>;
    onRefreshScreenRecordingPermission?: () => boolean;
    onRequestInputInjectionPermission?: () => Promise<boolean>;
    onRefreshInputInjectionPermission?: () => boolean;
    onRefreshXrdpServerStatus?: () => XrdpServerStatus;
    onStartXrdpServer?: () => Promise<XrdpServerStatus>;
    onOpenRemoteFilesDirectory?: () => void;
    isDark?: boolean;
    targetSection?: string;
    remoteAccessCode?: string;
    remoteAccessCodeGateEnabled?: boolean;
    screenRecordingPermissionGranted?: boolean;
    inputInjectionPermissionGranted?: boolean;
    xrdpServerRunning?: boolean;
    xrdpServerState?: string;
    xrdpServerPort?: number;
    xrdpServerMessage?: string;
    xrdpServerBusy?: boolean;
    screenRecordingPermissionBusy?: boolean;
    inputInjectionPermissionBusy?: boolean;
    localAccessCode?: string;
    localGateEnabled?: boolean;
    localScreenRecordingGranted?: boolean;
    localInputInjectionGranted?: boolean;
    localXrdpServerRunning?: boolean;
    localXrdpServerState?: string;
    localXrdpServerPort?: number;
    localXrdpServerMessage?: string;
    localXrdpServerBusy?: boolean;
    screenPermissionRequestPending?: boolean;
    inputPermissionRequestPending?: boolean;
}
import { SettingsPageHeader, SettingsSectionTitle, SettingsTheme } from "@normalized:N&&&common/src/main/ets/components/settings/SettingsUi&";
import type { XrdpServerStatus } from '../../rdp/XrdpServerController';
import { SettingsRemoteControlSection, SettingsText } from "@normalized:N&&&common/src/main/ets/components/settings/SettingsConstants&";
import { SettingsAccent, SettingsResources } from "@normalized:N&&&common/src/main/ets/components/settings/SettingsUi&";
import { RemoteAccessCard, RemoteFilesCard, RemotePermissionCard, XrdpServerCard } from "@normalized:N&&&common/src/main/ets/components/settings/RemoteControlCards&";
export class RemoteControlSettingsPage extends ViewPU {
    constructor(parent, params, __localStorage, elmtId = -1, paramsLambda = undefined, extraInfo) {
        super(parent, __localStorage, elmtId, extraInfo);
        if (typeof paramsLambda === "function") {
            this.paramsGenerator_ = paramsLambda;
        }
        this.onBack = () => {
            bjccovmshb1iaq.instrumentFunction(10);
        };
        this.onRemoteAccessCodeGateChange = (_enabled: boolean) => {
            bjccovmshb1iaq.instrumentFunction(11);
            bjccovmshb1iaq.instrumentRegion(11, 1);
            return '';
        };
        this.onRemoteAccessCodeRegenerate = () => {
            bjccovmshb1iaq.instrumentFunction(12);
            bjccovmshb1iaq.instrumentRegion(12, 1);
            return '';
        };
        this.onRequestScreenRecordingPermission = () => {
            bjccovmshb1iaq.instrumentFunction(13);
            bjccovmshb1iaq.instrumentRegion(13, 1);
            return Promise.resolve(false);
        };
        this.onRefreshScreenRecordingPermission = () => {
            bjccovmshb1iaq.instrumentFunction(14);
            bjccovmshb1iaq.instrumentRegion(14, 1);
            return false;
        };
        this.onRequestInputInjectionPermission = () => {
            bjccovmshb1iaq.instrumentFunction(15);
            bjccovmshb1iaq.instrumentRegion(15, 1);
            return Promise.resolve(false);
        };
        this.onRefreshInputInjectionPermission = () => {
            bjccovmshb1iaq.instrumentFunction(16);
            bjccovmshb1iaq.instrumentRegion(16, 1);
            return false;
        };
        this.onRefreshXrdpServerStatus = () => {
            bjccovmshb1iaq.instrumentFunction(17);
            bjccovmshb1iaq.instrumentRegion(17, 1);
            return {
                running: false,
                state: 'Stopped',
                port: 3390,
                message: ''
            };
        };
        this.onStartXrdpServer = () => {
            bjccovmshb1iaq.instrumentFunction(18);
            bjccovmshb1iaq.instrumentRegion(18, 1);
            const status: XrdpServerStatus = {
                running: false,
                state: 'Stopped',
                port: 3390,
                message: ''
            };
            return Promise.resolve(status);
        };
        this.onOpenRemoteFilesDirectory = () => {
            bjccovmshb1iaq.instrumentFunction(19);
        };
        this.__isDark = new SynchedPropertySimpleOneWayPU(params.isDark, this, "isDark");
        this.__targetSection = new SynchedPropertySimpleOneWayPU(params.targetSection, this, "targetSection");
        this.__remoteAccessCode = new SynchedPropertySimpleOneWayPU(params.remoteAccessCode, this, "remoteAccessCode");
        this.__remoteAccessCodeGateEnabled = new SynchedPropertySimpleOneWayPU(params.remoteAccessCodeGateEnabled, this, "remoteAccessCodeGateEnabled");
        this.__screenRecordingPermissionGranted = new SynchedPropertySimpleOneWayPU(params.screenRecordingPermissionGranted, this, "screenRecordingPermissionGranted");
        this.__inputInjectionPermissionGranted = new SynchedPropertySimpleOneWayPU(params.inputInjectionPermissionGranted, this, "inputInjectionPermissionGranted");
        this.__xrdpServerRunning = new SynchedPropertySimpleOneWayPU(params.xrdpServerRunning, this, "xrdpServerRunning");
        this.__xrdpServerState = new SynchedPropertySimpleOneWayPU(params.xrdpServerState, this, "xrdpServerState");
        this.__xrdpServerPort = new SynchedPropertySimpleOneWayPU(params.xrdpServerPort, this, "xrdpServerPort");
        this.__xrdpServerMessage = new SynchedPropertySimpleOneWayPU(params.xrdpServerMessage, this, "xrdpServerMessage");
        this.__xrdpServerBusy = new SynchedPropertySimpleOneWayPU(params.xrdpServerBusy, this, "xrdpServerBusy");
        this.__screenRecordingPermissionBusy = new SynchedPropertySimpleOneWayPU(params.screenRecordingPermissionBusy, this, "screenRecordingPermissionBusy");
        this.__inputInjectionPermissionBusy = new SynchedPropertySimpleOneWayPU(params.inputInjectionPermissionBusy, this, "inputInjectionPermissionBusy");
        this.__localAccessCode = new ObservedPropertySimplePU('000000', this, "localAccessCode");
        this.__localGateEnabled = new ObservedPropertySimplePU(false, this, "localGateEnabled");
        this.__localScreenRecordingGranted = new ObservedPropertySimplePU(false, this, "localScreenRecordingGranted");
        this.__localInputInjectionGranted = new ObservedPropertySimplePU(false, this, "localInputInjectionGranted");
        this.__localXrdpServerRunning = new ObservedPropertySimplePU(false, this, "localXrdpServerRunning");
        this.__localXrdpServerState = new ObservedPropertySimplePU('Stopped', this, "localXrdpServerState");
        this.__localXrdpServerPort = new ObservedPropertySimplePU(3390, this, "localXrdpServerPort");
        this.__localXrdpServerMessage = new ObservedPropertySimplePU('', this, "localXrdpServerMessage");
        this.__localXrdpServerBusy = new ObservedPropertySimplePU(false, this, "localXrdpServerBusy");
        this.screenPermissionRequestPending = false;
        this.inputPermissionRequestPending = false;
        this.setInitiallyProvidedValue(params);
        this.finalizeConstruction();
    }
    setInitiallyProvidedValue(params: RemoteControlSettingsPage_Params) {
        bjccovmshb1iaq.instrumentFunction(20);
        if (params.onBack !== undefined) {
            this.onBack = params.onBack;
        }
        else {
        }
        if (params.onRemoteAccessCodeGateChange !== undefined) {
            this.onRemoteAccessCodeGateChange = params.onRemoteAccessCodeGateChange;
        }
        else {
        }
        if (params.onRemoteAccessCodeRegenerate !== undefined) {
            this.onRemoteAccessCodeRegenerate = params.onRemoteAccessCodeRegenerate;
        }
        else {
        }
        if (params.onRequestScreenRecordingPermission !== undefined) {
            this.onRequestScreenRecordingPermission = params.onRequestScreenRecordingPermission;
        }
        else {
        }
        if (params.onRefreshScreenRecordingPermission !== undefined) {
            this.onRefreshScreenRecordingPermission = params.onRefreshScreenRecordingPermission;
        }
        else {
        }
        if (params.onRequestInputInjectionPermission !== undefined) {
            this.onRequestInputInjectionPermission = params.onRequestInputInjectionPermission;
        }
        else {
        }
        if (params.onRefreshInputInjectionPermission !== undefined) {
            this.onRefreshInputInjectionPermission = params.onRefreshInputInjectionPermission;
        }
        else {
        }
        if (params.onRefreshXrdpServerStatus !== undefined) {
            this.onRefreshXrdpServerStatus = params.onRefreshXrdpServerStatus;
        }
        else {
        }
        if (params.onStartXrdpServer !== undefined) {
            this.onStartXrdpServer = params.onStartXrdpServer;
        }
        else {
        }
        if (params.onOpenRemoteFilesDirectory !== undefined) {
            this.onOpenRemoteFilesDirectory = params.onOpenRemoteFilesDirectory;
        }
        else {
        }
        if (params.isDark === undefined) {
            this.__isDark.set(false);
        }
        else {
        }
        if (params.targetSection === undefined) {
            this.__targetSection.set('');
        }
        else {
        }
        if (params.remoteAccessCode === undefined) {
            this.__remoteAccessCode.set('000000');
        }
        else {
        }
        if (params.remoteAccessCodeGateEnabled === undefined) {
            this.__remoteAccessCodeGateEnabled.set(false);
        }
        else {
        }
        if (params.screenRecordingPermissionGranted === undefined) {
            this.__screenRecordingPermissionGranted.set(false);
        }
        else {
        }
        if (params.inputInjectionPermissionGranted === undefined) {
            this.__inputInjectionPermissionGranted.set(false);
        }
        else {
        }
        if (params.xrdpServerRunning === undefined) {
            this.__xrdpServerRunning.set(false);
        }
        else {
        }
        if (params.xrdpServerState === undefined) {
            this.__xrdpServerState.set('Stopped');
        }
        else {
        }
        if (params.xrdpServerPort === undefined) {
            this.__xrdpServerPort.set(3390);
        }
        else {
        }
        if (params.xrdpServerMessage === undefined) {
            this.__xrdpServerMessage.set('');
        }
        else {
        }
        if (params.xrdpServerBusy === undefined) {
            this.__xrdpServerBusy.set(false);
        }
        else {
        }
        if (params.screenRecordingPermissionBusy === undefined) {
            this.__screenRecordingPermissionBusy.set(false);
        }
        else {
        }
        if (params.inputInjectionPermissionBusy === undefined) {
            this.__inputInjectionPermissionBusy.set(false);
        }
        else {
        }
        if (params.localAccessCode !== undefined) {
            this.localAccessCode = params.localAccessCode;
        }
        else {
        }
        if (params.localGateEnabled !== undefined) {
            this.localGateEnabled = params.localGateEnabled;
        }
        else {
        }
        if (params.localScreenRecordingGranted !== undefined) {
            this.localScreenRecordingGranted = params.localScreenRecordingGranted;
        }
        else {
        }
        if (params.localInputInjectionGranted !== undefined) {
            this.localInputInjectionGranted = params.localInputInjectionGranted;
        }
        else {
        }
        if (params.localXrdpServerRunning !== undefined) {
            this.localXrdpServerRunning = params.localXrdpServerRunning;
        }
        else {
        }
        if (params.localXrdpServerState !== undefined) {
            this.localXrdpServerState = params.localXrdpServerState;
        }
        else {
        }
        if (params.localXrdpServerPort !== undefined) {
            this.localXrdpServerPort = params.localXrdpServerPort;
        }
        else {
        }
        if (params.localXrdpServerMessage !== undefined) {
            this.localXrdpServerMessage = params.localXrdpServerMessage;
        }
        else {
        }
        if (params.localXrdpServerBusy !== undefined) {
            this.localXrdpServerBusy = params.localXrdpServerBusy;
        }
        else {
        }
        if (params.screenPermissionRequestPending !== undefined) {
            this.screenPermissionRequestPending = params.screenPermissionRequestPending;
        }
        else {
        }
        if (params.inputPermissionRequestPending !== undefined) {
            this.inputPermissionRequestPending = params.inputPermissionRequestPending;
        }
        else {
        }
    }
    updateStateVars(params: RemoteControlSettingsPage_Params) {
        bjccovmshb1iaq.instrumentFunction(21);
        this.__isDark.reset(params.isDark);
        this.__targetSection.reset(params.targetSection);
        this.__remoteAccessCode.reset(params.remoteAccessCode);
        this.__remoteAccessCodeGateEnabled.reset(params.remoteAccessCodeGateEnabled);
        this.__screenRecordingPermissionGranted.reset(params.screenRecordingPermissionGranted);
        this.__inputInjectionPermissionGranted.reset(params.inputInjectionPermissionGranted);
        this.__xrdpServerRunning.reset(params.xrdpServerRunning);
        this.__xrdpServerState.reset(params.xrdpServerState);
        this.__xrdpServerPort.reset(params.xrdpServerPort);
        this.__xrdpServerMessage.reset(params.xrdpServerMessage);
        this.__xrdpServerBusy.reset(params.xrdpServerBusy);
        this.__screenRecordingPermissionBusy.reset(params.screenRecordingPermissionBusy);
        this.__inputInjectionPermissionBusy.reset(params.inputInjectionPermissionBusy);
    }
    purgeVariableDependenciesOnElmtId(rmElmtId) {
        this.__isDark.purgeDependencyOnElmtId(rmElmtId);
        this.__targetSection.purgeDependencyOnElmtId(rmElmtId);
        this.__remoteAccessCode.purgeDependencyOnElmtId(rmElmtId);
        this.__remoteAccessCodeGateEnabled.purgeDependencyOnElmtId(rmElmtId);
        this.__screenRecordingPermissionGranted.purgeDependencyOnElmtId(rmElmtId);
        this.__inputInjectionPermissionGranted.purgeDependencyOnElmtId(rmElmtId);
        this.__xrdpServerRunning.purgeDependencyOnElmtId(rmElmtId);
        this.__xrdpServerState.purgeDependencyOnElmtId(rmElmtId);
        this.__xrdpServerPort.purgeDependencyOnElmtId(rmElmtId);
        this.__xrdpServerMessage.purgeDependencyOnElmtId(rmElmtId);
        this.__xrdpServerBusy.purgeDependencyOnElmtId(rmElmtId);
        this.__screenRecordingPermissionBusy.purgeDependencyOnElmtId(rmElmtId);
        this.__inputInjectionPermissionBusy.purgeDependencyOnElmtId(rmElmtId);
        this.__localAccessCode.purgeDependencyOnElmtId(rmElmtId);
        this.__localGateEnabled.purgeDependencyOnElmtId(rmElmtId);
        this.__localScreenRecordingGranted.purgeDependencyOnElmtId(rmElmtId);
        this.__localInputInjectionGranted.purgeDependencyOnElmtId(rmElmtId);
        this.__localXrdpServerRunning.purgeDependencyOnElmtId(rmElmtId);
        this.__localXrdpServerState.purgeDependencyOnElmtId(rmElmtId);
        this.__localXrdpServerPort.purgeDependencyOnElmtId(rmElmtId);
        this.__localXrdpServerMessage.purgeDependencyOnElmtId(rmElmtId);
        this.__localXrdpServerBusy.purgeDependencyOnElmtId(rmElmtId);
    }
    aboutToBeDeleted() {
        this.__isDark.aboutToBeDeleted();
        this.__targetSection.aboutToBeDeleted();
        this.__remoteAccessCode.aboutToBeDeleted();
        this.__remoteAccessCodeGateEnabled.aboutToBeDeleted();
        this.__screenRecordingPermissionGranted.aboutToBeDeleted();
        this.__inputInjectionPermissionGranted.aboutToBeDeleted();
        this.__xrdpServerRunning.aboutToBeDeleted();
        this.__xrdpServerState.aboutToBeDeleted();
        this.__xrdpServerPort.aboutToBeDeleted();
        this.__xrdpServerMessage.aboutToBeDeleted();
        this.__xrdpServerBusy.aboutToBeDeleted();
        this.__screenRecordingPermissionBusy.aboutToBeDeleted();
        this.__inputInjectionPermissionBusy.aboutToBeDeleted();
        this.__localAccessCode.aboutToBeDeleted();
        this.__localGateEnabled.aboutToBeDeleted();
        this.__localScreenRecordingGranted.aboutToBeDeleted();
        this.__localInputInjectionGranted.aboutToBeDeleted();
        this.__localXrdpServerRunning.aboutToBeDeleted();
        this.__localXrdpServerState.aboutToBeDeleted();
        this.__localXrdpServerPort.aboutToBeDeleted();
        this.__localXrdpServerMessage.aboutToBeDeleted();
        this.__localXrdpServerBusy.aboutToBeDeleted();
        SubscriberManager.Get().delete(this.id__());
        this.aboutToBeDeletedInternal();
    }
    private onBack: () => void;
    private onRemoteAccessCodeGateChange: (enabled: boolean) => string;
    private onRemoteAccessCodeRegenerate: () => string;
    private onRequestScreenRecordingPermission: () => Promise<boolean>;
    private onRefreshScreenRecordingPermission: () => boolean;
    private onRequestInputInjectionPermission: () => Promise<boolean>;
    private onRefreshInputInjectionPermission: () => boolean;
    private onRefreshXrdpServerStatus: () => XrdpServerStatus;
    private onStartXrdpServer: () => Promise<XrdpServerStatus>;
    private onOpenRemoteFilesDirectory: () => void;
    private __isDark: SynchedPropertySimpleOneWayPU<boolean>;
    get isDark() {
        bjccovmshb1iaq.instrumentFunction(32);
        return this.__isDark.get();
    }
    set isDark(newValue: boolean) {
        bjccovmshb1iaq.instrumentFunction(33);
        this.__isDark.set(newValue);
    }
    private __targetSection: SynchedPropertySimpleOneWayPU<string>;
    get targetSection() {
        bjccovmshb1iaq.instrumentFunction(34);
        return this.__targetSection.get();
    }
    set targetSection(newValue: string) {
        bjccovmshb1iaq.instrumentFunction(35);
        this.__targetSection.set(newValue);
    }
    private __remoteAccessCode: SynchedPropertySimpleOneWayPU<string>;
    get remoteAccessCode() {
        bjccovmshb1iaq.instrumentFunction(36);
        return this.__remoteAccessCode.get();
    }
    set remoteAccessCode(newValue: string) {
        bjccovmshb1iaq.instrumentFunction(37);
        this.__remoteAccessCode.set(newValue);
    }
    private __remoteAccessCodeGateEnabled: SynchedPropertySimpleOneWayPU<boolean>;
    get remoteAccessCodeGateEnabled() {
        bjccovmshb1iaq.instrumentFunction(38);
        return this.__remoteAccessCodeGateEnabled.get();
    }
    set remoteAccessCodeGateEnabled(newValue: boolean) {
        bjccovmshb1iaq.instrumentFunction(39);
        this.__remoteAccessCodeGateEnabled.set(newValue);
    }
    private __screenRecordingPermissionGranted: SynchedPropertySimpleOneWayPU<boolean>;
    get screenRecordingPermissionGranted() {
        bjccovmshb1iaq.instrumentFunction(40);
        return this.__screenRecordingPermissionGranted.get();
    }
    set screenRecordingPermissionGranted(newValue: boolean) {
        bjccovmshb1iaq.instrumentFunction(41);
        this.__screenRecordingPermissionGranted.set(newValue);
    }
    private __inputInjectionPermissionGranted: SynchedPropertySimpleOneWayPU<boolean>;
    get inputInjectionPermissionGranted() {
        bjccovmshb1iaq.instrumentFunction(42);
        return this.__inputInjectionPermissionGranted.get();
    }
    set inputInjectionPermissionGranted(newValue: boolean) {
        bjccovmshb1iaq.instrumentFunction(43);
        this.__inputInjectionPermissionGranted.set(newValue);
    }
    private __xrdpServerRunning: SynchedPropertySimpleOneWayPU<boolean>;
    get xrdpServerRunning() {
        bjccovmshb1iaq.instrumentFunction(44);
        return this.__xrdpServerRunning.get();
    }
    set xrdpServerRunning(newValue: boolean) {
        bjccovmshb1iaq.instrumentFunction(45);
        this.__xrdpServerRunning.set(newValue);
    }
    private __xrdpServerState: SynchedPropertySimpleOneWayPU<string>;
    get xrdpServerState() {
        bjccovmshb1iaq.instrumentFunction(46);
        return this.__xrdpServerState.get();
    }
    set xrdpServerState(newValue: string) {
        bjccovmshb1iaq.instrumentFunction(47);
        this.__xrdpServerState.set(newValue);
    }
    private __xrdpServerPort: SynchedPropertySimpleOneWayPU<number>;
    get xrdpServerPort() {
        bjccovmshb1iaq.instrumentFunction(48);
        return this.__xrdpServerPort.get();
    }
    set xrdpServerPort(newValue: number) {
        bjccovmshb1iaq.instrumentFunction(49);
        this.__xrdpServerPort.set(newValue);
    }
    private __xrdpServerMessage: SynchedPropertySimpleOneWayPU<string>;
    get xrdpServerMessage() {
        bjccovmshb1iaq.instrumentFunction(50);
        return this.__xrdpServerMessage.get();
    }
    set xrdpServerMessage(newValue: string) {
        bjccovmshb1iaq.instrumentFunction(51);
        this.__xrdpServerMessage.set(newValue);
    }
    private __xrdpServerBusy: SynchedPropertySimpleOneWayPU<boolean>;
    get xrdpServerBusy() {
        bjccovmshb1iaq.instrumentFunction(52);
        return this.__xrdpServerBusy.get();
    }
    set xrdpServerBusy(newValue: boolean) {
        bjccovmshb1iaq.instrumentFunction(53);
        this.__xrdpServerBusy.set(newValue);
    }
    private __screenRecordingPermissionBusy: SynchedPropertySimpleOneWayPU<boolean>;
    get screenRecordingPermissionBusy() {
        bjccovmshb1iaq.instrumentFunction(54);
        return this.__screenRecordingPermissionBusy.get();
    }
    set screenRecordingPermissionBusy(newValue: boolean) {
        bjccovmshb1iaq.instrumentFunction(55);
        this.__screenRecordingPermissionBusy.set(newValue);
    }
    private __inputInjectionPermissionBusy: SynchedPropertySimpleOneWayPU<boolean>;
    get inputInjectionPermissionBusy() {
        bjccovmshb1iaq.instrumentFunction(56);
        return this.__inputInjectionPermissionBusy.get();
    }
    set inputInjectionPermissionBusy(newValue: boolean) {
        bjccovmshb1iaq.instrumentFunction(57);
        this.__inputInjectionPermissionBusy.set(newValue);
    }
    private __localAccessCode: ObservedPropertySimplePU<string>;
    get localAccessCode() {
        bjccovmshb1iaq.instrumentFunction(58);
        return this.__localAccessCode.get();
    }
    set localAccessCode(newValue: string) {
        bjccovmshb1iaq.instrumentFunction(59);
        this.__localAccessCode.set(newValue);
    }
    private __localGateEnabled: ObservedPropertySimplePU<boolean>;
    get localGateEnabled() {
        bjccovmshb1iaq.instrumentFunction(60);
        return this.__localGateEnabled.get();
    }
    set localGateEnabled(newValue: boolean) {
        bjccovmshb1iaq.instrumentFunction(61);
        this.__localGateEnabled.set(newValue);
    }
    private __localScreenRecordingGranted: ObservedPropertySimplePU<boolean>;
    get localScreenRecordingGranted() {
        bjccovmshb1iaq.instrumentFunction(62);
        return this.__localScreenRecordingGranted.get();
    }
    set localScreenRecordingGranted(newValue: boolean) {
        bjccovmshb1iaq.instrumentFunction(63);
        this.__localScreenRecordingGranted.set(newValue);
    }
    private __localInputInjectionGranted: ObservedPropertySimplePU<boolean>;
    get localInputInjectionGranted() {
        bjccovmshb1iaq.instrumentFunction(64);
        return this.__localInputInjectionGranted.get();
    }
    set localInputInjectionGranted(newValue: boolean) {
        bjccovmshb1iaq.instrumentFunction(65);
        this.__localInputInjectionGranted.set(newValue);
    }
    private __localXrdpServerRunning: ObservedPropertySimplePU<boolean>;
    get localXrdpServerRunning() {
        bjccovmshb1iaq.instrumentFunction(66);
        return this.__localXrdpServerRunning.get();
    }
    set localXrdpServerRunning(newValue: boolean) {
        bjccovmshb1iaq.instrumentFunction(67);
        this.__localXrdpServerRunning.set(newValue);
    }
    private __localXrdpServerState: ObservedPropertySimplePU<string>;
    get localXrdpServerState() {
        bjccovmshb1iaq.instrumentFunction(68);
        return this.__localXrdpServerState.get();
    }
    set localXrdpServerState(newValue: string) {
        bjccovmshb1iaq.instrumentFunction(69);
        this.__localXrdpServerState.set(newValue);
    }
    private __localXrdpServerPort: ObservedPropertySimplePU<number>;
    get localXrdpServerPort() {
        bjccovmshb1iaq.instrumentFunction(70);
        return this.__localXrdpServerPort.get();
    }
    set localXrdpServerPort(newValue: number) {
        bjccovmshb1iaq.instrumentFunction(71);
        this.__localXrdpServerPort.set(newValue);
    }
    private __localXrdpServerMessage: ObservedPropertySimplePU<string>;
    get localXrdpServerMessage() {
        bjccovmshb1iaq.instrumentFunction(72);
        return this.__localXrdpServerMessage.get();
    }
    set localXrdpServerMessage(newValue: string) {
        bjccovmshb1iaq.instrumentFunction(73);
        this.__localXrdpServerMessage.set(newValue);
    }
    private __localXrdpServerBusy: ObservedPropertySimplePU<boolean>;
    get localXrdpServerBusy() {
        bjccovmshb1iaq.instrumentFunction(74);
        return this.__localXrdpServerBusy.get();
    }
    set localXrdpServerBusy(newValue: boolean) {
        bjccovmshb1iaq.instrumentFunction(75);
        this.__localXrdpServerBusy.set(newValue);
    }
    private screenPermissionRequestPending: boolean;
    private inputPermissionRequestPending: boolean;
    aboutToAppear(): void {
        bjccovmshb1iaq.instrumentFunction(76);
        bjccovmshb1iaq.instrumentRegion(76, 1);
        this.localAccessCode = this.remoteAccessCode;
        this.localGateEnabled = this.remoteAccessCodeGateEnabled;
        this.localScreenRecordingGranted = this.screenRecordingPermissionGranted;
        this.localInputInjectionGranted = this.inputInjectionPermissionGranted;
        this.localXrdpServerRunning = this.xrdpServerRunning;
        this.localXrdpServerState = this.xrdpServerState;
        this.localXrdpServerPort = this.xrdpServerPort;
        this.localXrdpServerMessage = this.xrdpServerMessage;
        this.localXrdpServerBusy = false;
        this.refreshScreenRecordingState();
        this.refreshInputInjectionState();
        this.refreshXrdpServerStatus();
    }
    private setGateEnabled(enabled: boolean): void {
        bjccovmshb1iaq.instrumentFunction(77);
        this.localGateEnabled = enabled;
        const nextCode = this.onRemoteAccessCodeGateChange(enabled);
        if (nextCode.length > 0) {
            bjccovmshb1iaq.instrumentBranch(77, 0, true);
            bjccovmshb1iaq.instrumentRegion(77, 1);
            this.localAccessCode = nextCode;
        }
        else {
            bjccovmshb1iaq.instrumentBranch(77, 0, false);
        }
    }
    private regenerateAccessCode(): void {
        bjccovmshb1iaq.instrumentFunction(78);
        const nextCode = this.onRemoteAccessCodeRegenerate();
        if (nextCode.length > 0) {
            bjccovmshb1iaq.instrumentBranch(78, 0, true);
            bjccovmshb1iaq.instrumentRegion(78, 1);
            this.localAccessCode = nextCode;
        }
        else {
            bjccovmshb1iaq.instrumentBranch(78, 0, false);
        }
    }
    private applyXrdpServerStatus(status: XrdpServerStatus): void {
        bjccovmshb1iaq.instrumentFunction(79);
        bjccovmshb1iaq.instrumentRegion(79, 1);
        this.localXrdpServerRunning = status.running;
        this.localXrdpServerState = status.state.length > 0 ? (bjccovmshb1iaq.instrumentBranch(79, 0, true), status.state) : (bjccovmshb1iaq.instrumentBranch(79, 0, false), 'Stopped');
        this.localXrdpServerPort = status.port > 0 ? (bjccovmshb1iaq.instrumentBranch(79, 1, true), status.port) : (bjccovmshb1iaq.instrumentBranch(79, 1, false), 3390);
        this.localXrdpServerMessage = status.message;
    }
    private refreshXrdpServerStatus(): void {
        bjccovmshb1iaq.instrumentFunction(80);
        bjccovmshb1iaq.instrumentRegion(80, 1);
        this.applyXrdpServerStatus(this.onRefreshXrdpServerStatus());
    }
    private startXrdpServer(): void {
        bjccovmshb1iaq.instrumentFunction(81);
        if (this.localXrdpServerBusy || this.xrdpServerBusy || this.screenRecordingPermissionBusy) {
            bjccovmshb1iaq.instrumentBranch(81, 0, true);
            bjccovmshb1iaq.instrumentRegion(81, 1);
            return;
        }
        else {
            bjccovmshb1iaq.instrumentBranch(81, 0, false);
        }
        bjccovmshb1iaq.instrumentRegion(81, 2);
        this.localXrdpServerBusy = true;
        this.onStartXrdpServer()
            .then((status: XrdpServerStatus) => {
            bjccovmshb1iaq.instrumentFunction(82);
            this.applyXrdpServerStatus(status);
            bjccovmshb1iaq.instrumentRegion(81, 3);
            this.localScreenRecordingGranted = this.localScreenRecordingGranted ||
                this.onRefreshScreenRecordingPermission();
        })
            .catch((_error: Error) => {
            bjccovmshb1iaq.instrumentFunction(83);
            bjccovmshb1iaq.instrumentRegion(83, 1);
            this.refreshXrdpServerStatus();
        })
            .finally(() => {
            bjccovmshb1iaq.instrumentFunction(84);
            bjccovmshb1iaq.instrumentRegion(84, 1);
            this.localXrdpServerBusy = false;
        });
    }
    private refreshScreenRecordingState(): void {
        bjccovmshb1iaq.instrumentFunction(85);
        bjccovmshb1iaq.instrumentRegion(85, 1);
        this.localScreenRecordingGranted = this.onRefreshScreenRecordingPermission();
    }
    private requestScreenRecordingPermission(): void {
        bjccovmshb1iaq.instrumentFunction(86);
        if (this.screenPermissionRequestPending || this.screenRecordingPermissionBusy) {
            bjccovmshb1iaq.instrumentBranch(86, 0, true);
            bjccovmshb1iaq.instrumentRegion(86, 1);
            return;
        }
        else {
            bjccovmshb1iaq.instrumentBranch(86, 0, false);
        }
        bjccovmshb1iaq.instrumentRegion(86, 2);
        this.screenPermissionRequestPending = true;
        this.onRequestScreenRecordingPermission()
            .then((granted: boolean) => {
            bjccovmshb1iaq.instrumentFunction(87);
            const allowed = granted || this.onRefreshScreenRecordingPermission();
            this.localScreenRecordingGranted = allowed;
            if (!allowed) {
                bjccovmshb1iaq.instrumentBranch(87, 0, true);
                bjccovmshb1iaq.instrumentRegion(87, 1);
                this.localXrdpServerBusy = false;
            }
            else {
                bjccovmshb1iaq.instrumentBranch(87, 0, false);
            }
            bjccovmshb1iaq.instrumentRegion(86, 3);
            this.refreshXrdpServerStatus();
        })
            .catch((_error: Error) => {
            bjccovmshb1iaq.instrumentFunction(88);
            this.localScreenRecordingGranted = this.onRefreshScreenRecordingPermission();
            if (!this.localScreenRecordingGranted) {
                bjccovmshb1iaq.instrumentBranch(88, 0, true);
                bjccovmshb1iaq.instrumentRegion(88, 1);
                this.localXrdpServerBusy = false;
            }
            else {
                bjccovmshb1iaq.instrumentBranch(88, 0, false);
            }
        })
            .finally(() => {
            bjccovmshb1iaq.instrumentFunction(89);
            bjccovmshb1iaq.instrumentRegion(89, 1);
            this.screenPermissionRequestPending = false;
        });
    }
    private refreshInputInjectionState(): void {
        bjccovmshb1iaq.instrumentFunction(90);
        bjccovmshb1iaq.instrumentRegion(90, 1);
        this.localInputInjectionGranted = this.onRefreshInputInjectionPermission();
    }
    private requestInputInjectionPermission(): void {
        bjccovmshb1iaq.instrumentFunction(91);
        if (this.inputPermissionRequestPending || this.inputInjectionPermissionBusy) {
            bjccovmshb1iaq.instrumentBranch(91, 0, true);
            bjccovmshb1iaq.instrumentRegion(91, 1);
            return;
        }
        else {
            bjccovmshb1iaq.instrumentBranch(91, 0, false);
        }
        bjccovmshb1iaq.instrumentRegion(91, 2);
        this.inputPermissionRequestPending = true;
        this.onRequestInputInjectionPermission()
            .then((granted: boolean) => {
            bjccovmshb1iaq.instrumentFunction(92);
            bjccovmshb1iaq.instrumentRegion(92, 1);
            this.localInputInjectionGranted = granted || this.onRefreshInputInjectionPermission();
        })
            .catch((_error: Error) => {
            bjccovmshb1iaq.instrumentFunction(93);
            bjccovmshb1iaq.instrumentRegion(93, 1);
            this.localInputInjectionGranted = this.onRefreshInputInjectionPermission();
        })
            .finally(() => {
            bjccovmshb1iaq.instrumentFunction(94);
            bjccovmshb1iaq.instrumentRegion(94, 1);
            this.inputPermissionRequestPending = false;
        });
    }
    initialRender() {
        bjccovmshb1iaq.instrumentFunction(95);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1iaq.instrumentFunction(96);
            Column.create();
            Column.width('100%');
            Column.height('100%');
            Column.backgroundColor(SettingsTheme.pageBackground(this.isDark));
        }, Column);
        {
            this.observeComponentCreation2((elmtId, isInitialRender) => {
                bjccovmshb1iaq.instrumentFunction(97);
                if (isInitialRender) {
                    let componentCall = new SettingsPageHeader(this, {
                        title: SettingsText.REMOTE_CONTROL_TITLE,
                        subtitle: SettingsText.REMOTE_CONTROL_SUBTITLE,
                        isDark: this.isDark,
                        onBack: this.onBack
                    }, undefined, elmtId, () => { }, { page: "common/src/main/ets/components/settings/RemoteControlSettingsPage.ets", line: 194, col: 7 });
                    ViewPU.create(componentCall);
                    let paramsLambda = () => {
                        return {
                            title: SettingsText.REMOTE_CONTROL_TITLE,
                            subtitle: SettingsText.REMOTE_CONTROL_SUBTITLE,
                            isDark: this.isDark,
                            onBack: this.onBack
                        };
                    };
                    componentCall.paramsGenerator_ = paramsLambda;
                }
                else {
                    this.updateStateVarsOfChildByElmtId(elmtId, {
                        title: SettingsText.REMOTE_CONTROL_TITLE,
                        subtitle: SettingsText.REMOTE_CONTROL_SUBTITLE,
                        isDark: this.isDark
                    });
                }
            }, { name: "SettingsPageHeader" });
        }
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1iaq.instrumentFunction(98);
            Scroll.create();
            Scroll.layoutWeight(1);
        }, Scroll);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1iaq.instrumentFunction(99);
            Column.create({ space: 14 });
            Column.alignItems(HorizontalAlign.Start);
            Column.padding({ left: 20, right: 20, bottom: 24 });
            Column.width('100%');
        }, Column);
        {
            this.observeComponentCreation2((elmtId, isInitialRender) => {
                bjccovmshb1iaq.instrumentFunction(100);
                if (isInitialRender) {
                    let componentCall = new SettingsSectionTitle(this, {
                        title: SettingsText.REMOTE_CONTROL_TITLE,
                        subtitle: SettingsText.REMOTE_CONTROL_SUBTITLE,
                        isDark: this.isDark
                    }, undefined, elmtId, () => { }, { page: "common/src/main/ets/components/settings/RemoteControlSettingsPage.ets", line: 203, col: 11 });
                    ViewPU.create(componentCall);
                    let paramsLambda = () => {
                        return {
                            title: SettingsText.REMOTE_CONTROL_TITLE,
                            subtitle: SettingsText.REMOTE_CONTROL_SUBTITLE,
                            isDark: this.isDark
                        };
                    };
                    componentCall.paramsGenerator_ = paramsLambda;
                }
                else {
                    this.updateStateVarsOfChildByElmtId(elmtId, {
                        title: SettingsText.REMOTE_CONTROL_TITLE,
                        subtitle: SettingsText.REMOTE_CONTROL_SUBTITLE,
                        isDark: this.isDark
                    });
                }
            }, { name: "SettingsSectionTitle" });
        }
        {
            this.observeComponentCreation2((elmtId, isInitialRender) => {
                bjccovmshb1iaq.instrumentFunction(101);
                if (isInitialRender) {
                    let componentCall = new XrdpServerCard(this, {
                        isDark: this.isDark,
                        targetSection: this.targetSection,
                        running: this.localXrdpServerRunning,
                        busy: this.localXrdpServerBusy || this.xrdpServerBusy || this.screenRecordingPermissionBusy,
                        state: this.localXrdpServerState,
                        port: this.localXrdpServerPort,
                        message: this.localXrdpServerMessage,
                        onRefresh: (): void => {
                            bjccovmshb1iaq.instrumentFunction(102);
                            bjccovmshb1iaq.instrumentRegion(102, 1);
                            this.refreshXrdpServerStatus();
                        },
                        onStart: (): void => {
                            bjccovmshb1iaq.instrumentFunction(103);
                            bjccovmshb1iaq.instrumentRegion(103, 1);
                            this.startXrdpServer();
                        }
                    }, undefined, elmtId, () => { }, { page: "common/src/main/ets/components/settings/RemoteControlSettingsPage.ets", line: 209, col: 11 });
                    ViewPU.create(componentCall);
                    let paramsLambda = () => {
                        return {
                            isDark: this.isDark,
                            targetSection: this.targetSection,
                            running: this.localXrdpServerRunning,
                            busy: this.localXrdpServerBusy || this.xrdpServerBusy || this.screenRecordingPermissionBusy,
                            state: this.localXrdpServerState,
                            port: this.localXrdpServerPort,
                            message: this.localXrdpServerMessage,
                            onRefresh: (): void => {
                                this.refreshXrdpServerStatus();
                            },
                            onStart: (): void => {
                                this.startXrdpServer();
                            }
                        };
                    };
                    componentCall.paramsGenerator_ = paramsLambda;
                }
                else {
                    this.updateStateVarsOfChildByElmtId(elmtId, {
                        isDark: this.isDark,
                        targetSection: this.targetSection,
                        running: this.localXrdpServerRunning,
                        busy: this.localXrdpServerBusy || this.xrdpServerBusy || this.screenRecordingPermissionBusy,
                        state: this.localXrdpServerState,
                        port: this.localXrdpServerPort,
                        message: this.localXrdpServerMessage
                    });
                }
            }, { name: "XrdpServerCard" });
        }
        {
            this.observeComponentCreation2((elmtId, isInitialRender) => {
                bjccovmshb1iaq.instrumentFunction(104);
                if (isInitialRender) {
                    let componentCall = new RemotePermissionCard(this, {
                        isDark: this.isDark,
                        targetSection: this.targetSection,
                        section: SettingsRemoteControlSection.SCREEN,
                        granted: this.localScreenRecordingGranted,
                        busy: this.screenRecordingPermissionBusy,
                        onRequest: (): void => {
                            bjccovmshb1iaq.instrumentFunction(105);
                            bjccovmshb1iaq.instrumentRegion(105, 1);
                            this.requestScreenRecordingPermission();
                        }
                    }, undefined, elmtId, () => { }, { page: "common/src/main/ets/components/settings/RemoteControlSettingsPage.ets", line: 224, col: 11 });
                    ViewPU.create(componentCall);
                    let paramsLambda = () => {
                        return {
                            isDark: this.isDark,
                            targetSection: this.targetSection,
                            section: SettingsRemoteControlSection.SCREEN,
                            granted: this.localScreenRecordingGranted,
                            busy: this.screenRecordingPermissionBusy,
                            onRequest: (): void => {
                                this.requestScreenRecordingPermission();
                            }
                        };
                    };
                    componentCall.paramsGenerator_ = paramsLambda;
                }
                else {
                    this.updateStateVarsOfChildByElmtId(elmtId, {
                        isDark: this.isDark,
                        targetSection: this.targetSection,
                        section: SettingsRemoteControlSection.SCREEN,
                        granted: this.localScreenRecordingGranted,
                        busy: this.screenRecordingPermissionBusy
                    });
                }
            }, { name: "RemotePermissionCard" });
        }
        {
            this.observeComponentCreation2((elmtId, isInitialRender) => {
                bjccovmshb1iaq.instrumentFunction(106);
                if (isInitialRender) {
                    let componentCall = new RemotePermissionCard(this, {
                        isDark: this.isDark,
                        targetSection: this.targetSection,
                        section: SettingsRemoteControlSection.INPUT,
                        title: SettingsText.REMOTE_INPUT_PERMISSION_TITLE,
                        description: SettingsText.REMOTE_INPUT_PERMISSION_DESC,
                        icon: SettingsResources.REMOTE_ACCESS_ICON,
                        accent: SettingsAccent.PURPLE,
                        granted: this.localInputInjectionGranted,
                        busy: this.inputInjectionPermissionBusy,
                        onRequest: (): void => {
                            bjccovmshb1iaq.instrumentFunction(107);
                            bjccovmshb1iaq.instrumentRegion(107, 1);
                            this.requestInputInjectionPermission();
                        }
                    }, undefined, elmtId, () => { }, { page: "common/src/main/ets/components/settings/RemoteControlSettingsPage.ets", line: 234, col: 11 });
                    ViewPU.create(componentCall);
                    let paramsLambda = () => {
                        return {
                            isDark: this.isDark,
                            targetSection: this.targetSection,
                            section: SettingsRemoteControlSection.INPUT,
                            title: SettingsText.REMOTE_INPUT_PERMISSION_TITLE,
                            description: SettingsText.REMOTE_INPUT_PERMISSION_DESC,
                            icon: SettingsResources.REMOTE_ACCESS_ICON,
                            accent: SettingsAccent.PURPLE,
                            granted: this.localInputInjectionGranted,
                            busy: this.inputInjectionPermissionBusy,
                            onRequest: (): void => {
                                this.requestInputInjectionPermission();
                            }
                        };
                    };
                    componentCall.paramsGenerator_ = paramsLambda;
                }
                else {
                    this.updateStateVarsOfChildByElmtId(elmtId, {
                        isDark: this.isDark,
                        targetSection: this.targetSection,
                        section: SettingsRemoteControlSection.INPUT,
                        title: SettingsText.REMOTE_INPUT_PERMISSION_TITLE,
                        description: SettingsText.REMOTE_INPUT_PERMISSION_DESC,
                        icon: SettingsResources.REMOTE_ACCESS_ICON,
                        accent: SettingsAccent.PURPLE,
                        granted: this.localInputInjectionGranted,
                        busy: this.inputInjectionPermissionBusy
                    });
                }
            }, { name: "RemotePermissionCard" });
        }
        {
            this.observeComponentCreation2((elmtId, isInitialRender) => {
                bjccovmshb1iaq.instrumentFunction(108);
                if (isInitialRender) {
                    let componentCall = new RemoteAccessCard(this, {
                        isDark: this.isDark,
                        targetSection: this.targetSection,
                        gateEnabled: this.localGateEnabled,
                        accessCode: this.localAccessCode,
                        onGateChange: (enabled: boolean): void => {
                            bjccovmshb1iaq.instrumentFunction(109);
                            bjccovmshb1iaq.instrumentRegion(109, 1);
                            this.setGateEnabled(enabled);
                        },
                        onRegenerate: (): void => {
                            bjccovmshb1iaq.instrumentFunction(110);
                            bjccovmshb1iaq.instrumentRegion(110, 1);
                            this.regenerateAccessCode();
                        }
                    }, undefined, elmtId, () => { }, { page: "common/src/main/ets/components/settings/RemoteControlSettingsPage.ets", line: 248, col: 11 });
                    ViewPU.create(componentCall);
                    let paramsLambda = () => {
                        return {
                            isDark: this.isDark,
                            targetSection: this.targetSection,
                            gateEnabled: this.localGateEnabled,
                            accessCode: this.localAccessCode,
                            onGateChange: (enabled: boolean): void => {
                                this.setGateEnabled(enabled);
                            },
                            onRegenerate: (): void => {
                                this.regenerateAccessCode();
                            }
                        };
                    };
                    componentCall.paramsGenerator_ = paramsLambda;
                }
                else {
                    this.updateStateVarsOfChildByElmtId(elmtId, {
                        isDark: this.isDark,
                        targetSection: this.targetSection,
                        gateEnabled: this.localGateEnabled,
                        accessCode: this.localAccessCode
                    });
                }
            }, { name: "RemoteAccessCard" });
        }
        {
            this.observeComponentCreation2((elmtId, isInitialRender) => {
                bjccovmshb1iaq.instrumentFunction(111);
                if (isInitialRender) {
                    let componentCall = new RemoteFilesCard(this, {
                        isDark: this.isDark,
                        targetSection: this.targetSection,
                        onOpenRemoteFilesDirectory: this.onOpenRemoteFilesDirectory
                    }, undefined, elmtId, () => { }, { page: "common/src/main/ets/components/settings/RemoteControlSettingsPage.ets", line: 260, col: 11 });
                    ViewPU.create(componentCall);
                    let paramsLambda = () => {
                        return {
                            isDark: this.isDark,
                            targetSection: this.targetSection,
                            onOpenRemoteFilesDirectory: this.onOpenRemoteFilesDirectory
                        };
                    };
                    componentCall.paramsGenerator_ = paramsLambda;
                }
                else {
                    this.updateStateVarsOfChildByElmtId(elmtId, {
                        isDark: this.isDark,
                        targetSection: this.targetSection
                    });
                }
            }, { name: "RemoteFilesCard" });
        }
        Column.pop();
        Scroll.pop();
        Column.pop();
    }
    rerender() {
        this.updateDirtyElements();
    }
}
