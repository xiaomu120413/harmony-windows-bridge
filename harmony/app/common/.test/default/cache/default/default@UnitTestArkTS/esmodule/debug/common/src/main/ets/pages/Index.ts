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
let bjccovmshb1hr4 = new BjcCov({ version: "bjc v1.0.0", versionCode: 10000, path: "common/src/main/ets/pages/Index.ets", hash: "faec9723e409632a4abca72954ebfb7287157116d9fc57aa746edfa06cd81638", lineCnt: 935, count: 0, projectPath: "", functions: { 0: { name: "anonymous_1", count: 0, regions: { 0: { startLoc: { line: 91, col: 48 }, endLoc: { line: 93, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 92, col: 5 }, endLoc: { line: 93, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 0 }, 1: { name: "host", count: 0, regions: { 0: { startLoc: { line: 45, col: 18 }, endLoc: { line: 45, col: 22 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 1 }, 2: { name: "host", count: 0, regions: { 0: { startLoc: { line: 45, col: 18 }, endLoc: { line: 45, col: 30 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 2 }, 3: { name: "port", count: 0, regions: { 0: { startLoc: { line: 46, col: 18 }, endLoc: { line: 46, col: 22 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 3 }, 4: { name: "port", count: 0, regions: { 0: { startLoc: { line: 46, col: 18 }, endLoc: { line: 46, col: 30 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 4 }, 5: { name: "username", count: 0, regions: { 0: { startLoc: { line: 47, col: 18 }, endLoc: { line: 47, col: 26 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 5 }, 6: { name: "username", count: 0, regions: { 0: { startLoc: { line: 47, col: 18 }, endLoc: { line: 47, col: 34 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 6 }, 7: { name: "password", count: 0, regions: { 0: { startLoc: { line: 48, col: 18 }, endLoc: { line: 48, col: 26 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 7 }, 8: { name: "password", count: 0, regions: { 0: { startLoc: { line: 48, col: 18 }, endLoc: { line: 48, col: 34 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 8 }, 9: { name: "connectionProfiles", count: 0, regions: { 0: { startLoc: { line: 49, col: 18 }, endLoc: { line: 49, col: 36 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 9 }, 10: { name: "connectionProfiles", count: 0, regions: { 0: { startLoc: { line: 49, col: 18 }, endLoc: { line: 49, col: 64 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 10 }, 11: { name: "selectedConnectionProfileId", count: 0, regions: { 0: { startLoc: { line: 50, col: 18 }, endLoc: { line: 50, col: 45 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 11 }, 12: { name: "selectedConnectionProfileId", count: 0, regions: { 0: { startLoc: { line: 50, col: 18 }, endLoc: { line: 50, col: 53 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 12 }, 13: { name: "rememberConnectionPassword", count: 0, regions: { 0: { startLoc: { line: 51, col: 18 }, endLoc: { line: 51, col: 44 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 13 }, 14: { name: "rememberConnectionPassword", count: 0, regions: { 0: { startLoc: { line: 51, col: 18 }, endLoc: { line: 51, col: 53 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 14 }, 15: { name: "connectionProfilePasswordLoading", count: 0, regions: { 0: { startLoc: { line: 52, col: 18 }, endLoc: { line: 52, col: 50 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 15 }, 16: { name: "connectionProfilePasswordLoading", count: 0, regions: { 0: { startLoc: { line: 52, col: 18 }, endLoc: { line: 52, col: 59 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 16 }, 17: { name: "remoteAccessCode", count: 0, regions: { 0: { startLoc: { line: 55, col: 18 }, endLoc: { line: 55, col: 34 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 17 }, 18: { name: "remoteAccessCode", count: 0, regions: { 0: { startLoc: { line: 55, col: 18 }, endLoc: { line: 55, col: 42 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 18 }, 19: { name: "remoteAccessCodeGateEnabled", count: 0, regions: { 0: { startLoc: { line: 56, col: 18 }, endLoc: { line: 56, col: 45 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 19 }, 20: { name: "remoteAccessCodeGateEnabled", count: 0, regions: { 0: { startLoc: { line: 56, col: 18 }, endLoc: { line: 56, col: 54 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 20 }, 21: { name: "screenRecordingPermissionGranted", count: 0, regions: { 0: { startLoc: { line: 57, col: 18 }, endLoc: { line: 57, col: 50 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 21 }, 22: { name: "screenRecordingPermissionGranted", count: 0, regions: { 0: { startLoc: { line: 57, col: 18 }, endLoc: { line: 57, col: 59 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 22 }, 23: { name: "screenRecordingPermissionBusy", count: 0, regions: { 0: { startLoc: { line: 58, col: 18 }, endLoc: { line: 58, col: 47 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 23 }, 24: { name: "screenRecordingPermissionBusy", count: 0, regions: { 0: { startLoc: { line: 58, col: 18 }, endLoc: { line: 58, col: 56 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 24 }, 25: { name: "inputInjectionPermissionGranted", count: 0, regions: { 0: { startLoc: { line: 59, col: 18 }, endLoc: { line: 59, col: 49 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 25 }, 26: { name: "inputInjectionPermissionGranted", count: 0, regions: { 0: { startLoc: { line: 59, col: 18 }, endLoc: { line: 59, col: 58 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 26 }, 27: { name: "inputInjectionPermissionBusy", count: 0, regions: { 0: { startLoc: { line: 60, col: 18 }, endLoc: { line: 60, col: 46 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 27 }, 28: { name: "inputInjectionPermissionBusy", count: 0, regions: { 0: { startLoc: { line: 60, col: 18 }, endLoc: { line: 60, col: 55 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 28 }, 29: { name: "xrdpServerRunning", count: 0, regions: { 0: { startLoc: { line: 61, col: 18 }, endLoc: { line: 61, col: 35 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 29 }, 30: { name: "xrdpServerRunning", count: 0, regions: { 0: { startLoc: { line: 61, col: 18 }, endLoc: { line: 61, col: 44 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 30 }, 31: { name: "xrdpServerState", count: 0, regions: { 0: { startLoc: { line: 62, col: 18 }, endLoc: { line: 62, col: 33 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 31 }, 32: { name: "xrdpServerState", count: 0, regions: { 0: { startLoc: { line: 62, col: 18 }, endLoc: { line: 62, col: 41 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 32 }, 33: { name: "xrdpServerPort", count: 0, regions: { 0: { startLoc: { line: 63, col: 18 }, endLoc: { line: 63, col: 32 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 33 }, 34: { name: "xrdpServerPort", count: 0, regions: { 0: { startLoc: { line: 63, col: 18 }, endLoc: { line: 63, col: 40 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 34 }, 35: { name: "xrdpServerMessage", count: 0, regions: { 0: { startLoc: { line: 64, col: 18 }, endLoc: { line: 64, col: 35 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 35 }, 36: { name: "xrdpServerMessage", count: 0, regions: { 0: { startLoc: { line: 64, col: 18 }, endLoc: { line: 64, col: 43 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 36 }, 37: { name: "xrdpServerBusy", count: 0, regions: { 0: { startLoc: { line: 65, col: 18 }, endLoc: { line: 65, col: 32 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 37 }, 38: { name: "xrdpServerBusy", count: 0, regions: { 0: { startLoc: { line: 65, col: 18 }, endLoc: { line: 65, col: 41 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 38 }, 39: { name: "showSession", count: 0, regions: { 0: { startLoc: { line: 66, col: 18 }, endLoc: { line: 66, col: 29 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 39 }, 40: { name: "showSession", count: 0, regions: { 0: { startLoc: { line: 66, col: 18 }, endLoc: { line: 66, col: 38 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 40 }, 41: { name: "showSettings", count: 0, regions: { 0: { startLoc: { line: 67, col: 18 }, endLoc: { line: 67, col: 30 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 41 }, 42: { name: "showSettings", count: 0, regions: { 0: { startLoc: { line: 67, col: 18 }, endLoc: { line: 67, col: 39 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 42 }, 43: { name: "settingsInitialPageName", count: 0, regions: { 0: { startLoc: { line: 68, col: 18 }, endLoc: { line: 68, col: 41 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 43 }, 44: { name: "settingsInitialPageName", count: 0, regions: { 0: { startLoc: { line: 68, col: 18 }, endLoc: { line: 68, col: 49 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 44 }, 45: { name: "settingsRemoteControlSection", count: 0, regions: { 0: { startLoc: { line: 69, col: 18 }, endLoc: { line: 69, col: 46 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 45 }, 46: { name: "settingsRemoteControlSection", count: 0, regions: { 0: { startLoc: { line: 69, col: 18 }, endLoc: { line: 69, col: 54 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 46 }, 47: { name: "remoteLoginWaiting", count: 0, regions: { 0: { startLoc: { line: 70, col: 18 }, endLoc: { line: 70, col: 36 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 47 }, 48: { name: "remoteLoginWaiting", count: 0, regions: { 0: { startLoc: { line: 70, col: 18 }, endLoc: { line: 70, col: 45 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 48 }, 49: { name: "sessionNoticeTitle", count: 0, regions: { 0: { startLoc: { line: 71, col: 18 }, endLoc: { line: 71, col: 36 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 49 }, 50: { name: "sessionNoticeTitle", count: 0, regions: { 0: { startLoc: { line: 71, col: 18 }, endLoc: { line: 71, col: 44 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 50 }, 51: { name: "sessionNoticeSubtitle", count: 0, regions: { 0: { startLoc: { line: 72, col: 18 }, endLoc: { line: 72, col: 39 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 51 }, 52: { name: "sessionNoticeSubtitle", count: 0, regions: { 0: { startLoc: { line: 72, col: 18 }, endLoc: { line: 72, col: 47 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 52 }, 53: { name: "connectionFeedbackText", count: 0, regions: { 0: { startLoc: { line: 73, col: 18 }, endLoc: { line: 73, col: 40 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 53 }, 54: { name: "connectionFeedbackText", count: 0, regions: { 0: { startLoc: { line: 73, col: 18 }, endLoc: { line: 73, col: 48 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 54 }, 55: { name: "connectionFeedbackTone", count: 0, regions: { 0: { startLoc: { line: 74, col: 18 }, endLoc: { line: 74, col: 40 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 55 }, 56: { name: "connectionFeedbackTone", count: 0, regions: { 0: { startLoc: { line: 74, col: 18 }, endLoc: { line: 74, col: 60 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 56 }, 57: { name: "layoutMode", count: 0, regions: { 0: { startLoc: { line: 75, col: 18 }, endLoc: { line: 75, col: 28 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 57 }, 58: { name: "layoutMode", count: 0, regions: { 0: { startLoc: { line: 75, col: 18 }, endLoc: { line: 75, col: 40 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 58 }, 59: { name: "systemDark", count: 0, regions: { 0: { startLoc: { line: 77, col: 46 }, endLoc: { line: 77, col: 56 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 59 }, 60: { name: "systemDark", count: 0, regions: { 0: { startLoc: { line: 77, col: 46 }, endLoc: { line: 77, col: 65 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 60 }, 61: { name: "appearanceMode", count: 0, regions: { 0: { startLoc: { line: 78, col: 50 }, endLoc: { line: 78, col: 64 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 61 }, 62: { name: "appearanceMode", count: 0, regions: { 0: { startLoc: { line: 78, col: 50 }, endLoc: { line: 78, col: 72 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 62 }, 63: { name: "deferUiUpdate", count: 0, regions: { 0: { startLoc: { line: 95, col: 3 }, endLoc: { line: 99, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 96, col: 5 }, endLoc: { line: 99, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 63 }, 64: { name: "anonymous_2", count: 0, regions: { 0: { startLoc: { line: 95, col: 31 }, endLoc: { line: 95, col: 41 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 64 }, 65: { name: "anonymous_3", count: 0, regions: { 0: { startLoc: { line: 96, col: 16 }, endLoc: { line: 98, col: 6 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 97, col: 7 }, endLoc: { line: 98, col: 6 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 65 }, 66: { name: "aboutToAppear", count: 0, regions: { 0: { startLoc: { line: 101, col: 3 }, endLoc: { line: 111, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 105, col: 46 }, endLoc: { line: 110, col: 6 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 105, col: 9 }, endLoc: { line: 105, col: 44 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 66 }, 67: { name: "onPageShow", count: 0, regions: { 0: { startLoc: { line: 113, col: 3 }, endLoc: { line: 120, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 114, col: 41 }, endLoc: { line: 116, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 117, col: 46 }, endLoc: { line: 119, col: 6 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 114, col: 9 }, endLoc: { line: 114, col: 39 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 117, col: 9 }, endLoc: { line: 117, col: 44 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 67 }, 68: { name: "onPageHide", count: 0, regions: { 0: { startLoc: { line: 122, col: 3 }, endLoc: { line: 124, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 123, col: 5 }, endLoc: { line: 124, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 68 }, 69: { name: "aboutToDisappear", count: 0, regions: { 0: { startLoc: { line: 126, col: 3 }, endLoc: { line: 129, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 127, col: 5 }, endLoc: { line: 129, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 69 }, 70: { name: "registerWindowLayoutObserver", count: 0, regions: { 0: { startLoc: { line: 131, col: 3 }, endLoc: { line: 143, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 132, col: 9 }, endLoc: { line: 140, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 140, col: 7 }, endLoc: { line: 142, col: 6 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 135, col: 48 }, endLoc: { line: 137, col: 8 }, count: 0, ignored: 0 }, 4: { startLoc: { line: 138, col: 7 }, endLoc: { line: 140, col: 6 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 135, col: 11 }, endLoc: { line: 135, col: 46 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 70 }, 71: { name: "unregisterWindowLayoutObserver", count: 0, regions: { 0: { startLoc: { line: 145, col: 3 }, endLoc: { line: 156, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 146, col: 47 }, endLoc: { line: 148, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 149, col: 9 }, endLoc: { line: 153, col: 6 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 153, col: 7 }, endLoc: { line: 155, col: 6 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 146, col: 9 }, endLoc: { line: 146, col: 45 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 71 }, 72: { name: "getUiAbilityContext", count: 0, regions: { 0: { startLoc: { line: 158, col: 3 }, endLoc: { line: 169, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 159, col: 9 }, endLoc: { line: 165, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 165, col: 7 }, endLoc: { line: 168, col: 6 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 161, col: 25 }, endLoc: { line: 163, col: 8 }, count: 0, ignored: 0 }, 4: { startLoc: { line: 164, col: 7 }, endLoc: { line: 165, col: 6 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 161, col: 11 }, endLoc: { line: 161, col: 23 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 72 }, 73: { name: "remoteControlServerAvailable", count: 0, regions: { 0: { startLoc: { line: 171, col: 3 }, endLoc: { line: 173, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 172, col: 5 }, endLoc: { line: 173, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 73 }, 74: { name: "getPermissionManager", count: 0, regions: { 0: { startLoc: { line: 175, col: 3 }, endLoc: { line: 185, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 176, col: 42 }, endLoc: { line: 183, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 184, col: 5 }, endLoc: { line: 185, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 176, col: 9 }, endLoc: { line: 176, col: 40 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 74 }, 75: { name: "anonymous_4", count: 0, regions: { 0: { startLoc: { line: 178, col: 9 }, endLoc: { line: 178, col: 73 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 75 }, 76: { name: "anonymous_5", count: 0, regions: { 0: { startLoc: { line: 180, col: 67 }, endLoc: { line: 182, col: 8 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 181, col: 9 }, endLoc: { line: 182, col: 8 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 76 }, 77: { name: "getPermissionRequestCoordinator", count: 0, regions: { 0: { startLoc: { line: 187, col: 3 }, endLoc: { line: 195, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 188, col: 53 }, endLoc: { line: 193, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 194, col: 5 }, endLoc: { line: 195, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 188, col: 9 }, endLoc: { line: 188, col: 51 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 77 }, 78: { name: "anonymous_6", count: 0, regions: { 0: { startLoc: { line: 191, col: 9 }, endLoc: { line: 191, col: 61 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 78 }, 79: { name: "anonymous_7", count: 0, regions: { 0: { startLoc: { line: 191, col: 16 }, endLoc: { line: 191, col: 26 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 79 }, 80: { name: "getRemoteControlCoordinator", count: 0, regions: { 0: { startLoc: { line: 197, col: 3 }, endLoc: { line: 211, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 198, col: 49 }, endLoc: { line: 209, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 210, col: 5 }, endLoc: { line: 211, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 198, col: 9 }, endLoc: { line: 198, col: 47 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 80 }, 81: { name: "anonymous_8", count: 0, regions: { 0: { startLoc: { line: 202, col: 9 }, endLoc: { line: 202, col: 64 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 81 }, 82: { name: "anonymous_9", count: 0, regions: { 0: { startLoc: { line: 203, col: 9 }, endLoc: { line: 203, col: 44 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 82 }, 83: { name: "anonymous_10", count: 0, regions: { 0: { startLoc: { line: 204, col: 9 }, endLoc: { line: 206, col: 10 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 205, col: 11 }, endLoc: { line: 206, col: 10 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 83 }, 84: { name: "anonymous_11", count: 0, regions: { 0: { startLoc: { line: 205, col: 30 }, endLoc: { line: 205, col: 83 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 84 }, 85: { name: "getConnectionProfileCoordinator", count: 0, regions: { 0: { startLoc: { line: 213, col: 3 }, endLoc: { line: 225, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 214, col: 53 }, endLoc: { line: 216, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 219, col: 27 }, endLoc: { line: 221, col: 6 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 223, col: 5 }, endLoc: { line: 225, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 214, col: 9 }, endLoc: { line: 214, col: 51 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 219, col: 9 }, endLoc: { line: 219, col: 25 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 85 }, 86: { name: "loadConnectionProfiles", count: 0, regions: { 0: { startLoc: { line: 227, col: 3 }, endLoc: { line: 243, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 229, col: 31 }, endLoc: { line: 231, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 233, col: 5 }, endLoc: { line: 243, col: 4 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 238, col: 9 }, endLoc: { line: 242, col: 8 }, count: 0, ignored: 0 }, 4: { startLoc: { line: 240, col: 11 }, endLoc: { line: 241, col: 10 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 229, col: 9 }, endLoc: { line: 229, col: 29 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 86 }, 87: { name: "anonymous_12", count: 0, regions: { 0: { startLoc: { line: 234, col: 13 }, endLoc: { line: 242, col: 8 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 235, col: 32 }, endLoc: { line: 237, col: 10 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 235, col: 13 }, endLoc: { line: 235, col: 30 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 87 }, 88: { name: "anonymous_13", count: 0, regions: { 0: { startLoc: { line: 238, col: 28 }, endLoc: { line: 241, col: 10 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 88 }, 89: { name: "applyConnectionSnapshot", count: 0, regions: { 0: { startLoc: { line: 245, col: 3 }, endLoc: { line: 264, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 250, col: 32 }, endLoc: { line: 252, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 255, col: 35 }, endLoc: { line: 261, col: 6 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 257, col: 27 }, endLoc: { line: 259, col: 8 }, count: 0, ignored: 0 }, 4: { startLoc: { line: 263, col: 5 }, endLoc: { line: 264, col: 4 }, count: 0, ignored: 0 }, 5: { startLoc: { line: 260, col: 7 }, endLoc: { line: 261, col: 6 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 250, col: 9 }, endLoc: { line: 250, col: 30 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 255, col: 9 }, endLoc: { line: 255, col: 33 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 2: { startLoc: { line: 257, col: 11 }, endLoc: { line: 257, col: 25 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 89 }, 90: { name: "findConnectionProfile", count: 0, regions: { 0: { startLoc: { line: 266, col: 3 }, endLoc: { line: 268, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 267, col: 5 }, endLoc: { line: 268, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 90 }, 91: { name: "findConnectionProfileIn", count: 0, regions: { 0: { startLoc: { line: 270, col: 3 }, endLoc: { line: 277, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 271, col: 5 }, endLoc: { line: 275, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 272, col: 37 }, endLoc: { line: 274, col: 8 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 276, col: 5 }, endLoc: { line: 277, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 272, col: 11 }, endLoc: { line: 272, col: 35 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 91 }, 92: { name: "applyConnectionProfile", count: 0, regions: { 0: { startLoc: { line: 279, col: 3 }, endLoc: { line: 289, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 286, col: 40 }, endLoc: { line: 288, col: 6 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 286, col: 9 }, endLoc: { line: 286, col: 38 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 92 }, 93: { name: "selectConnectionProfile", count: 0, regions: { 0: { startLoc: { line: 291, col: 3 }, endLoc: { line: 334, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 292, col: 33 }, endLoc: { line: 295, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 298, col: 27 }, endLoc: { line: 300, col: 6 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 303, col: 36 }, endLoc: { line: 307, col: 6 }, count: 0, ignored: 0 }, 4: { startLoc: { line: 310, col: 31 }, endLoc: { line: 314, col: 6 }, count: 0, ignored: 0 }, 5: { startLoc: { line: 315, col: 5 }, endLoc: { line: 334, col: 4 }, count: 0, ignored: 0 }, 6: { startLoc: { line: 327, col: 9 }, endLoc: { line: 333, col: 8 }, count: 0, ignored: 0 }, 7: { startLoc: { line: 325, col: 11 }, endLoc: { line: 326, col: 10 }, count: 0, ignored: 0 }, 8: { startLoc: { line: 322, col: 15 }, endLoc: { line: 323, col: 14 }, count: 0, ignored: 0 }, 9: { startLoc: { line: 330, col: 13 }, endLoc: { line: 331, col: 12 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 292, col: 9 }, endLoc: { line: 292, col: 31 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 298, col: 9 }, endLoc: { line: 298, col: 25 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 2: { startLoc: { line: 303, col: 9 }, endLoc: { line: 303, col: 34 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 3: { startLoc: { line: 310, col: 9 }, endLoc: { line: 310, col: 29 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 93 }, 94: { name: "anonymous_14", count: 0, regions: { 0: { startLoc: { line: 317, col: 13 }, endLoc: { line: 333, col: 8 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 318, col: 37 }, endLoc: { line: 326, col: 10 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 318, col: 13 }, endLoc: { line: 318, col: 35 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 94 }, 95: { name: "anonymous_15", count: 0, regions: { 0: { startLoc: { line: 319, col: 30 }, endLoc: { line: 324, col: 12 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 320, col: 84 }, endLoc: { line: 323, col: 14 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 320, col: 17 }, endLoc: { line: 320, col: 82 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 95 }, 96: { name: "anonymous_16", count: 0, regions: { 0: { startLoc: { line: 327, col: 28 }, endLoc: { line: 332, col: 10 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 328, col: 82 }, endLoc: { line: 331, col: 12 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 328, col: 15 }, endLoc: { line: 328, col: 80 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 96 }, 97: { name: "startNewConnectionProfile", count: 0, regions: { 0: { startLoc: { line: 336, col: 3 }, endLoc: { line: 347, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 337, col: 5 }, endLoc: { line: 347, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 97 }, 98: { name: "deleteSelectedConnectionProfile", count: 0, regions: { 0: { startLoc: { line: 349, col: 3 }, endLoc: { line: 369, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 350, col: 56 }, endLoc: { line: 353, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 356, col: 31 }, endLoc: { line: 358, col: 6 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 360, col: 5 }, endLoc: { line: 369, col: 4 }, count: 0, ignored: 0 }, 4: { startLoc: { line: 365, col: 9 }, endLoc: { line: 368, col: 8 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 350, col: 9 }, endLoc: { line: 350, col: 54 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 356, col: 9 }, endLoc: { line: 356, col: 29 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 98 }, 99: { name: "anonymous_17", count: 0, regions: { 0: { startLoc: { line: 361, col: 13 }, endLoc: { line: 368, col: 8 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 362, col: 32 }, endLoc: { line: 364, col: 10 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 362, col: 13 }, endLoc: { line: 362, col: 30 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 99 }, 100: { name: "anonymous_18", count: 0, regions: { 0: { startLoc: { line: 365, col: 28 }, endLoc: { line: 367, col: 10 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 366, col: 11 }, endLoc: { line: 367, col: 10 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 100 }, 101: { name: "setRememberConnectionPassword", count: 0, regions: { 0: { startLoc: { line: 371, col: 3 }, endLoc: { line: 373, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 372, col: 5 }, endLoc: { line: 373, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 101 }, 102: { name: "saveConnectionProfile", count: 0, regions: { 0: { startLoc: { line: 375, col: 3 }, endLoc: { line: 395, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 377, col: 31 }, endLoc: { line: 379, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 381, col: 5 }, endLoc: { line: 395, col: 4 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 386, col: 9 }, endLoc: { line: 394, col: 8 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 377, col: 9 }, endLoc: { line: 377, col: 29 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 102 }, 103: { name: "anonymous_19", count: 0, regions: { 0: { startLoc: { line: 382, col: 13 }, endLoc: { line: 394, col: 8 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 383, col: 32 }, endLoc: { line: 385, col: 10 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 383, col: 13 }, endLoc: { line: 383, col: 30 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 103 }, 104: { name: "anonymous_20", count: 0, regions: { 0: { startLoc: { line: 386, col: 28 }, endLoc: { line: 393, col: 10 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 390, col: 34 }, endLoc: { line: 392, col: 12 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 390, col: 15 }, endLoc: { line: 390, col: 32 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 104 }, 105: { name: "persistPendingConnectionProfile", count: 0, regions: { 0: { startLoc: { line: 397, col: 3 }, endLoc: { line: 404, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 398, col: 53 }, endLoc: { line: 400, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 401, col: 5 }, endLoc: { line: 404, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 398, col: 9 }, endLoc: { line: 398, col: 51 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 105 }, 106: { name: "clearSelectedConnectionPassword", count: 0, regions: { 0: { startLoc: { line: 406, col: 3 }, endLoc: { line: 435, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 409, col: 56 }, endLoc: { line: 412, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 415, col: 31 }, endLoc: { line: 418, col: 6 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 420, col: 5 }, endLoc: { line: 435, col: 4 }, count: 0, ignored: 0 }, 4: { startLoc: { line: 428, col: 9 }, endLoc: { line: 434, col: 8 }, count: 0, ignored: 0 }, 5: { startLoc: { line: 426, col: 11 }, endLoc: { line: 427, col: 10 }, count: 0, ignored: 0 }, 6: { startLoc: { line: 430, col: 11 }, endLoc: { line: 433, col: 10 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 409, col: 9 }, endLoc: { line: 409, col: 54 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 415, col: 9 }, endLoc: { line: 415, col: 29 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 106 }, 107: { name: "anonymous_21", count: 0, regions: { 0: { startLoc: { line: 421, col: 13 }, endLoc: { line: 434, col: 8 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 422, col: 32 }, endLoc: { line: 427, col: 10 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 422, col: 13 }, endLoc: { line: 422, col: 30 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 107 }, 108: { name: "anonymous_22", count: 0, regions: { 0: { startLoc: { line: 423, col: 30 }, endLoc: { line: 425, col: 12 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 424, col: 13 }, endLoc: { line: 425, col: 12 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 108 }, 109: { name: "anonymous_23", count: 0, regions: { 0: { startLoc: { line: 428, col: 28 }, endLoc: { line: 433, col: 10 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 109 }, 110: { name: "clearConnectionFeedback", count: 0, regions: { 0: { startLoc: { line: 437, col: 3 }, endLoc: { line: 440, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 438, col: 5 }, endLoc: { line: 440, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 110 }, 111: { name: "setConnectionFeedback", count: 0, regions: { 0: { startLoc: { line: 442, col: 3 }, endLoc: { line: 445, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 443, col: 5 }, endLoc: { line: 445, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 111 }, 112: { name: "connectionFailureMessage", count: 0, regions: { 0: { startLoc: { line: 447, col: 3 }, endLoc: { line: 453, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 449, col: 30 }, endLoc: { line: 451, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 452, col: 5 }, endLoc: { line: 453, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 449, col: 9 }, endLoc: { line: 449, col: 28 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 112 }, 113: { name: "errorMessage", count: 0, regions: { 0: { startLoc: { line: 455, col: 3 }, endLoc: { line: 461, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 456, col: 35 }, endLoc: { line: 458, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 459, col: 5 }, endLoc: { line: 461, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 456, col: 9 }, endLoc: { line: 456, col: 33 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 460, col: 12 }, endLoc: { line: 460, col: 74 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 113 }, 114: { name: "validateConnectionForm", count: 0, regions: { 0: { startLoc: { line: 463, col: 3 }, endLoc: { line: 465, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 464, col: 5 }, endLoc: { line: 465, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 114 }, 115: { name: "xrdpServerDisplayMessages", count: 0, regions: { 0: { startLoc: { line: 467, col: 3 }, endLoc: { line: 476, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 468, col: 5 }, endLoc: { line: 476, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 115 }, 116: { name: "registerNativeCallbacks", count: 0, regions: { 0: { startLoc: { line: 478, col: 3 }, endLoc: { line: 540, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 479, col: 35 }, endLoc: { line: 481, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 483, col: 9 }, endLoc: { line: 537, col: 6 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 537, col: 7 }, endLoc: { line: 539, col: 6 }, count: 0, ignored: 0 }, 4: { startLoc: { line: 531, col: 31 }, endLoc: { line: 534, col: 8 }, count: 0, ignored: 0 }, 5: { startLoc: { line: 535, col: 7 }, endLoc: { line: 537, col: 6 }, count: 0, ignored: 0 }, 6: { startLoc: { line: 492, col: 15 }, endLoc: { line: 493, col: 14 }, count: 0, ignored: 0 }, 7: { startLoc: { line: 497, col: 15 }, endLoc: { line: 501, col: 14 }, count: 0, ignored: 0 }, 8: { startLoc: { line: 504, col: 15 }, endLoc: { line: 507, col: 14 }, count: 0, ignored: 0 }, 9: { startLoc: { line: 509, col: 15 }, endLoc: { line: 511, col: 14 }, count: 0, ignored: 0 }, 10: { startLoc: { line: 522, col: 13 }, endLoc: { line: 528, col: 12 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 479, col: 9 }, endLoc: { line: 479, col: 33 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 531, col: 11 }, endLoc: { line: 531, col: 29 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 116 }, 117: { name: "anonymous_24", count: 0, regions: { 0: { startLoc: { line: 485, col: 18 }, endLoc: { line: 516, col: 10 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 486, col: 11 }, endLoc: { line: 516, col: 10 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 117 }, 118: { name: "anonymous_25", count: 0, regions: { 0: { startLoc: { line: 486, col: 30 }, endLoc: { line: 515, col: 12 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 488, col: 105 }, endLoc: { line: 493, col: 14 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 489, col: 33 }, endLoc: { line: 491, col: 16 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 495, col: 49 }, endLoc: { line: 501, col: 14 }, count: 0, ignored: 0 }, 4: { startLoc: { line: 502, col: 37 }, endLoc: { line: 507, col: 14 }, count: 0, ignored: 0 }, 5: { startLoc: { line: 507, col: 20 }, endLoc: { line: 514, col: 14 }, count: 0, ignored: 0 }, 6: { startLoc: { line: 507, col: 118 }, endLoc: { line: 511, col: 14 }, count: 0, ignored: 0 }, 7: { startLoc: { line: 511, col: 20 }, endLoc: { line: 514, col: 14 }, count: 0, ignored: 0 }, 8: { startLoc: { line: 512, col: 81 }, endLoc: { line: 514, col: 14 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 488, col: 17 }, endLoc: { line: 488, col: 103 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 489, col: 19 }, endLoc: { line: 489, col: 31 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 2: { startLoc: { line: 495, col: 17 }, endLoc: { line: 495, col: 47 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 3: { startLoc: { line: 502, col: 17 }, endLoc: { line: 502, col: 35 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 4: { startLoc: { line: 507, col: 24 }, endLoc: { line: 507, col: 116 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 5: { startLoc: { line: 511, col: 24 }, endLoc: { line: 512, col: 79 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 118 }, 119: { name: "anonymous_26", count: 0, regions: { 0: { startLoc: { line: 517, col: 18 }, endLoc: { line: 529, col: 10 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 518, col: 11 }, endLoc: { line: 529, col: 10 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 119 }, 120: { name: "anonymous_27", count: 0, regions: { 0: { startLoc: { line: 518, col: 30 }, endLoc: { line: 528, col: 12 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 519, col: 31 }, endLoc: { line: 521, col: 14 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 519, col: 17 }, endLoc: { line: 519, col: 29 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 120 }, 121: { name: "isConnectedNativeState", count: 0, regions: { 0: { startLoc: { line: 542, col: 3 }, endLoc: { line: 544, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 543, col: 5 }, endLoc: { line: 544, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 121 }, 122: { name: "clearSessionNotice", count: 0, regions: { 0: { startLoc: { line: 546, col: 3 }, endLoc: { line: 550, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 547, col: 5 }, endLoc: { line: 550, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 122 }, 123: { name: "updateSessionNoticeForNativeState", count: 0, regions: { 0: { startLoc: { line: 552, col: 3 }, endLoc: { line: 586, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 553, col: 41 }, endLoc: { line: 558, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 561, col: 32 }, endLoc: { line: 565, col: 6 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 567, col: 111 }, endLoc: { line: 571, col: 6 }, count: 0, ignored: 0 }, 4: { startLoc: { line: 573, col: 32 }, endLoc: { line: 576, col: 6 }, count: 0, ignored: 0 }, 5: { startLoc: { line: 576, col: 12 }, endLoc: { line: 585, col: 6 }, count: 0, ignored: 0 }, 6: { startLoc: { line: 576, col: 43 }, endLoc: { line: 579, col: 6 }, count: 0, ignored: 0 }, 7: { startLoc: { line: 579, col: 12 }, endLoc: { line: 585, col: 6 }, count: 0, ignored: 0 }, 8: { startLoc: { line: 579, col: 41 }, endLoc: { line: 582, col: 6 }, count: 0, ignored: 0 }, 9: { startLoc: { line: 582, col: 12 }, endLoc: { line: 585, col: 6 }, count: 0, ignored: 0 }, 10: { startLoc: { line: 582, col: 44 }, endLoc: { line: 585, col: 6 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 553, col: 9 }, endLoc: { line: 553, col: 39 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 561, col: 9 }, endLoc: { line: 561, col: 30 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 2: { startLoc: { line: 567, col: 9 }, endLoc: { line: 567, col: 109 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 3: { startLoc: { line: 573, col: 9 }, endLoc: { line: 573, col: 30 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 4: { startLoc: { line: 576, col: 16 }, endLoc: { line: 576, col: 41 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 5: { startLoc: { line: 579, col: 16 }, endLoc: { line: 579, col: 39 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 6: { startLoc: { line: 582, col: 16 }, endLoc: { line: 582, col: 42 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 123 }, 124: { name: "getAppFilesDir", count: 0, regions: { 0: { startLoc: { line: 588, col: 3 }, endLoc: { line: 594, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 590, col: 27 }, endLoc: { line: 592, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 593, col: 5 }, endLoc: { line: 594, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 590, col: 9 }, endLoc: { line: 590, col: 25 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 124 }, 125: { name: "openRemoteFilesDirectoryFromSettings", count: 0, regions: { 0: { startLoc: { line: 596, col: 3 }, endLoc: { line: 603, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 597, col: 5 }, endLoc: { line: 603, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 125 }, 126: { name: "anonymous_28", count: 0, regions: { 0: { startLoc: { line: 600, col: 14 }, endLoc: { line: 602, col: 8 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 601, col: 9 }, endLoc: { line: 602, col: 8 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 126 }, 127: { name: "openSettingsFromHome", count: 0, regions: { 0: { startLoc: { line: 605, col: 3 }, endLoc: { line: 618, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 606, col: 46 }, endLoc: { line: 613, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 613, col: 12 }, endLoc: { line: 616, col: 6 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 617, col: 5 }, endLoc: { line: 618, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 606, col: 9 }, endLoc: { line: 606, col: 44 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 611, col: 38 }, endLoc: { line: 612, col: 31 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 127 }, 128: { name: "registerNativePermissionCallbacks", count: 0, regions: { 0: { startLoc: { line: 620, col: 3 }, endLoc: { line: 622, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 621, col: 5 }, endLoc: { line: 622, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 128 }, 129: { name: "refreshScreenRecordingPermissionState", count: 0, regions: { 0: { startLoc: { line: 624, col: 3 }, endLoc: { line: 626, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 625, col: 5 }, endLoc: { line: 626, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 129 }, 130: { name: "requestScreenRecordingPermissionFromSettings", count: 0, regions: { 0: { startLoc: { line: 628, col: 3 }, endLoc: { line: 630, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 629, col: 5 }, endLoc: { line: 630, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 130 }, 131: { name: "refreshInputInjectionPermissionState", count: 0, regions: { 0: { startLoc: { line: 632, col: 3 }, endLoc: { line: 634, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 633, col: 5 }, endLoc: { line: 634, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 131 }, 132: { name: "requestInputInjectionPermissionFromSettings", count: 0, regions: { 0: { startLoc: { line: 636, col: 3 }, endLoc: { line: 638, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 637, col: 5 }, endLoc: { line: 638, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 132 }, 133: { name: "refreshXrdpServerDiagnostics", count: 0, regions: { 0: { startLoc: { line: 640, col: 3 }, endLoc: { line: 642, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 641, col: 5 }, endLoc: { line: 642, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 133 }, 134: { name: "refreshRemoteControlReadiness", count: 0, regions: { 0: { startLoc: { line: 644, col: 3 }, endLoc: { line: 646, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 645, col: 5 }, endLoc: { line: 646, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 134 }, 135: { name: "startXrdpServerFromSettings", count: 0, regions: { 0: { startLoc: { line: 648, col: 3 }, endLoc: { line: 650, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 649, col: 5 }, endLoc: { line: 650, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 135 }, 136: { name: "setRemoteAccessCodeGateFromSettings", count: 0, regions: { 0: { startLoc: { line: 652, col: 3 }, endLoc: { line: 654, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 653, col: 5 }, endLoc: { line: 654, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 136 }, 137: { name: "regenerateRemoteAccessCodeFromSettings", count: 0, regions: { 0: { startLoc: { line: 656, col: 3 }, endLoc: { line: 658, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 657, col: 5 }, endLoc: { line: 658, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 137 }, 138: { name: "ensureXrdpServerStarted", count: 0, regions: { 0: { startLoc: { line: 660, col: 3 }, endLoc: { line: 664, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 662, col: 5 }, endLoc: { line: 664, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 138 }, 139: { name: "applyRemoteControlSnapshot", count: 0, regions: { 0: { startLoc: { line: 666, col: 3 }, endLoc: { line: 678, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 667, col: 5 }, endLoc: { line: 678, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 139 }, 140: { name: "connectNative", count: 0, regions: { 0: { startLoc: { line: 680, col: 3 }, endLoc: { line: 717, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 682, col: 25 }, endLoc: { line: 689, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 691, col: 5 }, endLoc: { line: 717, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 682, col: 9 }, endLoc: { line: 682, col: 23 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 140 }, 141: { name: "queueNativeConnect", count: 0, regions: { 0: { startLoc: { line: 719, col: 3 }, endLoc: { line: 724, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 721, col: 5 }, endLoc: { line: 724, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 141 }, 142: { name: "anonymous_29", count: 0, regions: { 0: { startLoc: { line: 721, col: 16 }, endLoc: { line: 723, col: 6 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 722, col: 7 }, endLoc: { line: 723, col: 6 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 142 }, 143: { name: "startNativeConnect", count: 0, regions: { 0: { startLoc: { line: 726, col: 3 }, endLoc: { line: 761, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 728, col: 9 }, endLoc: { line: 752, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 752, col: 7 }, endLoc: { line: 760, col: 6 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 737, col: 22 }, endLoc: { line: 744, col: 8 }, count: 0, ignored: 0 }, 4: { startLoc: { line: 744, col: 14 }, endLoc: { line: 751, col: 8 }, count: 0, ignored: 0 }, 5: { startLoc: { line: 739, col: 33 }, endLoc: { line: 743, col: 10 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 737, col: 11 }, endLoc: { line: 737, col: 20 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 739, col: 13 }, endLoc: { line: 739, col: 31 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 143 }, 144: { name: "setSessionVisible", count: 0, regions: { 0: { startLoc: { line: 763, col: 3 }, endLoc: { line: 771, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 764, col: 61 }, endLoc: { line: 766, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 768, col: 19 }, endLoc: { line: 770, col: 6 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 764, col: 9 }, endLoc: { line: 764, col: 59 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 768, col: 9 }, endLoc: { line: 768, col: 17 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 144 }, 145: { name: "isConnected", count: 0, regions: { 0: { startLoc: { line: 773, col: 3 }, endLoc: { line: 775, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 774, col: 5 }, endLoc: { line: 775, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 145 }, 146: { name: "focusRemoteSurface", count: 0, regions: { 0: { startLoc: { line: 777, col: 3 }, endLoc: { line: 787, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 778, col: 30 }, endLoc: { line: 780, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 781, col: 5 }, endLoc: { line: 787, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 778, col: 9 }, endLoc: { line: 778, col: 28 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 146 }, 147: { name: "anonymous_30", count: 0, regions: { 0: { startLoc: { line: 781, col: 24 }, endLoc: { line: 786, col: 6 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 783, col: 21 }, endLoc: { line: 785, col: 8 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 783, col: 11 }, endLoc: { line: 783, col: 19 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 147 }, 148: { name: "releaseActiveInput", count: 0, regions: { 0: { startLoc: { line: 789, col: 3 }, endLoc: { line: 798, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 790, col: 9 }, endLoc: { line: 795, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 795, col: 7 }, endLoc: { line: 797, col: 6 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 792, col: 23 }, endLoc: { line: 794, col: 8 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 792, col: 11 }, endLoc: { line: 792, col: 21 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 148 }, 149: { name: "getAppearanceMode", count: 0, regions: { 0: { startLoc: { line: 800, col: 3 }, endLoc: { line: 802, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 801, col: 5 }, endLoc: { line: 802, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 149 }, 150: { name: "isAppDark", count: 0, regions: { 0: { startLoc: { line: 804, col: 3 }, endLoc: { line: 813, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 806, col: 26 }, endLoc: { line: 808, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 809, col: 27 }, endLoc: { line: 811, col: 6 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 812, col: 5 }, endLoc: { line: 813, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 806, col: 9 }, endLoc: { line: 806, col: 24 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 809, col: 9 }, endLoc: { line: 809, col: 25 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 150 }, 151: { name: "initialRender", count: 0, regions: { 0: { startLoc: { line: 815, col: 3 }, endLoc: { line: 933, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 863, col: 13 }, endLoc: { line: 866, col: 12 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 863, col: 13 }, endLoc: { line: 866, col: 12 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 892, col: 13 }, endLoc: { line: 893, col: 12 }, count: 0, ignored: 0 }, 4: { startLoc: { line: 896, col: 13 }, endLoc: { line: 897, col: 12 }, count: 0, ignored: 0 }, 5: { startLoc: { line: 900, col: 13 }, endLoc: { line: 901, col: 12 }, count: 0, ignored: 0 }, 6: { startLoc: { line: 904, col: 13 }, endLoc: { line: 905, col: 12 }, count: 0, ignored: 0 }, 7: { startLoc: { line: 892, col: 13 }, endLoc: { line: 893, col: 12 }, count: 0, ignored: 0 }, 8: { startLoc: { line: 896, col: 13 }, endLoc: { line: 897, col: 12 }, count: 0, ignored: 0 }, 9: { startLoc: { line: 900, col: 13 }, endLoc: { line: 901, col: 12 }, count: 0, ignored: 0 }, 10: { startLoc: { line: 904, col: 13 }, endLoc: { line: 905, col: 12 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 151 }, 152: { name: "anonymous_31", count: 0, regions: { 0: { startLoc: { line: 816, col: 5 }, endLoc: { line: 932, col: 68 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 152 }, 153: { name: "anonymous_32", count: 0, regions: { 0: { startLoc: { line: 817, col: 7 }, endLoc: { line: 928, col: 8 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 817, col: 29 }, endLoc: { line: 832, col: 8 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 832, col: 14 }, endLoc: { line: 928, col: 8 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 832, col: 37 }, endLoc: { line: 868, col: 8 }, count: 0, ignored: 0 }, 4: { startLoc: { line: 868, col: 14 }, endLoc: { line: 928, col: 8 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 817, col: 11 }, endLoc: { line: 817, col: 27 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 832, col: 18 }, endLoc: { line: 832, col: 35 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 153 }, 154: { name: "anonymous_33", count: 0, regions: { 0: { startLoc: { line: 818, col: 9 }, endLoc: { line: 821, col: 54 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 154 }, 155: { name: "anonymous_34", count: 0, regions: { 0: { startLoc: { line: 818, col: 9 }, endLoc: { line: 821, col: 54 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 155 }, 156: { name: "anonymous_35", count: 0, regions: { 0: { startLoc: { line: 822, col: 28 }, endLoc: { line: 822, col: 105 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 156 }, 157: { name: "anonymous_36", count: 0, regions: { 0: { startLoc: { line: 823, col: 28 }, endLoc: { line: 825, col: 12 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 824, col: 13 }, endLoc: { line: 825, col: 12 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 157 }, 158: { name: "anonymous_37", count: 0, regions: { 0: { startLoc: { line: 826, col: 26 }, endLoc: { line: 830, col: 12 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 827, col: 13 }, endLoc: { line: 830, col: 12 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 158 }, 159: { name: "anonymous_38", count: 0, regions: { 0: { startLoc: { line: 827, col: 32 }, endLoc: { line: 829, col: 14 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 828, col: 15 }, endLoc: { line: 829, col: 14 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 159 }, 160: { name: "anonymous_40", count: 0, regions: { 0: { startLoc: { line: 833, col: 9 }, endLoc: { line: 848, col: 76 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 160 }, 161: { name: "anonymous_41", count: 0, regions: { 0: { startLoc: { line: 833, col: 9 }, endLoc: { line: 848, col: 76 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 161 }, 162: { name: "anonymous_42", count: 0, regions: { 0: { startLoc: { line: 849, col: 41 }, endLoc: { line: 849, col: 120 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 162 }, 163: { name: "anonymous_43", count: 0, regions: { 0: { startLoc: { line: 850, col: 41 }, endLoc: { line: 850, col: 100 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 163 }, 164: { name: "anonymous_44", count: 0, regions: { 0: { startLoc: { line: 851, col: 47 }, endLoc: { line: 851, col: 122 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 164 }, 165: { name: "anonymous_45", count: 0, regions: { 0: { startLoc: { line: 852, col: 47 }, endLoc: { line: 852, col: 106 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 165 }, 166: { name: "anonymous_46", count: 0, regions: { 0: { startLoc: { line: 853, col: 46 }, endLoc: { line: 854, col: 63 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 166 }, 167: { name: "anonymous_47", count: 0, regions: { 0: { startLoc: { line: 855, col: 46 }, endLoc: { line: 855, col: 104 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 167 }, 168: { name: "anonymous_48", count: 0, regions: { 0: { startLoc: { line: 856, col: 38 }, endLoc: { line: 856, col: 97 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 168 }, 169: { name: "anonymous_49", count: 0, regions: { 0: { startLoc: { line: 857, col: 30 }, endLoc: { line: 857, col: 97 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 169 }, 170: { name: "anonymous_50", count: 0, regions: { 0: { startLoc: { line: 858, col: 39 }, endLoc: { line: 858, col: 94 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 170 }, 171: { name: "anonymous_51", count: 0, regions: { 0: { startLoc: { line: 859, col: 20 }, endLoc: { line: 866, col: 12 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 860, col: 54 }, endLoc: { line: 862, col: 14 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 860, col: 17 }, endLoc: { line: 860, col: 52 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 171 }, 172: { name: "anonymous_53", count: 0, regions: { 0: { startLoc: { line: 869, col: 9 }, endLoc: { line: 889, col: 76 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 172 }, 173: { name: "anonymous_54", count: 0, regions: { 0: { startLoc: { line: 869, col: 9 }, endLoc: { line: 889, col: 76 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 173 }, 174: { name: "anonymous_55", count: 0, regions: { 0: { startLoc: { line: 890, col: 25 }, endLoc: { line: 893, col: 12 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 174 }, 175: { name: "anonymous_56", count: 0, regions: { 0: { startLoc: { line: 894, col: 25 }, endLoc: { line: 897, col: 12 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 175 }, 176: { name: "anonymous_57", count: 0, regions: { 0: { startLoc: { line: 898, col: 29 }, endLoc: { line: 901, col: 12 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 176 }, 177: { name: "anonymous_58", count: 0, regions: { 0: { startLoc: { line: 902, col: 29 }, endLoc: { line: 905, col: 12 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 177 }, 178: { name: "anonymous_59", count: 0, regions: { 0: { startLoc: { line: 906, col: 28 }, endLoc: { line: 908, col: 12 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 907, col: 13 }, endLoc: { line: 908, col: 12 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 178 }, 179: { name: "anonymous_60", count: 0, regions: { 0: { startLoc: { line: 909, col: 25 }, endLoc: { line: 911, col: 12 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 910, col: 13 }, endLoc: { line: 911, col: 12 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 179 }, 180: { name: "anonymous_61", count: 0, regions: { 0: { startLoc: { line: 912, col: 28 }, endLoc: { line: 914, col: 12 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 913, col: 13 }, endLoc: { line: 914, col: 12 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 180 }, 181: { name: "anonymous_62", count: 0, regions: { 0: { startLoc: { line: 915, col: 37 }, endLoc: { line: 917, col: 12 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 916, col: 13 }, endLoc: { line: 917, col: 12 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 181 }, 182: { name: "anonymous_63", count: 0, regions: { 0: { startLoc: { line: 918, col: 28 }, endLoc: { line: 920, col: 12 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 919, col: 13 }, endLoc: { line: 920, col: 12 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 182 }, 183: { name: "anonymous_64", count: 0, regions: { 0: { startLoc: { line: 921, col: 22 }, endLoc: { line: 923, col: 12 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 922, col: 13 }, endLoc: { line: 923, col: 12 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 183 }, 184: { name: "anonymous_65", count: 0, regions: { 0: { startLoc: { line: 924, col: 27 }, endLoc: { line: 926, col: 12 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 925, col: 13 }, endLoc: { line: 926, col: 12 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 184 } }, exeLine: { 0: 1, 1: 2, 2: 3, 3: 4, 4: 5, 5: 6, 6: 7, 7: 8, 8: 9, 9: 10, 10: 11, 11: 12, 12: 13, 13: 14, 14: 15, 15: 16, 16: 17, 17: 18, 18: 19, 19: 20, 20: 21, 21: 22, 22: 23, 23: 24, 24: 25, 25: 27, 26: 28, 27: 29, 28: 31, 29: 32, 30: 33, 31: 34, 32: 35, 33: 36, 34: 37, 35: 38, 36: 39, 37: 40, 38: 41, 39: 42, 40: 44, 41: 45, 42: 46, 43: 47, 44: 48, 45: 49, 46: 50, 47: 51, 48: 52, 49: 54, 50: 55, 51: 56, 52: 57, 53: 58, 54: 59, 55: 60, 56: 61, 57: 62, 58: 63, 59: 64, 60: 65, 61: 66, 62: 67, 63: 68, 64: 69, 65: 70, 66: 71, 67: 72, 68: 73, 69: 74, 70: 75, 71: 76, 72: 77, 73: 78, 74: 79, 75: 80, 76: 81, 77: 82, 78: 83, 79: 84, 80: 85, 81: 86, 82: 87, 83: 88, 84: 89, 85: 90, 86: 91, 87: 92, 88: 95, 89: 96, 90: 97, 91: 98, 92: 101, 93: 102, 94: 103, 95: 104, 96: 105, 97: 106, 98: 107, 99: 108, 100: 109, 101: 113, 102: 114, 103: 115, 104: 117, 105: 118, 106: 122, 107: 123, 108: 126, 109: 127, 110: 128, 111: 131, 112: 132, 113: 133, 114: 134, 115: 135, 116: 136, 117: 138, 118: 139, 119: 140, 120: 141, 121: 145, 122: 146, 123: 147, 124: 149, 125: 150, 126: 151, 127: 152, 128: 153, 129: 154, 130: 158, 131: 159, 132: 160, 133: 161, 134: 162, 135: 164, 136: 165, 137: 166, 138: 167, 139: 171, 140: 172, 141: 175, 142: 176, 143: 177, 144: 178, 145: 180, 146: 181, 147: 184, 148: 187, 149: 188, 150: 189, 151: 190, 152: 191, 153: 194, 154: 197, 155: 198, 156: 199, 157: 200, 158: 201, 159: 202, 160: 203, 161: 204, 162: 205, 163: 208, 164: 210, 165: 213, 166: 214, 167: 215, 168: 218, 169: 219, 170: 220, 171: 223, 172: 224, 173: 227, 174: 228, 175: 229, 176: 230, 177: 233, 178: 234, 179: 235, 180: 236, 181: 238, 182: 239, 183: 240, 184: 245, 185: 246, 186: 247, 187: 248, 188: 250, 189: 251, 190: 254, 191: 255, 192: 256, 193: 257, 194: 258, 195: 260, 196: 263, 197: 266, 198: 267, 199: 270, 200: 271, 201: 272, 202: 273, 203: 276, 204: 279, 205: 280, 206: 281, 207: 282, 208: 283, 209: 284, 210: 285, 211: 286, 212: 287, 213: 291, 214: 292, 215: 293, 216: 294, 217: 297, 218: 298, 219: 299, 220: 302, 221: 303, 222: 304, 223: 305, 224: 306, 225: 309, 226: 310, 227: 311, 228: 312, 229: 313, 230: 315, 231: 316, 232: 317, 233: 318, 234: 319, 235: 320, 236: 321, 237: 322, 238: 325, 239: 327, 240: 328, 241: 329, 242: 330, 243: 336, 244: 337, 245: 338, 246: 339, 247: 340, 248: 341, 249: 342, 250: 343, 251: 344, 252: 345, 253: 346, 254: 349, 255: 350, 256: 351, 257: 352, 258: 355, 259: 356, 260: 357, 261: 360, 262: 361, 263: 362, 264: 363, 265: 365, 266: 366, 267: 371, 268: 372, 269: 375, 270: 376, 271: 377, 272: 378, 273: 381, 274: 382, 275: 383, 276: 384, 277: 386, 278: 387, 279: 388, 280: 389, 281: 390, 282: 391, 283: 397, 284: 398, 285: 399, 286: 401, 287: 402, 288: 403, 289: 406, 290: 407, 291: 408, 292: 409, 293: 410, 294: 411, 295: 414, 296: 415, 297: 416, 298: 417, 299: 420, 300: 421, 301: 422, 302: 423, 303: 424, 304: 426, 305: 428, 306: 429, 307: 430, 308: 431, 309: 432, 310: 437, 311: 438, 312: 439, 313: 442, 314: 443, 315: 444, 316: 447, 317: 448, 318: 449, 319: 450, 320: 452, 321: 455, 322: 456, 323: 457, 324: 459, 325: 460, 326: 463, 327: 464, 328: 467, 329: 468, 330: 469, 331: 470, 332: 471, 333: 472, 334: 473, 335: 474, 336: 478, 337: 479, 338: 480, 339: 483, 340: 484, 341: 485, 342: 486, 343: 487, 344: 488, 345: 489, 346: 490, 347: 492, 348: 494, 349: 495, 350: 496, 351: 497, 352: 498, 353: 499, 354: 500, 355: 502, 356: 503, 357: 504, 358: 505, 359: 506, 360: 507, 361: 508, 362: 509, 363: 510, 364: 511, 365: 512, 366: 513, 367: 517, 368: 518, 369: 519, 370: 520, 371: 522, 372: 523, 373: 524, 374: 525, 375: 526, 376: 527, 377: 531, 378: 532, 379: 533, 380: 535, 381: 536, 382: 537, 383: 538, 384: 542, 385: 543, 386: 546, 387: 547, 388: 548, 389: 549, 390: 552, 391: 553, 392: 554, 393: 555, 394: 556, 395: 557, 396: 560, 397: 561, 398: 562, 399: 563, 400: 564, 401: 567, 402: 568, 403: 569, 404: 570, 405: 573, 406: 574, 407: 575, 408: 576, 409: 577, 410: 578, 411: 579, 412: 580, 413: 581, 414: 582, 415: 583, 416: 584, 417: 588, 418: 589, 419: 590, 420: 591, 421: 593, 422: 596, 423: 597, 424: 598, 425: 600, 426: 601, 427: 605, 428: 606, 429: 607, 430: 608, 431: 609, 432: 610, 433: 611, 434: 612, 435: 613, 436: 614, 437: 615, 438: 617, 439: 620, 440: 621, 441: 624, 442: 625, 443: 628, 444: 629, 445: 632, 446: 633, 447: 636, 448: 637, 449: 640, 450: 641, 451: 644, 452: 645, 453: 648, 454: 649, 455: 652, 456: 653, 457: 656, 458: 657, 459: 660, 460: 661, 461: 662, 462: 663, 463: 666, 464: 667, 465: 668, 466: 669, 467: 670, 468: 671, 469: 672, 470: 673, 471: 674, 472: 675, 473: 676, 474: 677, 475: 680, 476: 681, 477: 682, 478: 683, 479: 684, 480: 685, 481: 686, 482: 687, 483: 688, 484: 691, 485: 692, 486: 693, 487: 694, 488: 695, 489: 696, 490: 697, 491: 698, 492: 699, 493: 700, 494: 701, 495: 702, 496: 703, 497: 704, 498: 705, 499: 706, 500: 707, 501: 709, 502: 710, 503: 711, 504: 712, 505: 713, 506: 714, 507: 715, 508: 716, 509: 719, 510: 720, 511: 721, 512: 722, 513: 723, 514: 726, 515: 727, 516: 728, 517: 729, 518: 730, 519: 731, 520: 732, 521: 733, 522: 734, 523: 735, 524: 737, 525: 738, 526: 739, 527: 740, 528: 741, 529: 742, 530: 744, 531: 745, 532: 746, 533: 747, 534: 748, 535: 749, 536: 750, 537: 752, 538: 753, 539: 754, 540: 755, 541: 756, 542: 757, 543: 758, 544: 759, 545: 763, 546: 764, 547: 765, 548: 767, 549: 768, 550: 769, 551: 773, 552: 774, 553: 777, 554: 778, 555: 779, 556: 781, 557: 782, 558: 783, 559: 784, 560: 789, 561: 790, 562: 791, 563: 792, 564: 793, 565: 795, 566: 796, 567: 800, 568: 801, 569: 804, 570: 805, 571: 806, 572: 807, 573: 809, 574: 810, 575: 812, 576: 815, 577: 816, 578: 817, 579: 818, 580: 819, 581: 820, 582: 821, 583: 822, 584: 823, 585: 824, 586: 826, 587: 827, 588: 828, 589: 832, 590: 833, 591: 834, 592: 835, 593: 836, 594: 837, 595: 838, 596: 839, 597: 840, 598: 841, 599: 842, 600: 843, 601: 844, 602: 845, 603: 846, 604: 847, 605: 848, 606: 849, 607: 850, 608: 851, 609: 852, 610: 853, 611: 854, 612: 855, 613: 856, 614: 857, 615: 858, 616: 859, 617: 860, 618: 861, 619: 863, 620: 864, 621: 865, 622: 868, 623: 869, 624: 870, 625: 871, 626: 872, 627: 873, 628: 874, 629: 875, 630: 876, 631: 877, 632: 878, 633: 879, 634: 880, 635: 881, 636: 882, 637: 883, 638: 884, 639: 885, 640: 886, 641: 887, 642: 888, 643: 889, 644: 890, 645: 891, 646: 892, 647: 894, 648: 895, 649: 896, 650: 898, 651: 899, 652: 900, 653: 902, 654: 903, 655: 904, 656: 906, 657: 907, 658: 909, 659: 910, 660: 912, 661: 913, 662: 915, 663: 916, 664: 918, 665: 919, 666: 921, 667: 922, 668: 924, 669: 925, 670: 930, 671: 931, 672: 932 } });
if (!("finalizeConstruction" in ViewPU.prototype)) {
    Reflect.set(ViewPU.prototype, "finalizeConstruction", () => { });
}
interface MuHubApp_Params {
    host?: string;
    port?: string;
    username?: string;
    password?: string;
    connectionProfiles?: WindowsConnectionProfile[];
    selectedConnectionProfileId?: string;
    rememberConnectionPassword?: boolean;
    connectionProfilePasswordLoading?: boolean;
    certPolicy?: string;
    remoteAccessCode?: string;
    remoteAccessCodeGateEnabled?: boolean;
    screenRecordingPermissionGranted?: boolean;
    screenRecordingPermissionBusy?: boolean;
    inputInjectionPermissionGranted?: boolean;
    inputInjectionPermissionBusy?: boolean;
    xrdpServerRunning?: boolean;
    xrdpServerState?: string;
    xrdpServerPort?: number;
    xrdpServerMessage?: string;
    xrdpServerBusy?: boolean;
    showSession?: boolean;
    showSettings?: boolean;
    settingsInitialPageName?: string;
    settingsRemoteControlSection?: string;
    remoteLoginWaiting?: boolean;
    sessionNoticeTitle?: string;
    sessionNoticeSubtitle?: string;
    connectionFeedbackText?: string;
    connectionFeedbackTone?: SettingsStatusTone;
    layoutMode?: LayoutMode;
    deviceCapabilities?: DeviceCapabilitySnapshot;
    systemDark?: boolean;
    appearanceMode?: string;
    callbacksRegistered?: boolean;
    rdpClientController?: RdpClientController;
    rdpSurfaceContentHost?: RdpSurfaceContentHost;
    permissionManager?: RdpPermissionManager | null;
    permissionRequestCoordinator?: RdpPermissionRequestCoordinator | null;
    remoteControlCoordinator?: RemoteControlCoordinator | null;
    connectionProfileCoordinator?: WindowsConnectionProfileCoordinator | null;
    connectionProfilesLoaded?: boolean;
    connectionProfileSelectionGeneration?: number;
    pendingConnectionProfileSave?: WindowsConnectionSaveInput | null;
    lastConnectionErrorMessage?: string;
    windowLayoutObserverRegistered?: boolean;
    windowSizeLayoutBreakpointCallback?;
}
import type common from "@ohos:app.ability.common";
import type { NodeContent } from "@ohos:arkui.node";
import type uiObserver from "@ohos:arkui.observer";
import { LayoutMode, layoutModeForWidthBreakpoint } from "@normalized:N&&&common/src/main/ets/adaptive/WindowLayoutPolicy&";
import { currentDeviceCapabilities, hasRemoteControlServer } from "@normalized:N&&&common/src/main/ets/capability/DeviceCapabilityPolicy&";
import type { DeviceCapabilitySnapshot } from "@normalized:N&&&common/src/main/ets/capability/DeviceCapabilityPolicy&";
import { REMOTE_SURFACE_FOCUS_ID } from "@normalized:N&&&common/src/main/ets/rdp/RdpConstants&";
import { RdpClientController } from "@normalized:N&&&common/src/main/ets/rdp/RdpClientController&";
import { RdpConnectionValidator } from "@normalized:N&&&common/src/main/ets/rdp/RdpConnectionValidator&";
import type { ConnectionValidationResult } from "@normalized:N&&&common/src/main/ets/rdp/RdpConnectionValidator&";
import { RdpLogger } from "@normalized:N&&&common/src/main/ets/rdp/RdpLogger&";
import { RdpPermissionRequestCoordinator } from "@normalized:N&&&common/src/main/ets/rdp/RdpPermissionRequestCoordinator&";
import { RdpPermissionCallbackDelegate, RdpPermissionManager } from "@normalized:N&&&common/src/main/ets/rdp/RdpPermissions&";
import { RemoteControlCoordinator } from "@normalized:N&&&common/src/main/ets/rdp/RemoteControlCoordinator&";
import type { RemoteControlSnapshot } from "@normalized:N&&&common/src/main/ets/rdp/RemoteControlCoordinator&";
import { RemoteFilesDirectory } from "@normalized:N&&&common/src/main/ets/rdp/RemoteFilesDirectory&";
import { RdpSurfaceContentHost } from "@normalized:N&&&common/src/main/ets/rdp/RdpSurfaceContentHost&";
import type { XrdpServerDisplayMessages, XrdpServerStatus } from '../rdp/XrdpServerController';
import type { WindowsConnectionProfile, WindowsConnectionSaveInput, WindowsConnectionSnapshot } from '../rdp/WindowsConnectionStore';
import { WindowsConnectionProfileCoordinator } from "@normalized:N&&&common/src/main/ets/rdp/WindowsConnectionProfileCoordinator&";
import { HomeConnectionValidation } from "@normalized:N&&&common/src/main/ets/components/home/HomeConnectionValidation&";
import { HomePage } from "@normalized:N&&&common/src/main/ets/components/home/HomePage&";
import { HomeText } from "@normalized:N&&&common/src/main/ets/components/home/HomeText&";
import { RdpSessionPage } from "@normalized:N&&&common/src/main/ets/components/session/RdpSessionPage&";
import { SettingsPage } from "@normalized:N&&&common/src/main/ets/components/SettingsPage&";
import { SettingsRoute, SettingsText } from "@normalized:N&&&common/src/main/ets/components/settings/SettingsConstants&";
import { SettingsTheme } from "@normalized:N&&&common/src/main/ets/components/settings/SettingsUi&";
import type { SettingsAppearanceMode, SettingsStatusTone } from "@normalized:N&&&common/src/main/ets/components/settings/SettingsUi&";
export class MuHubApp extends ViewPU {
    constructor(parent, params, __localStorage, elmtId = -1, paramsLambda = undefined, extraInfo) {
        super(parent, __localStorage, elmtId, extraInfo);
        if (typeof paramsLambda === "function") {
            this.paramsGenerator_ = paramsLambda;
        }
        this.__host = new ObservedPropertySimplePU('', this, "host");
        this.__port = new ObservedPropertySimplePU('3389', this, "port");
        this.__username = new ObservedPropertySimplePU('', this, "username");
        this.__password = new ObservedPropertySimplePU('', this, "password");
        this.__connectionProfiles = new ObservedPropertyObjectPU([], this, "connectionProfiles");
        this.__selectedConnectionProfileId = new ObservedPropertySimplePU('', this, "selectedConnectionProfileId");
        this.__rememberConnectionPassword = new ObservedPropertySimplePU(false, this, "rememberConnectionPassword");
        this.__connectionProfilePasswordLoading = new ObservedPropertySimplePU(false
        // Temporarily pin TOFU certificate policy; restore policy selector when strict validation is needed.
        , this, "connectionProfilePasswordLoading");
        this.certPolicy = 'tofu';
        this.__remoteAccessCode = new ObservedPropertySimplePU('000000', this, "remoteAccessCode");
        this.__remoteAccessCodeGateEnabled = new ObservedPropertySimplePU(false, this, "remoteAccessCodeGateEnabled");
        this.__screenRecordingPermissionGranted = new ObservedPropertySimplePU(false, this, "screenRecordingPermissionGranted");
        this.__screenRecordingPermissionBusy = new ObservedPropertySimplePU(false, this, "screenRecordingPermissionBusy");
        this.__inputInjectionPermissionGranted = new ObservedPropertySimplePU(false, this, "inputInjectionPermissionGranted");
        this.__inputInjectionPermissionBusy = new ObservedPropertySimplePU(false, this, "inputInjectionPermissionBusy");
        this.__xrdpServerRunning = new ObservedPropertySimplePU(false, this, "xrdpServerRunning");
        this.__xrdpServerState = new ObservedPropertySimplePU('Stopped', this, "xrdpServerState");
        this.__xrdpServerPort = new ObservedPropertySimplePU(3390, this, "xrdpServerPort");
        this.__xrdpServerMessage = new ObservedPropertySimplePU('', this, "xrdpServerMessage");
        this.__xrdpServerBusy = new ObservedPropertySimplePU(false, this, "xrdpServerBusy");
        this.__showSession = new ObservedPropertySimplePU(false, this, "showSession");
        this.__showSettings = new ObservedPropertySimplePU(false, this, "showSettings");
        this.__settingsInitialPageName = new ObservedPropertySimplePU(SettingsRoute.SETTINGS, this, "settingsInitialPageName");
        this.__settingsRemoteControlSection = new ObservedPropertySimplePU('', this, "settingsRemoteControlSection");
        this.__remoteLoginWaiting = new ObservedPropertySimplePU(false, this, "remoteLoginWaiting");
        this.__sessionNoticeTitle = new ObservedPropertySimplePU('', this, "sessionNoticeTitle");
        this.__sessionNoticeSubtitle = new ObservedPropertySimplePU('', this, "sessionNoticeSubtitle");
        this.__connectionFeedbackText = new ObservedPropertySimplePU('', this, "connectionFeedbackText");
        this.__connectionFeedbackTone = new ObservedPropertySimplePU('neutral', this, "connectionFeedbackTone");
        this.__layoutMode = new ObservedPropertySimplePU(LayoutMode.COMPACT, this, "layoutMode");
        this.deviceCapabilities = currentDeviceCapabilities();
        this.__systemDark = this.createStorageLink('settingsSystemDark', false, "systemDark");
        this.__appearanceMode = this.createStorageLink('settingsAppearanceMode', 'system', "appearanceMode");
        this.callbacksRegistered = false;
        this.rdpClientController = new RdpClientController();
        this.rdpSurfaceContentHost = new RdpSurfaceContentHost();
        this.permissionManager = null;
        this.permissionRequestCoordinator = null;
        this.remoteControlCoordinator = null;
        this.connectionProfileCoordinator = null;
        this.connectionProfilesLoaded = false;
        this.connectionProfileSelectionGeneration = 0;
        this.pendingConnectionProfileSave = null;
        this.lastConnectionErrorMessage = '';
        this.windowLayoutObserverRegistered = false;
        this.windowSizeLayoutBreakpointCallback = (info: uiObserver.WindowSizeLayoutBreakpointInfo): void => {
            bjccovmshb1hr4.instrumentFunction(0);
            bjccovmshb1hr4.instrumentRegion(0, 1);
            this.layoutMode = layoutModeForWidthBreakpoint(info.widthBreakpoint);
        };
        this.setInitiallyProvidedValue(params);
        this.finalizeConstruction();
    }
    setInitiallyProvidedValue(params: MuHubApp_Params) {
        if (params.host !== undefined) {
            this.host = params.host;
        }
        if (params.port !== undefined) {
            this.port = params.port;
        }
        if (params.username !== undefined) {
            this.username = params.username;
        }
        if (params.password !== undefined) {
            this.password = params.password;
        }
        if (params.connectionProfiles !== undefined) {
            this.connectionProfiles = params.connectionProfiles;
        }
        if (params.selectedConnectionProfileId !== undefined) {
            this.selectedConnectionProfileId = params.selectedConnectionProfileId;
        }
        if (params.rememberConnectionPassword !== undefined) {
            this.rememberConnectionPassword = params.rememberConnectionPassword;
        }
        if (params.connectionProfilePasswordLoading !== undefined) {
            this.connectionProfilePasswordLoading = params.connectionProfilePasswordLoading;
        }
        if (params.certPolicy !== undefined) {
            this.certPolicy = params.certPolicy;
        }
        if (params.remoteAccessCode !== undefined) {
            this.remoteAccessCode = params.remoteAccessCode;
        }
        if (params.remoteAccessCodeGateEnabled !== undefined) {
            this.remoteAccessCodeGateEnabled = params.remoteAccessCodeGateEnabled;
        }
        if (params.screenRecordingPermissionGranted !== undefined) {
            this.screenRecordingPermissionGranted = params.screenRecordingPermissionGranted;
        }
        if (params.screenRecordingPermissionBusy !== undefined) {
            this.screenRecordingPermissionBusy = params.screenRecordingPermissionBusy;
        }
        if (params.inputInjectionPermissionGranted !== undefined) {
            this.inputInjectionPermissionGranted = params.inputInjectionPermissionGranted;
        }
        if (params.inputInjectionPermissionBusy !== undefined) {
            this.inputInjectionPermissionBusy = params.inputInjectionPermissionBusy;
        }
        if (params.xrdpServerRunning !== undefined) {
            this.xrdpServerRunning = params.xrdpServerRunning;
        }
        if (params.xrdpServerState !== undefined) {
            this.xrdpServerState = params.xrdpServerState;
        }
        if (params.xrdpServerPort !== undefined) {
            this.xrdpServerPort = params.xrdpServerPort;
        }
        if (params.xrdpServerMessage !== undefined) {
            this.xrdpServerMessage = params.xrdpServerMessage;
        }
        if (params.xrdpServerBusy !== undefined) {
            this.xrdpServerBusy = params.xrdpServerBusy;
        }
        if (params.showSession !== undefined) {
            this.showSession = params.showSession;
        }
        if (params.showSettings !== undefined) {
            this.showSettings = params.showSettings;
        }
        if (params.settingsInitialPageName !== undefined) {
            this.settingsInitialPageName = params.settingsInitialPageName;
        }
        if (params.settingsRemoteControlSection !== undefined) {
            this.settingsRemoteControlSection = params.settingsRemoteControlSection;
        }
        if (params.remoteLoginWaiting !== undefined) {
            this.remoteLoginWaiting = params.remoteLoginWaiting;
        }
        if (params.sessionNoticeTitle !== undefined) {
            this.sessionNoticeTitle = params.sessionNoticeTitle;
        }
        if (params.sessionNoticeSubtitle !== undefined) {
            this.sessionNoticeSubtitle = params.sessionNoticeSubtitle;
        }
        if (params.connectionFeedbackText !== undefined) {
            this.connectionFeedbackText = params.connectionFeedbackText;
        }
        if (params.connectionFeedbackTone !== undefined) {
            this.connectionFeedbackTone = params.connectionFeedbackTone;
        }
        if (params.layoutMode !== undefined) {
            this.layoutMode = params.layoutMode;
        }
        if (params.deviceCapabilities !== undefined) {
            this.deviceCapabilities = params.deviceCapabilities;
        }
        if (params.callbacksRegistered !== undefined) {
            this.callbacksRegistered = params.callbacksRegistered;
        }
        if (params.rdpClientController !== undefined) {
            this.rdpClientController = params.rdpClientController;
        }
        if (params.rdpSurfaceContentHost !== undefined) {
            this.rdpSurfaceContentHost = params.rdpSurfaceContentHost;
        }
        if (params.permissionManager !== undefined) {
            this.permissionManager = params.permissionManager;
        }
        if (params.permissionRequestCoordinator !== undefined) {
            this.permissionRequestCoordinator = params.permissionRequestCoordinator;
        }
        if (params.remoteControlCoordinator !== undefined) {
            this.remoteControlCoordinator = params.remoteControlCoordinator;
        }
        if (params.connectionProfileCoordinator !== undefined) {
            this.connectionProfileCoordinator = params.connectionProfileCoordinator;
        }
        if (params.connectionProfilesLoaded !== undefined) {
            this.connectionProfilesLoaded = params.connectionProfilesLoaded;
        }
        if (params.connectionProfileSelectionGeneration !== undefined) {
            this.connectionProfileSelectionGeneration = params.connectionProfileSelectionGeneration;
        }
        if (params.pendingConnectionProfileSave !== undefined) {
            this.pendingConnectionProfileSave = params.pendingConnectionProfileSave;
        }
        if (params.lastConnectionErrorMessage !== undefined) {
            this.lastConnectionErrorMessage = params.lastConnectionErrorMessage;
        }
        if (params.windowLayoutObserverRegistered !== undefined) {
            this.windowLayoutObserverRegistered = params.windowLayoutObserverRegistered;
        }
        if (params.windowSizeLayoutBreakpointCallback !== undefined) {
            this.windowSizeLayoutBreakpointCallback = params.windowSizeLayoutBreakpointCallback;
        }
    }
    updateStateVars(params: MuHubApp_Params) {
    }
    purgeVariableDependenciesOnElmtId(rmElmtId) {
        this.__host.purgeDependencyOnElmtId(rmElmtId);
        this.__port.purgeDependencyOnElmtId(rmElmtId);
        this.__username.purgeDependencyOnElmtId(rmElmtId);
        this.__password.purgeDependencyOnElmtId(rmElmtId);
        this.__connectionProfiles.purgeDependencyOnElmtId(rmElmtId);
        this.__selectedConnectionProfileId.purgeDependencyOnElmtId(rmElmtId);
        this.__rememberConnectionPassword.purgeDependencyOnElmtId(rmElmtId);
        this.__connectionProfilePasswordLoading.purgeDependencyOnElmtId(rmElmtId);
        this.__remoteAccessCode.purgeDependencyOnElmtId(rmElmtId);
        this.__remoteAccessCodeGateEnabled.purgeDependencyOnElmtId(rmElmtId);
        this.__screenRecordingPermissionGranted.purgeDependencyOnElmtId(rmElmtId);
        this.__screenRecordingPermissionBusy.purgeDependencyOnElmtId(rmElmtId);
        this.__inputInjectionPermissionGranted.purgeDependencyOnElmtId(rmElmtId);
        this.__inputInjectionPermissionBusy.purgeDependencyOnElmtId(rmElmtId);
        this.__xrdpServerRunning.purgeDependencyOnElmtId(rmElmtId);
        this.__xrdpServerState.purgeDependencyOnElmtId(rmElmtId);
        this.__xrdpServerPort.purgeDependencyOnElmtId(rmElmtId);
        this.__xrdpServerMessage.purgeDependencyOnElmtId(rmElmtId);
        this.__xrdpServerBusy.purgeDependencyOnElmtId(rmElmtId);
        this.__showSession.purgeDependencyOnElmtId(rmElmtId);
        this.__showSettings.purgeDependencyOnElmtId(rmElmtId);
        this.__settingsInitialPageName.purgeDependencyOnElmtId(rmElmtId);
        this.__settingsRemoteControlSection.purgeDependencyOnElmtId(rmElmtId);
        this.__remoteLoginWaiting.purgeDependencyOnElmtId(rmElmtId);
        this.__sessionNoticeTitle.purgeDependencyOnElmtId(rmElmtId);
        this.__sessionNoticeSubtitle.purgeDependencyOnElmtId(rmElmtId);
        this.__connectionFeedbackText.purgeDependencyOnElmtId(rmElmtId);
        this.__connectionFeedbackTone.purgeDependencyOnElmtId(rmElmtId);
        this.__layoutMode.purgeDependencyOnElmtId(rmElmtId);
        this.__systemDark.purgeDependencyOnElmtId(rmElmtId);
        this.__appearanceMode.purgeDependencyOnElmtId(rmElmtId);
    }
    aboutToBeDeleted() {
        this.__host.aboutToBeDeleted();
        this.__port.aboutToBeDeleted();
        this.__username.aboutToBeDeleted();
        this.__password.aboutToBeDeleted();
        this.__connectionProfiles.aboutToBeDeleted();
        this.__selectedConnectionProfileId.aboutToBeDeleted();
        this.__rememberConnectionPassword.aboutToBeDeleted();
        this.__connectionProfilePasswordLoading.aboutToBeDeleted();
        this.__remoteAccessCode.aboutToBeDeleted();
        this.__remoteAccessCodeGateEnabled.aboutToBeDeleted();
        this.__screenRecordingPermissionGranted.aboutToBeDeleted();
        this.__screenRecordingPermissionBusy.aboutToBeDeleted();
        this.__inputInjectionPermissionGranted.aboutToBeDeleted();
        this.__inputInjectionPermissionBusy.aboutToBeDeleted();
        this.__xrdpServerRunning.aboutToBeDeleted();
        this.__xrdpServerState.aboutToBeDeleted();
        this.__xrdpServerPort.aboutToBeDeleted();
        this.__xrdpServerMessage.aboutToBeDeleted();
        this.__xrdpServerBusy.aboutToBeDeleted();
        this.__showSession.aboutToBeDeleted();
        this.__showSettings.aboutToBeDeleted();
        this.__settingsInitialPageName.aboutToBeDeleted();
        this.__settingsRemoteControlSection.aboutToBeDeleted();
        this.__remoteLoginWaiting.aboutToBeDeleted();
        this.__sessionNoticeTitle.aboutToBeDeleted();
        this.__sessionNoticeSubtitle.aboutToBeDeleted();
        this.__connectionFeedbackText.aboutToBeDeleted();
        this.__connectionFeedbackTone.aboutToBeDeleted();
        this.__layoutMode.aboutToBeDeleted();
        this.__systemDark.aboutToBeDeleted();
        this.__appearanceMode.aboutToBeDeleted();
        SubscriberManager.Get().delete(this.id__());
        this.aboutToBeDeletedInternal();
    }
    private __host: ObservedPropertySimplePU<string>;
    get host() {
        bjccovmshb1hr4.instrumentFunction(1);
        return this.__host.get();
    }
    set host(newValue: string) {
        bjccovmshb1hr4.instrumentFunction(2);
        this.__host.set(newValue);
    }
    private __port: ObservedPropertySimplePU<string>;
    get port() {
        bjccovmshb1hr4.instrumentFunction(3);
        return this.__port.get();
    }
    set port(newValue: string) {
        bjccovmshb1hr4.instrumentFunction(4);
        this.__port.set(newValue);
    }
    private __username: ObservedPropertySimplePU<string>;
    get username() {
        bjccovmshb1hr4.instrumentFunction(5);
        return this.__username.get();
    }
    set username(newValue: string) {
        bjccovmshb1hr4.instrumentFunction(6);
        this.__username.set(newValue);
    }
    private __password: ObservedPropertySimplePU<string>;
    get password() {
        bjccovmshb1hr4.instrumentFunction(7);
        return this.__password.get();
    }
    set password(newValue: string) {
        bjccovmshb1hr4.instrumentFunction(8);
        this.__password.set(newValue);
    }
    private __connectionProfiles: ObservedPropertyObjectPU<WindowsConnectionProfile[]>;
    get connectionProfiles() {
        bjccovmshb1hr4.instrumentFunction(9);
        return this.__connectionProfiles.get();
    }
    set connectionProfiles(newValue: WindowsConnectionProfile[]) {
        bjccovmshb1hr4.instrumentFunction(10);
        this.__connectionProfiles.set(newValue);
    }
    private __selectedConnectionProfileId: ObservedPropertySimplePU<string>;
    get selectedConnectionProfileId() {
        bjccovmshb1hr4.instrumentFunction(11);
        return this.__selectedConnectionProfileId.get();
    }
    set selectedConnectionProfileId(newValue: string) {
        bjccovmshb1hr4.instrumentFunction(12);
        this.__selectedConnectionProfileId.set(newValue);
    }
    private __rememberConnectionPassword: ObservedPropertySimplePU<boolean>;
    get rememberConnectionPassword() {
        bjccovmshb1hr4.instrumentFunction(13);
        return this.__rememberConnectionPassword.get();
    }
    set rememberConnectionPassword(newValue: boolean) {
        bjccovmshb1hr4.instrumentFunction(14);
        this.__rememberConnectionPassword.set(newValue);
    }
    private __connectionProfilePasswordLoading: ObservedPropertySimplePU<boolean>;
    get connectionProfilePasswordLoading() {
        bjccovmshb1hr4.instrumentFunction(15);
        return this.__connectionProfilePasswordLoading.get();
    }
    set connectionProfilePasswordLoading(newValue: boolean) {
        bjccovmshb1hr4.instrumentFunction(16);
        this.__connectionProfilePasswordLoading.set(newValue);
    }
    // Temporarily pin TOFU certificate policy; restore policy selector when strict validation is needed.
    private certPolicy: string;
    private __remoteAccessCode: ObservedPropertySimplePU<string>;
    get remoteAccessCode() {
        bjccovmshb1hr4.instrumentFunction(17);
        return this.__remoteAccessCode.get();
    }
    set remoteAccessCode(newValue: string) {
        bjccovmshb1hr4.instrumentFunction(18);
        this.__remoteAccessCode.set(newValue);
    }
    private __remoteAccessCodeGateEnabled: ObservedPropertySimplePU<boolean>;
    get remoteAccessCodeGateEnabled() {
        bjccovmshb1hr4.instrumentFunction(19);
        return this.__remoteAccessCodeGateEnabled.get();
    }
    set remoteAccessCodeGateEnabled(newValue: boolean) {
        bjccovmshb1hr4.instrumentFunction(20);
        this.__remoteAccessCodeGateEnabled.set(newValue);
    }
    private __screenRecordingPermissionGranted: ObservedPropertySimplePU<boolean>;
    get screenRecordingPermissionGranted() {
        bjccovmshb1hr4.instrumentFunction(21);
        return this.__screenRecordingPermissionGranted.get();
    }
    set screenRecordingPermissionGranted(newValue: boolean) {
        bjccovmshb1hr4.instrumentFunction(22);
        this.__screenRecordingPermissionGranted.set(newValue);
    }
    private __screenRecordingPermissionBusy: ObservedPropertySimplePU<boolean>;
    get screenRecordingPermissionBusy() {
        bjccovmshb1hr4.instrumentFunction(23);
        return this.__screenRecordingPermissionBusy.get();
    }
    set screenRecordingPermissionBusy(newValue: boolean) {
        bjccovmshb1hr4.instrumentFunction(24);
        this.__screenRecordingPermissionBusy.set(newValue);
    }
    private __inputInjectionPermissionGranted: ObservedPropertySimplePU<boolean>;
    get inputInjectionPermissionGranted() {
        bjccovmshb1hr4.instrumentFunction(25);
        return this.__inputInjectionPermissionGranted.get();
    }
    set inputInjectionPermissionGranted(newValue: boolean) {
        bjccovmshb1hr4.instrumentFunction(26);
        this.__inputInjectionPermissionGranted.set(newValue);
    }
    private __inputInjectionPermissionBusy: ObservedPropertySimplePU<boolean>;
    get inputInjectionPermissionBusy() {
        bjccovmshb1hr4.instrumentFunction(27);
        return this.__inputInjectionPermissionBusy.get();
    }
    set inputInjectionPermissionBusy(newValue: boolean) {
        bjccovmshb1hr4.instrumentFunction(28);
        this.__inputInjectionPermissionBusy.set(newValue);
    }
    private __xrdpServerRunning: ObservedPropertySimplePU<boolean>;
    get xrdpServerRunning() {
        bjccovmshb1hr4.instrumentFunction(29);
        return this.__xrdpServerRunning.get();
    }
    set xrdpServerRunning(newValue: boolean) {
        bjccovmshb1hr4.instrumentFunction(30);
        this.__xrdpServerRunning.set(newValue);
    }
    private __xrdpServerState: ObservedPropertySimplePU<string>;
    get xrdpServerState() {
        bjccovmshb1hr4.instrumentFunction(31);
        return this.__xrdpServerState.get();
    }
    set xrdpServerState(newValue: string) {
        bjccovmshb1hr4.instrumentFunction(32);
        this.__xrdpServerState.set(newValue);
    }
    private __xrdpServerPort: ObservedPropertySimplePU<number>;
    get xrdpServerPort() {
        bjccovmshb1hr4.instrumentFunction(33);
        return this.__xrdpServerPort.get();
    }
    set xrdpServerPort(newValue: number) {
        bjccovmshb1hr4.instrumentFunction(34);
        this.__xrdpServerPort.set(newValue);
    }
    private __xrdpServerMessage: ObservedPropertySimplePU<string>;
    get xrdpServerMessage() {
        bjccovmshb1hr4.instrumentFunction(35);
        return this.__xrdpServerMessage.get();
    }
    set xrdpServerMessage(newValue: string) {
        bjccovmshb1hr4.instrumentFunction(36);
        this.__xrdpServerMessage.set(newValue);
    }
    private __xrdpServerBusy: ObservedPropertySimplePU<boolean>;
    get xrdpServerBusy() {
        bjccovmshb1hr4.instrumentFunction(37);
        return this.__xrdpServerBusy.get();
    }
    set xrdpServerBusy(newValue: boolean) {
        bjccovmshb1hr4.instrumentFunction(38);
        this.__xrdpServerBusy.set(newValue);
    }
    private __showSession: ObservedPropertySimplePU<boolean>;
    get showSession() {
        bjccovmshb1hr4.instrumentFunction(39);
        return this.__showSession.get();
    }
    set showSession(newValue: boolean) {
        bjccovmshb1hr4.instrumentFunction(40);
        this.__showSession.set(newValue);
    }
    private __showSettings: ObservedPropertySimplePU<boolean>;
    get showSettings() {
        bjccovmshb1hr4.instrumentFunction(41);
        return this.__showSettings.get();
    }
    set showSettings(newValue: boolean) {
        bjccovmshb1hr4.instrumentFunction(42);
        this.__showSettings.set(newValue);
    }
    private __settingsInitialPageName: ObservedPropertySimplePU<string>;
    get settingsInitialPageName() {
        bjccovmshb1hr4.instrumentFunction(43);
        return this.__settingsInitialPageName.get();
    }
    set settingsInitialPageName(newValue: string) {
        bjccovmshb1hr4.instrumentFunction(44);
        this.__settingsInitialPageName.set(newValue);
    }
    private __settingsRemoteControlSection: ObservedPropertySimplePU<string>;
    get settingsRemoteControlSection() {
        bjccovmshb1hr4.instrumentFunction(45);
        return this.__settingsRemoteControlSection.get();
    }
    set settingsRemoteControlSection(newValue: string) {
        bjccovmshb1hr4.instrumentFunction(46);
        this.__settingsRemoteControlSection.set(newValue);
    }
    private __remoteLoginWaiting: ObservedPropertySimplePU<boolean>;
    get remoteLoginWaiting() {
        bjccovmshb1hr4.instrumentFunction(47);
        return this.__remoteLoginWaiting.get();
    }
    set remoteLoginWaiting(newValue: boolean) {
        bjccovmshb1hr4.instrumentFunction(48);
        this.__remoteLoginWaiting.set(newValue);
    }
    private __sessionNoticeTitle: ObservedPropertySimplePU<string>;
    get sessionNoticeTitle() {
        bjccovmshb1hr4.instrumentFunction(49);
        return this.__sessionNoticeTitle.get();
    }
    set sessionNoticeTitle(newValue: string) {
        bjccovmshb1hr4.instrumentFunction(50);
        this.__sessionNoticeTitle.set(newValue);
    }
    private __sessionNoticeSubtitle: ObservedPropertySimplePU<string>;
    get sessionNoticeSubtitle() {
        bjccovmshb1hr4.instrumentFunction(51);
        return this.__sessionNoticeSubtitle.get();
    }
    set sessionNoticeSubtitle(newValue: string) {
        bjccovmshb1hr4.instrumentFunction(52);
        this.__sessionNoticeSubtitle.set(newValue);
    }
    private __connectionFeedbackText: ObservedPropertySimplePU<string>;
    get connectionFeedbackText() {
        bjccovmshb1hr4.instrumentFunction(53);
        return this.__connectionFeedbackText.get();
    }
    set connectionFeedbackText(newValue: string) {
        bjccovmshb1hr4.instrumentFunction(54);
        this.__connectionFeedbackText.set(newValue);
    }
    private __connectionFeedbackTone: ObservedPropertySimplePU<SettingsStatusTone>;
    get connectionFeedbackTone() {
        bjccovmshb1hr4.instrumentFunction(55);
        return this.__connectionFeedbackTone.get();
    }
    set connectionFeedbackTone(newValue: SettingsStatusTone) {
        bjccovmshb1hr4.instrumentFunction(56);
        this.__connectionFeedbackTone.set(newValue);
    }
    private __layoutMode: ObservedPropertySimplePU<LayoutMode>;
    get layoutMode() {
        bjccovmshb1hr4.instrumentFunction(57);
        return this.__layoutMode.get();
    }
    set layoutMode(newValue: LayoutMode) {
        bjccovmshb1hr4.instrumentFunction(58);
        this.__layoutMode.set(newValue);
    }
    private readonly deviceCapabilities: DeviceCapabilitySnapshot;
    private __systemDark: ObservedPropertyAbstractPU<boolean>;
    get systemDark() {
        bjccovmshb1hr4.instrumentFunction(59);
        return this.__systemDark.get();
    }
    set systemDark(newValue: boolean) {
        bjccovmshb1hr4.instrumentFunction(60);
        this.__systemDark.set(newValue);
    }
    private __appearanceMode: ObservedPropertyAbstractPU<string>;
    get appearanceMode() {
        bjccovmshb1hr4.instrumentFunction(61);
        return this.__appearanceMode.get();
    }
    set appearanceMode(newValue: string) {
        bjccovmshb1hr4.instrumentFunction(62);
        this.__appearanceMode.set(newValue);
    }
    private callbacksRegistered: boolean;
    private readonly rdpClientController: RdpClientController;
    private readonly rdpSurfaceContentHost: RdpSurfaceContentHost;
    private permissionManager: RdpPermissionManager | null;
    private permissionRequestCoordinator: RdpPermissionRequestCoordinator | null;
    private remoteControlCoordinator: RemoteControlCoordinator | null;
    private connectionProfileCoordinator: WindowsConnectionProfileCoordinator | null;
    private connectionProfilesLoaded: boolean;
    private connectionProfileSelectionGeneration: number;
    private pendingConnectionProfileSave: WindowsConnectionSaveInput | null;
    private lastConnectionErrorMessage: string;
    private windowLayoutObserverRegistered: boolean;
    private windowSizeLayoutBreakpointCallback;
    private deferUiUpdate(task: () => void): void {
        bjccovmshb1hr4.instrumentFunction(63);
        bjccovmshb1hr4.instrumentRegion(63, 1);
        setTimeout((): void => {
            bjccovmshb1hr4.instrumentFunction(65);
            bjccovmshb1hr4.instrumentRegion(65, 1);
            task();
        }, 0);
    }
    aboutToAppear(): void {
        bjccovmshb1hr4.instrumentFunction(66);
        this.registerWindowLayoutObserver();
        this.registerNativeCallbacks();
        this.loadConnectionProfiles();
        if (this.remoteControlServerAvailable()) {
            bjccovmshb1hr4.instrumentBranch(66, 0, true);
            bjccovmshb1hr4.instrumentRegion(66, 1);
            this.refreshScreenRecordingPermissionState();
            this.refreshInputInjectionPermissionState();
            this.refreshXrdpServerDiagnostics();
            this.ensureXrdpServerStarted('page appear');
        }
        else {
            bjccovmshb1hr4.instrumentBranch(66, 0, false);
        }
    }
    onPageShow(): void {
        bjccovmshb1hr4.instrumentFunction(67);
        if (!this.connectionProfilesLoaded) {
            bjccovmshb1hr4.instrumentBranch(67, 0, true);
            bjccovmshb1hr4.instrumentRegion(67, 1);
            this.loadConnectionProfiles();
        }
        else {
            bjccovmshb1hr4.instrumentBranch(67, 0, false);
        }
        if (this.remoteControlServerAvailable()) {
            bjccovmshb1hr4.instrumentBranch(67, 1, true);
            bjccovmshb1hr4.instrumentRegion(67, 2);
            this.refreshRemoteControlReadiness(this.xrdpServerBusy && !this.screenRecordingPermissionBusy);
        }
        else {
            bjccovmshb1hr4.instrumentBranch(67, 1, false);
        }
    }
    onPageHide(): void {
        bjccovmshb1hr4.instrumentFunction(68);
        bjccovmshb1hr4.instrumentRegion(68, 1);
        this.releaseActiveInput();
    }
    aboutToDisappear(): void {
        bjccovmshb1hr4.instrumentFunction(69);
        bjccovmshb1hr4.instrumentRegion(69, 1);
        this.unregisterWindowLayoutObserver();
        this.releaseActiveInput();
    }
    private registerWindowLayoutObserver(): void {
        bjccovmshb1hr4.instrumentFunction(70);
        try {
            bjccovmshb1hr4.instrumentRegion(70, 1);
            const uiContext = this.getUIContext();
            this.layoutMode = layoutModeForWidthBreakpoint(uiContext.getWindowWidthBreakpoint());
            if (this.windowLayoutObserverRegistered) {
                bjccovmshb1hr4.instrumentBranch(70, 0, true);
                bjccovmshb1hr4.instrumentRegion(70, 3);
                return;
            }
            else {
                bjccovmshb1hr4.instrumentBranch(70, 0, false);
            }
            bjccovmshb1hr4.instrumentRegion(70, 4);
            uiContext.getUIObserver().on('windowSizeLayoutBreakpointChange', this.windowSizeLayoutBreakpointCallback);
            this.windowLayoutObserverRegistered = true;
        }
        catch (error) {
            bjccovmshb1hr4.instrumentRegion(70, 2);
            RdpLogger.warn(`Window layout observer registration failed: ${JSON.stringify(error)}`);
        }
    }
    private unregisterWindowLayoutObserver(): void {
        bjccovmshb1hr4.instrumentFunction(71);
        if (!this.windowLayoutObserverRegistered) {
            bjccovmshb1hr4.instrumentBranch(71, 0, true);
            bjccovmshb1hr4.instrumentRegion(71, 1);
            return;
        }
        else {
            bjccovmshb1hr4.instrumentBranch(71, 0, false);
        }
        try {
            bjccovmshb1hr4.instrumentRegion(71, 2);
            this.getUIContext().getUIObserver().off('windowSizeLayoutBreakpointChange', this.windowSizeLayoutBreakpointCallback);
            this.windowLayoutObserverRegistered = false;
        }
        catch (error) {
            bjccovmshb1hr4.instrumentRegion(71, 3);
            RdpLogger.warn(`Window layout observer removal failed: ${JSON.stringify(error)}`);
        }
    }
    private getUiAbilityContext(): common.UIAbilityContext | null {
        bjccovmshb1hr4.instrumentFunction(72);
        try {
            bjccovmshb1hr4.instrumentRegion(72, 1);
            const hostContext = this.getUIContext().getHostContext();
            if (!hostContext) {
                bjccovmshb1hr4.instrumentBranch(72, 0, true);
                bjccovmshb1hr4.instrumentRegion(72, 3);
                return null;
            }
            else {
                bjccovmshb1hr4.instrumentBranch(72, 0, false);
            }
            bjccovmshb1hr4.instrumentRegion(72, 4);
            return hostContext as common.UIAbilityContext;
        }
        catch (error) {
            bjccovmshb1hr4.instrumentRegion(72, 2);
            RdpLogger.warn(`UIAbilityContext unavailable: ${JSON.stringify(error)}`);
            return null;
        }
    }
    private remoteControlServerAvailable(): boolean {
        bjccovmshb1hr4.instrumentFunction(73);
        bjccovmshb1hr4.instrumentRegion(73, 1);
        return hasRemoteControlServer(this.deviceCapabilities);
    }
    private getPermissionManager(): RdpPermissionManager {
        bjccovmshb1hr4.instrumentFunction(74);
        if (this.permissionManager === null) {
            bjccovmshb1hr4.instrumentBranch(74, 0, true);
            bjccovmshb1hr4.instrumentRegion(74, 1);
            const delegate = new RdpPermissionCallbackDelegate((): common.UIAbilityContext | null => { bjccovmshb1hr4.instrumentFunction(75); return this.getUiAbilityContext(); });
            this.permissionManager = new RdpPermissionManager(delegate, (): void => {
                bjccovmshb1hr4.instrumentFunction(76);
                bjccovmshb1hr4.instrumentRegion(76, 1);
                this.getRemoteControlCoordinator().permissionPromptOpened();
            });
        }
        else {
            bjccovmshb1hr4.instrumentBranch(74, 0, false);
        }
        bjccovmshb1hr4.instrumentRegion(74, 2);
        return this.permissionManager;
    }
    private getPermissionRequestCoordinator(): RdpPermissionRequestCoordinator {
        bjccovmshb1hr4.instrumentFunction(77);
        if (this.permissionRequestCoordinator === null) {
            bjccovmshb1hr4.instrumentBranch(77, 0, true);
            bjccovmshb1hr4.instrumentRegion(77, 1);
            this.permissionRequestCoordinator = new RdpPermissionRequestCoordinator(this.getPermissionManager(), (task: () => void): void => { bjccovmshb1hr4.instrumentFunction(78); return this.deferUiUpdate(task); });
        }
        else {
            bjccovmshb1hr4.instrumentBranch(77, 0, false);
        }
        bjccovmshb1hr4.instrumentRegion(77, 2);
        return this.permissionRequestCoordinator;
    }
    private getRemoteControlCoordinator(): RemoteControlCoordinator {
        bjccovmshb1hr4.instrumentFunction(80);
        if (this.remoteControlCoordinator === null) {
            bjccovmshb1hr4.instrumentBranch(80, 0, true);
            bjccovmshb1hr4.instrumentRegion(80, 1);
            this.remoteControlCoordinator = new RemoteControlCoordinator(this.remoteControlServerAvailable(), this.xrdpServerDisplayMessages(), (): RdpPermissionManager => { bjccovmshb1hr4.instrumentFunction(81); return this.getPermissionManager(); }, (): string => { bjccovmshb1hr4.instrumentFunction(82); return this.getAppFilesDir(); }, (snapshot: RemoteControlSnapshot): void => {
                bjccovmshb1hr4.instrumentFunction(83);
                bjccovmshb1hr4.instrumentRegion(83, 1);
                this.deferUiUpdate((): void => { bjccovmshb1hr4.instrumentFunction(84); return this.applyRemoteControlSnapshot(snapshot); });
            });
            this.applyRemoteControlSnapshot(this.remoteControlCoordinator.snapshot());
        }
        else {
            bjccovmshb1hr4.instrumentBranch(80, 0, false);
        }
        bjccovmshb1hr4.instrumentRegion(80, 2);
        return this.remoteControlCoordinator;
    }
    private getConnectionProfileCoordinator(): WindowsConnectionProfileCoordinator | null {
        bjccovmshb1hr4.instrumentFunction(85);
        if (this.connectionProfileCoordinator !== null) {
            bjccovmshb1hr4.instrumentBranch(85, 0, true);
            bjccovmshb1hr4.instrumentRegion(85, 1);
            return this.connectionProfileCoordinator;
        }
        else {
            bjccovmshb1hr4.instrumentBranch(85, 0, false);
        }
        const context = this.getUiAbilityContext();
        if (context === null) {
            bjccovmshb1hr4.instrumentBranch(85, 1, true);
            bjccovmshb1hr4.instrumentRegion(85, 2);
            return null;
        }
        else {
            bjccovmshb1hr4.instrumentBranch(85, 1, false);
        }
        bjccovmshb1hr4.instrumentRegion(85, 3);
        this.connectionProfileCoordinator = new WindowsConnectionProfileCoordinator(context);
        return this.connectionProfileCoordinator;
    }
    private loadConnectionProfiles(): void {
        bjccovmshb1hr4.instrumentFunction(86);
        const coordinator = this.getConnectionProfileCoordinator();
        if (coordinator === null) {
            bjccovmshb1hr4.instrumentBranch(86, 0, true);
            bjccovmshb1hr4.instrumentRegion(86, 1);
            return;
        }
        else {
            bjccovmshb1hr4.instrumentBranch(86, 0, false);
        }
        bjccovmshb1hr4.instrumentRegion(86, 2);
        coordinator.load()
            .then((snapshot: WindowsConnectionSnapshot | null) => {
            bjccovmshb1hr4.instrumentFunction(87);
            if (snapshot === null) {
                bjccovmshb1hr4.instrumentBranch(87, 0, true);
                bjccovmshb1hr4.instrumentRegion(87, 1);
                return;
            }
            else {
                bjccovmshb1hr4.instrumentBranch(87, 0, false);
            }
            bjccovmshb1hr4.instrumentRegion(86, 3);
            this.deferUiUpdate(() => {
                bjccovmshb1hr4.instrumentFunction(88);
                this.connectionProfilesLoaded = true;
                bjccovmshb1hr4.instrumentRegion(86, 4);
                this.applyConnectionSnapshot(snapshot, true, false);
            });
        });
    }
    private applyConnectionSnapshot(snapshot: WindowsConnectionSnapshot, applySelectedProfile: boolean, clearWhenEmpty: boolean): void {
        bjccovmshb1hr4.instrumentFunction(89);
        this.connectionProfiles = snapshot.profiles;
        this.selectedConnectionProfileId = snapshot.selectedProfileId;
        if (!applySelectedProfile) {
            bjccovmshb1hr4.instrumentBranch(89, 0, true);
            bjccovmshb1hr4.instrumentRegion(89, 1);
            return;
        }
        else {
            bjccovmshb1hr4.instrumentBranch(89, 0, false);
        }
        const selectedProfile = this.findConnectionProfileIn(snapshot.profiles, snapshot.selectedProfileId);
        if (selectedProfile === null) {
            bjccovmshb1hr4.instrumentBranch(89, 1, true);
            bjccovmshb1hr4.instrumentRegion(89, 2);
            this.rememberConnectionPassword = false;
            if (clearWhenEmpty) {
                bjccovmshb1hr4.instrumentBranch(89, 2, true);
                bjccovmshb1hr4.instrumentRegion(89, 3);
                this.startNewConnectionProfile();
            }
            else {
                bjccovmshb1hr4.instrumentBranch(89, 2, false);
            }
            bjccovmshb1hr4.instrumentRegion(89, 5);
            return;
        }
        else {
            bjccovmshb1hr4.instrumentBranch(89, 1, false);
        }
        bjccovmshb1hr4.instrumentRegion(89, 4);
        this.applyConnectionProfile(selectedProfile, snapshot.selectedPassword);
    }
    private findConnectionProfile(profileId: string): WindowsConnectionProfile | null {
        bjccovmshb1hr4.instrumentFunction(90);
        bjccovmshb1hr4.instrumentRegion(90, 1);
        return this.findConnectionProfileIn(this.connectionProfiles, profileId);
    }
    private findConnectionProfileIn(profiles: WindowsConnectionProfile[], profileId: string): WindowsConnectionProfile | null {
        bjccovmshb1hr4.instrumentFunction(91);
        for (const profile of profiles) {
            bjccovmshb1hr4.instrumentRegion(91, 1);
            if (profile.id === profileId) {
                bjccovmshb1hr4.instrumentBranch(91, 0, true);
                bjccovmshb1hr4.instrumentRegion(91, 2);
                return profile;
            }
            else {
                bjccovmshb1hr4.instrumentBranch(91, 0, false);
            }
        }
        bjccovmshb1hr4.instrumentRegion(91, 3);
        return null;
    }
    private applyConnectionProfile(profile: WindowsConnectionProfile, password: string): void {
        bjccovmshb1hr4.instrumentFunction(92);
        this.selectedConnectionProfileId = profile.id;
        this.host = profile.host;
        this.port = profile.port;
        this.username = profile.username;
        this.password = password;
        this.rememberConnectionPassword = profile.rememberPassword;
        if (profile.certPolicy.length > 0) {
            bjccovmshb1hr4.instrumentBranch(92, 0, true);
            bjccovmshb1hr4.instrumentRegion(92, 1);
            this.certPolicy = profile.certPolicy;
        }
        else {
            bjccovmshb1hr4.instrumentBranch(92, 0, false);
        }
    }
    private selectConnectionProfile(profileId: string): void {
        bjccovmshb1hr4.instrumentFunction(93);
        if (profileId.length === 0) {
            bjccovmshb1hr4.instrumentBranch(93, 0, true);
            bjccovmshb1hr4.instrumentRegion(93, 1);
            this.startNewConnectionProfile();
            return;
        }
        else {
            bjccovmshb1hr4.instrumentBranch(93, 0, false);
        }
        const profile = this.findConnectionProfile(profileId);
        if (profile === null) {
            bjccovmshb1hr4.instrumentBranch(93, 1, true);
            bjccovmshb1hr4.instrumentRegion(93, 2);
            return;
        }
        else {
            bjccovmshb1hr4.instrumentBranch(93, 1, false);
        }
        const selectionGeneration = ++this.connectionProfileSelectionGeneration;
        if (!profile.rememberPassword) {
            bjccovmshb1hr4.instrumentBranch(93, 2, true);
            bjccovmshb1hr4.instrumentRegion(93, 3);
            this.connectionProfilePasswordLoading = false;
            this.applyConnectionProfile(profile, '');
            return;
        }
        else {
            bjccovmshb1hr4.instrumentBranch(93, 2, false);
        }
        const coordinator = this.getConnectionProfileCoordinator();
        if (coordinator === null) {
            bjccovmshb1hr4.instrumentBranch(93, 3, true);
            bjccovmshb1hr4.instrumentRegion(93, 4);
            this.connectionProfilePasswordLoading = false;
            this.applyConnectionProfile(profile, '');
            return;
        }
        else {
            bjccovmshb1hr4.instrumentBranch(93, 3, false);
        }
        bjccovmshb1hr4.instrumentRegion(93, 5);
        this.connectionProfilePasswordLoading = true;
        coordinator.loadPassword(profile)
            .then((savedPassword: string | null) => {
            bjccovmshb1hr4.instrumentFunction(94);
            if (savedPassword === null) {
                bjccovmshb1hr4.instrumentBranch(94, 0, true);
                bjccovmshb1hr4.instrumentRegion(94, 1);
                this.deferUiUpdate(() => {
                    bjccovmshb1hr4.instrumentFunction(95);
                    if (this.connectionProfileSelectionGeneration === selectionGeneration) {
                        bjccovmshb1hr4.instrumentBranch(95, 0, true);
                        bjccovmshb1hr4.instrumentRegion(95, 1);
                        this.connectionProfilePasswordLoading = false;
                        bjccovmshb1hr4.instrumentRegion(93, 8);
                        this.applyConnectionProfile(profile, '');
                    }
                    else {
                        bjccovmshb1hr4.instrumentBranch(95, 0, false);
                    }
                });
                bjccovmshb1hr4.instrumentRegion(93, 7);
                return;
            }
            else {
                bjccovmshb1hr4.instrumentBranch(94, 0, false);
            }
            bjccovmshb1hr4.instrumentRegion(93, 6);
            this.deferUiUpdate(() => {
                bjccovmshb1hr4.instrumentFunction(96);
                if (this.connectionProfileSelectionGeneration === selectionGeneration) {
                    bjccovmshb1hr4.instrumentBranch(96, 0, true);
                    bjccovmshb1hr4.instrumentRegion(96, 1);
                    this.applyConnectionProfile(profile, savedPassword);
                    bjccovmshb1hr4.instrumentRegion(93, 9);
                    this.connectionProfilePasswordLoading = false;
                }
                else {
                    bjccovmshb1hr4.instrumentBranch(96, 0, false);
                }
            });
        });
    }
    private startNewConnectionProfile(): void {
        bjccovmshb1hr4.instrumentFunction(97);
        bjccovmshb1hr4.instrumentRegion(97, 1);
        this.connectionProfileSelectionGeneration++;
        this.connectionProfilePasswordLoading = false;
        this.selectedConnectionProfileId = '';
        this.host = '';
        this.port = '3389';
        this.username = '';
        this.password = '';
        this.rememberConnectionPassword = false;
        this.certPolicy = 'tofu';
        this.clearConnectionFeedback();
    }
    private deleteSelectedConnectionProfile(): void {
        bjccovmshb1hr4.instrumentFunction(98);
        if (this.selectedConnectionProfileId.length === 0) {
            bjccovmshb1hr4.instrumentBranch(98, 0, true);
            bjccovmshb1hr4.instrumentRegion(98, 1);
            this.startNewConnectionProfile();
            return;
        }
        else {
            bjccovmshb1hr4.instrumentBranch(98, 0, false);
        }
        const coordinator = this.getConnectionProfileCoordinator();
        if (coordinator === null) {
            bjccovmshb1hr4.instrumentBranch(98, 1, true);
            bjccovmshb1hr4.instrumentRegion(98, 2);
            return;
        }
        else {
            bjccovmshb1hr4.instrumentBranch(98, 1, false);
        }
        bjccovmshb1hr4.instrumentRegion(98, 3);
        coordinator.deleteProfile(this.selectedConnectionProfileId)
            .then((snapshot: WindowsConnectionSnapshot | null) => {
            bjccovmshb1hr4.instrumentFunction(99);
            if (snapshot === null) {
                bjccovmshb1hr4.instrumentBranch(99, 0, true);
                bjccovmshb1hr4.instrumentRegion(99, 1);
                return;
            }
            else {
                bjccovmshb1hr4.instrumentBranch(99, 0, false);
            }
            bjccovmshb1hr4.instrumentRegion(98, 4);
            this.deferUiUpdate(() => {
                bjccovmshb1hr4.instrumentFunction(100);
                bjccovmshb1hr4.instrumentRegion(100, 1);
                this.applyConnectionSnapshot(snapshot, true, true);
            });
        });
    }
    private setRememberConnectionPassword(remember: boolean): void {
        bjccovmshb1hr4.instrumentFunction(101);
        bjccovmshb1hr4.instrumentRegion(101, 1);
        this.rememberConnectionPassword = remember;
    }
    private saveConnectionProfile(input: WindowsConnectionSaveInput): void {
        bjccovmshb1hr4.instrumentFunction(102);
        const coordinator = this.getConnectionProfileCoordinator();
        if (coordinator === null) {
            bjccovmshb1hr4.instrumentBranch(102, 0, true);
            bjccovmshb1hr4.instrumentRegion(102, 1);
            return;
        }
        else {
            bjccovmshb1hr4.instrumentBranch(102, 0, false);
        }
        bjccovmshb1hr4.instrumentRegion(102, 2);
        coordinator.saveProfile(input)
            .then((snapshot: WindowsConnectionSnapshot | null) => {
            bjccovmshb1hr4.instrumentFunction(103);
            if (snapshot === null) {
                bjccovmshb1hr4.instrumentBranch(103, 0, true);
                bjccovmshb1hr4.instrumentRegion(103, 1);
                return;
            }
            else {
                bjccovmshb1hr4.instrumentBranch(103, 0, false);
            }
            bjccovmshb1hr4.instrumentRegion(102, 3);
            this.deferUiUpdate(() => {
                bjccovmshb1hr4.instrumentFunction(104);
                this.connectionProfiles = snapshot.profiles;
                this.selectedConnectionProfileId = snapshot.selectedProfileId;
                const selected = this.findConnectionProfileIn(snapshot.profiles, snapshot.selectedProfileId);
                if (selected !== null) {
                    bjccovmshb1hr4.instrumentBranch(104, 0, true);
                    bjccovmshb1hr4.instrumentRegion(104, 1);
                    this.rememberConnectionPassword = selected.rememberPassword;
                }
                else {
                    bjccovmshb1hr4.instrumentBranch(104, 0, false);
                }
            });
        });
    }
    private persistPendingConnectionProfile(): void {
        bjccovmshb1hr4.instrumentFunction(105);
        if (this.pendingConnectionProfileSave === null) {
            bjccovmshb1hr4.instrumentBranch(105, 0, true);
            bjccovmshb1hr4.instrumentRegion(105, 1);
            return;
        }
        else {
            bjccovmshb1hr4.instrumentBranch(105, 0, false);
        }
        bjccovmshb1hr4.instrumentRegion(105, 2);
        const saveInput = this.pendingConnectionProfileSave;
        this.pendingConnectionProfileSave = null;
        this.saveConnectionProfile(saveInput);
    }
    private clearSelectedConnectionPassword(): void {
        bjccovmshb1hr4.instrumentFunction(106);
        this.password = '';
        this.rememberConnectionPassword = false;
        if (this.selectedConnectionProfileId.length === 0) {
            bjccovmshb1hr4.instrumentBranch(106, 0, true);
            bjccovmshb1hr4.instrumentRegion(106, 1);
            this.setConnectionFeedback(HomeText.PASSWORD_CLEARED_CURRENT, 'neutral');
            return;
        }
        else {
            bjccovmshb1hr4.instrumentBranch(106, 0, false);
        }
        const coordinator = this.getConnectionProfileCoordinator();
        if (coordinator === null) {
            bjccovmshb1hr4.instrumentBranch(106, 1, true);
            bjccovmshb1hr4.instrumentRegion(106, 2);
            this.setConnectionFeedback(HomeText.PASSWORD_CLEAR_STORAGE_UNAVAILABLE, 'danger');
            return;
        }
        else {
            bjccovmshb1hr4.instrumentBranch(106, 1, false);
        }
        bjccovmshb1hr4.instrumentRegion(106, 3);
        coordinator.clearPassword(this.selectedConnectionProfileId)
            .then((snapshot: WindowsConnectionSnapshot | null) => {
            bjccovmshb1hr4.instrumentFunction(107);
            if (snapshot === null) {
                bjccovmshb1hr4.instrumentBranch(107, 0, true);
                bjccovmshb1hr4.instrumentRegion(107, 1);
                this.deferUiUpdate(() => {
                    bjccovmshb1hr4.instrumentFunction(108);
                    bjccovmshb1hr4.instrumentRegion(108, 1);
                    this.setConnectionFeedback(HomeText.PASSWORD_CLEAR_FAILED, 'danger');
                });
                bjccovmshb1hr4.instrumentRegion(106, 5);
                return;
            }
            else {
                bjccovmshb1hr4.instrumentBranch(107, 0, false);
            }
            bjccovmshb1hr4.instrumentRegion(106, 4);
            this.deferUiUpdate(() => {
                bjccovmshb1hr4.instrumentFunction(109);
                this.applyConnectionSnapshot(snapshot, true, false);
                bjccovmshb1hr4.instrumentRegion(106, 6);
                this.password = '';
                this.rememberConnectionPassword = false;
                this.setConnectionFeedback(HomeText.PASSWORD_CLEARED_SAVED, 'ok');
            });
        });
    }
    private clearConnectionFeedback(): void {
        bjccovmshb1hr4.instrumentFunction(110);
        bjccovmshb1hr4.instrumentRegion(110, 1);
        this.connectionFeedbackText = '';
        this.connectionFeedbackTone = 'neutral';
    }
    private setConnectionFeedback(text: string, tone: SettingsStatusTone): void {
        bjccovmshb1hr4.instrumentFunction(111);
        bjccovmshb1hr4.instrumentRegion(111, 1);
        this.connectionFeedbackText = text;
        this.connectionFeedbackTone = tone;
    }
    private connectionFailureMessage(message: string, fallback: string = HomeText.CONNECTION_FAILURE): string {
        bjccovmshb1hr4.instrumentFunction(112);
        const detail = message.trim();
        if (detail.length === 0) {
            bjccovmshb1hr4.instrumentBranch(112, 0, true);
            bjccovmshb1hr4.instrumentRegion(112, 1);
            return fallback;
        }
        else {
            bjccovmshb1hr4.instrumentBranch(112, 0, false);
        }
        bjccovmshb1hr4.instrumentRegion(112, 2);
        return HomeText.connectionFailureWithDetail(detail);
    }
    private errorMessage(error: Error): string {
        bjccovmshb1hr4.instrumentFunction(113);
        if (error.message.length > 0) {
            bjccovmshb1hr4.instrumentBranch(113, 0, true);
            bjccovmshb1hr4.instrumentRegion(113, 1);
            return error.message;
        }
        else {
            bjccovmshb1hr4.instrumentBranch(113, 0, false);
        }
        bjccovmshb1hr4.instrumentRegion(113, 2);
        const serialized = JSON.stringify(error);
        return serialized.length > 0 && serialized !== '{}' ? (bjccovmshb1hr4.instrumentBranch(113, 1, true), serialized) : (bjccovmshb1hr4.instrumentBranch(113, 1, false), '');
    }
    private validateConnectionForm(): ConnectionValidationResult {
        bjccovmshb1hr4.instrumentFunction(114);
        bjccovmshb1hr4.instrumentRegion(114, 1);
        return RdpConnectionValidator.validate(this.host, this.port, this.username, this.password);
    }
    private xrdpServerDisplayMessages(): XrdpServerDisplayMessages {
        bjccovmshb1hr4.instrumentFunction(115);
        bjccovmshb1hr4.instrumentRegion(115, 1);
        return {
            active: SettingsText.REMOTE_SERVER_MESSAGE_ACTIVE,
            listening: SettingsText.REMOTE_SERVER_MESSAGE_LISTENING,
            stopped: SettingsText.REMOTE_SERVER_MESSAGE_STOPPED,
            exited: SettingsText.REMOTE_SERVER_MESSAGE_EXITED,
            failed: SettingsText.REMOTE_SERVER_MESSAGE_FAILED,
            unavailable: SettingsText.REMOTE_SERVER_MESSAGE_UNAVAILABLE
        };
    }
    private registerNativeCallbacks(): void {
        bjccovmshb1hr4.instrumentFunction(116);
        if (this.callbacksRegistered) {
            bjccovmshb1hr4.instrumentBranch(116, 0, true);
            bjccovmshb1hr4.instrumentRegion(116, 1);
            return;
        }
        else {
            bjccovmshb1hr4.instrumentBranch(116, 0, false);
        }
        try {
            bjccovmshb1hr4.instrumentRegion(116, 2);
            const callbackResult = this.rdpClientController.registerCallbacks({
                onState: (state: string, wasConnected: boolean): void => {
                    bjccovmshb1hr4.instrumentFunction(117);
                    bjccovmshb1hr4.instrumentRegion(117, 1);
                    this.deferUiUpdate(() => {
                        bjccovmshb1hr4.instrumentFunction(118);
                        const nextConnected = this.isConnectedNativeState(state);
                        if (!nextConnected && (state === 'Disconnected' || state === 'Failed' || state === 'Idle')) {
                            bjccovmshb1hr4.instrumentBranch(118, 0, true);
                            bjccovmshb1hr4.instrumentRegion(118, 1);
                            if (wasConnected) {
                                bjccovmshb1hr4.instrumentBranch(118, 1, true);
                                bjccovmshb1hr4.instrumentRegion(118, 2);
                                this.releaseActiveInput();
                            }
                            else {
                                bjccovmshb1hr4.instrumentBranch(118, 1, false);
                            }
                            bjccovmshb1hr4.instrumentRegion(116, 6);
                            this.setSessionVisible(false);
                        }
                        else {
                            bjccovmshb1hr4.instrumentBranch(118, 0, false);
                        }
                        this.updateSessionNoticeForNativeState(state);
                        if (nextConnected && !wasConnected) {
                            bjccovmshb1hr4.instrumentBranch(118, 2, true);
                            bjccovmshb1hr4.instrumentRegion(118, 3);
                            this.setSessionVisible(true);
                            bjccovmshb1hr4.instrumentRegion(116, 7);
                            this.focusRemoteSurface();
                            this.lastConnectionErrorMessage = '';
                            this.setConnectionFeedback(HomeText.CONNECTION_SUCCESS, 'ok');
                            this.persistPendingConnectionProfile();
                        }
                        else {
                            bjccovmshb1hr4.instrumentBranch(118, 2, false);
                        }
                        if (state === 'Failed') {
                            bjccovmshb1hr4.instrumentBranch(118, 3, true);
                            bjccovmshb1hr4.instrumentRegion(118, 4);
                            this.pendingConnectionProfileSave = null;
                            bjccovmshb1hr4.instrumentRegion(116, 8);
                            this.setConnectionFeedback(this.connectionFailureMessage(this.lastConnectionErrorMessage, HomeText.CONNECTION_FAILURE_CREDENTIALS), 'danger');
                            RdpLogger.error(`Native state: ${state}`);
                        }
                        else {
                            bjccovmshb1hr4.instrumentBranch(118, 3, false);
                            bjccovmshb1hr4.instrumentRegion(118, 5);
                            if ((state === 'Disconnected' || state === 'Idle') && this.pendingConnectionProfileSave !== null) {
                                bjccovmshb1hr4.instrumentBranch(118, 4, true);
                                bjccovmshb1hr4.instrumentRegion(118, 6);
                                this.pendingConnectionProfileSave = null;
                                bjccovmshb1hr4.instrumentRegion(116, 9);
                                this.setConnectionFeedback(this.connectionFailureMessage(this.lastConnectionErrorMessage, HomeText.CONNECTION_FAILURE_NO_SESSION), 'danger');
                            }
                            else {
                                bjccovmshb1hr4.instrumentBranch(118, 4, false);
                                bjccovmshb1hr4.instrumentRegion(118, 7);
                                if (state === 'Connected' || state === 'Disconnected' ||
                                    state === 'RemoteLoginWaiting' || state === 'RemoteDesktopReady') {
                                    bjccovmshb1hr4.instrumentBranch(118, 5, true);
                                    bjccovmshb1hr4.instrumentRegion(118, 8);
                                    RdpLogger.info(`Native state: ${state}`);
                                }
                                else {
                                    bjccovmshb1hr4.instrumentBranch(118, 5, false);
                                }
                            }
                        }
                    });
                },
                onError: (message: string, wasConnected: boolean): void => {
                    bjccovmshb1hr4.instrumentFunction(119);
                    bjccovmshb1hr4.instrumentRegion(119, 1);
                    this.deferUiUpdate(() => {
                        bjccovmshb1hr4.instrumentFunction(120);
                        if (wasConnected) {
                            bjccovmshb1hr4.instrumentBranch(120, 0, true);
                            bjccovmshb1hr4.instrumentRegion(120, 1);
                            this.releaseActiveInput();
                        }
                        else {
                            bjccovmshb1hr4.instrumentBranch(120, 0, false);
                        }
                        bjccovmshb1hr4.instrumentRegion(116, 10);
                        this.setSessionVisible(false);
                        this.clearSessionNotice();
                        this.pendingConnectionProfileSave = null;
                        this.lastConnectionErrorMessage = message;
                        this.setConnectionFeedback(this.connectionFailureMessage(message), 'danger');
                        RdpLogger.error(`Native error: ${message}`);
                    });
                }
            });
            if (!callbackResult.ok) {
                bjccovmshb1hr4.instrumentBranch(116, 1, true);
                bjccovmshb1hr4.instrumentRegion(116, 4);
                RdpLogger.error(`Native callback registration failed: ${callbackResult.message}`);
                return;
            }
            else {
                bjccovmshb1hr4.instrumentBranch(116, 1, false);
            }
            bjccovmshb1hr4.instrumentRegion(116, 5);
            this.registerNativePermissionCallbacks();
            this.callbacksRegistered = true;
        }
        catch (error) {
            bjccovmshb1hr4.instrumentRegion(116, 3);
            RdpLogger.error(`Native callback registration failed: ${JSON.stringify(error)}`);
        }
    }
    private isConnectedNativeState(state: string): boolean {
        bjccovmshb1hr4.instrumentFunction(121);
        bjccovmshb1hr4.instrumentRegion(121, 1);
        return RdpClientController.isConnectedState(state);
    }
    private clearSessionNotice(): void {
        bjccovmshb1hr4.instrumentFunction(122);
        bjccovmshb1hr4.instrumentRegion(122, 1);
        this.remoteLoginWaiting = false;
        this.sessionNoticeTitle = '';
        this.sessionNoticeSubtitle = '';
    }
    private updateSessionNoticeForNativeState(state: string): void {
        bjccovmshb1hr4.instrumentFunction(123);
        if (state === 'RemoteLoginWaiting') {
            bjccovmshb1hr4.instrumentBranch(123, 0, true);
            bjccovmshb1hr4.instrumentRegion(123, 1);
            this.remoteLoginWaiting = true;
            this.sessionNoticeTitle = HomeText.SESSION_REMOTE_LOGIN_WAITING_TITLE;
            this.sessionNoticeSubtitle = HomeText.SESSION_REMOTE_LOGIN_WAITING_SUBTITLE;
            return;
        }
        else {
            bjccovmshb1hr4.instrumentBranch(123, 0, false);
        }
        this.remoteLoginWaiting = false;
        if (state === 'Connected') {
            bjccovmshb1hr4.instrumentBranch(123, 1, true);
            bjccovmshb1hr4.instrumentRegion(123, 2);
            this.sessionNoticeTitle = HomeText.SESSION_CONNECTED_TITLE;
            this.sessionNoticeSubtitle = HomeText.SESSION_CONNECTED_SUBTITLE;
            return;
        }
        else {
            bjccovmshb1hr4.instrumentBranch(123, 1, false);
        }
        if (state === 'RemoteDesktopReady' || state === 'Disconnected' || state === 'Failed' || state === 'Idle') {
            bjccovmshb1hr4.instrumentBranch(123, 2, true);
            bjccovmshb1hr4.instrumentRegion(123, 3);
            this.sessionNoticeTitle = '';
            this.sessionNoticeSubtitle = '';
            return;
        }
        else {
            bjccovmshb1hr4.instrumentBranch(123, 2, false);
        }
        if (state === 'Resolving') {
            bjccovmshb1hr4.instrumentBranch(123, 3, true);
            bjccovmshb1hr4.instrumentRegion(123, 4);
            this.sessionNoticeTitle = HomeText.SESSION_RESOLVING_TITLE;
            this.sessionNoticeSubtitle = HomeText.SESSION_RESOLVING_SUBTITLE;
        }
        else {
            bjccovmshb1hr4.instrumentBranch(123, 3, false);
            bjccovmshb1hr4.instrumentRegion(123, 5);
            if (state === 'TCP connected') {
                bjccovmshb1hr4.instrumentBranch(123, 4, true);
                bjccovmshb1hr4.instrumentRegion(123, 6);
                this.sessionNoticeTitle = HomeText.SESSION_TCP_CONNECTED_TITLE;
                this.sessionNoticeSubtitle = HomeText.SESSION_TCP_CONNECTED_SUBTITLE;
            }
            else {
                bjccovmshb1hr4.instrumentBranch(123, 4, false);
                bjccovmshb1hr4.instrumentRegion(123, 7);
                if (state === 'Negotiating') {
                    bjccovmshb1hr4.instrumentBranch(123, 5, true);
                    bjccovmshb1hr4.instrumentRegion(123, 8);
                    this.sessionNoticeTitle = HomeText.SESSION_NEGOTIATING_TITLE;
                    this.sessionNoticeSubtitle = HomeText.SESSION_NEGOTIATING_SUBTITLE;
                }
                else {
                    bjccovmshb1hr4.instrumentBranch(123, 5, false);
                    bjccovmshb1hr4.instrumentRegion(123, 9);
                    if (state === 'Authenticating') {
                        bjccovmshb1hr4.instrumentBranch(123, 6, true);
                        bjccovmshb1hr4.instrumentRegion(123, 10);
                        this.sessionNoticeTitle = HomeText.SESSION_AUTHENTICATING_TITLE;
                        this.sessionNoticeSubtitle = HomeText.SESSION_AUTHENTICATING_SUBTITLE;
                    }
                    else {
                        bjccovmshb1hr4.instrumentBranch(123, 6, false);
                    }
                }
            }
        }
    }
    private getAppFilesDir(): string {
        bjccovmshb1hr4.instrumentFunction(124);
        const context = this.getUiAbilityContext();
        if (context === null) {
            bjccovmshb1hr4.instrumentBranch(124, 0, true);
            bjccovmshb1hr4.instrumentRegion(124, 1);
            return '';
        }
        else {
            bjccovmshb1hr4.instrumentBranch(124, 0, false);
        }
        bjccovmshb1hr4.instrumentRegion(124, 2);
        return context.filesDir;
    }
    private openRemoteFilesDirectoryFromSettings(): void {
        bjccovmshb1hr4.instrumentFunction(125);
        bjccovmshb1hr4.instrumentRegion(125, 1);
        RemoteFilesDirectory.openSharedDirectory(this.getUiAbilityContext())
            .catch((error: Error) => {
            bjccovmshb1hr4.instrumentFunction(126);
            bjccovmshb1hr4.instrumentRegion(126, 1);
            RdpLogger.error(`remote files directory open failed: ${JSON.stringify(error)}`);
        });
    }
    private openSettingsFromHome(remoteControlSection: string): void {
        bjccovmshb1hr4.instrumentFunction(127);
        if (this.remoteControlServerAvailable()) {
            bjccovmshb1hr4.instrumentBranch(127, 0, true);
            bjccovmshb1hr4.instrumentRegion(127, 1);
            this.refreshScreenRecordingPermissionState();
            this.refreshInputInjectionPermissionState();
            this.refreshXrdpServerDiagnostics();
            this.settingsRemoteControlSection = remoteControlSection;
            this.settingsInitialPageName = remoteControlSection.length > 0 ? (bjccovmshb1hr4.instrumentBranch(127, 1, true), SettingsRoute.REMOTE_CONTROL) : (bjccovmshb1hr4.instrumentBranch(127, 1, false), SettingsRoute.SETTINGS);
        }
        else {
            bjccovmshb1hr4.instrumentBranch(127, 0, false);
            bjccovmshb1hr4.instrumentRegion(127, 2);
            this.settingsRemoteControlSection = '';
            this.settingsInitialPageName = SettingsRoute.SETTINGS;
        }
        bjccovmshb1hr4.instrumentRegion(127, 3);
        this.showSettings = true;
    }
    private registerNativePermissionCallbacks(): void {
        bjccovmshb1hr4.instrumentFunction(128);
        bjccovmshb1hr4.instrumentRegion(128, 1);
        this.getPermissionRequestCoordinator().register();
    }
    private refreshScreenRecordingPermissionState(): boolean {
        bjccovmshb1hr4.instrumentFunction(129);
        bjccovmshb1hr4.instrumentRegion(129, 1);
        return this.getRemoteControlCoordinator().refreshPermissionState();
    }
    private requestScreenRecordingPermissionFromSettings(): Promise<boolean> {
        bjccovmshb1hr4.instrumentFunction(130);
        bjccovmshb1hr4.instrumentRegion(130, 1);
        return this.getRemoteControlCoordinator().requestPermissionFromSettings();
    }
    private refreshInputInjectionPermissionState(): boolean {
        bjccovmshb1hr4.instrumentFunction(131);
        bjccovmshb1hr4.instrumentRegion(131, 1);
        return this.getRemoteControlCoordinator().refreshInputInjectionPermissionState();
    }
    private requestInputInjectionPermissionFromSettings(): Promise<boolean> {
        bjccovmshb1hr4.instrumentFunction(132);
        bjccovmshb1hr4.instrumentRegion(132, 1);
        return this.getRemoteControlCoordinator().requestInputInjectionPermissionFromSettings();
    }
    private refreshXrdpServerDiagnostics(): XrdpServerStatus {
        bjccovmshb1hr4.instrumentFunction(133);
        bjccovmshb1hr4.instrumentRegion(133, 1);
        return this.getRemoteControlCoordinator().refreshDiagnostics();
    }
    private refreshRemoteControlReadiness(clearBusy: boolean = false): XrdpServerStatus {
        bjccovmshb1hr4.instrumentFunction(134);
        bjccovmshb1hr4.instrumentRegion(134, 1);
        return this.getRemoteControlCoordinator().refreshReadiness(clearBusy);
    }
    private startXrdpServerFromSettings(): Promise<XrdpServerStatus> {
        bjccovmshb1hr4.instrumentFunction(135);
        bjccovmshb1hr4.instrumentRegion(135, 1);
        return this.getRemoteControlCoordinator().startFromSettings();
    }
    private setRemoteAccessCodeGateFromSettings(enabled: boolean): string {
        bjccovmshb1hr4.instrumentFunction(136);
        bjccovmshb1hr4.instrumentRegion(136, 1);
        return this.getRemoteControlCoordinator().setAccessCodeGate(enabled);
    }
    private regenerateRemoteAccessCodeFromSettings(): string {
        bjccovmshb1hr4.instrumentFunction(137);
        bjccovmshb1hr4.instrumentRegion(137, 1);
        return this.getRemoteControlCoordinator().regenerateAccessCode();
    }
    private ensureXrdpServerStarted(trigger: string, restartIfRunning: boolean = false, requestScreenRecordingImmediately: boolean = false): Promise<XrdpServerStatus> {
        bjccovmshb1hr4.instrumentFunction(138);
        bjccovmshb1hr4.instrumentRegion(138, 1);
        return this.getRemoteControlCoordinator().ensureStarted(trigger, restartIfRunning, requestScreenRecordingImmediately);
    }
    private applyRemoteControlSnapshot(snapshot: RemoteControlSnapshot): void {
        bjccovmshb1hr4.instrumentFunction(139);
        bjccovmshb1hr4.instrumentRegion(139, 1);
        this.remoteAccessCode = snapshot.accessCode;
        this.remoteAccessCodeGateEnabled = snapshot.accessCodeGateEnabled;
        this.screenRecordingPermissionGranted = snapshot.screenRecordingPermissionGranted;
        this.screenRecordingPermissionBusy = snapshot.screenRecordingPermissionBusy;
        this.inputInjectionPermissionGranted = snapshot.inputInjectionPermissionGranted;
        this.inputInjectionPermissionBusy = snapshot.inputInjectionPermissionBusy;
        this.xrdpServerRunning = snapshot.serverRunning;
        this.xrdpServerState = snapshot.serverState;
        this.xrdpServerPort = snapshot.serverPort;
        this.xrdpServerMessage = snapshot.serverMessage;
        this.xrdpServerBusy = snapshot.serverBusy;
    }
    private connectNative(): void {
        bjccovmshb1hr4.instrumentFunction(140);
        const validation = this.validateConnectionForm();
        if (!validation.ok) {
            bjccovmshb1hr4.instrumentBranch(140, 0, true);
            bjccovmshb1hr4.instrumentRegion(140, 1);
            const validationMessage = HomeConnectionValidation.message(validation.issues);
            this.rdpClientController.beginConnect();
            this.pendingConnectionProfileSave = null;
            this.setConnectionFeedback(validationMessage, 'warning');
            RdpLogger.warn(`Connection validation failed: ${validationMessage}`);
            return;
        }
        else {
            bjccovmshb1hr4.instrumentBranch(140, 0, false);
        }
        bjccovmshb1hr4.instrumentRegion(140, 2);
        const policy = this.certPolicy;
        const host = validation.host;
        const port = validation.port;
        const username = validation.username;
        const password = validation.password;
        const appFilesDir = this.getAppFilesDir();
        this.host = host;
        this.port = port;
        this.username = username;
        this.pendingConnectionProfileSave = {
            profileId: this.selectedConnectionProfileId,
            host: host,
            port: port,
            username: username,
            password: password,
            certPolicy: policy,
            rememberPassword: this.rememberConnectionPassword
        };
        this.lastConnectionErrorMessage = '';
        this.rdpClientController.beginConnect();
        this.remoteLoginWaiting = false;
        this.setConnectionFeedback(HomeText.CONNECTION_CONNECTING, 'info');
        this.sessionNoticeTitle = HomeText.SESSION_CONNECTING_TITLE;
        this.sessionNoticeSubtitle = HomeText.SESSION_CONNECTING_SUBTITLE;
        this.setSessionVisible(true);
        this.queueNativeConnect(host, port, username, password, policy, appFilesDir);
    }
    private queueNativeConnect(host: string, port: string, username: string, password: string, policy: string, appFilesDir: string): void {
        bjccovmshb1hr4.instrumentFunction(141);
        bjccovmshb1hr4.instrumentRegion(141, 1);
        setTimeout((): void => {
            bjccovmshb1hr4.instrumentFunction(142);
            bjccovmshb1hr4.instrumentRegion(142, 1);
            this.startNativeConnect(host, port, username, password, policy, appFilesDir);
        }, 0);
    }
    private startNativeConnect(host: string, port: string, username: string, password: string, policy: string, appFilesDir: string): void {
        bjccovmshb1hr4.instrumentFunction(143);
        try {
            bjccovmshb1hr4.instrumentRegion(143, 1);
            const result = this.rdpClientController.connect({
                host: host,
                port: port,
                username: username,
                password: password,
                certPolicy: policy,
                appFilesDir: appFilesDir
            });
            if (result.ok) {
                bjccovmshb1hr4.instrumentBranch(143, 0, true);
                bjccovmshb1hr4.instrumentRegion(143, 3);
                this.setSessionVisible(true);
                if (this.isConnected()) {
                    bjccovmshb1hr4.instrumentBranch(143, 1, true);
                    bjccovmshb1hr4.instrumentRegion(143, 5);
                    this.lastConnectionErrorMessage = '';
                    this.setConnectionFeedback(HomeText.CONNECTION_SUCCESS, 'ok');
                    this.persistPendingConnectionProfile();
                }
                else {
                    bjccovmshb1hr4.instrumentBranch(143, 1, false);
                }
            }
            else {
                bjccovmshb1hr4.instrumentBranch(143, 0, false);
                bjccovmshb1hr4.instrumentRegion(143, 4);
                RdpLogger.error(result.message);
                this.setSessionVisible(false);
                this.clearSessionNotice();
                this.pendingConnectionProfileSave = null;
                this.lastConnectionErrorMessage = result.message;
                this.setConnectionFeedback(this.connectionFailureMessage(result.message), 'danger');
            }
        }
        catch (error) {
            bjccovmshb1hr4.instrumentRegion(143, 2);
            this.setSessionVisible(false);
            this.clearSessionNotice();
            this.pendingConnectionProfileSave = null;
            this.lastConnectionErrorMessage = this.errorMessage(error as Error);
            this.setConnectionFeedback(this.connectionFailureMessage(this.lastConnectionErrorMessage, HomeText.CONNECTION_FAILURE_CREDENTIALS), 'danger');
            RdpLogger.error(`Native connect failed: ${JSON.stringify(error)}`);
        }
    }
    private setSessionVisible(visible: boolean): void {
        bjccovmshb1hr4.instrumentFunction(144);
        if (!visible && this.showSession && this.isConnected()) {
            bjccovmshb1hr4.instrumentBranch(144, 0, true);
            bjccovmshb1hr4.instrumentRegion(144, 1);
            this.releaseActiveInput();
        }
        else {
            bjccovmshb1hr4.instrumentBranch(144, 0, false);
        }
        this.showSession = visible;
        if (!visible) {
            bjccovmshb1hr4.instrumentBranch(144, 1, true);
            bjccovmshb1hr4.instrumentRegion(144, 2);
            this.clearSessionNotice();
        }
        else {
            bjccovmshb1hr4.instrumentBranch(144, 1, false);
        }
    }
    private isConnected(): boolean {
        bjccovmshb1hr4.instrumentFunction(145);
        bjccovmshb1hr4.instrumentRegion(145, 1);
        return this.rdpClientController.isConnected();
    }
    private focusRemoteSurface(): void {
        bjccovmshb1hr4.instrumentFunction(146);
        if (!this.isConnected()) {
            bjccovmshb1hr4.instrumentBranch(146, 0, true);
            bjccovmshb1hr4.instrumentRegion(146, 1);
            return;
        }
        else {
            bjccovmshb1hr4.instrumentBranch(146, 0, false);
        }
        bjccovmshb1hr4.instrumentRegion(146, 2);
        this.deferUiUpdate(() => {
            bjccovmshb1hr4.instrumentFunction(147);
            const focused = focusControl.requestFocus(REMOTE_SURFACE_FOCUS_ID);
            if (!focused) {
                bjccovmshb1hr4.instrumentBranch(147, 0, true);
                bjccovmshb1hr4.instrumentRegion(147, 1);
                RdpLogger.warn('Remote surface focus failed');
            }
            else {
                bjccovmshb1hr4.instrumentBranch(147, 0, false);
            }
        });
    }
    private releaseActiveInput(): void {
        bjccovmshb1hr4.instrumentFunction(148);
        try {
            bjccovmshb1hr4.instrumentRegion(148, 1);
            const result = this.rdpClientController.releaseAllInput();
            if (!result.ok) {
                bjccovmshb1hr4.instrumentBranch(148, 0, true);
                bjccovmshb1hr4.instrumentRegion(148, 3);
                RdpLogger.warn(result.message);
            }
            else {
                bjccovmshb1hr4.instrumentBranch(148, 0, false);
            }
        }
        catch (error) {
            bjccovmshb1hr4.instrumentRegion(148, 2);
            RdpLogger.error(`Native release all input failed: ${JSON.stringify(error)}`);
        }
    }
    private getAppearanceMode(): SettingsAppearanceMode {
        bjccovmshb1hr4.instrumentFunction(149);
        bjccovmshb1hr4.instrumentRegion(149, 1);
        return SettingsTheme.normalizeAppearanceMode(this.appearanceMode);
    }
    private isAppDark(): boolean {
        bjccovmshb1hr4.instrumentFunction(150);
        const mode = this.getAppearanceMode();
        if (mode === 'dark') {
            bjccovmshb1hr4.instrumentBranch(150, 0, true);
            bjccovmshb1hr4.instrumentRegion(150, 1);
            return true;
        }
        else {
            bjccovmshb1hr4.instrumentBranch(150, 0, false);
        }
        if (mode === 'light') {
            bjccovmshb1hr4.instrumentBranch(150, 1, true);
            bjccovmshb1hr4.instrumentRegion(150, 2);
            return false;
        }
        else {
            bjccovmshb1hr4.instrumentBranch(150, 1, false);
        }
        bjccovmshb1hr4.instrumentRegion(150, 3);
        return this.systemDark;
    }
    initialRender() {
        bjccovmshb1hr4.instrumentFunction(151);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1hr4.instrumentFunction(152);
            Column.create();
            Column.width('100%');
            Column.height('100%');
            Column.backgroundColor(SettingsTheme.pageBackground(this.isAppDark()));
        }, Column);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1hr4.instrumentFunction(153);
            If.create();
            if (this.showSession) {
                bjccovmshb1hr4.instrumentBranch(153, 0, true);
                bjccovmshb1hr4.instrumentRegion(153, 1);
                this.ifElseBranchUpdateFunction(0, () => {
                    bjccovmshb1hr4.instrumentFunction(154);
                    {
                        this.observeComponentCreation2((elmtId, isInitialRender) => {
                            bjccovmshb1hr4.instrumentFunction(155);
                            if (isInitialRender) {
                                let componentCall = new RdpSessionPage(this, {
                                    noticeTitle: this.sessionNoticeTitle,
                                    noticeSubtitle: this.sessionNoticeSubtitle,
                                    remoteLoginWaiting: this.remoteLoginWaiting,
                                    onAttachContent: (content: NodeContent): boolean => { bjccovmshb1hr4.instrumentFunction(156); return this.rdpSurfaceContentHost.attach(content); },
                                    onDetachContent: (): void => {
                                        bjccovmshb1hr4.instrumentFunction(157);
                                        bjccovmshb1hr4.instrumentRegion(157, 1);
                                        this.rdpSurfaceContentHost.detach();
                                    },
                                    onSurfaceLoad: (): void => {
                                        bjccovmshb1hr4.instrumentFunction(158);
                                        bjccovmshb1hr4.instrumentRegion(158, 1);
                                        this.deferUiUpdate(() => {
                                            bjccovmshb1hr4.instrumentFunction(159);
                                            bjccovmshb1hr4.instrumentRegion(159, 1);
                                            this.focusRemoteSurface();
                                        });
                                    }
                                }, undefined, elmtId, () => { }, { page: "common/src/main/ets/pages/Index.ets", line: 818, col: 9 });
                                ViewPU.create(componentCall);
                                let paramsLambda = () => {
                                    return {
                                        noticeTitle: this.sessionNoticeTitle,
                                        noticeSubtitle: this.sessionNoticeSubtitle,
                                        remoteLoginWaiting: this.remoteLoginWaiting,
                                        onAttachContent: (content: NodeContent): boolean => this.rdpSurfaceContentHost.attach(content),
                                        onDetachContent: (): void => {
                                            this.rdpSurfaceContentHost.detach();
                                        },
                                        onSurfaceLoad: (): void => {
                                            this.deferUiUpdate(() => {
                                                this.focusRemoteSurface();
                                            });
                                        }
                                    };
                                };
                                componentCall.paramsGenerator_ = paramsLambda;
                            }
                            else {
                                this.updateStateVarsOfChildByElmtId(elmtId, {
                                    noticeTitle: this.sessionNoticeTitle,
                                    noticeSubtitle: this.sessionNoticeSubtitle,
                                    remoteLoginWaiting: this.remoteLoginWaiting
                                });
                            }
                        }, { name: "RdpSessionPage" });
                    }
                });
            }
            else {
                bjccovmshb1hr4.instrumentBranch(153, 0, false);
                bjccovmshb1hr4.instrumentRegion(153, 2);
                if (this.showSettings) {
                    bjccovmshb1hr4.instrumentBranch(153, 1, true);
                    bjccovmshb1hr4.instrumentRegion(153, 3);
                    this.ifElseBranchUpdateFunction(1, () => {
                        bjccovmshb1hr4.instrumentFunction(160);
                        {
                            this.observeComponentCreation2((elmtId, isInitialRender) => {
                                bjccovmshb1hr4.instrumentFunction(161);
                                if (isInitialRender) {
                                    let componentCall = new SettingsPage(this, {
                                        layoutMode: this.layoutMode,
                                        initialPageName: this.settingsInitialPageName,
                                        initialRemoteControlSection: this.settingsRemoteControlSection,
                                        remoteAccessCode: this.remoteAccessCode,
                                        remoteAccessCodeGateEnabled: this.remoteAccessCodeGateEnabled,
                                        screenRecordingPermissionGranted: this.screenRecordingPermissionGranted,
                                        screenRecordingPermissionBusy: this.screenRecordingPermissionBusy,
                                        inputInjectionPermissionGranted: this.inputInjectionPermissionGranted,
                                        inputInjectionPermissionBusy: this.inputInjectionPermissionBusy,
                                        xrdpServerRunning: this.xrdpServerRunning,
                                        xrdpServerState: this.xrdpServerState,
                                        xrdpServerPort: this.xrdpServerPort,
                                        xrdpServerMessage: this.xrdpServerMessage,
                                        xrdpServerBusy: this.xrdpServerBusy,
                                        remoteControlServerAvailable: this.remoteControlServerAvailable(),
                                        onRemoteAccessCodeGateChange: (enabled: boolean): string => { bjccovmshb1hr4.instrumentFunction(162); return this.setRemoteAccessCodeGateFromSettings(enabled); },
                                        onRemoteAccessCodeRegenerate: (): string => { bjccovmshb1hr4.instrumentFunction(163); return this.regenerateRemoteAccessCodeFromSettings(); },
                                        onRequestScreenRecordingPermission: (): Promise<boolean> => { bjccovmshb1hr4.instrumentFunction(164); return this.requestScreenRecordingPermissionFromSettings(); },
                                        onRefreshScreenRecordingPermission: (): boolean => { bjccovmshb1hr4.instrumentFunction(165); return this.refreshScreenRecordingPermissionState(); },
                                        onRequestInputInjectionPermission: (): Promise<boolean> => { bjccovmshb1hr4.instrumentFunction(166); return this.requestInputInjectionPermissionFromSettings(); },
                                        onRefreshInputInjectionPermission: (): boolean => { bjccovmshb1hr4.instrumentFunction(167); return this.refreshInputInjectionPermissionState(); },
                                        onRefreshXrdpServerStatus: (): XrdpServerStatus => { bjccovmshb1hr4.instrumentFunction(168); return this.refreshXrdpServerDiagnostics(); },
                                        onStartXrdpServer: (): Promise<XrdpServerStatus> => { bjccovmshb1hr4.instrumentFunction(169); return this.startXrdpServerFromSettings(); },
                                        onOpenRemoteFilesDirectory: (): void => { bjccovmshb1hr4.instrumentFunction(170); return this.openRemoteFilesDirectoryFromSettings(); },
                                        onClose: () => {
                                            bjccovmshb1hr4.instrumentFunction(171);
                                            if (this.remoteControlServerAvailable()) {
                                                bjccovmshb1hr4.instrumentBranch(171, 0, true);
                                                bjccovmshb1hr4.instrumentRegion(171, 1);
                                                this.refreshRemoteControlReadiness(this.xrdpServerBusy && !this.screenRecordingPermissionBusy);
                                            }
                                            else {
                                                bjccovmshb1hr4.instrumentBranch(171, 0, false);
                                            }
                                            bjccovmshb1hr4.instrumentRegion(151, 1);
                                            this.showSettings = false;
                                            this.settingsInitialPageName = SettingsRoute.SETTINGS;
                                            this.settingsRemoteControlSection = '';
                                        }
                                    }, undefined, elmtId, () => { }, { page: "common/src/main/ets/pages/Index.ets", line: 833, col: 9 });
                                    ViewPU.create(componentCall);
                                    let paramsLambda = () => {
                                        return {
                                            layoutMode: this.layoutMode,
                                            initialPageName: this.settingsInitialPageName,
                                            initialRemoteControlSection: this.settingsRemoteControlSection,
                                            remoteAccessCode: this.remoteAccessCode,
                                            remoteAccessCodeGateEnabled: this.remoteAccessCodeGateEnabled,
                                            screenRecordingPermissionGranted: this.screenRecordingPermissionGranted,
                                            screenRecordingPermissionBusy: this.screenRecordingPermissionBusy,
                                            inputInjectionPermissionGranted: this.inputInjectionPermissionGranted,
                                            inputInjectionPermissionBusy: this.inputInjectionPermissionBusy,
                                            xrdpServerRunning: this.xrdpServerRunning,
                                            xrdpServerState: this.xrdpServerState,
                                            xrdpServerPort: this.xrdpServerPort,
                                            xrdpServerMessage: this.xrdpServerMessage,
                                            xrdpServerBusy: this.xrdpServerBusy,
                                            remoteControlServerAvailable: this.remoteControlServerAvailable(),
                                            onRemoteAccessCodeGateChange: (enabled: boolean): string => this.setRemoteAccessCodeGateFromSettings(enabled),
                                            onRemoteAccessCodeRegenerate: (): string => this.regenerateRemoteAccessCodeFromSettings(),
                                            onRequestScreenRecordingPermission: (): Promise<boolean> => this.requestScreenRecordingPermissionFromSettings(),
                                            onRefreshScreenRecordingPermission: (): boolean => this.refreshScreenRecordingPermissionState(),
                                            onRequestInputInjectionPermission: (): Promise<boolean> => this.requestInputInjectionPermissionFromSettings(),
                                            onRefreshInputInjectionPermission: (): boolean => this.refreshInputInjectionPermissionState(),
                                            onRefreshXrdpServerStatus: (): XrdpServerStatus => this.refreshXrdpServerDiagnostics(),
                                            onStartXrdpServer: (): Promise<XrdpServerStatus> => this.startXrdpServerFromSettings(),
                                            onOpenRemoteFilesDirectory: (): void => this.openRemoteFilesDirectoryFromSettings(),
                                            onClose: () => {
                                                if (this.remoteControlServerAvailable()) {
                                                    this.refreshRemoteControlReadiness(this.xrdpServerBusy && !this.screenRecordingPermissionBusy);
                                                }
                                                bjccovmshb1hr4.instrumentRegion(151, 2);
                                                this.showSettings = false;
                                                this.settingsInitialPageName = SettingsRoute.SETTINGS;
                                                this.settingsRemoteControlSection = '';
                                            }
                                        };
                                    };
                                    componentCall.paramsGenerator_ = paramsLambda;
                                }
                                else {
                                    this.updateStateVarsOfChildByElmtId(elmtId, {
                                        layoutMode: this.layoutMode,
                                        initialPageName: this.settingsInitialPageName,
                                        initialRemoteControlSection: this.settingsRemoteControlSection,
                                        remoteAccessCode: this.remoteAccessCode,
                                        remoteAccessCodeGateEnabled: this.remoteAccessCodeGateEnabled,
                                        screenRecordingPermissionGranted: this.screenRecordingPermissionGranted,
                                        screenRecordingPermissionBusy: this.screenRecordingPermissionBusy,
                                        inputInjectionPermissionGranted: this.inputInjectionPermissionGranted,
                                        inputInjectionPermissionBusy: this.inputInjectionPermissionBusy,
                                        xrdpServerRunning: this.xrdpServerRunning,
                                        xrdpServerState: this.xrdpServerState,
                                        xrdpServerPort: this.xrdpServerPort,
                                        xrdpServerMessage: this.xrdpServerMessage,
                                        xrdpServerBusy: this.xrdpServerBusy,
                                        remoteControlServerAvailable: this.remoteControlServerAvailable()
                                    });
                                }
                            }, { name: "SettingsPage" });
                        }
                    });
                }
                else {
                    bjccovmshb1hr4.instrumentBranch(153, 1, false);
                    bjccovmshb1hr4.instrumentRegion(153, 4);
                    this.ifElseBranchUpdateFunction(2, () => {
                        bjccovmshb1hr4.instrumentFunction(172);
                        {
                            this.observeComponentCreation2((elmtId, isInitialRender) => {
                                bjccovmshb1hr4.instrumentFunction(173);
                                if (isInitialRender) {
                                    let componentCall = new HomePage(this, {
                                        layoutMode: this.layoutMode,
                                        host: this.__host,
                                        port: this.__port,
                                        username: this.__username,
                                        password: this.__password,
                                        connectionProfiles: this.connectionProfiles,
                                        selectedConnectionProfileId: this.selectedConnectionProfileId,
                                        rememberPassword: this.__rememberConnectionPassword,
                                        passwordLoading: this.connectionProfilePasswordLoading,
                                        connectionFeedbackText: this.connectionFeedbackText,
                                        connectionFeedbackTone: this.connectionFeedbackTone,
                                        isDark: this.isAppDark(),
                                        xrdpServerBusy: this.xrdpServerBusy,
                                        xrdpServerState: this.xrdpServerState,
                                        xrdpServerRunning: this.xrdpServerRunning,
                                        screenRecordingPermissionGranted: this.screenRecordingPermissionGranted,
                                        screenRecordingPermissionBusy: this.screenRecordingPermissionBusy,
                                        inputInjectionPermissionGranted: this.inputInjectionPermissionGranted,
                                        inputInjectionPermissionBusy: this.inputInjectionPermissionBusy,
                                        remoteControlServerAvailable: this.remoteControlServerAvailable(),
                                        onHostChange: (value: string): void => {
                                            bjccovmshb1hr4.instrumentFunction(174);
                                            this.host = value;
                                            bjccovmshb1hr4.instrumentRegion(151, 3);
                                            this.clearConnectionFeedback();
                                        },
                                        onPortChange: (value: string): void => {
                                            bjccovmshb1hr4.instrumentFunction(175);
                                            this.port = value;
                                            bjccovmshb1hr4.instrumentRegion(151, 4);
                                            this.clearConnectionFeedback();
                                        },
                                        onUsernameChange: (value: string): void => {
                                            bjccovmshb1hr4.instrumentFunction(176);
                                            this.username = value;
                                            bjccovmshb1hr4.instrumentRegion(151, 5);
                                            this.clearConnectionFeedback();
                                        },
                                        onPasswordChange: (value: string): void => {
                                            bjccovmshb1hr4.instrumentFunction(177);
                                            this.password = value;
                                            bjccovmshb1hr4.instrumentRegion(151, 6);
                                            this.clearConnectionFeedback();
                                        },
                                        onProfileSelect: (profileId: string): void => {
                                            bjccovmshb1hr4.instrumentFunction(178);
                                            bjccovmshb1hr4.instrumentRegion(178, 1);
                                            this.selectConnectionProfile(profileId);
                                        },
                                        onNewProfile: (): void => {
                                            bjccovmshb1hr4.instrumentFunction(179);
                                            bjccovmshb1hr4.instrumentRegion(179, 1);
                                            this.startNewConnectionProfile();
                                        },
                                        onDeleteProfile: (): void => {
                                            bjccovmshb1hr4.instrumentFunction(180);
                                            bjccovmshb1hr4.instrumentRegion(180, 1);
                                            this.deleteSelectedConnectionProfile();
                                        },
                                        onRememberPasswordChange: (remember: boolean): void => {
                                            bjccovmshb1hr4.instrumentFunction(181);
                                            bjccovmshb1hr4.instrumentRegion(181, 1);
                                            this.setRememberConnectionPassword(remember);
                                        },
                                        onClearPassword: (): void => {
                                            bjccovmshb1hr4.instrumentFunction(182);
                                            bjccovmshb1hr4.instrumentRegion(182, 1);
                                            this.clearSelectedConnectionPassword();
                                        },
                                        onConnect: (): void => {
                                            bjccovmshb1hr4.instrumentFunction(183);
                                            bjccovmshb1hr4.instrumentRegion(183, 1);
                                            this.connectNative();
                                        },
                                        onOpenSettings: (remoteControlSection: string): void => {
                                            bjccovmshb1hr4.instrumentFunction(184);
                                            bjccovmshb1hr4.instrumentRegion(184, 1);
                                            this.openSettingsFromHome(remoteControlSection);
                                        }
                                    }, undefined, elmtId, () => { }, { page: "common/src/main/ets/pages/Index.ets", line: 869, col: 9 });
                                    ViewPU.create(componentCall);
                                    let paramsLambda = () => {
                                        return {
                                            layoutMode: this.layoutMode,
                                            host: this.host,
                                            port: this.port,
                                            username: this.username,
                                            password: this.password,
                                            connectionProfiles: this.connectionProfiles,
                                            selectedConnectionProfileId: this.selectedConnectionProfileId,
                                            rememberPassword: this.rememberConnectionPassword,
                                            passwordLoading: this.connectionProfilePasswordLoading,
                                            connectionFeedbackText: this.connectionFeedbackText,
                                            connectionFeedbackTone: this.connectionFeedbackTone,
                                            isDark: this.isAppDark(),
                                            xrdpServerBusy: this.xrdpServerBusy,
                                            xrdpServerState: this.xrdpServerState,
                                            xrdpServerRunning: this.xrdpServerRunning,
                                            screenRecordingPermissionGranted: this.screenRecordingPermissionGranted,
                                            screenRecordingPermissionBusy: this.screenRecordingPermissionBusy,
                                            inputInjectionPermissionGranted: this.inputInjectionPermissionGranted,
                                            inputInjectionPermissionBusy: this.inputInjectionPermissionBusy,
                                            remoteControlServerAvailable: this.remoteControlServerAvailable(),
                                            onHostChange: (value: string): void => {
                                                this.host = value;
                                                bjccovmshb1hr4.instrumentRegion(151, 7);
                                                this.clearConnectionFeedback();
                                            },
                                            onPortChange: (value: string): void => {
                                                this.port = value;
                                                bjccovmshb1hr4.instrumentRegion(151, 8);
                                                this.clearConnectionFeedback();
                                            },
                                            onUsernameChange: (value: string): void => {
                                                this.username = value;
                                                bjccovmshb1hr4.instrumentRegion(151, 9);
                                                this.clearConnectionFeedback();
                                            },
                                            onPasswordChange: (value: string): void => {
                                                this.password = value;
                                                bjccovmshb1hr4.instrumentRegion(151, 10);
                                                this.clearConnectionFeedback();
                                            },
                                            onProfileSelect: (profileId: string): void => {
                                                this.selectConnectionProfile(profileId);
                                            },
                                            onNewProfile: (): void => {
                                                this.startNewConnectionProfile();
                                            },
                                            onDeleteProfile: (): void => {
                                                this.deleteSelectedConnectionProfile();
                                            },
                                            onRememberPasswordChange: (remember: boolean): void => {
                                                this.setRememberConnectionPassword(remember);
                                            },
                                            onClearPassword: (): void => {
                                                this.clearSelectedConnectionPassword();
                                            },
                                            onConnect: (): void => {
                                                this.connectNative();
                                            },
                                            onOpenSettings: (remoteControlSection: string): void => {
                                                this.openSettingsFromHome(remoteControlSection);
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
                                        passwordLoading: this.connectionProfilePasswordLoading,
                                        connectionFeedbackText: this.connectionFeedbackText,
                                        connectionFeedbackTone: this.connectionFeedbackTone,
                                        isDark: this.isAppDark(),
                                        xrdpServerBusy: this.xrdpServerBusy,
                                        xrdpServerState: this.xrdpServerState,
                                        xrdpServerRunning: this.xrdpServerRunning,
                                        screenRecordingPermissionGranted: this.screenRecordingPermissionGranted,
                                        screenRecordingPermissionBusy: this.screenRecordingPermissionBusy,
                                        inputInjectionPermissionGranted: this.inputInjectionPermissionGranted,
                                        inputInjectionPermissionBusy: this.inputInjectionPermissionBusy,
                                        remoteControlServerAvailable: this.remoteControlServerAvailable()
                                    });
                                }
                            }, { name: "HomePage" });
                        }
                    });
                }
            }
        }, If);
        If.pop();
        Column.pop();
    }
    rerender() {
        this.updateDirtyElements();
    }
}
