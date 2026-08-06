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
let bjccovmshb1hxg = new BjcCov({ version: "bjc v1.0.0", versionCode: 10000, path: "common/src/main/ets/components/SettingsPage.ets", hash: "9d060b31a6af0b31292c34dc0d78a5e8bf8e7edd71d7c1ac9a71d7745fdb9626", lineCnt: 476, count: 0, projectPath: "", functions: { 0: { name: "anonymous_1", count: 0, regions: { 0: { startLoc: { line: 24, col: 12 }, endLoc: { line: 24, col: 22 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 0 }, 1: { name: "anonymous_2", count: 0, regions: { 0: { startLoc: { line: 26, col: 33 }, endLoc: { line: 26, col: 61 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 1 }, 2: { name: "anonymous_3", count: 0, regions: { 0: { startLoc: { line: 29, col: 33 }, endLoc: { line: 29, col: 45 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 2 }, 3: { name: "anonymous_4", count: 0, regions: { 0: { startLoc: { line: 32, col: 39 }, endLoc: { line: 32, col: 61 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 3 }, 4: { name: "anonymous_5", count: 0, regions: { 0: { startLoc: { line: 35, col: 39 }, endLoc: { line: 35, col: 52 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 4 }, 5: { name: "anonymous_6", count: 0, regions: { 0: { startLoc: { line: 38, col: 38 }, endLoc: { line: 38, col: 60 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 5 }, 6: { name: "anonymous_7", count: 0, regions: { 0: { startLoc: { line: 41, col: 38 }, endLoc: { line: 41, col: 51 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 6 }, 7: { name: "anonymous_8", count: 0, regions: { 0: { startLoc: { line: 44, col: 30 }, endLoc: { line: 44, col: 52 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 7 }, 8: { name: "anonymous_9", count: 0, regions: { 0: { startLoc: { line: 52, col: 22 }, endLoc: { line: 52, col: 53 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 8 }, 9: { name: "anonymous_10", count: 0, regions: { 0: { startLoc: { line: 61, col: 31 }, endLoc: { line: 61, col: 41 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 9 }, 10: { name: "anonymous_11", count: 0, regions: { 0: { startLoc: { line: 24, col: 25 }, endLoc: { line: 25, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 10 }, 11: { name: "anonymous_12", count: 0, regions: { 0: { startLoc: { line: 26, col: 64 }, endLoc: { line: 28, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 27, col: 5 }, endLoc: { line: 28, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 11 }, 12: { name: "anonymous_13", count: 0, regions: { 0: { startLoc: { line: 29, col: 48 }, endLoc: { line: 31, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 30, col: 5 }, endLoc: { line: 31, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 12 }, 13: { name: "anonymous_14", count: 0, regions: { 0: { startLoc: { line: 32, col: 64 }, endLoc: { line: 34, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 33, col: 5 }, endLoc: { line: 34, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 13 }, 14: { name: "anonymous_15", count: 0, regions: { 0: { startLoc: { line: 35, col: 55 }, endLoc: { line: 37, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 36, col: 5 }, endLoc: { line: 37, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 14 }, 15: { name: "anonymous_16", count: 0, regions: { 0: { startLoc: { line: 38, col: 63 }, endLoc: { line: 40, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 39, col: 5 }, endLoc: { line: 40, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 15 }, 16: { name: "anonymous_17", count: 0, regions: { 0: { startLoc: { line: 41, col: 54 }, endLoc: { line: 43, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 42, col: 5 }, endLoc: { line: 43, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 16 }, 17: { name: "anonymous_18", count: 0, regions: { 0: { startLoc: { line: 44, col: 55 }, endLoc: { line: 51, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 45, col: 5 }, endLoc: { line: 51, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 17 }, 18: { name: "anonymous_19", count: 0, regions: { 0: { startLoc: { line: 52, col: 56 }, endLoc: { line: 60, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 53, col: 5 }, endLoc: { line: 60, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 18 }, 19: { name: "anonymous_20", count: 0, regions: { 0: { startLoc: { line: 61, col: 44 }, endLoc: { line: 62, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 19 }, 20: { name: "setInitiallyProvidedValue", count: 0, regions: { 0: { startLoc: { line: 23, col: 34 }, endLoc: { line: 76, col: 53 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 20 }, 21: { name: "updateStateVars", count: 0, regions: { 0: { startLoc: { line: 23, col: 9 }, endLoc: { line: 76, col: 37 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 21 }, 22: { name: "layoutMode", count: 0, regions: { 0: { startLoc: { line: 23, col: 9 }, endLoc: { line: 23, col: 19 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 22 }, 23: { name: "layoutMode", count: 0, regions: { 0: { startLoc: { line: 23, col: 9 }, endLoc: { line: 23, col: 31 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 23 }, 24: { name: "anonymous_21", count: 0, regions: { 0: { startLoc: { line: 24, col: 12 }, endLoc: { line: 24, col: 22 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 24 }, 25: { name: "anonymous_22", count: 0, regions: { 0: { startLoc: { line: 26, col: 33 }, endLoc: { line: 26, col: 61 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 25 }, 26: { name: "anonymous_23", count: 0, regions: { 0: { startLoc: { line: 29, col: 33 }, endLoc: { line: 29, col: 45 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 26 }, 27: { name: "anonymous_24", count: 0, regions: { 0: { startLoc: { line: 32, col: 39 }, endLoc: { line: 32, col: 61 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 27 }, 28: { name: "anonymous_25", count: 0, regions: { 0: { startLoc: { line: 35, col: 39 }, endLoc: { line: 35, col: 52 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 28 }, 29: { name: "anonymous_26", count: 0, regions: { 0: { startLoc: { line: 38, col: 38 }, endLoc: { line: 38, col: 60 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 29 }, 30: { name: "anonymous_27", count: 0, regions: { 0: { startLoc: { line: 41, col: 38 }, endLoc: { line: 41, col: 51 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 30 }, 31: { name: "anonymous_28", count: 0, regions: { 0: { startLoc: { line: 44, col: 30 }, endLoc: { line: 44, col: 52 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 31 }, 32: { name: "anonymous_29", count: 0, regions: { 0: { startLoc: { line: 52, col: 22 }, endLoc: { line: 52, col: 53 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 32 }, 33: { name: "anonymous_30", count: 0, regions: { 0: { startLoc: { line: 61, col: 31 }, endLoc: { line: 61, col: 41 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 33 }, 34: { name: "initialPageName", count: 0, regions: { 0: { startLoc: { line: 63, col: 9 }, endLoc: { line: 63, col: 24 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 34 }, 35: { name: "initialPageName", count: 0, regions: { 0: { startLoc: { line: 63, col: 9 }, endLoc: { line: 63, col: 32 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 35 }, 36: { name: "initialRemoteControlSection", count: 0, regions: { 0: { startLoc: { line: 64, col: 9 }, endLoc: { line: 64, col: 36 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 36 }, 37: { name: "initialRemoteControlSection", count: 0, regions: { 0: { startLoc: { line: 64, col: 9 }, endLoc: { line: 64, col: 44 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 37 }, 38: { name: "remoteAccessCode", count: 0, regions: { 0: { startLoc: { line: 65, col: 9 }, endLoc: { line: 65, col: 25 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 38 }, 39: { name: "remoteAccessCode", count: 0, regions: { 0: { startLoc: { line: 65, col: 9 }, endLoc: { line: 65, col: 33 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 39 }, 40: { name: "remoteAccessCodeGateEnabled", count: 0, regions: { 0: { startLoc: { line: 66, col: 9 }, endLoc: { line: 66, col: 36 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 40 }, 41: { name: "remoteAccessCodeGateEnabled", count: 0, regions: { 0: { startLoc: { line: 66, col: 9 }, endLoc: { line: 66, col: 45 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 41 }, 42: { name: "screenRecordingPermissionGranted", count: 0, regions: { 0: { startLoc: { line: 67, col: 9 }, endLoc: { line: 67, col: 41 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 42 }, 43: { name: "screenRecordingPermissionGranted", count: 0, regions: { 0: { startLoc: { line: 67, col: 9 }, endLoc: { line: 67, col: 50 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 43 }, 44: { name: "inputInjectionPermissionGranted", count: 0, regions: { 0: { startLoc: { line: 68, col: 9 }, endLoc: { line: 68, col: 40 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 44 }, 45: { name: "inputInjectionPermissionGranted", count: 0, regions: { 0: { startLoc: { line: 68, col: 9 }, endLoc: { line: 68, col: 49 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 45 }, 46: { name: "xrdpServerRunning", count: 0, regions: { 0: { startLoc: { line: 69, col: 9 }, endLoc: { line: 69, col: 26 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 46 }, 47: { name: "xrdpServerRunning", count: 0, regions: { 0: { startLoc: { line: 69, col: 9 }, endLoc: { line: 69, col: 35 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 47 }, 48: { name: "xrdpServerState", count: 0, regions: { 0: { startLoc: { line: 70, col: 9 }, endLoc: { line: 70, col: 24 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 48 }, 49: { name: "xrdpServerState", count: 0, regions: { 0: { startLoc: { line: 70, col: 9 }, endLoc: { line: 70, col: 32 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 49 }, 50: { name: "xrdpServerPort", count: 0, regions: { 0: { startLoc: { line: 71, col: 9 }, endLoc: { line: 71, col: 23 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 50 }, 51: { name: "xrdpServerPort", count: 0, regions: { 0: { startLoc: { line: 71, col: 9 }, endLoc: { line: 71, col: 31 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 51 }, 52: { name: "xrdpServerMessage", count: 0, regions: { 0: { startLoc: { line: 72, col: 9 }, endLoc: { line: 72, col: 26 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 52 }, 53: { name: "xrdpServerMessage", count: 0, regions: { 0: { startLoc: { line: 72, col: 9 }, endLoc: { line: 72, col: 34 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 53 }, 54: { name: "xrdpServerBusy", count: 0, regions: { 0: { startLoc: { line: 73, col: 9 }, endLoc: { line: 73, col: 23 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 54 }, 55: { name: "xrdpServerBusy", count: 0, regions: { 0: { startLoc: { line: 73, col: 9 }, endLoc: { line: 73, col: 32 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 55 }, 56: { name: "screenRecordingPermissionBusy", count: 0, regions: { 0: { startLoc: { line: 74, col: 9 }, endLoc: { line: 74, col: 38 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 56 }, 57: { name: "screenRecordingPermissionBusy", count: 0, regions: { 0: { startLoc: { line: 74, col: 9 }, endLoc: { line: 74, col: 47 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 57 }, 58: { name: "inputInjectionPermissionBusy", count: 0, regions: { 0: { startLoc: { line: 75, col: 9 }, endLoc: { line: 75, col: 37 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 58 }, 59: { name: "inputInjectionPermissionBusy", count: 0, regions: { 0: { startLoc: { line: 75, col: 9 }, endLoc: { line: 75, col: 46 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 59 }, 60: { name: "remoteControlServerAvailable", count: 0, regions: { 0: { startLoc: { line: 76, col: 9 }, endLoc: { line: 76, col: 37 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 60 }, 61: { name: "remoteControlServerAvailable", count: 0, regions: { 0: { startLoc: { line: 76, col: 9 }, endLoc: { line: 76, col: 46 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 61 }, 62: { name: "pageName", count: 0, regions: { 0: { startLoc: { line: 77, col: 18 }, endLoc: { line: 77, col: 26 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 62 }, 63: { name: "pageName", count: 0, regions: { 0: { startLoc: { line: 77, col: 18 }, endLoc: { line: 77, col: 34 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 63 }, 64: { name: "remoteControlTargetSection", count: 0, regions: { 0: { startLoc: { line: 78, col: 18 }, endLoc: { line: 78, col: 44 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 64 }, 65: { name: "remoteControlTargetSection", count: 0, regions: { 0: { startLoc: { line: 78, col: 18 }, endLoc: { line: 78, col: 52 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 65 }, 66: { name: "appearanceMode", count: 0, regions: { 0: { startLoc: { line: 79, col: 18 }, endLoc: { line: 79, col: 32 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 66 }, 67: { name: "appearanceMode", count: 0, regions: { 0: { startLoc: { line: 79, col: 18 }, endLoc: { line: 79, col: 56 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 67 }, 68: { name: "systemDark", count: 0, regions: { 0: { startLoc: { line: 80, col: 46 }, endLoc: { line: 80, col: 56 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 68 }, 69: { name: "systemDark", count: 0, regions: { 0: { startLoc: { line: 80, col: 46 }, endLoc: { line: 80, col: 65 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 69 }, 70: { name: "aboutToAppear", count: 0, regions: { 0: { startLoc: { line: 82, col: 3 }, endLoc: { line: 90, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 85, col: 95 }, endLoc: { line: 87, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 88, col: 5 }, endLoc: { line: 90, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 84, col: 21 }, endLoc: { line: 84, col: 100 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 85, col: 9 }, endLoc: { line: 85, col: 93 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 2: { startLoc: { line: 88, col: 39 }, endLoc: { line: 89, col: 44 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 70 }, 71: { name: "isDarkMode", count: 0, regions: { 0: { startLoc: { line: 92, col: 3 }, endLoc: { line: 97, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 93, col: 43 }, endLoc: { line: 95, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 96, col: 5 }, endLoc: { line: 97, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 93, col: 9 }, endLoc: { line: 93, col: 41 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 71 }, 72: { name: "xrdpStatusTone", count: 0, regions: { 0: { startLoc: { line: 99, col: 3 }, endLoc: { line: 101, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 100, col: 5 }, endLoc: { line: 101, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 72 }, 73: { name: "xrdpStatusText", count: 0, regions: { 0: { startLoc: { line: 103, col: 3 }, endLoc: { line: 106, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 104, col: 5 }, endLoc: { line: 106, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 73 }, 74: { name: "screenStatusTone", count: 0, regions: { 0: { startLoc: { line: 108, col: 3 }, endLoc: { line: 113, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 109, col: 45 }, endLoc: { line: 111, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 112, col: 5 }, endLoc: { line: 113, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 109, col: 9 }, endLoc: { line: 109, col: 43 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 112, col: 12 }, endLoc: { line: 112, col: 68 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 74 }, 75: { name: "screenStatusText", count: 0, regions: { 0: { startLoc: { line: 115, col: 3 }, endLoc: { line: 121, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 116, col: 45 }, endLoc: { line: 118, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 119, col: 5 }, endLoc: { line: 121, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 116, col: 9 }, endLoc: { line: 116, col: 43 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 119, col: 12 }, endLoc: { line: 120, col: 45 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 75 }, 76: { name: "gateStatusTone", count: 0, regions: { 0: { startLoc: { line: 123, col: 3 }, endLoc: { line: 125, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 124, col: 5 }, endLoc: { line: 125, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 124, col: 12 }, endLoc: { line: 124, col: 63 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 76 }, 77: { name: "gateStatusText", count: 0, regions: { 0: { startLoc: { line: 127, col: 3 }, endLoc: { line: 129, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 128, col: 5 }, endLoc: { line: 129, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 128, col: 12 }, endLoc: { line: 128, col: 119 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 77 }, 78: { name: "buildDesktopNav", count: 0, regions: { 0: { startLoc: { line: 131, col: 3 }, endLoc: { line: 215, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 165, col: 13 }, endLoc: { line: 166, col: 12 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 165, col: 13 }, endLoc: { line: 166, col: 12 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 176, col: 13 }, endLoc: { line: 177, col: 12 }, count: 0, ignored: 0 }, 4: { startLoc: { line: 176, col: 13 }, endLoc: { line: 177, col: 12 }, count: 0, ignored: 0 }, 5: { startLoc: { line: 188, col: 15 }, endLoc: { line: 189, col: 14 }, count: 0, ignored: 0 }, 6: { startLoc: { line: 188, col: 15 }, endLoc: { line: 189, col: 14 }, count: 0, ignored: 0 }, 7: { startLoc: { line: 200, col: 13 }, endLoc: { line: 201, col: 12 }, count: 0, ignored: 0 }, 8: { startLoc: { line: 200, col: 13 }, endLoc: { line: 201, col: 12 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 78 }, 79: { name: "anonymous_31", count: 0, regions: { 0: { startLoc: { line: 133, col: 5 }, endLoc: { line: 214, col: 6 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 79 }, 80: { name: "anonymous_32", count: 0, regions: { 0: { startLoc: { line: 134, col: 7 }, endLoc: { line: 154, col: 29 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 80 }, 81: { name: "anonymous_33", count: 0, regions: { 0: { startLoc: { line: 135, col: 9 }, endLoc: { line: 136, col: 36 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 81 }, 82: { name: "anonymous_34", count: 0, regions: { 0: { startLoc: { line: 137, col: 19 }, endLoc: { line: 139, col: 12 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 138, col: 13 }, endLoc: { line: 139, col: 12 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 82 }, 83: { name: "anonymous_36", count: 0, regions: { 0: { startLoc: { line: 142, col: 9 }, endLoc: { line: 150, col: 24 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 83 }, 84: { name: "anonymous_37", count: 0, regions: { 0: { startLoc: { line: 143, col: 11 }, endLoc: { line: 147, col: 26 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 84 }, 85: { name: "anonymous_38", count: 0, regions: { 0: { startLoc: { line: 156, col: 7 }, endLoc: { line: 204, col: 20 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 85 }, 86: { name: "anonymous_39", count: 0, regions: { 0: { startLoc: { line: 157, col: 9 }, endLoc: { line: 162, col: 36 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 86 }, 87: { name: "anonymous_40", count: 0, regions: { 0: { startLoc: { line: 163, col: 20 }, endLoc: { line: 166, col: 12 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 87 }, 88: { name: "anonymous_42", count: 0, regions: { 0: { startLoc: { line: 168, col: 9 }, endLoc: { line: 173, col: 36 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 88 }, 89: { name: "anonymous_43", count: 0, regions: { 0: { startLoc: { line: 174, col: 20 }, endLoc: { line: 177, col: 12 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 89 }, 90: { name: "anonymous_45", count: 0, regions: { 0: { startLoc: { line: 179, col: 9 }, endLoc: { line: 191, col: 10 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 179, col: 48 }, endLoc: { line: 191, col: 10 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 179, col: 13 }, endLoc: { line: 179, col: 46 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 90 }, 91: { name: "anonymous_46", count: 0, regions: { 0: { startLoc: { line: 180, col: 11 }, endLoc: { line: 185, col: 38 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 91 }, 92: { name: "anonymous_47", count: 0, regions: { 0: { startLoc: { line: 180, col: 11 }, endLoc: { line: 185, col: 38 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 92 }, 93: { name: "anonymous_48", count: 0, regions: { 0: { startLoc: { line: 186, col: 22 }, endLoc: { line: 189, col: 14 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 93 }, 94: { name: "anonymous_51", count: 0, regions: { 0: { startLoc: { line: 192, col: 9 }, endLoc: { line: 197, col: 36 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 94 }, 95: { name: "anonymous_52", count: 0, regions: { 0: { startLoc: { line: 198, col: 20 }, endLoc: { line: 201, col: 12 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 95 }, 96: { name: "buildCompactTopBar", count: 0, regions: { 0: { startLoc: { line: 217, col: 3 }, endLoc: { line: 239, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 96 }, 97: { name: "anonymous_54", count: 0, regions: { 0: { startLoc: { line: 219, col: 5 }, endLoc: { line: 238, col: 69 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 97 }, 98: { name: "anonymous_55", count: 0, regions: { 0: { startLoc: { line: 220, col: 7 }, endLoc: { line: 221, col: 34 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 98 }, 99: { name: "anonymous_56", count: 0, regions: { 0: { startLoc: { line: 222, col: 17 }, endLoc: { line: 224, col: 10 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 223, col: 11 }, endLoc: { line: 224, col: 10 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 99 }, 100: { name: "anonymous_58", count: 0, regions: { 0: { startLoc: { line: 227, col: 7 }, endLoc: { line: 233, col: 58 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 100 }, 101: { name: "overviewEntry", count: 0, regions: { 0: { startLoc: { line: 241, col: 3 }, endLoc: { line: 253, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 101 }, 102: { name: "anonymous_59", count: 0, regions: { 0: { startLoc: { line: 243, col: 29 }, endLoc: { line: 243, col: 39 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 102 }, 103: { name: "anonymous_60", count: 0, regions: { 0: { startLoc: { line: 244, col: 5 }, endLoc: { line: 250, col: 32 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 103 }, 104: { name: "buildOverviewPage", count: 0, regions: { 0: { startLoc: { line: 255, col: 3 }, endLoc: { line: 368, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 341, col: 13 }, endLoc: { line: 342, col: 12 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 347, col: 15 }, endLoc: { line: 348, col: 14 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 358, col: 13 }, endLoc: { line: 359, col: 12 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 104 }, 105: { name: "anonymous_62", count: 0, regions: { 0: { startLoc: { line: 257, col: 5 }, endLoc: { line: 367, col: 16 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 105 }, 106: { name: "anonymous_63", count: 0, regions: { 0: { startLoc: { line: 258, col: 7 }, endLoc: { line: 263, col: 30 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 106 }, 107: { name: "anonymous_64", count: 0, regions: { 0: { startLoc: { line: 264, col: 7 }, endLoc: { line: 270, col: 31 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 264, col: 12 }, endLoc: { line: 265, col: 55 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 107 }, 108: { name: "anonymous_65", count: 0, regions: { 0: { startLoc: { line: 272, col: 7 }, endLoc: { line: 334, col: 8 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 272, col: 46 }, endLoc: { line: 334, col: 8 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 272, col: 11 }, endLoc: { line: 272, col: 44 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 108 }, 109: { name: "anonymous_66", count: 0, regions: { 0: { startLoc: { line: 273, col: 9 }, endLoc: { line: 273, col: 15 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 109 }, 110: { name: "anonymous_67", count: 0, regions: { 0: { startLoc: { line: 273, col: 9 }, endLoc: { line: 333, col: 31 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 110 }, 111: { name: "anonymous_68", count: 0, regions: { 0: { startLoc: { line: 274, col: 11 }, endLoc: { line: 289, col: 33 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 111 }, 112: { name: "anonymous_69", count: 0, regions: { 0: { startLoc: { line: 275, col: 13 }, endLoc: { line: 279, col: 30 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 112 }, 113: { name: "anonymous_70", count: 0, regions: { 0: { startLoc: { line: 281, col: 13 }, endLoc: { line: 284, col: 40 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 113 }, 114: { name: "anonymous_72", count: 0, regions: { 0: { startLoc: { line: 291, col: 11 }, endLoc: { line: 323, col: 24 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 114 }, 115: { name: "anonymous_73", count: 0, regions: { 0: { startLoc: { line: 292, col: 13 }, endLoc: { line: 305, col: 28 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 115 }, 116: { name: "anonymous_74", count: 0, regions: { 0: { startLoc: { line: 293, col: 15 }, endLoc: { line: 296, col: 42 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 116 }, 117: { name: "anonymous_76", count: 0, regions: { 0: { startLoc: { line: 298, col: 15 }, endLoc: { line: 302, col: 42 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 117 }, 118: { name: "anonymous_78", count: 0, regions: { 0: { startLoc: { line: 307, col: 13 }, endLoc: { line: 321, col: 28 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 118 }, 119: { name: "anonymous_79", count: 0, regions: { 0: { startLoc: { line: 308, col: 15 }, endLoc: { line: 312, col: 42 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 119 }, 120: { name: "anonymous_81", count: 0, regions: { 0: { startLoc: { line: 314, col: 15 }, endLoc: { line: 318, col: 42 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 120 }, 121: { name: "anonymous_84", count: 0, regions: { 0: { startLoc: { line: 336, col: 7 }, endLoc: { line: 362, col: 20 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 121 }, 122: { name: "anonymous_85", count: 0, regions: { 0: { startLoc: { line: 339, col: 11 }, endLoc: { line: 342, col: 12 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 122 }, 123: { name: "anonymous_86", count: 0, regions: { 0: { startLoc: { line: 343, col: 9 }, endLoc: { line: 354, col: 10 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 343, col: 48 }, endLoc: { line: 349, col: 10 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 349, col: 16 }, endLoc: { line: 354, col: 10 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 343, col: 13 }, endLoc: { line: 343, col: 46 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 123 }, 124: { name: "anonymous_87", count: 0, regions: { 0: { startLoc: { line: 344, col: 11 }, endLoc: { line: 348, col: 15 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 124 }, 125: { name: "anonymous_88", count: 0, regions: { 0: { startLoc: { line: 345, col: 96 }, endLoc: { line: 348, col: 14 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 125 }, 126: { name: "anonymous_89", count: 0, regions: { 0: { startLoc: { line: 350, col: 11 }, endLoc: { line: 353, col: 15 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 126 }, 127: { name: "anonymous_90", count: 0, regions: { 0: { startLoc: { line: 351, col: 76 }, endLoc: { line: 353, col: 14 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 352, col: 15 }, endLoc: { line: 353, col: 14 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 127 }, 128: { name: "anonymous_91", count: 0, regions: { 0: { startLoc: { line: 356, col: 75 }, endLoc: { line: 359, col: 12 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 128 }, 129: { name: "buildSettingsContent", count: 0, regions: { 0: { startLoc: { line: 370, col: 3 }, endLoc: { line: 424, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 407, col: 11 }, endLoc: { line: 408, col: 10 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 407, col: 11 }, endLoc: { line: 408, col: 10 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 129 }, 130: { name: "anonymous_92", count: 0, regions: { 0: { startLoc: { line: 372, col: 5 }, endLoc: { line: 423, col: 6 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 372, col: 48 }, endLoc: { line: 381, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 381, col: 12 }, endLoc: { line: 423, col: 6 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 381, col: 101 }, endLoc: { line: 410, col: 6 }, count: 0, ignored: 0 }, 4: { startLoc: { line: 410, col: 12 }, endLoc: { line: 423, col: 6 }, count: 0, ignored: 0 }, 5: { startLoc: { line: 410, col: 62 }, endLoc: { line: 417, col: 6 }, count: 0, ignored: 0 }, 6: { startLoc: { line: 417, col: 12 }, endLoc: { line: 423, col: 6 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 372, col: 9 }, endLoc: { line: 372, col: 46 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 381, col: 16 }, endLoc: { line: 381, col: 99 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 2: { startLoc: { line: 410, col: 16 }, endLoc: { line: 410, col: 60 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 130 }, 131: { name: "anonymous_93", count: 0, regions: { 0: { startLoc: { line: 373, col: 7 }, endLoc: { line: 379, col: 10 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 131 }, 132: { name: "anonymous_94", count: 0, regions: { 0: { startLoc: { line: 373, col: 7 }, endLoc: { line: 379, col: 10 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 132 }, 133: { name: "anonymous_95", count: 0, regions: { 0: { startLoc: { line: 374, col: 23 }, endLoc: { line: 376, col: 10 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 375, col: 11 }, endLoc: { line: 376, col: 10 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 133 }, 134: { name: "anonymous_96", count: 0, regions: { 0: { startLoc: { line: 377, col: 17 }, endLoc: { line: 379, col: 10 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 378, col: 11 }, endLoc: { line: 379, col: 10 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 134 }, 135: { name: "anonymous_98", count: 0, regions: { 0: { startLoc: { line: 382, col: 7 }, endLoc: { line: 395, col: 72 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 135 }, 136: { name: "anonymous_99", count: 0, regions: { 0: { startLoc: { line: 382, col: 7 }, endLoc: { line: 395, col: 72 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 136 }, 137: { name: "anonymous_100", count: 0, regions: { 0: { startLoc: { line: 405, col: 17 }, endLoc: { line: 408, col: 10 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 137 }, 138: { name: "anonymous_102", count: 0, regions: { 0: { startLoc: { line: 411, col: 7 }, endLoc: { line: 412, col: 34 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 138 }, 139: { name: "anonymous_103", count: 0, regions: { 0: { startLoc: { line: 411, col: 7 }, endLoc: { line: 412, col: 34 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 139 }, 140: { name: "anonymous_104", count: 0, regions: { 0: { startLoc: { line: 413, col: 17 }, endLoc: { line: 415, col: 10 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 414, col: 11 }, endLoc: { line: 415, col: 10 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 140 }, 141: { name: "anonymous_106", count: 0, regions: { 0: { startLoc: { line: 418, col: 7 }, endLoc: { line: 418, col: 13 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 141 }, 142: { name: "anonymous_107", count: 0, regions: { 0: { startLoc: { line: 418, col: 7 }, endLoc: { line: 422, col: 21 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 142 }, 143: { name: "buildContentPane", count: 0, regions: { 0: { startLoc: { line: 426, col: 3 }, endLoc: { line: 437, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 143 }, 144: { name: "anonymous_108", count: 0, regions: { 0: { startLoc: { line: 428, col: 5 }, endLoc: { line: 436, col: 21 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 144 }, 145: { name: "buildCompactLayout", count: 0, regions: { 0: { startLoc: { line: 439, col: 3 }, endLoc: { line: 450, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 145 }, 146: { name: "anonymous_109", count: 0, regions: { 0: { startLoc: { line: 441, col: 5 }, endLoc: { line: 449, col: 69 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 146 }, 147: { name: "anonymous_110", count: 0, regions: { 0: { startLoc: { line: 442, col: 7 }, endLoc: { line: 444, col: 8 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 442, col: 53 }, endLoc: { line: 444, col: 8 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 442, col: 11 }, endLoc: { line: 442, col: 51 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 147 }, 148: { name: "anonymous_111", count: 0, regions: { 0: { startLoc: { line: 443, col: 9 }, endLoc: { line: 443, col: 34 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 148 }, 149: { name: "buildExpandedLayout", count: 0, regions: { 0: { startLoc: { line: 452, col: 3 }, endLoc: { line: 462, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 149 }, 150: { name: "anonymous_113", count: 0, regions: { 0: { startLoc: { line: 454, col: 5 }, endLoc: { line: 461, col: 69 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 150 }, 151: { name: "initialRender", count: 0, regions: { 0: { startLoc: { line: 464, col: 3 }, endLoc: { line: 474, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 151 }, 152: { name: "anonymous_114", count: 0, regions: { 0: { startLoc: { line: 465, col: 5 }, endLoc: { line: 473, col: 19 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 152 }, 153: { name: "anonymous_115", count: 0, regions: { 0: { startLoc: { line: 466, col: 7 }, endLoc: { line: 470, col: 8 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 466, col: 51 }, endLoc: { line: 468, col: 8 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 468, col: 14 }, endLoc: { line: 470, col: 8 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 466, col: 11 }, endLoc: { line: 466, col: 49 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 153 }, 154: { name: "anonymous_116", count: 0, regions: { 0: { startLoc: { line: 467, col: 9 }, endLoc: { line: 467, col: 34 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 154 }, 155: { name: "anonymous_117", count: 0, regions: { 0: { startLoc: { line: 469, col: 9 }, endLoc: { line: 469, col: 35 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 155 } }, exeLine: { 0: 1, 1: 2, 2: 3, 3: 4, 4: 5, 5: 6, 6: 7, 7: 8, 8: 9, 9: 10, 10: 11, 11: 12, 12: 13, 13: 14, 14: 15, 15: 16, 16: 17, 17: 18, 18: 19, 19: 22, 20: 23, 21: 24, 22: 26, 23: 27, 24: 29, 25: 30, 26: 32, 27: 33, 28: 35, 29: 36, 30: 38, 31: 39, 32: 41, 33: 42, 34: 44, 35: 45, 36: 46, 37: 47, 38: 48, 39: 49, 40: 52, 41: 53, 42: 54, 43: 55, 44: 56, 45: 57, 46: 59, 47: 61, 48: 63, 49: 64, 50: 65, 51: 66, 52: 67, 53: 68, 54: 69, 55: 70, 56: 71, 57: 72, 58: 73, 59: 74, 60: 75, 61: 76, 62: 77, 63: 78, 64: 79, 65: 80, 66: 82, 67: 83, 68: 84, 69: 85, 70: 86, 71: 88, 72: 89, 73: 92, 74: 93, 75: 94, 76: 96, 77: 99, 78: 100, 79: 103, 80: 104, 81: 105, 82: 108, 83: 109, 84: 110, 85: 112, 86: 115, 87: 116, 88: 117, 89: 119, 90: 120, 91: 123, 92: 124, 93: 127, 94: 128, 95: 132, 96: 133, 97: 134, 98: 135, 99: 136, 100: 137, 101: 138, 102: 142, 103: 143, 104: 144, 105: 145, 106: 146, 107: 147, 108: 149, 109: 150, 110: 152, 111: 153, 112: 154, 113: 156, 114: 157, 115: 158, 116: 159, 117: 160, 118: 161, 119: 162, 120: 163, 121: 164, 122: 165, 123: 168, 124: 169, 125: 170, 126: 171, 127: 172, 128: 173, 129: 174, 130: 175, 131: 176, 132: 179, 133: 180, 134: 181, 135: 182, 136: 183, 137: 184, 138: 185, 139: 186, 140: 187, 141: 188, 142: 192, 143: 193, 144: 194, 145: 195, 146: 196, 147: 197, 148: 198, 149: 199, 150: 200, 151: 204, 152: 206, 153: 207, 154: 208, 155: 209, 156: 210, 157: 211, 158: 212, 159: 213, 160: 218, 161: 219, 162: 220, 163: 221, 164: 222, 165: 223, 166: 227, 167: 228, 168: 229, 169: 230, 170: 231, 171: 232, 172: 233, 173: 235, 174: 236, 175: 237, 176: 238, 177: 242, 178: 243, 179: 244, 180: 245, 181: 246, 182: 247, 183: 248, 184: 249, 185: 250, 186: 251, 187: 256, 188: 257, 189: 258, 190: 259, 191: 260, 192: 261, 193: 262, 194: 263, 195: 264, 196: 265, 197: 266, 198: 267, 199: 268, 200: 269, 201: 270, 202: 272, 203: 273, 204: 274, 205: 275, 206: 276, 207: 277, 208: 278, 209: 279, 210: 281, 211: 282, 212: 283, 213: 284, 214: 287, 215: 288, 216: 289, 217: 291, 218: 292, 219: 293, 220: 294, 221: 295, 222: 296, 223: 298, 224: 299, 225: 300, 226: 301, 227: 302, 228: 305, 229: 307, 230: 308, 231: 309, 232: 310, 233: 311, 234: 312, 235: 314, 236: 315, 237: 316, 238: 317, 239: 318, 240: 321, 241: 323, 242: 325, 243: 326, 244: 327, 245: 328, 246: 329, 247: 330, 248: 331, 249: 333, 250: 336, 251: 337, 252: 338, 253: 339, 254: 340, 255: 341, 256: 343, 257: 344, 258: 345, 259: 346, 260: 347, 261: 349, 262: 350, 263: 351, 264: 352, 265: 355, 266: 356, 267: 357, 268: 358, 269: 361, 270: 362, 271: 364, 272: 365, 273: 366, 274: 367, 275: 371, 276: 372, 277: 373, 278: 374, 279: 375, 280: 377, 281: 378, 282: 381, 283: 382, 284: 383, 285: 384, 286: 385, 287: 386, 288: 387, 289: 388, 290: 389, 291: 390, 292: 391, 293: 392, 294: 393, 295: 394, 296: 395, 297: 396, 298: 397, 299: 398, 300: 399, 301: 400, 302: 401, 303: 402, 304: 403, 305: 404, 306: 405, 307: 406, 308: 407, 309: 410, 310: 411, 311: 412, 312: 413, 313: 414, 314: 417, 315: 418, 316: 419, 317: 421, 318: 422, 319: 427, 320: 428, 321: 429, 322: 431, 323: 432, 324: 433, 325: 434, 326: 435, 327: 436, 328: 440, 329: 441, 330: 442, 331: 443, 332: 445, 333: 447, 334: 448, 335: 449, 336: 453, 337: 454, 338: 455, 339: 456, 340: 458, 341: 459, 342: 460, 343: 461, 344: 464, 345: 465, 346: 466, 347: 467, 348: 468, 349: 469, 350: 472, 351: 473 } });
if (!("finalizeConstruction" in ViewPU.prototype)) {
    Reflect.set(ViewPU.prototype, "finalizeConstruction", () => { });
}
interface SettingsPage_Params {
    layoutMode?: LayoutMode;
    onClose?: () => void;
    onRemoteAccessCodeGateChange?: (enabled: boolean) => string;
    onRemoteAccessCodeRegenerate?: () => string;
    onRequestScreenRecordingPermission?: () => Promise<boolean>;
    onRefreshScreenRecordingPermission?: () => boolean;
    onRequestInputInjectionPermission?: () => Promise<boolean>;
    onRefreshInputInjectionPermission?: () => boolean;
    onRefreshXrdpServerStatus?: () => XrdpServerStatus;
    onStartXrdpServer?: () => Promise<XrdpServerStatus>;
    onOpenRemoteFilesDirectory?: () => void;
    initialPageName?: string;
    initialRemoteControlSection?: string;
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
    remoteControlServerAvailable?: boolean;
    pageName?: string;
    remoteControlTargetSection?: string;
    appearanceMode?: SettingsAppearanceMode;
    systemDark?: boolean;
}
import { BasicSettingsPage } from "@normalized:N&&&common/src/main/ets/components/settings/BasicSettingsPage&";
import { LayoutMode } from "@normalized:N&&&common/src/main/ets/adaptive/WindowLayoutPolicy&";
import { ProjectHelpPage } from "@normalized:N&&&common/src/main/ets/components/settings/ProjectHelpPage&";
import { RemoteControlSettingsPage } from "@normalized:N&&&common/src/main/ets/components/settings/RemoteControlSettingsPage&";
import type { XrdpServerStatus } from '../rdp/XrdpServerController';
import { SettingsRoute, SettingsText } from "@normalized:N&&&common/src/main/ets/components/settings/SettingsConstants&";
import { SettingsAccent, SettingsBackButton, SettingsDesktopNavItem, SettingsKeyValueRow, SettingsListItem, SettingsResources, SettingsStatusChip, SettingsTheme } from "@normalized:N&&&common/src/main/ets/components/settings/SettingsUi&";
import type { SettingsAccentName, SettingsAppearanceMode, SettingsStatusTone } from "@normalized:N&&&common/src/main/ets/components/settings/SettingsUi&";
export class SettingsPage extends ViewPU {
    constructor(parent, params, __localStorage, elmtId = -1, paramsLambda = undefined, extraInfo) {
        super(parent, __localStorage, elmtId, extraInfo);
        if (typeof paramsLambda === "function") {
            this.paramsGenerator_ = paramsLambda;
        }
        this.__layoutMode = new SynchedPropertySimpleOneWayPU(params.layoutMode, this, "layoutMode");
        this.onClose = () => {
            bjccovmshb1hxg.instrumentFunction(10);
        };
        this.onRemoteAccessCodeGateChange = (_enabled: boolean) => {
            bjccovmshb1hxg.instrumentFunction(11);
            bjccovmshb1hxg.instrumentRegion(11, 1);
            return '';
        };
        this.onRemoteAccessCodeRegenerate = () => {
            bjccovmshb1hxg.instrumentFunction(12);
            bjccovmshb1hxg.instrumentRegion(12, 1);
            return '';
        };
        this.onRequestScreenRecordingPermission = () => {
            bjccovmshb1hxg.instrumentFunction(13);
            bjccovmshb1hxg.instrumentRegion(13, 1);
            return Promise.resolve(false);
        };
        this.onRefreshScreenRecordingPermission = () => {
            bjccovmshb1hxg.instrumentFunction(14);
            bjccovmshb1hxg.instrumentRegion(14, 1);
            return false;
        };
        this.onRequestInputInjectionPermission = () => {
            bjccovmshb1hxg.instrumentFunction(15);
            bjccovmshb1hxg.instrumentRegion(15, 1);
            return Promise.resolve(false);
        };
        this.onRefreshInputInjectionPermission = () => {
            bjccovmshb1hxg.instrumentFunction(16);
            bjccovmshb1hxg.instrumentRegion(16, 1);
            return false;
        };
        this.onRefreshXrdpServerStatus = () => {
            bjccovmshb1hxg.instrumentFunction(17);
            bjccovmshb1hxg.instrumentRegion(17, 1);
            return {
                running: false,
                state: 'Stopped',
                port: 3390,
                message: ''
            };
        };
        this.onStartXrdpServer = () => {
            bjccovmshb1hxg.instrumentFunction(18);
            bjccovmshb1hxg.instrumentRegion(18, 1);
            const status: XrdpServerStatus = {
                running: false,
                state: 'Stopped',
                port: 3390,
                message: ''
            };
            return Promise.resolve(status);
        };
        this.onOpenRemoteFilesDirectory = () => {
            bjccovmshb1hxg.instrumentFunction(19);
        };
        this.__initialPageName = new SynchedPropertySimpleOneWayPU(params.initialPageName, this, "initialPageName");
        this.__initialRemoteControlSection = new SynchedPropertySimpleOneWayPU(params.initialRemoteControlSection, this, "initialRemoteControlSection");
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
        this.__remoteControlServerAvailable = new SynchedPropertySimpleOneWayPU(params.remoteControlServerAvailable, this, "remoteControlServerAvailable");
        this.__pageName = new ObservedPropertySimplePU(SettingsRoute.SETTINGS, this, "pageName");
        this.__remoteControlTargetSection = new ObservedPropertySimplePU('', this, "remoteControlTargetSection");
        this.__appearanceMode = new ObservedPropertySimplePU(SettingsTheme.getStoredAppearanceMode(), this, "appearanceMode");
        this.__systemDark = this.createStorageLink('settingsSystemDark', false, "systemDark");
        this.setInitiallyProvidedValue(params);
        this.finalizeConstruction();
    }
    setInitiallyProvidedValue(params: SettingsPage_Params) {
        bjccovmshb1hxg.instrumentFunction(20);
        if (params.layoutMode === undefined) {
            this.__layoutMode.set(LayoutMode.COMPACT);
        }
        else {
        }
        if (params.onClose !== undefined) {
            this.onClose = params.onClose;
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
        if (params.initialPageName === undefined) {
            this.__initialPageName.set(SettingsRoute.SETTINGS);
        }
        else {
        }
        if (params.initialRemoteControlSection === undefined) {
            this.__initialRemoteControlSection.set('');
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
        if (params.remoteControlServerAvailable === undefined) {
            this.__remoteControlServerAvailable.set(true);
        }
        else {
        }
        if (params.pageName !== undefined) {
            this.pageName = params.pageName;
        }
        else {
        }
        if (params.remoteControlTargetSection !== undefined) {
            this.remoteControlTargetSection = params.remoteControlTargetSection;
        }
        else {
        }
        if (params.appearanceMode !== undefined) {
            this.appearanceMode = params.appearanceMode;
        }
        else {
        }
    }
    updateStateVars(params: SettingsPage_Params) {
        bjccovmshb1hxg.instrumentFunction(21);
        this.__layoutMode.reset(params.layoutMode);
        this.__initialPageName.reset(params.initialPageName);
        this.__initialRemoteControlSection.reset(params.initialRemoteControlSection);
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
        this.__remoteControlServerAvailable.reset(params.remoteControlServerAvailable);
    }
    purgeVariableDependenciesOnElmtId(rmElmtId) {
        this.__layoutMode.purgeDependencyOnElmtId(rmElmtId);
        this.__initialPageName.purgeDependencyOnElmtId(rmElmtId);
        this.__initialRemoteControlSection.purgeDependencyOnElmtId(rmElmtId);
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
        this.__remoteControlServerAvailable.purgeDependencyOnElmtId(rmElmtId);
        this.__pageName.purgeDependencyOnElmtId(rmElmtId);
        this.__remoteControlTargetSection.purgeDependencyOnElmtId(rmElmtId);
        this.__appearanceMode.purgeDependencyOnElmtId(rmElmtId);
        this.__systemDark.purgeDependencyOnElmtId(rmElmtId);
    }
    aboutToBeDeleted() {
        this.__layoutMode.aboutToBeDeleted();
        this.__initialPageName.aboutToBeDeleted();
        this.__initialRemoteControlSection.aboutToBeDeleted();
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
        this.__remoteControlServerAvailable.aboutToBeDeleted();
        this.__pageName.aboutToBeDeleted();
        this.__remoteControlTargetSection.aboutToBeDeleted();
        this.__appearanceMode.aboutToBeDeleted();
        this.__systemDark.aboutToBeDeleted();
        SubscriberManager.Get().delete(this.id__());
        this.aboutToBeDeletedInternal();
    }
    private __layoutMode: SynchedPropertySimpleOneWayPU<LayoutMode>;
    get layoutMode() {
        bjccovmshb1hxg.instrumentFunction(22);
        return this.__layoutMode.get();
    }
    set layoutMode(newValue: LayoutMode) {
        bjccovmshb1hxg.instrumentFunction(23);
        this.__layoutMode.set(newValue);
    }
    private onClose: () => void;
    private onRemoteAccessCodeGateChange: (enabled: boolean) => string;
    private onRemoteAccessCodeRegenerate: () => string;
    private onRequestScreenRecordingPermission: () => Promise<boolean>;
    private onRefreshScreenRecordingPermission: () => boolean;
    private onRequestInputInjectionPermission: () => Promise<boolean>;
    private onRefreshInputInjectionPermission: () => boolean;
    private onRefreshXrdpServerStatus: () => XrdpServerStatus;
    private onStartXrdpServer: () => Promise<XrdpServerStatus>;
    private onOpenRemoteFilesDirectory: () => void;
    private __initialPageName: SynchedPropertySimpleOneWayPU<string>;
    get initialPageName() {
        bjccovmshb1hxg.instrumentFunction(34);
        return this.__initialPageName.get();
    }
    set initialPageName(newValue: string) {
        bjccovmshb1hxg.instrumentFunction(35);
        this.__initialPageName.set(newValue);
    }
    private __initialRemoteControlSection: SynchedPropertySimpleOneWayPU<string>;
    get initialRemoteControlSection() {
        bjccovmshb1hxg.instrumentFunction(36);
        return this.__initialRemoteControlSection.get();
    }
    set initialRemoteControlSection(newValue: string) {
        bjccovmshb1hxg.instrumentFunction(37);
        this.__initialRemoteControlSection.set(newValue);
    }
    private __remoteAccessCode: SynchedPropertySimpleOneWayPU<string>;
    get remoteAccessCode() {
        bjccovmshb1hxg.instrumentFunction(38);
        return this.__remoteAccessCode.get();
    }
    set remoteAccessCode(newValue: string) {
        bjccovmshb1hxg.instrumentFunction(39);
        this.__remoteAccessCode.set(newValue);
    }
    private __remoteAccessCodeGateEnabled: SynchedPropertySimpleOneWayPU<boolean>;
    get remoteAccessCodeGateEnabled() {
        bjccovmshb1hxg.instrumentFunction(40);
        return this.__remoteAccessCodeGateEnabled.get();
    }
    set remoteAccessCodeGateEnabled(newValue: boolean) {
        bjccovmshb1hxg.instrumentFunction(41);
        this.__remoteAccessCodeGateEnabled.set(newValue);
    }
    private __screenRecordingPermissionGranted: SynchedPropertySimpleOneWayPU<boolean>;
    get screenRecordingPermissionGranted() {
        bjccovmshb1hxg.instrumentFunction(42);
        return this.__screenRecordingPermissionGranted.get();
    }
    set screenRecordingPermissionGranted(newValue: boolean) {
        bjccovmshb1hxg.instrumentFunction(43);
        this.__screenRecordingPermissionGranted.set(newValue);
    }
    private __inputInjectionPermissionGranted: SynchedPropertySimpleOneWayPU<boolean>;
    get inputInjectionPermissionGranted() {
        bjccovmshb1hxg.instrumentFunction(44);
        return this.__inputInjectionPermissionGranted.get();
    }
    set inputInjectionPermissionGranted(newValue: boolean) {
        bjccovmshb1hxg.instrumentFunction(45);
        this.__inputInjectionPermissionGranted.set(newValue);
    }
    private __xrdpServerRunning: SynchedPropertySimpleOneWayPU<boolean>;
    get xrdpServerRunning() {
        bjccovmshb1hxg.instrumentFunction(46);
        return this.__xrdpServerRunning.get();
    }
    set xrdpServerRunning(newValue: boolean) {
        bjccovmshb1hxg.instrumentFunction(47);
        this.__xrdpServerRunning.set(newValue);
    }
    private __xrdpServerState: SynchedPropertySimpleOneWayPU<string>;
    get xrdpServerState() {
        bjccovmshb1hxg.instrumentFunction(48);
        return this.__xrdpServerState.get();
    }
    set xrdpServerState(newValue: string) {
        bjccovmshb1hxg.instrumentFunction(49);
        this.__xrdpServerState.set(newValue);
    }
    private __xrdpServerPort: SynchedPropertySimpleOneWayPU<number>;
    get xrdpServerPort() {
        bjccovmshb1hxg.instrumentFunction(50);
        return this.__xrdpServerPort.get();
    }
    set xrdpServerPort(newValue: number) {
        bjccovmshb1hxg.instrumentFunction(51);
        this.__xrdpServerPort.set(newValue);
    }
    private __xrdpServerMessage: SynchedPropertySimpleOneWayPU<string>;
    get xrdpServerMessage() {
        bjccovmshb1hxg.instrumentFunction(52);
        return this.__xrdpServerMessage.get();
    }
    set xrdpServerMessage(newValue: string) {
        bjccovmshb1hxg.instrumentFunction(53);
        this.__xrdpServerMessage.set(newValue);
    }
    private __xrdpServerBusy: SynchedPropertySimpleOneWayPU<boolean>;
    get xrdpServerBusy() {
        bjccovmshb1hxg.instrumentFunction(54);
        return this.__xrdpServerBusy.get();
    }
    set xrdpServerBusy(newValue: boolean) {
        bjccovmshb1hxg.instrumentFunction(55);
        this.__xrdpServerBusy.set(newValue);
    }
    private __screenRecordingPermissionBusy: SynchedPropertySimpleOneWayPU<boolean>;
    get screenRecordingPermissionBusy() {
        bjccovmshb1hxg.instrumentFunction(56);
        return this.__screenRecordingPermissionBusy.get();
    }
    set screenRecordingPermissionBusy(newValue: boolean) {
        bjccovmshb1hxg.instrumentFunction(57);
        this.__screenRecordingPermissionBusy.set(newValue);
    }
    private __inputInjectionPermissionBusy: SynchedPropertySimpleOneWayPU<boolean>;
    get inputInjectionPermissionBusy() {
        bjccovmshb1hxg.instrumentFunction(58);
        return this.__inputInjectionPermissionBusy.get();
    }
    set inputInjectionPermissionBusy(newValue: boolean) {
        bjccovmshb1hxg.instrumentFunction(59);
        this.__inputInjectionPermissionBusy.set(newValue);
    }
    private __remoteControlServerAvailable: SynchedPropertySimpleOneWayPU<boolean>;
    get remoteControlServerAvailable() {
        bjccovmshb1hxg.instrumentFunction(60);
        return this.__remoteControlServerAvailable.get();
    }
    set remoteControlServerAvailable(newValue: boolean) {
        bjccovmshb1hxg.instrumentFunction(61);
        this.__remoteControlServerAvailable.set(newValue);
    }
    private __pageName: ObservedPropertySimplePU<string>;
    get pageName() {
        bjccovmshb1hxg.instrumentFunction(62);
        return this.__pageName.get();
    }
    set pageName(newValue: string) {
        bjccovmshb1hxg.instrumentFunction(63);
        this.__pageName.set(newValue);
    }
    private __remoteControlTargetSection: ObservedPropertySimplePU<string>;
    get remoteControlTargetSection() {
        bjccovmshb1hxg.instrumentFunction(64);
        return this.__remoteControlTargetSection.get();
    }
    set remoteControlTargetSection(newValue: string) {
        bjccovmshb1hxg.instrumentFunction(65);
        this.__remoteControlTargetSection.set(newValue);
    }
    private __appearanceMode: ObservedPropertySimplePU<SettingsAppearanceMode>;
    get appearanceMode() {
        bjccovmshb1hxg.instrumentFunction(66);
        return this.__appearanceMode.get();
    }
    set appearanceMode(newValue: SettingsAppearanceMode) {
        bjccovmshb1hxg.instrumentFunction(67);
        this.__appearanceMode.set(newValue);
    }
    private __systemDark: ObservedPropertyAbstractPU<boolean>;
    get systemDark() {
        bjccovmshb1hxg.instrumentFunction(68);
        return this.__systemDark.get();
    }
    set systemDark(newValue: boolean) {
        bjccovmshb1hxg.instrumentFunction(69);
        this.__systemDark.set(newValue);
    }
    aboutToAppear(): void {
        bjccovmshb1hxg.instrumentFunction(70);
        this.appearanceMode = SettingsTheme.getStoredAppearanceMode();
        this.pageName = this.initialPageName.length > 0 ? (bjccovmshb1hxg.instrumentBranch(70, 0, true), this.initialPageName) : (bjccovmshb1hxg.instrumentBranch(70, 0, false), SettingsRoute.SETTINGS);
        if (!this.remoteControlServerAvailable && this.pageName === SettingsRoute.REMOTE_CONTROL) {
            bjccovmshb1hxg.instrumentBranch(70, 1, true);
            bjccovmshb1hxg.instrumentRegion(70, 1);
            this.pageName = SettingsRoute.SETTINGS;
        }
        else {
            bjccovmshb1hxg.instrumentBranch(70, 1, false);
        }
        bjccovmshb1hxg.instrumentRegion(70, 2);
        this.remoteControlTargetSection = this.pageName === SettingsRoute.REMOTE_CONTROL ? (bjccovmshb1hxg.instrumentBranch(70, 2, true), this.initialRemoteControlSection) : (bjccovmshb1hxg.instrumentBranch(70, 2, false), '');
    }
    private isDarkMode(): boolean {
        bjccovmshb1hxg.instrumentFunction(71);
        if (this.appearanceMode === 'system') {
            bjccovmshb1hxg.instrumentBranch(71, 0, true);
            bjccovmshb1hxg.instrumentRegion(71, 1);
            return this.systemDark;
        }
        else {
            bjccovmshb1hxg.instrumentBranch(71, 0, false);
        }
        bjccovmshb1hxg.instrumentRegion(71, 2);
        return SettingsTheme.isDark(this.getUIContext(), this.appearanceMode);
    }
    private xrdpStatusTone(): SettingsStatusTone {
        bjccovmshb1hxg.instrumentFunction(72);
        bjccovmshb1hxg.instrumentRegion(72, 1);
        return SettingsText.remoteServerStatusTone(this.xrdpServerRunning, this.xrdpServerBusy, this.xrdpServerState);
    }
    private xrdpStatusText(): string {
        bjccovmshb1hxg.instrumentFunction(73);
        bjccovmshb1hxg.instrumentRegion(73, 1);
        return SettingsText.remoteServerStatusText(this.xrdpServerRunning, this.xrdpServerBusy, this.xrdpServerState, this.xrdpServerPort);
    }
    private screenStatusTone(): SettingsStatusTone {
        bjccovmshb1hxg.instrumentFunction(74);
        if (this.screenRecordingPermissionBusy) {
            bjccovmshb1hxg.instrumentBranch(74, 0, true);
            bjccovmshb1hxg.instrumentRegion(74, 1);
            return 'info';
        }
        else {
            bjccovmshb1hxg.instrumentBranch(74, 0, false);
        }
        bjccovmshb1hxg.instrumentRegion(74, 2);
        return this.screenRecordingPermissionGranted ? (bjccovmshb1hxg.instrumentBranch(74, 1, true), 'ok') : (bjccovmshb1hxg.instrumentBranch(74, 1, false), 'warning');
    }
    private screenStatusText(): string {
        bjccovmshb1hxg.instrumentFunction(75);
        if (this.screenRecordingPermissionBusy) {
            bjccovmshb1hxg.instrumentBranch(75, 0, true);
            bjccovmshb1hxg.instrumentRegion(75, 1);
            return SettingsText.REMOTE_PERMISSION_BUSY;
        }
        else {
            bjccovmshb1hxg.instrumentBranch(75, 0, false);
        }
        bjccovmshb1hxg.instrumentRegion(75, 2);
        return this.screenRecordingPermissionGranted ? (bjccovmshb1hxg.instrumentBranch(75, 1, true), SettingsText.REMOTE_PERMISSION_GRANTED) : (bjccovmshb1hxg.instrumentBranch(75, 1, false), SettingsText.REMOTE_PERMISSION_MISSING);
    }
    private gateStatusTone(): SettingsStatusTone {
        bjccovmshb1hxg.instrumentFunction(76);
        bjccovmshb1hxg.instrumentRegion(76, 1);
        return this.remoteAccessCodeGateEnabled ? (bjccovmshb1hxg.instrumentBranch(76, 0, true), 'ok') : (bjccovmshb1hxg.instrumentBranch(76, 0, false), 'neutral');
    }
    private gateStatusText(): string {
        bjccovmshb1hxg.instrumentFunction(77);
        bjccovmshb1hxg.instrumentRegion(77, 1);
        return this.remoteAccessCodeGateEnabled ? (bjccovmshb1hxg.instrumentBranch(77, 0, true), SettingsText.REMOTE_ACCESS_GATE_ON) : (bjccovmshb1hxg.instrumentBranch(77, 0, false), SettingsText.REMOTE_ACCESS_GATE_OFF);
    }
    private buildDesktopNav(parent = null) {
        bjccovmshb1hxg.instrumentFunction(78);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1hxg.instrumentFunction(79);
            Column.create();
            Column.width(196);
            Column.height('100%');
            Column.padding(14);
            Column.backgroundColor(SettingsTheme.cardBackground(this.isDarkMode()));
            Column.borderRadius(16);
            Column.border({
                width: 1,
                color: SettingsTheme.borderColor(this.isDarkMode())
            });
        }, Column);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1hxg.instrumentFunction(80);
            Row.create();
            Row.alignItems(VerticalAlign.Center);
            Row.width('100%');
            Row.margin({ bottom: 20 });
        }, Row);
        {
            this.observeComponentCreation2((elmtId, isInitialRender) => {
                bjccovmshb1hxg.instrumentFunction(81);
                if (isInitialRender) {
                    let componentCall = new SettingsBackButton(this, {
                        isDark: this.isDarkMode(),
                        onBack: () => {
                            bjccovmshb1hxg.instrumentFunction(82);
                            bjccovmshb1hxg.instrumentRegion(82, 1);
                            this.onClose();
                        }
                    }, undefined, elmtId, () => { }, { page: "common/src/main/ets/components/SettingsPage.ets", line: 135, col: 9 });
                    ViewPU.create(componentCall);
                    let paramsLambda = () => {
                        return {
                            isDark: this.isDarkMode(),
                            onBack: () => {
                                this.onClose();
                            }
                        };
                    };
                    componentCall.paramsGenerator_ = paramsLambda;
                }
                else {
                    this.updateStateVarsOfChildByElmtId(elmtId, {
                        isDark: this.isDarkMode()
                    });
                }
            }, { name: "SettingsBackButton" });
        }
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1hxg.instrumentFunction(83);
            Column.create();
            Column.alignItems(HorizontalAlign.Start);
            Column.layoutWeight(1);
        }, Column);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1hxg.instrumentFunction(84);
            Text.create(SettingsText.SETTINGS_TITLE);
            Text.fontSize(20);
            Text.fontWeight(FontWeight.Bold);
            Text.fontColor(SettingsTheme.primaryText(this.isDarkMode()));
            Text.width('100%');
        }, Text);
        Text.pop();
        Column.pop();
        Row.pop();
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1hxg.instrumentFunction(85);
            Column.create({ space: 8 });
            Column.width('100%');
        }, Column);
        {
            this.observeComponentCreation2((elmtId, isInitialRender) => {
                bjccovmshb1hxg.instrumentFunction(86);
                if (isInitialRender) {
                    let componentCall = new SettingsDesktopNavItem(this, {
                        title: SettingsText.SETTINGS_NAV_OVERVIEW,
                        iconResource: SettingsResources.OVERVIEW_ICON,
                        accentName: SettingsAccent.BLUE,
                        selected: this.pageName === SettingsRoute.SETTINGS,
                        isDark: this.isDarkMode(),
                        onPress: () => {
                            bjccovmshb1hxg.instrumentFunction(87);
                            this.pageName = SettingsRoute.SETTINGS;
                            bjccovmshb1hxg.instrumentRegion(78, 1);
                            this.remoteControlTargetSection = '';
                        }
                    }, undefined, elmtId, () => { }, { page: "common/src/main/ets/components/SettingsPage.ets", line: 157, col: 9 });
                    ViewPU.create(componentCall);
                    let paramsLambda = () => {
                        return {
                            title: SettingsText.SETTINGS_NAV_OVERVIEW,
                            iconResource: SettingsResources.OVERVIEW_ICON,
                            accentName: SettingsAccent.BLUE,
                            selected: this.pageName === SettingsRoute.SETTINGS,
                            isDark: this.isDarkMode(),
                            onPress: () => {
                                this.pageName = SettingsRoute.SETTINGS;
                                bjccovmshb1hxg.instrumentRegion(78, 2);
                                this.remoteControlTargetSection = '';
                            }
                        };
                    };
                    componentCall.paramsGenerator_ = paramsLambda;
                }
                else {
                    this.updateStateVarsOfChildByElmtId(elmtId, {
                        title: SettingsText.SETTINGS_NAV_OVERVIEW,
                        iconResource: SettingsResources.OVERVIEW_ICON,
                        accentName: SettingsAccent.BLUE,
                        selected: this.pageName === SettingsRoute.SETTINGS,
                        isDark: this.isDarkMode()
                    });
                }
            }, { name: "SettingsDesktopNavItem" });
        }
        {
            this.observeComponentCreation2((elmtId, isInitialRender) => {
                bjccovmshb1hxg.instrumentFunction(88);
                if (isInitialRender) {
                    let componentCall = new SettingsDesktopNavItem(this, {
                        title: SettingsText.BASIC_SETTINGS_ENTRY_TITLE,
                        iconResource: SettingsResources.BASIC_ICON,
                        accentName: SettingsAccent.CYAN,
                        selected: this.pageName === SettingsRoute.BASIC,
                        isDark: this.isDarkMode(),
                        onPress: () => {
                            bjccovmshb1hxg.instrumentFunction(89);
                            this.pageName = SettingsRoute.BASIC;
                            bjccovmshb1hxg.instrumentRegion(78, 3);
                            this.remoteControlTargetSection = '';
                        }
                    }, undefined, elmtId, () => { }, { page: "common/src/main/ets/components/SettingsPage.ets", line: 168, col: 9 });
                    ViewPU.create(componentCall);
                    let paramsLambda = () => {
                        return {
                            title: SettingsText.BASIC_SETTINGS_ENTRY_TITLE,
                            iconResource: SettingsResources.BASIC_ICON,
                            accentName: SettingsAccent.CYAN,
                            selected: this.pageName === SettingsRoute.BASIC,
                            isDark: this.isDarkMode(),
                            onPress: () => {
                                this.pageName = SettingsRoute.BASIC;
                                bjccovmshb1hxg.instrumentRegion(78, 4);
                                this.remoteControlTargetSection = '';
                            }
                        };
                    };
                    componentCall.paramsGenerator_ = paramsLambda;
                }
                else {
                    this.updateStateVarsOfChildByElmtId(elmtId, {
                        title: SettingsText.BASIC_SETTINGS_ENTRY_TITLE,
                        iconResource: SettingsResources.BASIC_ICON,
                        accentName: SettingsAccent.CYAN,
                        selected: this.pageName === SettingsRoute.BASIC,
                        isDark: this.isDarkMode()
                    });
                }
            }, { name: "SettingsDesktopNavItem" });
        }
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1hxg.instrumentFunction(90);
            If.create();
            if (this.remoteControlServerAvailable) {
                bjccovmshb1hxg.instrumentBranch(90, 0, true);
                bjccovmshb1hxg.instrumentRegion(90, 1);
                this.ifElseBranchUpdateFunction(0, () => {
                    bjccovmshb1hxg.instrumentFunction(91);
                    {
                        this.observeComponentCreation2((elmtId, isInitialRender) => {
                            bjccovmshb1hxg.instrumentFunction(92);
                            if (isInitialRender) {
                                let componentCall = new SettingsDesktopNavItem(this, {
                                    title: SettingsText.REMOTE_CONTROL_ENTRY_TITLE,
                                    iconResource: SettingsResources.REMOTE_CONTROL_ICON,
                                    accentName: SettingsAccent.BLUE,
                                    selected: this.pageName === SettingsRoute.REMOTE_CONTROL,
                                    isDark: this.isDarkMode(),
                                    onPress: () => {
                                        bjccovmshb1hxg.instrumentFunction(93);
                                        this.pageName = SettingsRoute.REMOTE_CONTROL;
                                        bjccovmshb1hxg.instrumentRegion(78, 5);
                                        this.remoteControlTargetSection = '';
                                    }
                                }, undefined, elmtId, () => { }, { page: "common/src/main/ets/components/SettingsPage.ets", line: 180, col: 11 });
                                ViewPU.create(componentCall);
                                let paramsLambda = () => {
                                    return {
                                        title: SettingsText.REMOTE_CONTROL_ENTRY_TITLE,
                                        iconResource: SettingsResources.REMOTE_CONTROL_ICON,
                                        accentName: SettingsAccent.BLUE,
                                        selected: this.pageName === SettingsRoute.REMOTE_CONTROL,
                                        isDark: this.isDarkMode(),
                                        onPress: () => {
                                            this.pageName = SettingsRoute.REMOTE_CONTROL;
                                            bjccovmshb1hxg.instrumentRegion(78, 6);
                                            this.remoteControlTargetSection = '';
                                        }
                                    };
                                };
                                componentCall.paramsGenerator_ = paramsLambda;
                            }
                            else {
                                this.updateStateVarsOfChildByElmtId(elmtId, {
                                    title: SettingsText.REMOTE_CONTROL_ENTRY_TITLE,
                                    iconResource: SettingsResources.REMOTE_CONTROL_ICON,
                                    accentName: SettingsAccent.BLUE,
                                    selected: this.pageName === SettingsRoute.REMOTE_CONTROL,
                                    isDark: this.isDarkMode()
                                });
                            }
                        }, { name: "SettingsDesktopNavItem" });
                    }
                });
            }
            else {
                bjccovmshb1hxg.instrumentBranch(90, 0, false);
                this.ifElseBranchUpdateFunction(1, () => {
                });
            }
        }, If);
        If.pop();
        {
            this.observeComponentCreation2((elmtId, isInitialRender) => {
                bjccovmshb1hxg.instrumentFunction(94);
                if (isInitialRender) {
                    let componentCall = new SettingsDesktopNavItem(this, {
                        title: SettingsText.PROJECT_HELP_ENTRY_TITLE,
                        iconResource: SettingsResources.PROJECT_HELP_ICON,
                        accentName: SettingsAccent.PURPLE,
                        selected: this.pageName === SettingsRoute.PROJECT_HELP,
                        isDark: this.isDarkMode(),
                        onPress: () => {
                            bjccovmshb1hxg.instrumentFunction(95);
                            this.pageName = SettingsRoute.PROJECT_HELP;
                            bjccovmshb1hxg.instrumentRegion(78, 7);
                            this.remoteControlTargetSection = '';
                        }
                    }, undefined, elmtId, () => { }, { page: "common/src/main/ets/components/SettingsPage.ets", line: 192, col: 9 });
                    ViewPU.create(componentCall);
                    let paramsLambda = () => {
                        return {
                            title: SettingsText.PROJECT_HELP_ENTRY_TITLE,
                            iconResource: SettingsResources.PROJECT_HELP_ICON,
                            accentName: SettingsAccent.PURPLE,
                            selected: this.pageName === SettingsRoute.PROJECT_HELP,
                            isDark: this.isDarkMode(),
                            onPress: () => {
                                this.pageName = SettingsRoute.PROJECT_HELP;
                                bjccovmshb1hxg.instrumentRegion(78, 8);
                                this.remoteControlTargetSection = '';
                            }
                        };
                    };
                    componentCall.paramsGenerator_ = paramsLambda;
                }
                else {
                    this.updateStateVarsOfChildByElmtId(elmtId, {
                        title: SettingsText.PROJECT_HELP_ENTRY_TITLE,
                        iconResource: SettingsResources.PROJECT_HELP_ICON,
                        accentName: SettingsAccent.PURPLE,
                        selected: this.pageName === SettingsRoute.PROJECT_HELP,
                        isDark: this.isDarkMode()
                    });
                }
            }, { name: "SettingsDesktopNavItem" });
        }
        Column.pop();
        Column.pop();
    }
    private buildCompactTopBar(parent = null) {
        bjccovmshb1hxg.instrumentFunction(96);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1hxg.instrumentFunction(97);
            Row.create({ space: 12 });
            Row.width('100%');
            Row.padding({ left: 16, right: 16, top: 8, bottom: 8 });
            Row.alignItems(VerticalAlign.Center);
            Row.backgroundColor(SettingsTheme.pageBackground(this.isDarkMode()));
        }, Row);
        {
            this.observeComponentCreation2((elmtId, isInitialRender) => {
                bjccovmshb1hxg.instrumentFunction(98);
                if (isInitialRender) {
                    let componentCall = new SettingsBackButton(this, {
                        isDark: this.isDarkMode(),
                        onBack: () => {
                            bjccovmshb1hxg.instrumentFunction(99);
                            bjccovmshb1hxg.instrumentRegion(99, 1);
                            this.onClose();
                        }
                    }, undefined, elmtId, () => { }, { page: "common/src/main/ets/components/SettingsPage.ets", line: 220, col: 7 });
                    ViewPU.create(componentCall);
                    let paramsLambda = () => {
                        return {
                            isDark: this.isDarkMode(),
                            onBack: () => {
                                this.onClose();
                            }
                        };
                    };
                    componentCall.paramsGenerator_ = paramsLambda;
                }
                else {
                    this.updateStateVarsOfChildByElmtId(elmtId, {
                        isDark: this.isDarkMode()
                    });
                }
            }, { name: "SettingsBackButton" });
        }
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1hxg.instrumentFunction(100);
            Text.create(SettingsText.SETTINGS_TITLE);
            Text.fontSize(20);
            Text.fontWeight(FontWeight.Bold);
            Text.fontColor(SettingsTheme.primaryText(this.isDarkMode()));
            Text.layoutWeight(1);
            Text.maxLines(1);
            Text.textOverflow({ overflow: TextOverflow.Ellipsis });
        }, Text);
        Text.pop();
        Row.pop();
    }
    private overviewEntry(title: string, description: string, iconResource: Resource, accentName: SettingsAccentName, value: string, onPress: () => void, parent = null) {
        bjccovmshb1hxg.instrumentFunction(101);
        {
            this.observeComponentCreation2((elmtId, isInitialRender) => {
                bjccovmshb1hxg.instrumentFunction(103);
                if (isInitialRender) {
                    let componentCall = new SettingsListItem(this, {
                        title: title,
                        description: description,
                        iconResource: iconResource,
                        accentName: accentName,
                        value: value,
                        isDark: this.isDarkMode(),
                        onPress: onPress
                    }, undefined, elmtId, () => { }, { page: "common/src/main/ets/components/SettingsPage.ets", line: 244, col: 5 });
                    ViewPU.create(componentCall);
                    let paramsLambda = () => {
                        return {
                            title: title,
                            description: description,
                            iconResource: iconResource,
                            accentName: accentName,
                            value: value,
                            isDark: this.isDarkMode(),
                            onPress: onPress
                        };
                    };
                    componentCall.paramsGenerator_ = paramsLambda;
                }
                else {
                    this.updateStateVarsOfChildByElmtId(elmtId, {
                        title: title,
                        description: description,
                        iconResource: iconResource,
                        accentName: accentName,
                        value: value,
                        isDark: this.isDarkMode()
                    });
                }
            }, { name: "SettingsListItem" });
        }
    }
    private buildOverviewPage(parent = null) {
        bjccovmshb1hxg.instrumentFunction(104);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1hxg.instrumentFunction(105);
            Column.create();
            Column.alignItems(HorizontalAlign.Start);
            Column.justifyContent(FlexAlign.Start);
            Column.width('100%');
            Column.padding(22);
        }, Column);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1hxg.instrumentFunction(106);
            Text.create(SettingsText.SETTINGS_OVERVIEW_TITLE);
            Text.fontSize(24);
            Text.fontWeight(FontWeight.Bold);
            Text.fontColor(SettingsTheme.primaryText(this.isDarkMode()));
            Text.width('100%');
            Text.margin({ bottom: 6 });
        }, Text);
        Text.pop();
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1hxg.instrumentFunction(107);
            Text.create(this.remoteControlServerAvailable ? (bjccovmshb1hxg.instrumentBranch(107, 0, true), SettingsText.SETTINGS_OVERVIEW_SUBTITLE) : (bjccovmshb1hxg.instrumentBranch(107, 0, false), SettingsText.SETTINGS_OVERVIEW_CLIENT_SUBTITLE));
            Text.fontSize(13);
            Text.fontColor(SettingsTheme.mutedText(this.isDarkMode()));
            Text.lineHeight(20);
            Text.width('100%');
            Text.margin({ bottom: 18 });
        }, Text);
        Text.pop();
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1hxg.instrumentFunction(108);
            If.create();
            if (this.remoteControlServerAvailable) {
                bjccovmshb1hxg.instrumentBranch(108, 0, true);
                bjccovmshb1hxg.instrumentRegion(108, 1);
                this.ifElseBranchUpdateFunction(0, () => {
                    bjccovmshb1hxg.instrumentFunction(109);
                    this.observeComponentCreation2((elmtId, isInitialRender) => {
                        bjccovmshb1hxg.instrumentFunction(110);
                        Column.create();
                        Column.width('100%');
                        Column.padding(18);
                        Column.backgroundColor(SettingsTheme.cardBackground(this.isDarkMode()));
                        Column.borderRadius(SettingsTheme.CARD_RADIUS);
                        Column.border({
                            width: 1,
                            color: SettingsTheme.borderColor(this.isDarkMode())
                        });
                        Column.margin({ bottom: 18 });
                    }, Column);
                    this.observeComponentCreation2((elmtId, isInitialRender) => {
                        bjccovmshb1hxg.instrumentFunction(111);
                        Row.create();
                        Row.alignItems(VerticalAlign.Center);
                        Row.width('100%');
                        Row.margin({ bottom: 14 });
                    }, Row);
                    this.observeComponentCreation2((elmtId, isInitialRender) => {
                        bjccovmshb1hxg.instrumentFunction(112);
                        Text.create(SettingsText.SETTINGS_CURRENT_STATE);
                        Text.fontSize(15);
                        Text.fontWeight(FontWeight.Bold);
                        Text.fontColor(SettingsTheme.primaryText(this.isDarkMode()));
                        Text.layoutWeight(1);
                    }, Text);
                    Text.pop();
                    {
                        this.observeComponentCreation2((elmtId, isInitialRender) => {
                            bjccovmshb1hxg.instrumentFunction(113);
                            if (isInitialRender) {
                                let componentCall = new SettingsStatusChip(this, {
                                    text: this.xrdpStatusText(),
                                    tone: this.xrdpStatusTone(),
                                    isDark: this.isDarkMode()
                                }, undefined, elmtId, () => { }, { page: "common/src/main/ets/components/SettingsPage.ets", line: 281, col: 13 });
                                ViewPU.create(componentCall);
                                let paramsLambda = () => {
                                    return {
                                        text: this.xrdpStatusText(),
                                        tone: this.xrdpStatusTone(),
                                        isDark: this.isDarkMode()
                                    };
                                };
                                componentCall.paramsGenerator_ = paramsLambda;
                            }
                            else {
                                this.updateStateVarsOfChildByElmtId(elmtId, {
                                    text: this.xrdpStatusText(),
                                    tone: this.xrdpStatusTone(),
                                    isDark: this.isDarkMode()
                                });
                            }
                        }, { name: "SettingsStatusChip" });
                    }
                    Row.pop();
                    this.observeComponentCreation2((elmtId, isInitialRender) => {
                        bjccovmshb1hxg.instrumentFunction(114);
                        Row.create({ space: 24 });
                        Row.width('100%');
                    }, Row);
                    this.observeComponentCreation2((elmtId, isInitialRender) => {
                        bjccovmshb1hxg.instrumentFunction(115);
                        Column.create();
                        Column.layoutWeight(1);
                    }, Column);
                    {
                        this.observeComponentCreation2((elmtId, isInitialRender) => {
                            bjccovmshb1hxg.instrumentFunction(116);
                            if (isInitialRender) {
                                let componentCall = new SettingsKeyValueRow(this, {
                                    label: SettingsText.BASIC_APPEARANCE_SECTION,
                                    value: SettingsText.appearanceValue(this.appearanceMode),
                                    isDark: this.isDarkMode()
                                }, undefined, elmtId, () => { }, { page: "common/src/main/ets/components/SettingsPage.ets", line: 293, col: 15 });
                                ViewPU.create(componentCall);
                                let paramsLambda = () => {
                                    return {
                                        label: SettingsText.BASIC_APPEARANCE_SECTION,
                                        value: SettingsText.appearanceValue(this.appearanceMode),
                                        isDark: this.isDarkMode()
                                    };
                                };
                                componentCall.paramsGenerator_ = paramsLambda;
                            }
                            else {
                                this.updateStateVarsOfChildByElmtId(elmtId, {
                                    label: SettingsText.BASIC_APPEARANCE_SECTION,
                                    value: SettingsText.appearanceValue(this.appearanceMode),
                                    isDark: this.isDarkMode()
                                });
                            }
                        }, { name: "SettingsKeyValueRow" });
                    }
                    {
                        this.observeComponentCreation2((elmtId, isInitialRender) => {
                            bjccovmshb1hxg.instrumentFunction(117);
                            if (isInitialRender) {
                                let componentCall = new SettingsKeyValueRow(this, {
                                    label: SettingsText.REMOTE_SERVER_STATUS_LABEL,
                                    value: this.xrdpStatusText(),
                                    tone: this.xrdpStatusTone(),
                                    isDark: this.isDarkMode()
                                }, undefined, elmtId, () => { }, { page: "common/src/main/ets/components/SettingsPage.ets", line: 298, col: 15 });
                                ViewPU.create(componentCall);
                                let paramsLambda = () => {
                                    return {
                                        label: SettingsText.REMOTE_SERVER_STATUS_LABEL,
                                        value: this.xrdpStatusText(),
                                        tone: this.xrdpStatusTone(),
                                        isDark: this.isDarkMode()
                                    };
                                };
                                componentCall.paramsGenerator_ = paramsLambda;
                            }
                            else {
                                this.updateStateVarsOfChildByElmtId(elmtId, {
                                    label: SettingsText.REMOTE_SERVER_STATUS_LABEL,
                                    value: this.xrdpStatusText(),
                                    tone: this.xrdpStatusTone(),
                                    isDark: this.isDarkMode()
                                });
                            }
                        }, { name: "SettingsKeyValueRow" });
                    }
                    Column.pop();
                    this.observeComponentCreation2((elmtId, isInitialRender) => {
                        bjccovmshb1hxg.instrumentFunction(118);
                        Column.create();
                        Column.layoutWeight(1);
                    }, Column);
                    {
                        this.observeComponentCreation2((elmtId, isInitialRender) => {
                            bjccovmshb1hxg.instrumentFunction(119);
                            if (isInitialRender) {
                                let componentCall = new SettingsKeyValueRow(this, {
                                    label: SettingsText.REMOTE_SCREEN_SECTION,
                                    value: this.screenStatusText(),
                                    tone: this.screenStatusTone(),
                                    isDark: this.isDarkMode()
                                }, undefined, elmtId, () => { }, { page: "common/src/main/ets/components/SettingsPage.ets", line: 308, col: 15 });
                                ViewPU.create(componentCall);
                                let paramsLambda = () => {
                                    return {
                                        label: SettingsText.REMOTE_SCREEN_SECTION,
                                        value: this.screenStatusText(),
                                        tone: this.screenStatusTone(),
                                        isDark: this.isDarkMode()
                                    };
                                };
                                componentCall.paramsGenerator_ = paramsLambda;
                            }
                            else {
                                this.updateStateVarsOfChildByElmtId(elmtId, {
                                    label: SettingsText.REMOTE_SCREEN_SECTION,
                                    value: this.screenStatusText(),
                                    tone: this.screenStatusTone(),
                                    isDark: this.isDarkMode()
                                });
                            }
                        }, { name: "SettingsKeyValueRow" });
                    }
                    {
                        this.observeComponentCreation2((elmtId, isInitialRender) => {
                            bjccovmshb1hxg.instrumentFunction(120);
                            if (isInitialRender) {
                                let componentCall = new SettingsKeyValueRow(this, {
                                    label: SettingsText.REMOTE_ACCESS_SECTION,
                                    value: this.gateStatusText(),
                                    tone: this.gateStatusTone(),
                                    isDark: this.isDarkMode()
                                }, undefined, elmtId, () => { }, { page: "common/src/main/ets/components/SettingsPage.ets", line: 314, col: 15 });
                                ViewPU.create(componentCall);
                                let paramsLambda = () => {
                                    return {
                                        label: SettingsText.REMOTE_ACCESS_SECTION,
                                        value: this.gateStatusText(),
                                        tone: this.gateStatusTone(),
                                        isDark: this.isDarkMode()
                                    };
                                };
                                componentCall.paramsGenerator_ = paramsLambda;
                            }
                            else {
                                this.updateStateVarsOfChildByElmtId(elmtId, {
                                    label: SettingsText.REMOTE_ACCESS_SECTION,
                                    value: this.gateStatusText(),
                                    tone: this.gateStatusTone(),
                                    isDark: this.isDarkMode()
                                });
                            }
                        }, { name: "SettingsKeyValueRow" });
                    }
                    Column.pop();
                    Row.pop();
                    Column.pop();
                });
            }
            else {
                bjccovmshb1hxg.instrumentBranch(108, 0, false);
                this.ifElseBranchUpdateFunction(1, () => {
                });
            }
        }, If);
        If.pop();
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1hxg.instrumentFunction(121);
            Column.create();
            Column.alignItems(HorizontalAlign.Start);
            Column.width('100%');
        }, Column);
        this.overviewEntry.bind(this)(SettingsText.BASIC_SETTINGS_ENTRY_TITLE, SettingsText.BASIC_SETTINGS_ENTRY_DESC, SettingsResources.BASIC_ICON, SettingsAccent.CYAN, SettingsText.appearanceValue(this.appearanceMode), () => {
            bjccovmshb1hxg.instrumentFunction(122);
            this.pageName = SettingsRoute.BASIC;
            bjccovmshb1hxg.instrumentRegion(104, 1);
            this.remoteControlTargetSection = '';
        });
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1hxg.instrumentFunction(123);
            If.create();
            if (this.remoteControlServerAvailable) {
                bjccovmshb1hxg.instrumentBranch(123, 0, true);
                bjccovmshb1hxg.instrumentRegion(123, 1);
                this.ifElseBranchUpdateFunction(0, () => {
                    bjccovmshb1hxg.instrumentFunction(124);
                    this.overviewEntry.bind(this)(SettingsText.REMOTE_CONTROL_ENTRY_TITLE, SettingsText.REMOTE_CONTROL_ENTRY_DESC, SettingsResources.REMOTE_CONTROL_ICON, SettingsAccent.BLUE, this.xrdpStatusText(), () => {
                        bjccovmshb1hxg.instrumentFunction(125);
                        this.pageName = SettingsRoute.REMOTE_CONTROL;
                        bjccovmshb1hxg.instrumentRegion(104, 2);
                        this.remoteControlTargetSection = '';
                    });
                });
            }
            else {
                bjccovmshb1hxg.instrumentBranch(123, 0, false);
                bjccovmshb1hxg.instrumentRegion(123, 2);
                this.ifElseBranchUpdateFunction(1, () => {
                    bjccovmshb1hxg.instrumentFunction(126);
                    this.overviewEntry.bind(this)(SettingsText.REMOTE_FILES_FEATURE_TITLE, SettingsText.REMOTE_FILES_FEATURE_DESC, SettingsResources.REMOTE_FILES_ICON, SettingsAccent.GREEN, '', () => {
                        bjccovmshb1hxg.instrumentFunction(127);
                        bjccovmshb1hxg.instrumentRegion(127, 1);
                        this.onOpenRemoteFilesDirectory();
                    });
                });
            }
        }, If);
        If.pop();
        this.overviewEntry.bind(this)(SettingsText.PROJECT_HELP_ENTRY_TITLE, SettingsText.PROJECT_HELP_ENTRY_DESC, SettingsResources.PROJECT_HELP_ICON, SettingsAccent.PURPLE, '', () => {
            bjccovmshb1hxg.instrumentFunction(128);
            this.pageName = SettingsRoute.PROJECT_HELP;
            bjccovmshb1hxg.instrumentRegion(104, 3);
            this.remoteControlTargetSection = '';
        });
        Column.pop();
        Column.pop();
    }
    private buildSettingsContent(parent = null) {
        bjccovmshb1hxg.instrumentFunction(129);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1hxg.instrumentFunction(130);
            If.create();
            if (this.pageName === SettingsRoute.BASIC) {
                bjccovmshb1hxg.instrumentBranch(130, 0, true);
                bjccovmshb1hxg.instrumentRegion(130, 1);
                this.ifElseBranchUpdateFunction(0, () => {
                    bjccovmshb1hxg.instrumentFunction(131);
                    {
                        this.observeComponentCreation2((elmtId, isInitialRender) => {
                            bjccovmshb1hxg.instrumentFunction(132);
                            if (isInitialRender) {
                                let componentCall = new BasicSettingsPage(this, {
                                    onModeChange: (mode: SettingsAppearanceMode) => {
                                        bjccovmshb1hxg.instrumentFunction(133);
                                        bjccovmshb1hxg.instrumentRegion(133, 1);
                                        this.appearanceMode = mode;
                                    },
                                    onBack: () => {
                                        bjccovmshb1hxg.instrumentFunction(134);
                                        bjccovmshb1hxg.instrumentRegion(134, 1);
                                        this.pageName = SettingsRoute.SETTINGS;
                                    }
                                }, undefined, elmtId, () => { }, { page: "common/src/main/ets/components/SettingsPage.ets", line: 373, col: 7 });
                                ViewPU.create(componentCall);
                                let paramsLambda = () => {
                                    return {
                                        onModeChange: (mode: SettingsAppearanceMode) => {
                                            this.appearanceMode = mode;
                                        },
                                        onBack: () => {
                                            this.pageName = SettingsRoute.SETTINGS;
                                        }
                                    };
                                };
                                componentCall.paramsGenerator_ = paramsLambda;
                            }
                            else {
                                this.updateStateVarsOfChildByElmtId(elmtId, {});
                            }
                        }, { name: "BasicSettingsPage" });
                    }
                });
            }
            else {
                bjccovmshb1hxg.instrumentBranch(130, 0, false);
                bjccovmshb1hxg.instrumentRegion(130, 2);
                if (this.pageName === SettingsRoute.REMOTE_CONTROL && this.remoteControlServerAvailable) {
                    bjccovmshb1hxg.instrumentBranch(130, 1, true);
                    bjccovmshb1hxg.instrumentRegion(130, 3);
                    this.ifElseBranchUpdateFunction(1, () => {
                        bjccovmshb1hxg.instrumentFunction(135);
                        {
                            this.observeComponentCreation2((elmtId, isInitialRender) => {
                                bjccovmshb1hxg.instrumentFunction(136);
                                if (isInitialRender) {
                                    let componentCall = new RemoteControlSettingsPage(this, {
                                        isDark: this.isDarkMode(),
                                        targetSection: this.remoteControlTargetSection,
                                        remoteAccessCode: this.remoteAccessCode,
                                        remoteAccessCodeGateEnabled: this.remoteAccessCodeGateEnabled,
                                        screenRecordingPermissionGranted: this.screenRecordingPermissionGranted,
                                        inputInjectionPermissionGranted: this.inputInjectionPermissionGranted,
                                        xrdpServerRunning: this.xrdpServerRunning,
                                        xrdpServerState: this.xrdpServerState,
                                        xrdpServerPort: this.xrdpServerPort,
                                        xrdpServerMessage: this.xrdpServerMessage,
                                        xrdpServerBusy: this.xrdpServerBusy,
                                        screenRecordingPermissionBusy: this.screenRecordingPermissionBusy,
                                        inputInjectionPermissionBusy: this.inputInjectionPermissionBusy,
                                        onRemoteAccessCodeGateChange: this.onRemoteAccessCodeGateChange,
                                        onRemoteAccessCodeRegenerate: this.onRemoteAccessCodeRegenerate,
                                        onRequestScreenRecordingPermission: this.onRequestScreenRecordingPermission,
                                        onRefreshScreenRecordingPermission: this.onRefreshScreenRecordingPermission,
                                        onRequestInputInjectionPermission: this.onRequestInputInjectionPermission,
                                        onRefreshInputInjectionPermission: this.onRefreshInputInjectionPermission,
                                        onRefreshXrdpServerStatus: this.onRefreshXrdpServerStatus,
                                        onStartXrdpServer: this.onStartXrdpServer,
                                        onOpenRemoteFilesDirectory: this.onOpenRemoteFilesDirectory,
                                        onBack: () => {
                                            bjccovmshb1hxg.instrumentFunction(137);
                                            this.pageName = SettingsRoute.SETTINGS;
                                            bjccovmshb1hxg.instrumentRegion(129, 1);
                                            this.remoteControlTargetSection = '';
                                        }
                                    }, undefined, elmtId, () => { }, { page: "common/src/main/ets/components/SettingsPage.ets", line: 382, col: 7 });
                                    ViewPU.create(componentCall);
                                    let paramsLambda = () => {
                                        return {
                                            isDark: this.isDarkMode(),
                                            targetSection: this.remoteControlTargetSection,
                                            remoteAccessCode: this.remoteAccessCode,
                                            remoteAccessCodeGateEnabled: this.remoteAccessCodeGateEnabled,
                                            screenRecordingPermissionGranted: this.screenRecordingPermissionGranted,
                                            inputInjectionPermissionGranted: this.inputInjectionPermissionGranted,
                                            xrdpServerRunning: this.xrdpServerRunning,
                                            xrdpServerState: this.xrdpServerState,
                                            xrdpServerPort: this.xrdpServerPort,
                                            xrdpServerMessage: this.xrdpServerMessage,
                                            xrdpServerBusy: this.xrdpServerBusy,
                                            screenRecordingPermissionBusy: this.screenRecordingPermissionBusy,
                                            inputInjectionPermissionBusy: this.inputInjectionPermissionBusy,
                                            onRemoteAccessCodeGateChange: this.onRemoteAccessCodeGateChange,
                                            onRemoteAccessCodeRegenerate: this.onRemoteAccessCodeRegenerate,
                                            onRequestScreenRecordingPermission: this.onRequestScreenRecordingPermission,
                                            onRefreshScreenRecordingPermission: this.onRefreshScreenRecordingPermission,
                                            onRequestInputInjectionPermission: this.onRequestInputInjectionPermission,
                                            onRefreshInputInjectionPermission: this.onRefreshInputInjectionPermission,
                                            onRefreshXrdpServerStatus: this.onRefreshXrdpServerStatus,
                                            onStartXrdpServer: this.onStartXrdpServer,
                                            onOpenRemoteFilesDirectory: this.onOpenRemoteFilesDirectory,
                                            onBack: () => {
                                                this.pageName = SettingsRoute.SETTINGS;
                                                bjccovmshb1hxg.instrumentRegion(129, 2);
                                                this.remoteControlTargetSection = '';
                                            }
                                        };
                                    };
                                    componentCall.paramsGenerator_ = paramsLambda;
                                }
                                else {
                                    this.updateStateVarsOfChildByElmtId(elmtId, {
                                        isDark: this.isDarkMode(),
                                        targetSection: this.remoteControlTargetSection,
                                        remoteAccessCode: this.remoteAccessCode,
                                        remoteAccessCodeGateEnabled: this.remoteAccessCodeGateEnabled,
                                        screenRecordingPermissionGranted: this.screenRecordingPermissionGranted,
                                        inputInjectionPermissionGranted: this.inputInjectionPermissionGranted,
                                        xrdpServerRunning: this.xrdpServerRunning,
                                        xrdpServerState: this.xrdpServerState,
                                        xrdpServerPort: this.xrdpServerPort,
                                        xrdpServerMessage: this.xrdpServerMessage,
                                        xrdpServerBusy: this.xrdpServerBusy,
                                        screenRecordingPermissionBusy: this.screenRecordingPermissionBusy,
                                        inputInjectionPermissionBusy: this.inputInjectionPermissionBusy
                                    });
                                }
                            }, { name: "RemoteControlSettingsPage" });
                        }
                    });
                }
                else {
                    bjccovmshb1hxg.instrumentBranch(130, 1, false);
                    bjccovmshb1hxg.instrumentRegion(130, 4);
                    if (this.pageName === SettingsRoute.PROJECT_HELP) {
                        bjccovmshb1hxg.instrumentBranch(130, 2, true);
                        bjccovmshb1hxg.instrumentRegion(130, 5);
                        this.ifElseBranchUpdateFunction(2, () => {
                            bjccovmshb1hxg.instrumentFunction(138);
                            {
                                this.observeComponentCreation2((elmtId, isInitialRender) => {
                                    bjccovmshb1hxg.instrumentFunction(139);
                                    if (isInitialRender) {
                                        let componentCall = new ProjectHelpPage(this, {
                                            isDark: this.isDarkMode(),
                                            onBack: () => {
                                                bjccovmshb1hxg.instrumentFunction(140);
                                                bjccovmshb1hxg.instrumentRegion(140, 1);
                                                this.pageName = SettingsRoute.SETTINGS;
                                            }
                                        }, undefined, elmtId, () => { }, { page: "common/src/main/ets/components/SettingsPage.ets", line: 411, col: 7 });
                                        ViewPU.create(componentCall);
                                        let paramsLambda = () => {
                                            return {
                                                isDark: this.isDarkMode(),
                                                onBack: () => {
                                                    this.pageName = SettingsRoute.SETTINGS;
                                                }
                                            };
                                        };
                                        componentCall.paramsGenerator_ = paramsLambda;
                                    }
                                    else {
                                        this.updateStateVarsOfChildByElmtId(elmtId, {
                                            isDark: this.isDarkMode()
                                        });
                                    }
                                }, { name: "ProjectHelpPage" });
                            }
                        });
                    }
                    else {
                        bjccovmshb1hxg.instrumentBranch(130, 2, false);
                        bjccovmshb1hxg.instrumentRegion(130, 6);
                        this.ifElseBranchUpdateFunction(3, () => {
                            bjccovmshb1hxg.instrumentFunction(141);
                            this.observeComponentCreation2((elmtId, isInitialRender) => {
                                bjccovmshb1hxg.instrumentFunction(142);
                                Scroll.create();
                                Scroll.width('100%');
                                Scroll.height('100%');
                            }, Scroll);
                            this.buildOverviewPage.bind(this)();
                            Scroll.pop();
                        });
                    }
                }
            }
        }, If);
        If.pop();
    }
    private buildContentPane(parent = null) {
        bjccovmshb1hxg.instrumentFunction(143);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1hxg.instrumentFunction(144);
            Column.create();
            Column.layoutWeight(1);
            Column.height('100%');
            Column.alignItems(HorizontalAlign.Start);
            Column.justifyContent(FlexAlign.Start);
            Column.backgroundColor(SettingsTheme.pageBackground(this.isDarkMode()));
            Column.borderRadius(16);
        }, Column);
        this.buildSettingsContent.bind(this)();
        Column.pop();
    }
    private buildCompactLayout(parent = null) {
        bjccovmshb1hxg.instrumentFunction(145);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1hxg.instrumentFunction(146);
            Column.create();
            Column.width('100%');
            Column.height('100%');
            Column.backgroundColor(SettingsTheme.pageBackground(this.isDarkMode()));
        }, Column);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1hxg.instrumentFunction(147);
            If.create();
            if (this.pageName === SettingsRoute.SETTINGS) {
                bjccovmshb1hxg.instrumentBranch(147, 0, true);
                bjccovmshb1hxg.instrumentRegion(147, 1);
                this.ifElseBranchUpdateFunction(0, () => {
                    bjccovmshb1hxg.instrumentFunction(148);
                    this.buildCompactTopBar.bind(this)();
                });
            }
            else {
                bjccovmshb1hxg.instrumentBranch(147, 0, false);
                this.ifElseBranchUpdateFunction(1, () => {
                });
            }
        }, If);
        If.pop();
        this.buildContentPane.bind(this)();
        Column.pop();
    }
    private buildExpandedLayout(parent = null) {
        bjccovmshb1hxg.instrumentFunction(149);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1hxg.instrumentFunction(150);
            Row.create({ space: 16 });
            Row.width('100%');
            Row.height('100%');
            Row.padding(16);
            Row.backgroundColor(SettingsTheme.pageBackground(this.isDarkMode()));
        }, Row);
        this.buildDesktopNav.bind(this)();
        this.buildContentPane.bind(this)();
        Row.pop();
    }
    initialRender() {
        bjccovmshb1hxg.instrumentFunction(151);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1hxg.instrumentFunction(152);
            Stack.create();
            Stack.width('100%');
            Stack.height('100%');
        }, Stack);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1hxg.instrumentFunction(153);
            If.create();
            if (this.layoutMode === LayoutMode.COMPACT) {
                bjccovmshb1hxg.instrumentBranch(153, 0, true);
                bjccovmshb1hxg.instrumentRegion(153, 1);
                this.ifElseBranchUpdateFunction(0, () => {
                    bjccovmshb1hxg.instrumentFunction(154);
                    this.buildCompactLayout.bind(this)();
                });
            }
            else {
                bjccovmshb1hxg.instrumentBranch(153, 0, false);
                bjccovmshb1hxg.instrumentRegion(153, 2);
                this.ifElseBranchUpdateFunction(1, () => {
                    bjccovmshb1hxg.instrumentFunction(155);
                    this.buildExpandedLayout.bind(this)();
                });
            }
        }, If);
        If.pop();
        Stack.pop();
    }
    rerender() {
        this.updateDirtyElements();
    }
}
