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
let bjccovmshb1i7x = new BjcCov({ version: "bjc v1.0.0", versionCode: 10000, path: "common/src/main/ets/components/home/HomeStatusFooter.ets", hash: "ece6df6acf86bb7bf31c0e90c5d2aeaef964850fc8337cb55b041289278a6211", lineCnt: 242, count: 0, projectPath: "", functions: { 0: { name: "anonymous_1", count: 0, regions: { 0: { startLoc: { line: 49, col: 19 }, endLoc: { line: 49, col: 57 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 0 }, 1: { name: "anonymous_2", count: 0, regions: { 0: { startLoc: { line: 49, col: 60 }, endLoc: { line: 50, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 1 }, 2: { name: "setInitiallyProvidedValue", count: 0, regions: { 0: { startLoc: { line: 39, col: 34 }, endLoc: { line: 48, col: 53 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 2 }, 3: { name: "updateStateVars", count: 0, regions: { 0: { startLoc: { line: 39, col: 9 }, endLoc: { line: 48, col: 37 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 3 }, 4: { name: "layoutMode", count: 0, regions: { 0: { startLoc: { line: 39, col: 9 }, endLoc: { line: 39, col: 19 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 4 }, 5: { name: "layoutMode", count: 0, regions: { 0: { startLoc: { line: 39, col: 9 }, endLoc: { line: 39, col: 31 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 5 }, 6: { name: "isDark", count: 0, regions: { 0: { startLoc: { line: 40, col: 9 }, endLoc: { line: 40, col: 15 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 6 }, 7: { name: "isDark", count: 0, regions: { 0: { startLoc: { line: 40, col: 9 }, endLoc: { line: 40, col: 24 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 7 }, 8: { name: "xrdpServerBusy", count: 0, regions: { 0: { startLoc: { line: 41, col: 9 }, endLoc: { line: 41, col: 23 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 8 }, 9: { name: "xrdpServerBusy", count: 0, regions: { 0: { startLoc: { line: 41, col: 9 }, endLoc: { line: 41, col: 32 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 9 }, 10: { name: "xrdpServerState", count: 0, regions: { 0: { startLoc: { line: 42, col: 9 }, endLoc: { line: 42, col: 24 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 10 }, 11: { name: "xrdpServerState", count: 0, regions: { 0: { startLoc: { line: 42, col: 9 }, endLoc: { line: 42, col: 32 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 11 }, 12: { name: "xrdpServerRunning", count: 0, regions: { 0: { startLoc: { line: 43, col: 9 }, endLoc: { line: 43, col: 26 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 12 }, 13: { name: "xrdpServerRunning", count: 0, regions: { 0: { startLoc: { line: 43, col: 9 }, endLoc: { line: 43, col: 35 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 13 }, 14: { name: "screenRecordingPermissionGranted", count: 0, regions: { 0: { startLoc: { line: 44, col: 9 }, endLoc: { line: 44, col: 41 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 14 }, 15: { name: "screenRecordingPermissionGranted", count: 0, regions: { 0: { startLoc: { line: 44, col: 9 }, endLoc: { line: 44, col: 50 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 15 }, 16: { name: "screenRecordingPermissionBusy", count: 0, regions: { 0: { startLoc: { line: 45, col: 9 }, endLoc: { line: 45, col: 38 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 16 }, 17: { name: "screenRecordingPermissionBusy", count: 0, regions: { 0: { startLoc: { line: 45, col: 9 }, endLoc: { line: 45, col: 47 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 17 }, 18: { name: "inputInjectionPermissionGranted", count: 0, regions: { 0: { startLoc: { line: 46, col: 9 }, endLoc: { line: 46, col: 40 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 18 }, 19: { name: "inputInjectionPermissionGranted", count: 0, regions: { 0: { startLoc: { line: 46, col: 9 }, endLoc: { line: 46, col: 49 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 19 }, 20: { name: "inputInjectionPermissionBusy", count: 0, regions: { 0: { startLoc: { line: 47, col: 9 }, endLoc: { line: 47, col: 37 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 20 }, 21: { name: "inputInjectionPermissionBusy", count: 0, regions: { 0: { startLoc: { line: 47, col: 9 }, endLoc: { line: 47, col: 46 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 21 }, 22: { name: "remoteControlServerAvailable", count: 0, regions: { 0: { startLoc: { line: 48, col: 9 }, endLoc: { line: 48, col: 37 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 22 }, 23: { name: "remoteControlServerAvailable", count: 0, regions: { 0: { startLoc: { line: 48, col: 9 }, endLoc: { line: 48, col: 46 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 23 }, 24: { name: "anonymous_3", count: 0, regions: { 0: { startLoc: { line: 49, col: 19 }, endLoc: { line: 49, col: 57 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 24 }, 25: { name: "hoveredSection", count: 0, regions: { 0: { startLoc: { line: 51, col: 18 }, endLoc: { line: 51, col: 32 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 25 }, 26: { name: "hoveredSection", count: 0, regions: { 0: { startLoc: { line: 51, col: 18 }, endLoc: { line: 51, col: 40 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 26 }, 27: { name: "pressedSection", count: 0, regions: { 0: { startLoc: { line: 52, col: 18 }, endLoc: { line: 52, col: 32 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 27 }, 28: { name: "pressedSection", count: 0, regions: { 0: { startLoc: { line: 52, col: 18 }, endLoc: { line: 52, col: 40 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 28 }, 29: { name: "serviceStatusTone", count: 0, regions: { 0: { startLoc: { line: 54, col: 3 }, endLoc: { line: 56, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 55, col: 5 }, endLoc: { line: 56, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 29 }, 30: { name: "serviceStatusText", count: 0, regions: { 0: { startLoc: { line: 58, col: 3 }, endLoc: { line: 63, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 59, col: 30 }, endLoc: { line: 61, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 62, col: 5 }, endLoc: { line: 63, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 59, col: 9 }, endLoc: { line: 59, col: 28 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 30 }, 31: { name: "screenStatusTone", count: 0, regions: { 0: { startLoc: { line: 65, col: 3 }, endLoc: { line: 70, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 66, col: 45 }, endLoc: { line: 68, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 69, col: 5 }, endLoc: { line: 70, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 66, col: 9 }, endLoc: { line: 66, col: 43 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 69, col: 12 }, endLoc: { line: 69, col: 68 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 31 }, 32: { name: "screenStatusText", count: 0, regions: { 0: { startLoc: { line: 72, col: 3 }, endLoc: { line: 78, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 73, col: 45 }, endLoc: { line: 75, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 76, col: 5 }, endLoc: { line: 78, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 73, col: 9 }, endLoc: { line: 73, col: 43 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 76, col: 12 }, endLoc: { line: 77, col: 43 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 32 }, 33: { name: "inputStatusTone", count: 0, regions: { 0: { startLoc: { line: 80, col: 3 }, endLoc: { line: 85, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 81, col: 44 }, endLoc: { line: 83, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 84, col: 5 }, endLoc: { line: 85, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 81, col: 9 }, endLoc: { line: 81, col: 42 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 84, col: 12 }, endLoc: { line: 84, col: 67 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 33 }, 34: { name: "inputStatusText", count: 0, regions: { 0: { startLoc: { line: 87, col: 3 }, endLoc: { line: 93, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 88, col: 44 }, endLoc: { line: 90, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 91, col: 5 }, endLoc: { line: 93, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 88, col: 9 }, endLoc: { line: 88, col: 42 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 91, col: 12 }, endLoc: { line: 92, col: 45 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 34 }, 35: { name: "cardValue", count: 0, regions: { 0: { startLoc: { line: 95, col: 3 }, endLoc: { line: 102, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 96, col: 5 }, endLoc: { line: 102, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 35 }, 36: { name: "cardTone", count: 0, regions: { 0: { startLoc: { line: 104, col: 3 }, endLoc: { line: 111, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 105, col: 5 }, endLoc: { line: 111, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 36 }, 37: { name: "visibleStatusCards", count: 0, regions: { 0: { startLoc: { line: 113, col: 3 }, endLoc: { line: 118, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 114, col: 44 }, endLoc: { line: 116, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 117, col: 5 }, endLoc: { line: 118, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 114, col: 9 }, endLoc: { line: 114, col: 42 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 37 }, 38: { name: "setHoveredSection", count: 0, regions: { 0: { startLoc: { line: 120, col: 3 }, endLoc: { line: 124, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 121, col: 5 }, endLoc: { line: 124, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 38 }, 39: { name: "anonymous_4", count: 0, regions: { 0: { startLoc: { line: 121, col: 48 }, endLoc: { line: 123, col: 6 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 122, col: 7 }, endLoc: { line: 123, col: 6 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 122, col: 29 }, endLoc: { line: 122, col: 51 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 39 }, 40: { name: "buildStatusCard", count: 0, regions: { 0: { startLoc: { line: 126, col: 3 }, endLoc: { line: 194, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 184, col: 9 }, endLoc: { line: 185, col: 8 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 192, col: 7 }, endLoc: { line: 193, col: 6 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 40 }, 41: { name: "anonymous_5", count: 0, regions: { 0: { startLoc: { line: 128, col: 5 }, endLoc: { line: 193, col: 6 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 160, col: 12 }, endLoc: { line: 160, col: 38 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 161, col: 19 }, endLoc: { line: 161, col: 35 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 2: { startLoc: { line: 164, col: 22 }, endLoc: { line: 167, col: 45 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 3: { startLoc: { line: 171, col: 14 }, endLoc: { line: 172, col: 119 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 4: { startLoc: { line: 175, col: 10 }, endLoc: { line: 175, col: 59 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 5: { startLoc: { line: 176, col: 10 }, endLoc: { line: 176, col: 59 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 41 }, 42: { name: "anonymous_6", count: 0, regions: { 0: { startLoc: { line: 178, col: 14 }, endLoc: { line: 180, col: 6 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 179, col: 7 }, endLoc: { line: 180, col: 6 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 42 }, 43: { name: "anonymous_7", count: 0, regions: { 0: { startLoc: { line: 181, col: 14 }, endLoc: { line: 189, col: 6 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 182, col: 42 }, endLoc: { line: 185, col: 8 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 186, col: 75 }, endLoc: { line: 188, col: 8 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 182, col: 11 }, endLoc: { line: 182, col: 40 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 186, col: 11 }, endLoc: { line: 186, col: 73 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 43 }, 44: { name: "anonymous_8", count: 0, regions: { 0: { startLoc: { line: 190, col: 14 }, endLoc: { line: 193, col: 6 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 44 }, 45: { name: "anonymous_9", count: 0, regions: { 0: { startLoc: { line: 129, col: 7 }, endLoc: { line: 141, col: 98 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 45 }, 46: { name: "anonymous_10", count: 0, regions: { 0: { startLoc: { line: 130, col: 9 }, endLoc: { line: 134, col: 27 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 46 }, 47: { name: "anonymous_11", count: 0, regions: { 0: { startLoc: { line: 142, col: 7 }, endLoc: { line: 146, col: 20 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 47 }, 48: { name: "anonymous_12", count: 0, regions: { 0: { startLoc: { line: 147, col: 7 }, endLoc: { line: 152, col: 24 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 48 }, 49: { name: "anonymous_13", count: 0, regions: { 0: { startLoc: { line: 153, col: 7 }, endLoc: { line: 157, col: 94 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 49 }, 50: { name: "buildExpandedFooter", count: 0, regions: { 0: { startLoc: { line: 196, col: 3 }, endLoc: { line: 211, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 50 }, 51: { name: "anonymous_14", count: 0, regions: { 0: { startLoc: { line: 198, col: 5 }, endLoc: { line: 210, col: 6 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 51 }, 52: { name: "anonymous_15", count: 0, regions: { 0: { startLoc: { line: 199, col: 7 }, endLoc: { line: 201, col: 66 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 52 }, 53: { name: "anonymous_16", count: 0, regions: { 0: { startLoc: { line: 200, col: 9 }, endLoc: { line: 200, col: 43 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 53 }, 54: { name: "anonymous_17", count: 0, regions: { 0: { startLoc: { line: 201, col: 10 }, endLoc: { line: 201, col: 66 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 54 }, 55: { name: "buildCompactFooter", count: 0, regions: { 0: { startLoc: { line: 213, col: 3 }, endLoc: { line: 229, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 55 }, 56: { name: "anonymous_18", count: 0, regions: { 0: { startLoc: { line: 215, col: 5 }, endLoc: { line: 228, col: 6 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 56 }, 57: { name: "anonymous_19", count: 0, regions: { 0: { startLoc: { line: 216, col: 7 }, endLoc: { line: 220, col: 66 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 57 }, 58: { name: "anonymous_20", count: 0, regions: { 0: { startLoc: { line: 217, col: 9 }, endLoc: { line: 217, col: 16 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 58 }, 59: { name: "anonymous_21", count: 0, regions: { 0: { startLoc: { line: 217, col: 9 }, endLoc: { line: 217, col: 28 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 59 }, 60: { name: "anonymous_22", count: 0, regions: { 0: { startLoc: { line: 220, col: 10 }, endLoc: { line: 220, col: 66 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 60 }, 61: { name: "initialRender", count: 0, regions: { 0: { startLoc: { line: 231, col: 3 }, endLoc: { line: 240, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 61 }, 62: { name: "anonymous_23", count: 0, regions: { 0: { startLoc: { line: 232, col: 5 }, endLoc: { line: 239, col: 18 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 62 }, 63: { name: "anonymous_24", count: 0, regions: { 0: { startLoc: { line: 233, col: 7 }, endLoc: { line: 237, col: 8 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 233, col: 51 }, endLoc: { line: 235, col: 8 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 235, col: 14 }, endLoc: { line: 237, col: 8 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 233, col: 11 }, endLoc: { line: 233, col: 49 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 63 }, 64: { name: "anonymous_25", count: 0, regions: { 0: { startLoc: { line: 234, col: 9 }, endLoc: { line: 234, col: 34 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 64 }, 65: { name: "anonymous_26", count: 0, regions: { 0: { startLoc: { line: 236, col: 9 }, endLoc: { line: 236, col: 35 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 65 } }, exeLine: { 0: 1, 1: 2, 2: 3, 3: 4, 4: 5, 5: 6, 6: 8, 7: 9, 8: 10, 9: 11, 10: 14, 11: 15, 12: 16, 13: 17, 14: 18, 15: 20, 16: 21, 17: 22, 18: 23, 19: 25, 20: 26, 21: 27, 22: 28, 23: 30, 24: 31, 25: 32, 26: 33, 27: 38, 28: 39, 29: 40, 30: 41, 31: 42, 32: 43, 33: 44, 34: 45, 35: 46, 36: 47, 37: 48, 38: 49, 39: 51, 40: 52, 41: 54, 42: 55, 43: 58, 44: 59, 45: 60, 46: 62, 47: 65, 48: 66, 49: 67, 50: 69, 51: 72, 52: 73, 53: 74, 54: 76, 55: 77, 56: 80, 57: 81, 58: 82, 59: 84, 60: 87, 61: 88, 62: 89, 63: 91, 64: 92, 65: 95, 66: 96, 67: 97, 68: 98, 69: 99, 70: 100, 71: 101, 72: 104, 73: 105, 74: 106, 75: 107, 76: 108, 77: 109, 78: 110, 79: 113, 80: 114, 81: 115, 82: 117, 83: 120, 84: 121, 85: 122, 86: 127, 87: 128, 88: 129, 89: 130, 90: 131, 91: 132, 92: 133, 93: 134, 94: 136, 95: 137, 96: 138, 97: 139, 98: 140, 99: 141, 100: 142, 101: 143, 102: 144, 103: 145, 104: 146, 105: 147, 106: 148, 107: 149, 108: 150, 109: 151, 110: 152, 111: 153, 112: 154, 113: 155, 114: 156, 115: 157, 116: 159, 117: 160, 118: 161, 119: 162, 120: 163, 121: 164, 122: 165, 123: 166, 124: 167, 125: 168, 126: 169, 127: 170, 128: 171, 129: 172, 130: 174, 131: 175, 132: 176, 133: 178, 134: 179, 135: 181, 136: 182, 137: 183, 138: 184, 139: 186, 140: 187, 141: 190, 142: 191, 143: 192, 144: 197, 145: 198, 146: 199, 147: 200, 148: 201, 149: 203, 150: 204, 151: 205, 152: 206, 153: 207, 154: 208, 155: 209, 156: 214, 157: 215, 158: 216, 159: 217, 160: 218, 161: 220, 162: 222, 163: 223, 164: 224, 165: 225, 166: 226, 167: 227, 168: 231, 169: 232, 170: 233, 171: 234, 172: 235, 173: 236, 174: 239 } });
if (!("finalizeConstruction" in ViewPU.prototype)) {
    Reflect.set(ViewPU.prototype, "finalizeConstruction", () => { });
}
interface HomeStatusFooter_Params {
    layoutMode?: LayoutMode;
    isDark?: boolean;
    xrdpServerBusy?: boolean;
    xrdpServerState?: string;
    xrdpServerRunning?: boolean;
    screenRecordingPermissionGranted?: boolean;
    screenRecordingPermissionBusy?: boolean;
    inputInjectionPermissionGranted?: boolean;
    inputInjectionPermissionBusy?: boolean;
    remoteControlServerAvailable?: boolean;
    onOpenSettings?: (remoteControlSection: string) => void;
    hoveredSection?: string;
    pressedSection?: string;
}
import { LayoutMode } from "@normalized:N&&&common/src/main/ets/adaptive/WindowLayoutPolicy&";
import { SettingsRemoteControlSection, SettingsText } from "@normalized:N&&&common/src/main/ets/components/settings/SettingsConstants&";
import { SettingsTheme } from "@normalized:N&&&common/src/main/ets/components/settings/SettingsUi&";
import type { SettingsStatusTone } from "@normalized:N&&&common/src/main/ets/components/settings/SettingsUi&";
import { HomeResources } from "@normalized:N&&&common/src/main/ets/components/home/HomeResources&";
import { HomeText } from "@normalized:N&&&common/src/main/ets/components/home/HomeText&";
import { HomeTheme } from "@normalized:N&&&common/src/main/ets/components/home/HomeTheme&";
interface HomeStatusCardConfig {
    section: string;
    label: string;
    icon: Resource;
}
const HOME_STATUS_CARDS: HomeStatusCardConfig[] = [
    {
        section: SettingsRemoteControlSection.SERVER,
        label: HomeText.STATUS_SERVICE_LABEL,
        icon: HomeResources.SHIELD_ICON
    },
    {
        section: SettingsRemoteControlSection.SCREEN,
        label: HomeText.STATUS_SCREEN_LABEL,
        icon: HomeResources.MONITOR_ICON
    },
    {
        section: SettingsRemoteControlSection.INPUT,
        label: HomeText.STATUS_INPUT_LABEL,
        icon: HomeResources.SHIELD_ICON
    },
    {
        section: SettingsRemoteControlSection.FILES,
        label: HomeText.STATUS_FILES_LABEL,
        icon: HomeResources.LOCK_ICON
    }
];
export class HomeStatusFooter extends ViewPU {
    constructor(parent, params, __localStorage, elmtId = -1, paramsLambda = undefined, extraInfo) {
        super(parent, __localStorage, elmtId, extraInfo);
        if (typeof paramsLambda === "function") {
            this.paramsGenerator_ = paramsLambda;
        }
        this.__layoutMode = new SynchedPropertySimpleOneWayPU(params.layoutMode, this, "layoutMode");
        this.__isDark = new SynchedPropertySimpleOneWayPU(params.isDark, this, "isDark");
        this.__xrdpServerBusy = new SynchedPropertySimpleOneWayPU(params.xrdpServerBusy, this, "xrdpServerBusy");
        this.__xrdpServerState = new SynchedPropertySimpleOneWayPU(params.xrdpServerState, this, "xrdpServerState");
        this.__xrdpServerRunning = new SynchedPropertySimpleOneWayPU(params.xrdpServerRunning, this, "xrdpServerRunning");
        this.__screenRecordingPermissionGranted = new SynchedPropertySimpleOneWayPU(params.screenRecordingPermissionGranted, this, "screenRecordingPermissionGranted");
        this.__screenRecordingPermissionBusy = new SynchedPropertySimpleOneWayPU(params.screenRecordingPermissionBusy, this, "screenRecordingPermissionBusy");
        this.__inputInjectionPermissionGranted = new SynchedPropertySimpleOneWayPU(params.inputInjectionPermissionGranted, this, "inputInjectionPermissionGranted");
        this.__inputInjectionPermissionBusy = new SynchedPropertySimpleOneWayPU(params.inputInjectionPermissionBusy, this, "inputInjectionPermissionBusy");
        this.__remoteControlServerAvailable = new SynchedPropertySimpleOneWayPU(params.remoteControlServerAvailable, this, "remoteControlServerAvailable");
        this.onOpenSettings = (_remoteControlSection: string) => {
            bjccovmshb1i7x.instrumentFunction(1);
        };
        this.__hoveredSection = new ObservedPropertySimplePU('', this, "hoveredSection");
        this.__pressedSection = new ObservedPropertySimplePU('', this, "pressedSection");
        this.setInitiallyProvidedValue(params);
        this.finalizeConstruction();
    }
    setInitiallyProvidedValue(params: HomeStatusFooter_Params) {
        bjccovmshb1i7x.instrumentFunction(2);
        if (params.layoutMode === undefined) {
            this.__layoutMode.set(LayoutMode.COMPACT);
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
        if (params.onOpenSettings !== undefined) {
            this.onOpenSettings = params.onOpenSettings;
        }
        else {
        }
        if (params.hoveredSection !== undefined) {
            this.hoveredSection = params.hoveredSection;
        }
        else {
        }
        if (params.pressedSection !== undefined) {
            this.pressedSection = params.pressedSection;
        }
        else {
        }
    }
    updateStateVars(params: HomeStatusFooter_Params) {
        bjccovmshb1i7x.instrumentFunction(3);
        this.__layoutMode.reset(params.layoutMode);
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
        this.__isDark.purgeDependencyOnElmtId(rmElmtId);
        this.__xrdpServerBusy.purgeDependencyOnElmtId(rmElmtId);
        this.__xrdpServerState.purgeDependencyOnElmtId(rmElmtId);
        this.__xrdpServerRunning.purgeDependencyOnElmtId(rmElmtId);
        this.__screenRecordingPermissionGranted.purgeDependencyOnElmtId(rmElmtId);
        this.__screenRecordingPermissionBusy.purgeDependencyOnElmtId(rmElmtId);
        this.__inputInjectionPermissionGranted.purgeDependencyOnElmtId(rmElmtId);
        this.__inputInjectionPermissionBusy.purgeDependencyOnElmtId(rmElmtId);
        this.__remoteControlServerAvailable.purgeDependencyOnElmtId(rmElmtId);
        this.__hoveredSection.purgeDependencyOnElmtId(rmElmtId);
        this.__pressedSection.purgeDependencyOnElmtId(rmElmtId);
    }
    aboutToBeDeleted() {
        this.__layoutMode.aboutToBeDeleted();
        this.__isDark.aboutToBeDeleted();
        this.__xrdpServerBusy.aboutToBeDeleted();
        this.__xrdpServerState.aboutToBeDeleted();
        this.__xrdpServerRunning.aboutToBeDeleted();
        this.__screenRecordingPermissionGranted.aboutToBeDeleted();
        this.__screenRecordingPermissionBusy.aboutToBeDeleted();
        this.__inputInjectionPermissionGranted.aboutToBeDeleted();
        this.__inputInjectionPermissionBusy.aboutToBeDeleted();
        this.__remoteControlServerAvailable.aboutToBeDeleted();
        this.__hoveredSection.aboutToBeDeleted();
        this.__pressedSection.aboutToBeDeleted();
        SubscriberManager.Get().delete(this.id__());
        this.aboutToBeDeletedInternal();
    }
    private __layoutMode: SynchedPropertySimpleOneWayPU<LayoutMode>;
    get layoutMode() {
        bjccovmshb1i7x.instrumentFunction(4);
        return this.__layoutMode.get();
    }
    set layoutMode(newValue: LayoutMode) {
        bjccovmshb1i7x.instrumentFunction(5);
        this.__layoutMode.set(newValue);
    }
    private __isDark: SynchedPropertySimpleOneWayPU<boolean>;
    get isDark() {
        bjccovmshb1i7x.instrumentFunction(6);
        return this.__isDark.get();
    }
    set isDark(newValue: boolean) {
        bjccovmshb1i7x.instrumentFunction(7);
        this.__isDark.set(newValue);
    }
    private __xrdpServerBusy: SynchedPropertySimpleOneWayPU<boolean>;
    get xrdpServerBusy() {
        bjccovmshb1i7x.instrumentFunction(8);
        return this.__xrdpServerBusy.get();
    }
    set xrdpServerBusy(newValue: boolean) {
        bjccovmshb1i7x.instrumentFunction(9);
        this.__xrdpServerBusy.set(newValue);
    }
    private __xrdpServerState: SynchedPropertySimpleOneWayPU<string>;
    get xrdpServerState() {
        bjccovmshb1i7x.instrumentFunction(10);
        return this.__xrdpServerState.get();
    }
    set xrdpServerState(newValue: string) {
        bjccovmshb1i7x.instrumentFunction(11);
        this.__xrdpServerState.set(newValue);
    }
    private __xrdpServerRunning: SynchedPropertySimpleOneWayPU<boolean>;
    get xrdpServerRunning() {
        bjccovmshb1i7x.instrumentFunction(12);
        return this.__xrdpServerRunning.get();
    }
    set xrdpServerRunning(newValue: boolean) {
        bjccovmshb1i7x.instrumentFunction(13);
        this.__xrdpServerRunning.set(newValue);
    }
    private __screenRecordingPermissionGranted: SynchedPropertySimpleOneWayPU<boolean>;
    get screenRecordingPermissionGranted() {
        bjccovmshb1i7x.instrumentFunction(14);
        return this.__screenRecordingPermissionGranted.get();
    }
    set screenRecordingPermissionGranted(newValue: boolean) {
        bjccovmshb1i7x.instrumentFunction(15);
        this.__screenRecordingPermissionGranted.set(newValue);
    }
    private __screenRecordingPermissionBusy: SynchedPropertySimpleOneWayPU<boolean>;
    get screenRecordingPermissionBusy() {
        bjccovmshb1i7x.instrumentFunction(16);
        return this.__screenRecordingPermissionBusy.get();
    }
    set screenRecordingPermissionBusy(newValue: boolean) {
        bjccovmshb1i7x.instrumentFunction(17);
        this.__screenRecordingPermissionBusy.set(newValue);
    }
    private __inputInjectionPermissionGranted: SynchedPropertySimpleOneWayPU<boolean>;
    get inputInjectionPermissionGranted() {
        bjccovmshb1i7x.instrumentFunction(18);
        return this.__inputInjectionPermissionGranted.get();
    }
    set inputInjectionPermissionGranted(newValue: boolean) {
        bjccovmshb1i7x.instrumentFunction(19);
        this.__inputInjectionPermissionGranted.set(newValue);
    }
    private __inputInjectionPermissionBusy: SynchedPropertySimpleOneWayPU<boolean>;
    get inputInjectionPermissionBusy() {
        bjccovmshb1i7x.instrumentFunction(20);
        return this.__inputInjectionPermissionBusy.get();
    }
    set inputInjectionPermissionBusy(newValue: boolean) {
        bjccovmshb1i7x.instrumentFunction(21);
        this.__inputInjectionPermissionBusy.set(newValue);
    }
    private __remoteControlServerAvailable: SynchedPropertySimpleOneWayPU<boolean>;
    get remoteControlServerAvailable() {
        bjccovmshb1i7x.instrumentFunction(22);
        return this.__remoteControlServerAvailable.get();
    }
    set remoteControlServerAvailable(newValue: boolean) {
        bjccovmshb1i7x.instrumentFunction(23);
        this.__remoteControlServerAvailable.set(newValue);
    }
    private onOpenSettings: (remoteControlSection: string) => void;
    private __hoveredSection: ObservedPropertySimplePU<string>;
    get hoveredSection() {
        bjccovmshb1i7x.instrumentFunction(25);
        return this.__hoveredSection.get();
    }
    set hoveredSection(newValue: string) {
        bjccovmshb1i7x.instrumentFunction(26);
        this.__hoveredSection.set(newValue);
    }
    private __pressedSection: ObservedPropertySimplePU<string>;
    get pressedSection() {
        bjccovmshb1i7x.instrumentFunction(27);
        return this.__pressedSection.get();
    }
    set pressedSection(newValue: string) {
        bjccovmshb1i7x.instrumentFunction(28);
        this.__pressedSection.set(newValue);
    }
    private serviceStatusTone(): SettingsStatusTone {
        bjccovmshb1i7x.instrumentFunction(29);
        bjccovmshb1i7x.instrumentRegion(29, 1);
        return SettingsText.remoteServerStatusTone(this.xrdpServerRunning, this.xrdpServerBusy, this.xrdpServerState);
    }
    private serviceStatusText(): string {
        bjccovmshb1i7x.instrumentFunction(30);
        if (this.xrdpServerBusy) {
            bjccovmshb1i7x.instrumentBranch(30, 0, true);
            bjccovmshb1i7x.instrumentRegion(30, 1);
            return SettingsText.REMOTE_SERVER_BUSY;
        }
        else {
            bjccovmshb1i7x.instrumentBranch(30, 0, false);
        }
        bjccovmshb1i7x.instrumentRegion(30, 2);
        return SettingsText.remoteServerStateLabel(this.xrdpServerRunning, this.xrdpServerState);
    }
    private screenStatusTone(): SettingsStatusTone {
        bjccovmshb1i7x.instrumentFunction(31);
        if (this.screenRecordingPermissionBusy) {
            bjccovmshb1i7x.instrumentBranch(31, 0, true);
            bjccovmshb1i7x.instrumentRegion(31, 1);
            return 'info';
        }
        else {
            bjccovmshb1i7x.instrumentBranch(31, 0, false);
        }
        bjccovmshb1i7x.instrumentRegion(31, 2);
        return this.screenRecordingPermissionGranted ? (bjccovmshb1i7x.instrumentBranch(31, 1, true), 'ok') : (bjccovmshb1i7x.instrumentBranch(31, 1, false), 'neutral');
    }
    private screenStatusText(): string {
        bjccovmshb1i7x.instrumentFunction(32);
        if (this.screenRecordingPermissionBusy) {
            bjccovmshb1i7x.instrumentBranch(32, 0, true);
            bjccovmshb1i7x.instrumentRegion(32, 1);
            return SettingsText.REMOTE_PERMISSION_BUSY;
        }
        else {
            bjccovmshb1i7x.instrumentBranch(32, 0, false);
        }
        bjccovmshb1i7x.instrumentRegion(32, 2);
        return this.screenRecordingPermissionGranted ? (bjccovmshb1i7x.instrumentBranch(32, 1, true), SettingsText.REMOTE_PERMISSION_GRANTED) : (bjccovmshb1i7x.instrumentBranch(32, 1, false), SettingsText.REMOTE_PERMISSION_MISSING);
    }
    private inputStatusTone(): SettingsStatusTone {
        bjccovmshb1i7x.instrumentFunction(33);
        if (this.inputInjectionPermissionBusy) {
            bjccovmshb1i7x.instrumentBranch(33, 0, true);
            bjccovmshb1i7x.instrumentRegion(33, 1);
            return 'info';
        }
        else {
            bjccovmshb1i7x.instrumentBranch(33, 0, false);
        }
        bjccovmshb1i7x.instrumentRegion(33, 2);
        return this.inputInjectionPermissionGranted ? (bjccovmshb1i7x.instrumentBranch(33, 1, true), 'ok') : (bjccovmshb1i7x.instrumentBranch(33, 1, false), 'neutral');
    }
    private inputStatusText(): string {
        bjccovmshb1i7x.instrumentFunction(34);
        if (this.inputInjectionPermissionBusy) {
            bjccovmshb1i7x.instrumentBranch(34, 0, true);
            bjccovmshb1i7x.instrumentRegion(34, 1);
            return SettingsText.REMOTE_PERMISSION_BUSY;
        }
        else {
            bjccovmshb1i7x.instrumentBranch(34, 0, false);
        }
        bjccovmshb1i7x.instrumentRegion(34, 2);
        return this.inputInjectionPermissionGranted ? (bjccovmshb1i7x.instrumentBranch(34, 1, true), SettingsText.REMOTE_PERMISSION_GRANTED) : (bjccovmshb1i7x.instrumentBranch(34, 1, false), SettingsText.REMOTE_PERMISSION_MISSING);
    }
    private cardValue(section: string): string {
        bjccovmshb1i7x.instrumentFunction(35);
        bjccovmshb1i7x.instrumentRegion(35, 1);
        const values = new Map<string, string>();
        values.set(SettingsRemoteControlSection.SERVER, this.serviceStatusText());
        values.set(SettingsRemoteControlSection.SCREEN, this.screenStatusText());
        values.set(SettingsRemoteControlSection.INPUT, this.inputStatusText());
        values.set(SettingsRemoteControlSection.FILES, HomeText.STATUS_FILES_CONFIGURED);
        return values.get(section) || HomeText.STATUS_FILES_CONFIGURED;
    }
    private cardTone(section: string): SettingsStatusTone {
        bjccovmshb1i7x.instrumentFunction(36);
        bjccovmshb1i7x.instrumentRegion(36, 1);
        const tones = new Map<string, SettingsStatusTone>();
        tones.set(SettingsRemoteControlSection.SERVER, this.serviceStatusTone());
        tones.set(SettingsRemoteControlSection.SCREEN, this.screenStatusTone());
        tones.set(SettingsRemoteControlSection.INPUT, this.inputStatusTone());
        tones.set(SettingsRemoteControlSection.FILES, 'ok');
        return tones.get(section) || 'ok';
    }
    private visibleStatusCards(): HomeStatusCardConfig[] {
        bjccovmshb1i7x.instrumentFunction(37);
        if (this.remoteControlServerAvailable) {
            bjccovmshb1i7x.instrumentBranch(37, 0, true);
            bjccovmshb1i7x.instrumentRegion(37, 1);
            return HOME_STATUS_CARDS;
        }
        else {
            bjccovmshb1i7x.instrumentBranch(37, 0, false);
        }
        bjccovmshb1i7x.instrumentRegion(37, 2);
        return [HOME_STATUS_CARDS[3]];
    }
    private setHoveredSection(section: string, hovered: boolean): void {
        bjccovmshb1i7x.instrumentFunction(38);
        bjccovmshb1i7x.instrumentRegion(38, 1);
        SettingsTheme.animate(this.getUIContext(), () => {
            bjccovmshb1i7x.instrumentFunction(39);
            bjccovmshb1i7x.instrumentRegion(39, 1);
            this.hoveredSection = hovered ? (bjccovmshb1i7x.instrumentBranch(39, 0, true), section) : (bjccovmshb1i7x.instrumentBranch(39, 0, false), '');
        });
    }
    private buildStatusCard(config: HomeStatusCardConfig, expanded: boolean, parent = null) {
        bjccovmshb1i7x.instrumentFunction(40);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i7x.instrumentFunction(41);
            Row.create({ space: 12 });
            Row.height(52);
            Row.width(expanded ? (bjccovmshb1i7x.instrumentBranch(41, 0, true), 'auto') : (bjccovmshb1i7x.instrumentBranch(41, 0, false), '100%'));
            Row.layoutWeight(expanded ? (bjccovmshb1i7x.instrumentBranch(41, 1, true), 1) : (bjccovmshb1i7x.instrumentBranch(41, 1, false), 0));
            Row.padding({ left: 14, right: 14 });
            Row.alignItems(VerticalAlign.Center);
            Row.backgroundColor(this.hoveredSection === config.section || this.pressedSection === config.section ? (bjccovmshb1i7x.instrumentBranch(41, 2, true), HomeTheme.inactivePanelBackground(this.isDark, this.hoveredSection === config.section, this.pressedSection === config.section)) : (bjccovmshb1i7x.instrumentBranch(41, 2, false), HomeTheme.panelBackground(this.isDark)));
            Row.borderRadius(8);
            Row.border({
                width: 1,
                color: this.hoveredSection === config.section || this.pressedSection === config.section ? (bjccovmshb1i7x.instrumentBranch(41, 3, true), SettingsTheme.statusText(this.isDark, this.cardTone(config.section))) : (bjccovmshb1i7x.instrumentBranch(41, 3, false), HomeTheme.mutedBorderColor(this.isDark))
            });
            Row.scale({
                x: this.pressedSection === config.section ? (bjccovmshb1i7x.instrumentBranch(41, 4, true), 0.99) : (bjccovmshb1i7x.instrumentBranch(41, 4, false), 1),
                y: this.pressedSection === config.section ? (bjccovmshb1i7x.instrumentBranch(41, 5, true), 0.99) : (bjccovmshb1i7x.instrumentBranch(41, 5, false), 1)
            });
            Row.onHover((hovered: boolean) => {
                bjccovmshb1i7x.instrumentFunction(42);
                bjccovmshb1i7x.instrumentRegion(42, 1);
                this.setHoveredSection(config.section, hovered);
            });
            Row.onTouch((event: TouchEvent) => {
                bjccovmshb1i7x.instrumentFunction(43);
                if (event.type === TouchType.Down) {
                    bjccovmshb1i7x.instrumentBranch(43, 0, true);
                    bjccovmshb1i7x.instrumentRegion(43, 1);
                    this.pressedSection = config.section;
                    bjccovmshb1i7x.instrumentRegion(40, 1);
                    return;
                }
                else {
                    bjccovmshb1i7x.instrumentBranch(43, 0, false);
                }
                if (event.type === TouchType.Up || event.type === TouchType.Cancel) {
                    bjccovmshb1i7x.instrumentBranch(43, 1, true);
                    bjccovmshb1i7x.instrumentRegion(43, 2);
                    this.pressedSection = '';
                }
                else {
                    bjccovmshb1i7x.instrumentBranch(43, 1, false);
                }
            });
            Row.onClick(() => {
                bjccovmshb1i7x.instrumentFunction(44);
                this.pressedSection = '';
                bjccovmshb1i7x.instrumentRegion(40, 2);
                this.onOpenSettings(config.section);
            });
        }, Row);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i7x.instrumentFunction(45);
            Column.create();
            Column.width(32);
            Column.height(32);
            Column.borderRadius(8);
            Column.justifyContent(FlexAlign.Center);
            Column.alignItems(HorizontalAlign.Center);
            Column.backgroundColor(SettingsTheme.statusBackground(this.isDark, this.cardTone(config.section)));
        }, Column);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i7x.instrumentFunction(46);
            Image.create(config.icon);
            Image.width(20);
            Image.height(20);
            Image.fillColor(SettingsTheme.statusText(this.isDark, this.cardTone(config.section)));
            Image.draggable(false);
        }, Image);
        Column.pop();
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i7x.instrumentFunction(47);
            Text.create(config.label);
            Text.fontSize(15);
            Text.fontWeight(FontWeight.Bold);
            Text.fontColor(SettingsTheme.primaryText(this.isDark));
            Text.maxLines(1);
        }, Text);
        Text.pop();
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i7x.instrumentFunction(48);
            Text.create(this.cardValue(config.section));
            Text.fontSize(14);
            Text.fontColor(SettingsTheme.secondaryText(this.isDark));
            Text.maxLines(1);
            Text.textOverflow({ overflow: TextOverflow.Ellipsis });
            Text.layoutWeight(1);
        }, Text);
        Text.pop();
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i7x.instrumentFunction(49);
            Column.create();
            Column.width(9);
            Column.height(9);
            Column.borderRadius(4.5);
            Column.backgroundColor(SettingsTheme.statusText(this.isDark, this.cardTone(config.section)));
        }, Column);
        Column.pop();
        Row.pop();
    }
    private buildExpandedFooter(parent = null) {
        bjccovmshb1i7x.instrumentFunction(50);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i7x.instrumentFunction(51);
            Row.create({ space: 22 });
            Row.width('100%');
            Row.height(70);
            Row.padding({ left: 30, right: 30, top: 8, bottom: 10 });
            Row.backgroundColor(HomeTheme.appBackground(this.isDark));
            Row.border({
                width: { top: 1 },
                color: HomeTheme.borderColor(this.isDark)
            });
        }, Row);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i7x.instrumentFunction(52);
            ForEach.create();
            const forEachItemGenFunction = _item => {
                bjccovmshb1i7x.instrumentFunction(53);
                const config = _item;
                this.buildStatusCard.bind(this)(config, true);
            };
            this.forEachUpdateFunction(elmtId, this.visibleStatusCards(), forEachItemGenFunction, (config: HomeStatusCardConfig): string => { bjccovmshb1i7x.instrumentFunction(54); return config.section; }, false, false);
        }, ForEach);
        ForEach.pop();
        Row.pop();
    }
    private buildCompactFooter(parent = null) {
        bjccovmshb1i7x.instrumentFunction(55);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i7x.instrumentFunction(56);
            GridRow.create({ columns: 2, gutter: { x: 12, y: 12 } });
            GridRow.width('100%');
            GridRow.padding({ left: 12, right: 12, top: 8, bottom: 10 });
            GridRow.backgroundColor(HomeTheme.appBackground(this.isDark));
            GridRow.border({
                width: { top: 1 },
                color: HomeTheme.borderColor(this.isDark)
            });
        }, GridRow);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i7x.instrumentFunction(57);
            ForEach.create();
            const forEachItemGenFunction = _item => {
                bjccovmshb1i7x.instrumentFunction(58);
                const config = _item;
                this.observeComponentCreation2((elmtId, isInitialRender) => {
                    bjccovmshb1i7x.instrumentFunction(59);
                    GridCol.create({ span: 1 });
                }, GridCol);
                this.buildStatusCard.bind(this)(config, false);
                GridCol.pop();
            };
            this.forEachUpdateFunction(elmtId, this.visibleStatusCards(), forEachItemGenFunction, (config: HomeStatusCardConfig): string => { bjccovmshb1i7x.instrumentFunction(60); return config.section; }, false, false);
        }, ForEach);
        ForEach.pop();
        GridRow.pop();
    }
    initialRender() {
        bjccovmshb1i7x.instrumentFunction(61);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i7x.instrumentFunction(62);
            Stack.create();
            Stack.width('100%');
        }, Stack);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i7x.instrumentFunction(63);
            If.create();
            if (this.layoutMode === LayoutMode.COMPACT) {
                bjccovmshb1i7x.instrumentBranch(63, 0, true);
                bjccovmshb1i7x.instrumentRegion(63, 1);
                this.ifElseBranchUpdateFunction(0, () => {
                    bjccovmshb1i7x.instrumentFunction(64);
                    this.buildCompactFooter.bind(this)();
                });
            }
            else {
                bjccovmshb1i7x.instrumentBranch(63, 0, false);
                bjccovmshb1i7x.instrumentRegion(63, 2);
                this.ifElseBranchUpdateFunction(1, () => {
                    bjccovmshb1i7x.instrumentFunction(65);
                    this.buildExpandedFooter.bind(this)();
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
