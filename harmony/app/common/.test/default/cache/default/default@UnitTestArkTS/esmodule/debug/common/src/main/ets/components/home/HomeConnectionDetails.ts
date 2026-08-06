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
let bjccovmshb1i48 = new BjcCov({ version: "bjc v1.0.0", versionCode: 10000, path: "common/src/main/ets/components/home/HomeConnectionDetails.ets", hash: "17f8fb4f98a9707cd204aa05ef3ca7377d763b0f04e80390ca012e3e24a865aa", lineCnt: 385, count: 0, projectPath: "", functions: { 0: { name: "anonymous_1", count: 0, regions: { 0: { startLoc: { line: 20, col: 17 }, endLoc: { line: 20, col: 40 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 0 }, 1: { name: "anonymous_2", count: 0, regions: { 0: { startLoc: { line: 22, col: 17 }, endLoc: { line: 22, col: 40 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 1 }, 2: { name: "anonymous_3", count: 0, regions: { 0: { startLoc: { line: 24, col: 21 }, endLoc: { line: 24, col: 44 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 2 }, 3: { name: "anonymous_4", count: 0, regions: { 0: { startLoc: { line: 26, col: 21 }, endLoc: { line: 26, col: 44 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 3 }, 4: { name: "anonymous_5", count: 0, regions: { 0: { startLoc: { line: 28, col: 20 }, endLoc: { line: 28, col: 30 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 4 }, 5: { name: "anonymous_6", count: 0, regions: { 0: { startLoc: { line: 30, col: 29 }, endLoc: { line: 30, col: 56 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 5 }, 6: { name: "anonymous_7", count: 0, regions: { 0: { startLoc: { line: 32, col: 20 }, endLoc: { line: 32, col: 30 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 6 }, 7: { name: "anonymous_8", count: 0, regions: { 0: { startLoc: { line: 34, col: 14 }, endLoc: { line: 34, col: 24 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 7 }, 8: { name: "anonymous_9", count: 0, regions: { 0: { startLoc: { line: 20, col: 43 }, endLoc: { line: 21, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 8 }, 9: { name: "anonymous_10", count: 0, regions: { 0: { startLoc: { line: 22, col: 43 }, endLoc: { line: 23, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 9 }, 10: { name: "anonymous_11", count: 0, regions: { 0: { startLoc: { line: 24, col: 47 }, endLoc: { line: 25, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 10 }, 11: { name: "anonymous_12", count: 0, regions: { 0: { startLoc: { line: 26, col: 47 }, endLoc: { line: 27, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 11 }, 12: { name: "anonymous_13", count: 0, regions: { 0: { startLoc: { line: 28, col: 33 }, endLoc: { line: 29, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 12 }, 13: { name: "anonymous_14", count: 0, regions: { 0: { startLoc: { line: 30, col: 59 }, endLoc: { line: 31, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 13 }, 14: { name: "anonymous_15", count: 0, regions: { 0: { startLoc: { line: 32, col: 33 }, endLoc: { line: 33, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 14 }, 15: { name: "anonymous_16", count: 0, regions: { 0: { startLoc: { line: 34, col: 27 }, endLoc: { line: 35, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 15 }, 16: { name: "setInitiallyProvidedValue", count: 0, regions: { 0: { startLoc: { line: 9, col: 34 }, endLoc: { line: 19, col: 32 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 16 }, 17: { name: "updateStateVars", count: 0, regions: { 0: { startLoc: { line: 9, col: 9 }, endLoc: { line: 19, col: 15 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 17 }, 18: { name: "layoutMode", count: 0, regions: { 0: { startLoc: { line: 9, col: 9 }, endLoc: { line: 9, col: 19 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 18 }, 19: { name: "layoutMode", count: 0, regions: { 0: { startLoc: { line: 9, col: 9 }, endLoc: { line: 9, col: 31 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 19 }, 20: { name: "host", count: 0, regions: { 0: { startLoc: { line: 10, col: 9 }, endLoc: { line: 10, col: 13 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 20 }, 21: { name: "host", count: 0, regions: { 0: { startLoc: { line: 10, col: 9 }, endLoc: { line: 10, col: 21 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 21 }, 22: { name: "port", count: 0, regions: { 0: { startLoc: { line: 11, col: 9 }, endLoc: { line: 11, col: 13 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 22 }, 23: { name: "port", count: 0, regions: { 0: { startLoc: { line: 11, col: 9 }, endLoc: { line: 11, col: 21 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 23 }, 24: { name: "username", count: 0, regions: { 0: { startLoc: { line: 12, col: 9 }, endLoc: { line: 12, col: 17 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 24 }, 25: { name: "username", count: 0, regions: { 0: { startLoc: { line: 12, col: 9 }, endLoc: { line: 12, col: 25 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 25 }, 26: { name: "password", count: 0, regions: { 0: { startLoc: { line: 13, col: 9 }, endLoc: { line: 13, col: 17 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 26 }, 27: { name: "password", count: 0, regions: { 0: { startLoc: { line: 13, col: 9 }, endLoc: { line: 13, col: 25 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 27 }, 28: { name: "rememberPassword", count: 0, regions: { 0: { startLoc: { line: 14, col: 9 }, endLoc: { line: 14, col: 25 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 28 }, 29: { name: "rememberPassword", count: 0, regions: { 0: { startLoc: { line: 14, col: 9 }, endLoc: { line: 14, col: 34 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 29 }, 30: { name: "passwordLoading", count: 0, regions: { 0: { startLoc: { line: 15, col: 9 }, endLoc: { line: 15, col: 24 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 30 }, 31: { name: "passwordLoading", count: 0, regions: { 0: { startLoc: { line: 15, col: 9 }, endLoc: { line: 15, col: 33 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 31 }, 32: { name: "selectedConnectionProfileId", count: 0, regions: { 0: { startLoc: { line: 16, col: 9 }, endLoc: { line: 16, col: 36 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 32 }, 33: { name: "selectedConnectionProfileId", count: 0, regions: { 0: { startLoc: { line: 16, col: 9 }, endLoc: { line: 16, col: 44 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 33 }, 34: { name: "connectionFeedbackText", count: 0, regions: { 0: { startLoc: { line: 17, col: 9 }, endLoc: { line: 17, col: 31 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 34 }, 35: { name: "connectionFeedbackText", count: 0, regions: { 0: { startLoc: { line: 17, col: 9 }, endLoc: { line: 17, col: 39 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 35 }, 36: { name: "connectionFeedbackTone", count: 0, regions: { 0: { startLoc: { line: 18, col: 9 }, endLoc: { line: 18, col: 31 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 36 }, 37: { name: "connectionFeedbackTone", count: 0, regions: { 0: { startLoc: { line: 18, col: 9 }, endLoc: { line: 18, col: 51 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 37 }, 38: { name: "isDark", count: 0, regions: { 0: { startLoc: { line: 19, col: 9 }, endLoc: { line: 19, col: 15 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 38 }, 39: { name: "isDark", count: 0, regions: { 0: { startLoc: { line: 19, col: 9 }, endLoc: { line: 19, col: 24 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 39 }, 40: { name: "anonymous_17", count: 0, regions: { 0: { startLoc: { line: 20, col: 17 }, endLoc: { line: 20, col: 40 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 40 }, 41: { name: "anonymous_18", count: 0, regions: { 0: { startLoc: { line: 22, col: 17 }, endLoc: { line: 22, col: 40 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 41 }, 42: { name: "anonymous_19", count: 0, regions: { 0: { startLoc: { line: 24, col: 21 }, endLoc: { line: 24, col: 44 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 42 }, 43: { name: "anonymous_20", count: 0, regions: { 0: { startLoc: { line: 26, col: 21 }, endLoc: { line: 26, col: 44 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 43 }, 44: { name: "anonymous_21", count: 0, regions: { 0: { startLoc: { line: 28, col: 20 }, endLoc: { line: 28, col: 30 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 44 }, 45: { name: "anonymous_22", count: 0, regions: { 0: { startLoc: { line: 30, col: 29 }, endLoc: { line: 30, col: 56 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 45 }, 46: { name: "anonymous_23", count: 0, regions: { 0: { startLoc: { line: 32, col: 20 }, endLoc: { line: 32, col: 30 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 46 }, 47: { name: "anonymous_24", count: 0, regions: { 0: { startLoc: { line: 34, col: 14 }, endLoc: { line: 34, col: 24 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 47 }, 48: { name: "hoveredAction", count: 0, regions: { 0: { startLoc: { line: 36, col: 18 }, endLoc: { line: 36, col: 31 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 48 }, 49: { name: "hoveredAction", count: 0, regions: { 0: { startLoc: { line: 36, col: 18 }, endLoc: { line: 36, col: 39 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 49 }, 50: { name: "pressedAction", count: 0, regions: { 0: { startLoc: { line: 37, col: 18 }, endLoc: { line: 37, col: 31 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 50 }, 51: { name: "pressedAction", count: 0, regions: { 0: { startLoc: { line: 37, col: 18 }, endLoc: { line: 37, col: 39 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 51 }, 52: { name: "connectHovered", count: 0, regions: { 0: { startLoc: { line: 38, col: 18 }, endLoc: { line: 38, col: 32 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 52 }, 53: { name: "connectHovered", count: 0, regions: { 0: { startLoc: { line: 38, col: 18 }, endLoc: { line: 38, col: 41 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 53 }, 54: { name: "connectPressed", count: 0, regions: { 0: { startLoc: { line: 39, col: 18 }, endLoc: { line: 39, col: 32 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 54 }, 55: { name: "connectPressed", count: 0, regions: { 0: { startLoc: { line: 39, col: 18 }, endLoc: { line: 39, col: 41 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 55 }, 56: { name: "formBreakpoint", count: 0, regions: { 0: { startLoc: { line: 40, col: 18 }, endLoc: { line: 40, col: 32 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 56 }, 57: { name: "formBreakpoint", count: 0, regions: { 0: { startLoc: { line: 40, col: 18 }, endLoc: { line: 40, col: 40 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 57 }, 58: { name: "hasSelectedProfile", count: 0, regions: { 0: { startLoc: { line: 42, col: 3 }, endLoc: { line: 44, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 43, col: 5 }, endLoc: { line: 44, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 58 }, 59: { name: "setHoveredAction", count: 0, regions: { 0: { startLoc: { line: 46, col: 3 }, endLoc: { line: 50, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 47, col: 5 }, endLoc: { line: 50, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 59 }, 60: { name: "anonymous_25", count: 0, regions: { 0: { startLoc: { line: 47, col: 48 }, endLoc: { line: 49, col: 6 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 48, col: 7 }, endLoc: { line: 49, col: 6 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 48, col: 28 }, endLoc: { line: 48, col: 49 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 60 }, 61: { name: "buildFieldLabel", count: 0, regions: { 0: { startLoc: { line: 52, col: 3 }, endLoc: { line: 61, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 61 }, 62: { name: "anonymous_26", count: 0, regions: { 0: { startLoc: { line: 54, col: 5 }, endLoc: { line: 60, col: 18 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 57, col: 18 }, endLoc: { line: 57, col: 80 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 62 }, 63: { name: "buildHostInput", count: 0, regions: { 0: { startLoc: { line: 63, col: 3 }, endLoc: { line: 83, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 81, col: 9 }, endLoc: { line: 82, col: 8 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 63 }, 64: { name: "anonymous_27", count: 0, regions: { 0: { startLoc: { line: 65, col: 5 }, endLoc: { line: 82, col: 8 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 64 }, 65: { name: "anonymous_28", count: 0, regions: { 0: { startLoc: { line: 65, col: 52 }, endLoc: { line: 65, col: 63 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 65 }, 66: { name: "anonymous_29", count: 0, regions: { 0: { startLoc: { line: 79, col: 17 }, endLoc: { line: 82, col: 8 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 66 }, 67: { name: "buildPortInput", count: 0, regions: { 0: { startLoc: { line: 85, col: 3 }, endLoc: { line: 105, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 103, col: 9 }, endLoc: { line: 104, col: 8 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 67 }, 68: { name: "anonymous_30", count: 0, regions: { 0: { startLoc: { line: 87, col: 5 }, endLoc: { line: 104, col: 8 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 68 }, 69: { name: "anonymous_31", count: 0, regions: { 0: { startLoc: { line: 87, col: 44 }, endLoc: { line: 87, col: 55 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 69 }, 70: { name: "anonymous_32", count: 0, regions: { 0: { startLoc: { line: 101, col: 17 }, endLoc: { line: 104, col: 8 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 70 }, 71: { name: "buildUsernameInput", count: 0, regions: { 0: { startLoc: { line: 107, col: 3 }, endLoc: { line: 127, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 125, col: 9 }, endLoc: { line: 126, col: 8 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 71 }, 72: { name: "anonymous_33", count: 0, regions: { 0: { startLoc: { line: 109, col: 5 }, endLoc: { line: 126, col: 8 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 72 }, 73: { name: "anonymous_34", count: 0, regions: { 0: { startLoc: { line: 109, col: 75 }, endLoc: { line: 109, col: 90 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 73 }, 74: { name: "anonymous_35", count: 0, regions: { 0: { startLoc: { line: 123, col: 17 }, endLoc: { line: 126, col: 8 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 74 }, 75: { name: "buildPasswordInput", count: 0, regions: { 0: { startLoc: { line: 129, col: 3 }, endLoc: { line: 150, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 148, col: 9 }, endLoc: { line: 149, col: 8 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 75 }, 76: { name: "anonymous_36", count: 0, regions: { 0: { startLoc: { line: 131, col: 5 }, endLoc: { line: 149, col: 8 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 76 }, 77: { name: "anonymous_37", count: 0, regions: { 0: { startLoc: { line: 131, col: 48 }, endLoc: { line: 131, col: 63 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 77 }, 78: { name: "anonymous_38", count: 0, regions: { 0: { startLoc: { line: 146, col: 17 }, endLoc: { line: 149, col: 8 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 78 }, 79: { name: "buildRememberControl", count: 0, regions: { 0: { startLoc: { line: 152, col: 3 }, endLoc: { line: 172, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 158, col: 11 }, endLoc: { line: 159, col: 10 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 79 }, 80: { name: "anonymous_39", count: 0, regions: { 0: { startLoc: { line: 154, col: 5 }, endLoc: { line: 171, col: 37 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 80 }, 81: { name: "anonymous_40", count: 0, regions: { 0: { startLoc: { line: 155, col: 7 }, endLoc: { line: 159, col: 10 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 81 }, 82: { name: "anonymous_41", count: 0, regions: { 0: { startLoc: { line: 156, col: 19 }, endLoc: { line: 159, col: 10 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 82 }, 83: { name: "anonymous_42", count: 0, regions: { 0: { startLoc: { line: 160, col: 7 }, endLoc: { line: 167, col: 8 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 160, col: 51 }, endLoc: { line: 167, col: 8 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 160, col: 11 }, endLoc: { line: 160, col: 49 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 83 }, 84: { name: "anonymous_43", count: 0, regions: { 0: { startLoc: { line: 161, col: 9 }, endLoc: { line: 161, col: 13 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 84 }, 85: { name: "anonymous_44", count: 0, regions: { 0: { startLoc: { line: 161, col: 9 }, endLoc: { line: 166, col: 26 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 85 }, 86: { name: "buildConnectButton", count: 0, regions: { 0: { startLoc: { line: 174, col: 3 }, endLoc: { line: 216, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 206, col: 9 }, endLoc: { line: 207, col: 8 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 214, col: 7 }, endLoc: { line: 215, col: 6 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 86 }, 87: { name: "anonymous_46", count: 0, regions: { 0: { startLoc: { line: 176, col: 5 }, endLoc: { line: 215, col: 6 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 199, col: 17 }, endLoc: { line: 199, col: 47 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 199, col: 52 }, endLoc: { line: 199, col: 82 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 87 }, 88: { name: "anonymous_47", count: 0, regions: { 0: { startLoc: { line: 200, col: 14 }, endLoc: { line: 202, col: 6 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 201, col: 7 }, endLoc: { line: 202, col: 6 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 88 }, 89: { name: "anonymous_48", count: 0, regions: { 0: { startLoc: { line: 203, col: 14 }, endLoc: { line: 211, col: 6 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 204, col: 42 }, endLoc: { line: 207, col: 8 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 208, col: 75 }, endLoc: { line: 210, col: 8 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 204, col: 11 }, endLoc: { line: 204, col: 40 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 208, col: 11 }, endLoc: { line: 208, col: 73 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 89 }, 90: { name: "anonymous_49", count: 0, regions: { 0: { startLoc: { line: 212, col: 14 }, endLoc: { line: 215, col: 6 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 90 }, 91: { name: "anonymous_50", count: 0, regions: { 0: { startLoc: { line: 177, col: 7 }, endLoc: { line: 190, col: 39 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 91 }, 92: { name: "anonymous_51", count: 0, regions: { 0: { startLoc: { line: 178, col: 9 }, endLoc: { line: 182, col: 27 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 92 }, 93: { name: "anonymous_52", count: 0, regions: { 0: { startLoc: { line: 183, col: 9 }, endLoc: { line: 187, col: 22 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 93 }, 94: { name: "buildActionButton", count: 0, regions: { 0: { startLoc: { line: 218, col: 3 }, endLoc: { line: 270, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 260, col: 9 }, endLoc: { line: 261, col: 8 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 268, col: 7 }, endLoc: { line: 269, col: 6 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 94 }, 95: { name: "anonymous_53", count: 0, regions: { 0: { startLoc: { line: 220, col: 14 }, endLoc: { line: 220, col: 24 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 95 }, 96: { name: "anonymous_54", count: 0, regions: { 0: { startLoc: { line: 221, col: 5 }, endLoc: { line: 269, col: 6 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 244, col: 22 }, endLoc: { line: 246, col: 48 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 245, col: 8 }, endLoc: { line: 246, col: 47 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 2: { startLoc: { line: 249, col: 14 }, endLoc: { line: 250, col: 100 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 3: { startLoc: { line: 253, col: 17 }, endLoc: { line: 253, col: 57 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 4: { startLoc: { line: 253, col: 62 }, endLoc: { line: 253, col: 102 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 96 }, 97: { name: "anonymous_55", count: 0, regions: { 0: { startLoc: { line: 254, col: 14 }, endLoc: { line: 256, col: 6 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 255, col: 7 }, endLoc: { line: 256, col: 6 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 97 }, 98: { name: "anonymous_56", count: 0, regions: { 0: { startLoc: { line: 257, col: 14 }, endLoc: { line: 265, col: 6 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 258, col: 42 }, endLoc: { line: 261, col: 8 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 262, col: 75 }, endLoc: { line: 264, col: 8 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 258, col: 11 }, endLoc: { line: 258, col: 40 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 262, col: 11 }, endLoc: { line: 262, col: 73 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 98 }, 99: { name: "anonymous_57", count: 0, regions: { 0: { startLoc: { line: 266, col: 14 }, endLoc: { line: 269, col: 6 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 99 }, 100: { name: "anonymous_58", count: 0, regions: { 0: { startLoc: { line: 222, col: 7 }, endLoc: { line: 238, col: 37 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 100 }, 101: { name: "anonymous_59", count: 0, regions: { 0: { startLoc: { line: 223, col: 9 }, endLoc: { line: 227, col: 27 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 101 }, 102: { name: "anonymous_60", count: 0, regions: { 0: { startLoc: { line: 228, col: 9 }, endLoc: { line: 233, col: 60 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 102 }, 103: { name: "buildDeviceActions", count: 0, regions: { 0: { startLoc: { line: 272, col: 3 }, endLoc: { line: 303, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 103 }, 104: { name: "anonymous_61", count: 0, regions: { 0: { startLoc: { line: 274, col: 5 }, endLoc: { line: 302, col: 6 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 104 }, 105: { name: "anonymous_62", count: 0, regions: { 0: { startLoc: { line: 300, col: 25 }, endLoc: { line: 302, col: 6 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 301, col: 7 }, endLoc: { line: 302, col: 6 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 105 }, 106: { name: "anonymous_63", count: 0, regions: { 0: { startLoc: { line: 279, col: 7 }, endLoc: { line: 279, col: 42 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 106 }, 107: { name: "anonymous_64", count: 0, regions: { 0: { startLoc: { line: 282, col: 7 }, endLoc: { line: 282, col: 42 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 107 }, 108: { name: "anonymous_65", count: 0, regions: { 0: { startLoc: { line: 283, col: 9 }, endLoc: { line: 296, col: 22 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 108 }, 109: { name: "anonymous_66", count: 0, regions: { 0: { startLoc: { line: 284, col: 11 }, endLoc: { line: 284, col: 30 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 109 }, 110: { name: "anonymous_67", count: 0, regions: { 0: { startLoc: { line: 285, col: 106 }, endLoc: { line: 287, col: 14 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 286, col: 15 }, endLoc: { line: 287, col: 14 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 110 }, 111: { name: "anonymous_68", count: 0, regions: { 0: { startLoc: { line: 289, col: 11 }, endLoc: { line: 289, col: 30 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 111 }, 112: { name: "anonymous_69", count: 0, regions: { 0: { startLoc: { line: 291, col: 51 }, endLoc: { line: 293, col: 16 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 292, col: 17 }, endLoc: { line: 293, col: 16 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 112 }, 113: { name: "buildConnectionFormFields", count: 0, regions: { 0: { startLoc: { line: 305, col: 3 }, endLoc: { line: 351, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 113 }, 114: { name: "anonymous_70", count: 0, regions: { 0: { startLoc: { line: 307, col: 5 }, endLoc: { line: 350, col: 6 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 114 }, 115: { name: "anonymous_71", count: 0, regions: { 0: { startLoc: { line: 348, col: 25 }, endLoc: { line: 350, col: 6 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 349, col: 7 }, endLoc: { line: 350, col: 6 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 115 }, 116: { name: "anonymous_72", count: 0, regions: { 0: { startLoc: { line: 312, col: 7 }, endLoc: { line: 312, col: 42 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 116 }, 117: { name: "anonymous_73", count: 0, regions: { 0: { startLoc: { line: 315, col: 7 }, endLoc: { line: 315, col: 42 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 117 }, 118: { name: "anonymous_74", count: 0, regions: { 0: { startLoc: { line: 318, col: 7 }, endLoc: { line: 318, col: 42 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 118 }, 119: { name: "anonymous_75", count: 0, regions: { 0: { startLoc: { line: 321, col: 7 }, endLoc: { line: 321, col: 42 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 119 }, 120: { name: "anonymous_76", count: 0, regions: { 0: { startLoc: { line: 324, col: 7 }, endLoc: { line: 324, col: 42 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 120 }, 121: { name: "anonymous_77", count: 0, regions: { 0: { startLoc: { line: 327, col: 7 }, endLoc: { line: 327, col: 42 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 121 }, 122: { name: "anonymous_78", count: 0, regions: { 0: { startLoc: { line: 330, col: 7 }, endLoc: { line: 330, col: 42 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 122 }, 123: { name: "anonymous_79", count: 0, regions: { 0: { startLoc: { line: 333, col: 7 }, endLoc: { line: 333, col: 42 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 123 }, 124: { name: "anonymous_80", count: 0, regions: { 0: { startLoc: { line: 336, col: 7 }, endLoc: { line: 336, col: 42 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 124 }, 125: { name: "anonymous_81", count: 0, regions: { 0: { startLoc: { line: 339, col: 7 }, endLoc: { line: 339, col: 42 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 125 }, 126: { name: "anonymous_82", count: 0, regions: { 0: { startLoc: { line: 342, col: 7 }, endLoc: { line: 342, col: 68 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 126 }, 127: { name: "initialRender", count: 0, regions: { 0: { startLoc: { line: 353, col: 3 }, endLoc: { line: 383, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 127 }, 128: { name: "anonymous_83", count: 0, regions: { 0: { startLoc: { line: 354, col: 5 }, endLoc: { line: 382, col: 6 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 374, col: 14 }, endLoc: { line: 375, col: 98 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 128 }, 129: { name: "anonymous_84", count: 0, regions: { 0: { startLoc: { line: 355, col: 7 }, endLoc: { line: 360, col: 20 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 129 }, 130: { name: "anonymous_85", count: 0, regions: { 0: { startLoc: { line: 364, col: 7 }, endLoc: { line: 367, col: 38 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 130 }, 131: { name: "anonymous_86", count: 0, regions: { 0: { startLoc: { line: 369, col: 7 }, endLoc: { line: 371, col: 8 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 369, col: 38 }, endLoc: { line: 371, col: 8 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 369, col: 11 }, endLoc: { line: 369, col: 36 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 131 }, 132: { name: "anonymous_87", count: 0, regions: { 0: { startLoc: { line: 370, col: 9 }, endLoc: { line: 370, col: 34 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 132 } }, exeLine: { 0: 1, 1: 2, 2: 3, 3: 4, 4: 5, 5: 8, 6: 9, 7: 10, 8: 11, 9: 12, 10: 13, 11: 14, 12: 15, 13: 16, 14: 17, 15: 18, 16: 19, 17: 20, 18: 22, 19: 24, 20: 26, 21: 28, 22: 30, 23: 32, 24: 34, 25: 36, 26: 37, 27: 38, 28: 39, 29: 40, 30: 42, 31: 43, 32: 46, 33: 47, 34: 48, 35: 53, 36: 54, 37: 55, 38: 56, 39: 57, 40: 58, 41: 59, 42: 60, 43: 64, 44: 65, 45: 66, 46: 67, 47: 68, 48: 69, 49: 70, 50: 71, 51: 72, 52: 73, 53: 74, 54: 75, 55: 76, 56: 77, 57: 79, 58: 80, 59: 81, 60: 86, 61: 87, 62: 88, 63: 89, 64: 90, 65: 91, 66: 92, 67: 93, 68: 94, 69: 95, 70: 96, 71: 97, 72: 98, 73: 99, 74: 101, 75: 102, 76: 103, 77: 108, 78: 109, 79: 110, 80: 111, 81: 112, 82: 113, 83: 114, 84: 115, 85: 116, 86: 117, 87: 118, 88: 119, 89: 120, 90: 121, 91: 123, 92: 124, 93: 125, 94: 130, 95: 131, 96: 132, 97: 133, 98: 134, 99: 135, 100: 136, 101: 137, 102: 138, 103: 139, 104: 140, 105: 141, 106: 142, 107: 143, 108: 145, 109: 146, 110: 147, 111: 148, 112: 153, 113: 154, 114: 155, 115: 156, 116: 157, 117: 158, 118: 160, 119: 161, 120: 162, 121: 163, 122: 164, 123: 165, 124: 166, 125: 169, 126: 170, 127: 171, 128: 175, 129: 176, 130: 177, 131: 178, 132: 179, 133: 180, 134: 181, 135: 182, 136: 183, 137: 184, 138: 185, 139: 186, 140: 187, 141: 189, 142: 190, 143: 192, 144: 193, 145: 194, 146: 195, 147: 196, 148: 197, 149: 198, 150: 199, 151: 200, 152: 201, 153: 203, 154: 204, 155: 205, 156: 206, 157: 208, 158: 209, 159: 212, 160: 213, 161: 214, 162: 219, 163: 220, 164: 221, 165: 222, 166: 223, 167: 224, 168: 225, 169: 226, 170: 227, 171: 228, 172: 229, 173: 230, 174: 231, 175: 232, 176: 233, 177: 235, 178: 236, 179: 237, 180: 238, 181: 240, 182: 241, 183: 242, 184: 243, 185: 244, 186: 245, 187: 246, 188: 247, 189: 248, 190: 249, 191: 250, 192: 252, 193: 253, 194: 254, 195: 255, 196: 257, 197: 258, 198: 259, 199: 260, 200: 262, 201: 263, 202: 266, 203: 267, 204: 268, 205: 273, 206: 274, 207: 275, 208: 276, 209: 277, 210: 279, 211: 280, 212: 282, 213: 283, 214: 284, 215: 285, 216: 286, 217: 289, 218: 290, 219: 291, 220: 292, 221: 296, 222: 299, 223: 300, 224: 301, 225: 306, 226: 307, 227: 308, 228: 309, 229: 310, 230: 312, 231: 313, 232: 315, 233: 316, 234: 318, 235: 319, 236: 321, 237: 322, 238: 324, 239: 325, 240: 327, 241: 328, 242: 330, 243: 331, 244: 333, 245: 334, 246: 336, 247: 337, 248: 339, 249: 340, 250: 342, 251: 343, 252: 346, 253: 347, 254: 348, 255: 349, 256: 353, 257: 354, 258: 355, 259: 356, 260: 357, 261: 358, 262: 359, 263: 360, 264: 362, 265: 364, 266: 365, 267: 366, 268: 367, 269: 369, 270: 370, 271: 373, 272: 374, 273: 375, 274: 376, 275: 377, 276: 378, 277: 379, 278: 380, 279: 381 } });
if (!("finalizeConstruction" in ViewPU.prototype)) {
    Reflect.set(ViewPU.prototype, "finalizeConstruction", () => { });
}
interface HomeConnectionDetails_Params {
    layoutMode?: LayoutMode;
    host?: string;
    port?: string;
    username?: string;
    password?: string;
    rememberPassword?: boolean;
    passwordLoading?: boolean;
    selectedConnectionProfileId?: string;
    connectionFeedbackText?: string;
    connectionFeedbackTone?: SettingsStatusTone;
    isDark?: boolean;
    onHostChange?: (value: string) => void;
    onPortChange?: (value: string) => void;
    onUsernameChange?: (value: string) => void;
    onPasswordChange?: (value: string) => void;
    onDeleteProfile?: () => void;
    onRememberPasswordChange?: (remember: boolean) => void;
    onClearPassword?: () => void;
    onConnect?: () => void;
    hoveredAction?: string;
    pressedAction?: string;
    connectHovered?: boolean;
    connectPressed?: boolean;
    formBreakpoint?: string;
}
import { LayoutMode } from "@normalized:N&&&common/src/main/ets/adaptive/WindowLayoutPolicy&";
import { SettingsTheme } from "@normalized:N&&&common/src/main/ets/components/settings/SettingsUi&";
import type { SettingsStatusTone } from "@normalized:N&&&common/src/main/ets/components/settings/SettingsUi&";
import { HomeResources } from "@normalized:N&&&common/src/main/ets/components/home/HomeResources&";
import { HomeText } from "@normalized:N&&&common/src/main/ets/components/home/HomeText&";
import { HomeTheme } from "@normalized:N&&&common/src/main/ets/components/home/HomeTheme&";
export class HomeConnectionDetails extends ViewPU {
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
        this.__rememberPassword = new SynchedPropertySimpleTwoWayPU(params.rememberPassword, this, "rememberPassword");
        this.__passwordLoading = new SynchedPropertySimpleOneWayPU(params.passwordLoading, this, "passwordLoading");
        this.__selectedConnectionProfileId = new SynchedPropertySimpleOneWayPU(params.selectedConnectionProfileId, this, "selectedConnectionProfileId");
        this.__connectionFeedbackText = new SynchedPropertySimpleOneWayPU(params.connectionFeedbackText, this, "connectionFeedbackText");
        this.__connectionFeedbackTone = new SynchedPropertySimpleOneWayPU(params.connectionFeedbackTone, this, "connectionFeedbackTone");
        this.__isDark = new SynchedPropertySimpleOneWayPU(params.isDark, this, "isDark");
        this.onHostChange = (_value: string) => {
            bjccovmshb1i48.instrumentFunction(8);
        };
        this.onPortChange = (_value: string) => {
            bjccovmshb1i48.instrumentFunction(9);
        };
        this.onUsernameChange = (_value: string) => {
            bjccovmshb1i48.instrumentFunction(10);
        };
        this.onPasswordChange = (_value: string) => {
            bjccovmshb1i48.instrumentFunction(11);
        };
        this.onDeleteProfile = () => {
            bjccovmshb1i48.instrumentFunction(12);
        };
        this.onRememberPasswordChange = (_remember: boolean) => {
            bjccovmshb1i48.instrumentFunction(13);
        };
        this.onClearPassword = () => {
            bjccovmshb1i48.instrumentFunction(14);
        };
        this.onConnect = () => {
            bjccovmshb1i48.instrumentFunction(15);
        };
        this.__hoveredAction = new ObservedPropertySimplePU('', this, "hoveredAction");
        this.__pressedAction = new ObservedPropertySimplePU('', this, "pressedAction");
        this.__connectHovered = new ObservedPropertySimplePU(false, this, "connectHovered");
        this.__connectPressed = new ObservedPropertySimplePU(false, this, "connectPressed");
        this.__formBreakpoint = new ObservedPropertySimplePU('xs', this, "formBreakpoint");
        this.setInitiallyProvidedValue(params);
        this.finalizeConstruction();
    }
    setInitiallyProvidedValue(params: HomeConnectionDetails_Params) {
        bjccovmshb1i48.instrumentFunction(16);
        if (params.layoutMode === undefined) {
            this.__layoutMode.set(LayoutMode.COMPACT);
        }
        else {
        }
        if (params.passwordLoading === undefined) {
            this.__passwordLoading.set(false);
        }
        else {
        }
        if (params.selectedConnectionProfileId === undefined) {
            this.__selectedConnectionProfileId.set('');
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
        if (params.hoveredAction !== undefined) {
            this.hoveredAction = params.hoveredAction;
        }
        else {
        }
        if (params.pressedAction !== undefined) {
            this.pressedAction = params.pressedAction;
        }
        else {
        }
        if (params.connectHovered !== undefined) {
            this.connectHovered = params.connectHovered;
        }
        else {
        }
        if (params.connectPressed !== undefined) {
            this.connectPressed = params.connectPressed;
        }
        else {
        }
        if (params.formBreakpoint !== undefined) {
            this.formBreakpoint = params.formBreakpoint;
        }
        else {
        }
    }
    updateStateVars(params: HomeConnectionDetails_Params) {
        bjccovmshb1i48.instrumentFunction(17);
        this.__layoutMode.reset(params.layoutMode);
        this.__passwordLoading.reset(params.passwordLoading);
        this.__selectedConnectionProfileId.reset(params.selectedConnectionProfileId);
        this.__connectionFeedbackText.reset(params.connectionFeedbackText);
        this.__connectionFeedbackTone.reset(params.connectionFeedbackTone);
        this.__isDark.reset(params.isDark);
    }
    purgeVariableDependenciesOnElmtId(rmElmtId) {
        this.__layoutMode.purgeDependencyOnElmtId(rmElmtId);
        this.__host.purgeDependencyOnElmtId(rmElmtId);
        this.__port.purgeDependencyOnElmtId(rmElmtId);
        this.__username.purgeDependencyOnElmtId(rmElmtId);
        this.__password.purgeDependencyOnElmtId(rmElmtId);
        this.__rememberPassword.purgeDependencyOnElmtId(rmElmtId);
        this.__passwordLoading.purgeDependencyOnElmtId(rmElmtId);
        this.__selectedConnectionProfileId.purgeDependencyOnElmtId(rmElmtId);
        this.__connectionFeedbackText.purgeDependencyOnElmtId(rmElmtId);
        this.__connectionFeedbackTone.purgeDependencyOnElmtId(rmElmtId);
        this.__isDark.purgeDependencyOnElmtId(rmElmtId);
        this.__hoveredAction.purgeDependencyOnElmtId(rmElmtId);
        this.__pressedAction.purgeDependencyOnElmtId(rmElmtId);
        this.__connectHovered.purgeDependencyOnElmtId(rmElmtId);
        this.__connectPressed.purgeDependencyOnElmtId(rmElmtId);
        this.__formBreakpoint.purgeDependencyOnElmtId(rmElmtId);
    }
    aboutToBeDeleted() {
        this.__layoutMode.aboutToBeDeleted();
        this.__host.aboutToBeDeleted();
        this.__port.aboutToBeDeleted();
        this.__username.aboutToBeDeleted();
        this.__password.aboutToBeDeleted();
        this.__rememberPassword.aboutToBeDeleted();
        this.__passwordLoading.aboutToBeDeleted();
        this.__selectedConnectionProfileId.aboutToBeDeleted();
        this.__connectionFeedbackText.aboutToBeDeleted();
        this.__connectionFeedbackTone.aboutToBeDeleted();
        this.__isDark.aboutToBeDeleted();
        this.__hoveredAction.aboutToBeDeleted();
        this.__pressedAction.aboutToBeDeleted();
        this.__connectHovered.aboutToBeDeleted();
        this.__connectPressed.aboutToBeDeleted();
        this.__formBreakpoint.aboutToBeDeleted();
        SubscriberManager.Get().delete(this.id__());
        this.aboutToBeDeletedInternal();
    }
    private __layoutMode: SynchedPropertySimpleOneWayPU<LayoutMode>;
    get layoutMode() {
        bjccovmshb1i48.instrumentFunction(18);
        return this.__layoutMode.get();
    }
    set layoutMode(newValue: LayoutMode) {
        bjccovmshb1i48.instrumentFunction(19);
        this.__layoutMode.set(newValue);
    }
    private __host: SynchedPropertySimpleTwoWayPU<string>;
    get host() {
        bjccovmshb1i48.instrumentFunction(20);
        return this.__host.get();
    }
    set host(newValue: string) {
        bjccovmshb1i48.instrumentFunction(21);
        this.__host.set(newValue);
    }
    private __port: SynchedPropertySimpleTwoWayPU<string>;
    get port() {
        bjccovmshb1i48.instrumentFunction(22);
        return this.__port.get();
    }
    set port(newValue: string) {
        bjccovmshb1i48.instrumentFunction(23);
        this.__port.set(newValue);
    }
    private __username: SynchedPropertySimpleTwoWayPU<string>;
    get username() {
        bjccovmshb1i48.instrumentFunction(24);
        return this.__username.get();
    }
    set username(newValue: string) {
        bjccovmshb1i48.instrumentFunction(25);
        this.__username.set(newValue);
    }
    private __password: SynchedPropertySimpleTwoWayPU<string>;
    get password() {
        bjccovmshb1i48.instrumentFunction(26);
        return this.__password.get();
    }
    set password(newValue: string) {
        bjccovmshb1i48.instrumentFunction(27);
        this.__password.set(newValue);
    }
    private __rememberPassword: SynchedPropertySimpleTwoWayPU<boolean>;
    get rememberPassword() {
        bjccovmshb1i48.instrumentFunction(28);
        return this.__rememberPassword.get();
    }
    set rememberPassword(newValue: boolean) {
        bjccovmshb1i48.instrumentFunction(29);
        this.__rememberPassword.set(newValue);
    }
    private __passwordLoading: SynchedPropertySimpleOneWayPU<boolean>;
    get passwordLoading() {
        bjccovmshb1i48.instrumentFunction(30);
        return this.__passwordLoading.get();
    }
    set passwordLoading(newValue: boolean) {
        bjccovmshb1i48.instrumentFunction(31);
        this.__passwordLoading.set(newValue);
    }
    private __selectedConnectionProfileId: SynchedPropertySimpleOneWayPU<string>;
    get selectedConnectionProfileId() {
        bjccovmshb1i48.instrumentFunction(32);
        return this.__selectedConnectionProfileId.get();
    }
    set selectedConnectionProfileId(newValue: string) {
        bjccovmshb1i48.instrumentFunction(33);
        this.__selectedConnectionProfileId.set(newValue);
    }
    private __connectionFeedbackText: SynchedPropertySimpleOneWayPU<string>;
    get connectionFeedbackText() {
        bjccovmshb1i48.instrumentFunction(34);
        return this.__connectionFeedbackText.get();
    }
    set connectionFeedbackText(newValue: string) {
        bjccovmshb1i48.instrumentFunction(35);
        this.__connectionFeedbackText.set(newValue);
    }
    private __connectionFeedbackTone: SynchedPropertySimpleOneWayPU<SettingsStatusTone>;
    get connectionFeedbackTone() {
        bjccovmshb1i48.instrumentFunction(36);
        return this.__connectionFeedbackTone.get();
    }
    set connectionFeedbackTone(newValue: SettingsStatusTone) {
        bjccovmshb1i48.instrumentFunction(37);
        this.__connectionFeedbackTone.set(newValue);
    }
    private __isDark: SynchedPropertySimpleOneWayPU<boolean>;
    get isDark() {
        bjccovmshb1i48.instrumentFunction(38);
        return this.__isDark.get();
    }
    set isDark(newValue: boolean) {
        bjccovmshb1i48.instrumentFunction(39);
        this.__isDark.set(newValue);
    }
    private onHostChange: (value: string) => void;
    private onPortChange: (value: string) => void;
    private onUsernameChange: (value: string) => void;
    private onPasswordChange: (value: string) => void;
    private onDeleteProfile: () => void;
    private onRememberPasswordChange: (remember: boolean) => void;
    private onClearPassword: () => void;
    private onConnect: () => void;
    private __hoveredAction: ObservedPropertySimplePU<string>;
    get hoveredAction() {
        bjccovmshb1i48.instrumentFunction(48);
        return this.__hoveredAction.get();
    }
    set hoveredAction(newValue: string) {
        bjccovmshb1i48.instrumentFunction(49);
        this.__hoveredAction.set(newValue);
    }
    private __pressedAction: ObservedPropertySimplePU<string>;
    get pressedAction() {
        bjccovmshb1i48.instrumentFunction(50);
        return this.__pressedAction.get();
    }
    set pressedAction(newValue: string) {
        bjccovmshb1i48.instrumentFunction(51);
        this.__pressedAction.set(newValue);
    }
    private __connectHovered: ObservedPropertySimplePU<boolean>;
    get connectHovered() {
        bjccovmshb1i48.instrumentFunction(52);
        return this.__connectHovered.get();
    }
    set connectHovered(newValue: boolean) {
        bjccovmshb1i48.instrumentFunction(53);
        this.__connectHovered.set(newValue);
    }
    private __connectPressed: ObservedPropertySimplePU<boolean>;
    get connectPressed() {
        bjccovmshb1i48.instrumentFunction(54);
        return this.__connectPressed.get();
    }
    set connectPressed(newValue: boolean) {
        bjccovmshb1i48.instrumentFunction(55);
        this.__connectPressed.set(newValue);
    }
    private __formBreakpoint: ObservedPropertySimplePU<string>;
    get formBreakpoint() {
        bjccovmshb1i48.instrumentFunction(56);
        return this.__formBreakpoint.get();
    }
    set formBreakpoint(newValue: string) {
        bjccovmshb1i48.instrumentFunction(57);
        this.__formBreakpoint.set(newValue);
    }
    private hasSelectedProfile(): boolean {
        bjccovmshb1i48.instrumentFunction(58);
        bjccovmshb1i48.instrumentRegion(58, 1);
        return this.selectedConnectionProfileId.length > 0;
    }
    private setHoveredAction(action: string, hovered: boolean): void {
        bjccovmshb1i48.instrumentFunction(59);
        bjccovmshb1i48.instrumentRegion(59, 1);
        SettingsTheme.animate(this.getUIContext(), () => {
            bjccovmshb1i48.instrumentFunction(60);
            bjccovmshb1i48.instrumentRegion(60, 1);
            this.hoveredAction = hovered ? (bjccovmshb1i48.instrumentBranch(60, 0, true), action) : (bjccovmshb1i48.instrumentBranch(60, 0, false), '');
        });
    }
    private buildFieldLabel(text: string, parent = null) {
        bjccovmshb1i48.instrumentFunction(61);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i48.instrumentFunction(62);
            Text.create(text);
            Text.fontSize(16);
            Text.fontColor(SettingsTheme.primaryText(this.isDark));
            Text.textAlign(this.formBreakpoint === 'xs' ? (bjccovmshb1i48.instrumentBranch(62, 0, true), TextAlign.Start) : (bjccovmshb1i48.instrumentBranch(62, 0, false), TextAlign.End));
            Text.width('100%');
            Text.constraintSize({ minHeight: 48 });
            Text.maxLines(2);
        }, Text);
        Text.pop();
    }
    private buildHostInput(parent = null) {
        bjccovmshb1i48.instrumentFunction(63);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i48.instrumentFunction(64);
            TextInput.create({ placeholder: 'Windows host', text: { value: this.host, changeEvent: newValue => { bjccovmshb1i48.instrumentFunction(65); this.host = newValue; } } });
            TextInput.type(InputType.Normal);
            TextInput.constraintSize({ minHeight: 48 });
            TextInput.width('100%');
            TextInput.fontSize(16);
            TextInput.fontColor(SettingsTheme.primaryText(this.isDark));
            TextInput.placeholderColor(SettingsTheme.mutedText(this.isDark));
            TextInput.backgroundColor(HomeTheme.fieldBackground(this.isDark));
            TextInput.borderRadius(6);
            TextInput.border({
                width: 1,
                color: HomeTheme.mutedBorderColor(this.isDark),
                style: BorderStyle.Solid
            });
            TextInput.onChange((value: string) => {
                bjccovmshb1i48.instrumentFunction(66);
                this.host = value;
                bjccovmshb1i48.instrumentRegion(63, 1);
                this.onHostChange(value);
            });
        }, TextInput);
    }
    private buildPortInput(parent = null) {
        bjccovmshb1i48.instrumentFunction(67);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i48.instrumentFunction(68);
            TextInput.create({ placeholder: 'Port', text: { value: this.port, changeEvent: newValue => { bjccovmshb1i48.instrumentFunction(69); this.port = newValue; } } });
            TextInput.type(InputType.Normal);
            TextInput.constraintSize({ minHeight: 48 });
            TextInput.width('100%');
            TextInput.fontSize(16);
            TextInput.fontColor(SettingsTheme.primaryText(this.isDark));
            TextInput.placeholderColor(SettingsTheme.mutedText(this.isDark));
            TextInput.backgroundColor(HomeTheme.fieldBackground(this.isDark));
            TextInput.borderRadius(6);
            TextInput.border({
                width: 1,
                color: HomeTheme.mutedBorderColor(this.isDark),
                style: BorderStyle.Solid
            });
            TextInput.onChange((value: string) => {
                bjccovmshb1i48.instrumentFunction(70);
                this.port = value;
                bjccovmshb1i48.instrumentRegion(67, 1);
                this.onPortChange(value);
            });
        }, TextInput);
    }
    private buildUsernameInput(parent = null) {
        bjccovmshb1i48.instrumentFunction(71);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i48.instrumentFunction(72);
            TextInput.create({ placeholder: HomeText.WINDOWS_USERNAME_PLACEHOLDER, text: { value: this.username, changeEvent: newValue => { bjccovmshb1i48.instrumentFunction(73); this.username = newValue; } } });
            TextInput.type(InputType.Normal);
            TextInput.constraintSize({ minHeight: 48 });
            TextInput.width('100%');
            TextInput.fontSize(16);
            TextInput.fontColor(SettingsTheme.primaryText(this.isDark));
            TextInput.placeholderColor(SettingsTheme.mutedText(this.isDark));
            TextInput.backgroundColor(HomeTheme.fieldBackground(this.isDark));
            TextInput.borderRadius(6);
            TextInput.border({
                width: 1,
                color: HomeTheme.mutedBorderColor(this.isDark),
                style: BorderStyle.Solid
            });
            TextInput.onChange((value: string) => {
                bjccovmshb1i48.instrumentFunction(74);
                this.username = value;
                bjccovmshb1i48.instrumentRegion(71, 1);
                this.onUsernameChange(value);
            });
        }, TextInput);
    }
    private buildPasswordInput(parent = null) {
        bjccovmshb1i48.instrumentFunction(75);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i48.instrumentFunction(76);
            TextInput.create({ placeholder: 'Password', text: { value: this.password, changeEvent: newValue => { bjccovmshb1i48.instrumentFunction(77); this.password = newValue; } } });
            TextInput.type(InputType.Password);
            TextInput.constraintSize({ minHeight: 48 });
            TextInput.width('100%');
            TextInput.fontSize(16);
            TextInput.fontColor(SettingsTheme.primaryText(this.isDark));
            TextInput.placeholderColor(SettingsTheme.mutedText(this.isDark));
            TextInput.backgroundColor(HomeTheme.fieldBackground(this.isDark));
            TextInput.borderRadius(6);
            TextInput.border({
                width: 1,
                color: HomeTheme.mutedBorderColor(this.isDark),
                style: BorderStyle.Solid
            });
            TextInput.enabled(!this.passwordLoading);
            TextInput.onChange((value: string) => {
                bjccovmshb1i48.instrumentFunction(78);
                this.password = value;
                bjccovmshb1i48.instrumentRegion(75, 1);
                this.onPasswordChange(value);
            });
        }, TextInput);
    }
    private buildRememberControl(parent = null) {
        bjccovmshb1i48.instrumentFunction(79);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i48.instrumentFunction(80);
            Row.create({ space: 12 });
            Row.width('100%');
            Row.constraintSize({ minHeight: 48 });
            Row.alignItems(VerticalAlign.Center);
        }, Row);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i48.instrumentFunction(81);
            Toggle.create({ type: ToggleType.Switch, isOn: this.rememberPassword });
            Toggle.onChange((enabled: boolean) => {
                bjccovmshb1i48.instrumentFunction(82);
                this.rememberPassword = enabled;
                bjccovmshb1i48.instrumentRegion(79, 1);
                this.onRememberPasswordChange(enabled);
            });
        }, Toggle);
        Toggle.pop();
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i48.instrumentFunction(83);
            If.create();
            if (this.connectionFeedbackText.length > 0) {
                bjccovmshb1i48.instrumentBranch(83, 0, true);
                bjccovmshb1i48.instrumentRegion(83, 1);
                this.ifElseBranchUpdateFunction(0, () => {
                    bjccovmshb1i48.instrumentFunction(84);
                    this.observeComponentCreation2((elmtId, isInitialRender) => {
                        bjccovmshb1i48.instrumentFunction(85);
                        Text.create(this.connectionFeedbackText);
                        Text.fontSize(12);
                        Text.fontColor(SettingsTheme.statusText(this.isDark, this.connectionFeedbackTone));
                        Text.maxLines(3);
                        Text.textOverflow({ overflow: TextOverflow.Ellipsis });
                        Text.layoutWeight(1);
                    }, Text);
                    Text.pop();
                });
            }
            else {
                bjccovmshb1i48.instrumentBranch(83, 0, false);
                this.ifElseBranchUpdateFunction(1, () => {
                });
            }
        }, If);
        If.pop();
        Row.pop();
    }
    private buildConnectButton(parent = null) {
        bjccovmshb1i48.instrumentFunction(86);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i48.instrumentFunction(87);
            Button.createWithChild();
            Button.type(ButtonType.Normal);
            Button.constraintSize({ minHeight: 48 });
            Button.width('100%');
            Button.borderRadius(7);
            Button.backgroundColor(HomeTheme.primaryButtonBackground(this.isDark, this.connectHovered, this.connectPressed));
            Button.stateEffect(false);
            Button.enabled(!this.passwordLoading);
            Button.scale({ x: this.connectPressed ? (bjccovmshb1i48.instrumentBranch(87, 0, true), 0.99) : (bjccovmshb1i48.instrumentBranch(87, 0, false), 1), y: this.connectPressed ? (bjccovmshb1i48.instrumentBranch(87, 1, true), 0.99) : (bjccovmshb1i48.instrumentBranch(87, 1, false), 1) });
            Button.onHover((hovered: boolean) => {
                bjccovmshb1i48.instrumentFunction(88);
                bjccovmshb1i48.instrumentRegion(88, 1);
                this.connectHovered = hovered;
            });
            Button.onTouch((event: TouchEvent) => {
                bjccovmshb1i48.instrumentFunction(89);
                if (event.type === TouchType.Down) {
                    bjccovmshb1i48.instrumentBranch(89, 0, true);
                    bjccovmshb1i48.instrumentRegion(89, 1);
                    this.connectPressed = true;
                    bjccovmshb1i48.instrumentRegion(86, 1);
                    return;
                }
                else {
                    bjccovmshb1i48.instrumentBranch(89, 0, false);
                }
                if (event.type === TouchType.Up || event.type === TouchType.Cancel) {
                    bjccovmshb1i48.instrumentBranch(89, 1, true);
                    bjccovmshb1i48.instrumentRegion(89, 2);
                    this.connectPressed = false;
                }
                else {
                    bjccovmshb1i48.instrumentBranch(89, 1, false);
                }
            });
            Button.onClick(() => {
                bjccovmshb1i48.instrumentFunction(90);
                this.connectPressed = false;
                bjccovmshb1i48.instrumentRegion(86, 2);
                this.onConnect();
            });
        }, Button);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i48.instrumentFunction(91);
            Row.create({ space: 10 });
            Row.justifyContent(FlexAlign.Center);
            Row.alignItems(VerticalAlign.Center);
        }, Row);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i48.instrumentFunction(92);
            Image.create(HomeResources.LINK_ICON);
            Image.width(24);
            Image.height(24);
            Image.fillColor(Color.White);
            Image.draggable(false);
        }, Image);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i48.instrumentFunction(93);
            Text.create('Connect');
            Text.fontSize(20);
            Text.fontWeight(FontWeight.Bold);
            Text.fontColor(Color.White);
            Text.maxLines(2);
        }, Text);
        Text.pop();
        Row.pop();
        Button.pop();
    }
    private buildActionButton(action: string, text: string, icon: Resource, tone: SettingsStatusTone, onPress: () => void, parent = null) {
        bjccovmshb1i48.instrumentFunction(94);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i48.instrumentFunction(96);
            Button.createWithChild();
            Button.type(ButtonType.Normal);
            Button.constraintSize({ minHeight: 48 });
            Button.width('100%');
            Button.borderRadius(7);
            Button.backgroundColor(this.pressedAction === action ? (bjccovmshb1i48.instrumentBranch(96, 0, true), SettingsTheme.statusBackground(this.isDark, tone)) : (bjccovmshb1i48.instrumentBranch(96, 0, false), (this.hoveredAction === action ? (bjccovmshb1i48.instrumentBranch(96, 1, true), SettingsTheme.statusBackground(this.isDark, tone)) : (bjccovmshb1i48.instrumentBranch(96, 1, false), HomeTheme.fieldBackground(this.isDark)))));
            Button.border({
                width: 1,
                color: this.hoveredAction === action || this.pressedAction === action ? (bjccovmshb1i48.instrumentBranch(96, 2, true), SettingsTheme.statusText(this.isDark, tone)) : (bjccovmshb1i48.instrumentBranch(96, 2, false), SettingsTheme.statusBorder(this.isDark, tone))
            });
            Button.stateEffect(false);
            Button.scale({ x: this.pressedAction === action ? (bjccovmshb1i48.instrumentBranch(96, 3, true), 0.98) : (bjccovmshb1i48.instrumentBranch(96, 3, false), 1), y: this.pressedAction === action ? (bjccovmshb1i48.instrumentBranch(96, 4, true), 0.98) : (bjccovmshb1i48.instrumentBranch(96, 4, false), 1) });
            Button.onHover((hovered: boolean) => {
                bjccovmshb1i48.instrumentFunction(97);
                bjccovmshb1i48.instrumentRegion(97, 1);
                this.setHoveredAction(action, hovered);
            });
            Button.onTouch((event: TouchEvent) => {
                bjccovmshb1i48.instrumentFunction(98);
                if (event.type === TouchType.Down) {
                    bjccovmshb1i48.instrumentBranch(98, 0, true);
                    bjccovmshb1i48.instrumentRegion(98, 1);
                    this.pressedAction = action;
                    bjccovmshb1i48.instrumentRegion(94, 1);
                    return;
                }
                else {
                    bjccovmshb1i48.instrumentBranch(98, 0, false);
                }
                if (event.type === TouchType.Up || event.type === TouchType.Cancel) {
                    bjccovmshb1i48.instrumentBranch(98, 1, true);
                    bjccovmshb1i48.instrumentRegion(98, 2);
                    this.pressedAction = '';
                }
                else {
                    bjccovmshb1i48.instrumentBranch(98, 1, false);
                }
            });
            Button.onClick(() => {
                bjccovmshb1i48.instrumentFunction(99);
                this.pressedAction = '';
                bjccovmshb1i48.instrumentRegion(94, 2);
                onPress();
            });
        }, Button);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i48.instrumentFunction(100);
            Row.create({ space: 8 });
            Row.justifyContent(FlexAlign.Center);
            Row.alignItems(VerticalAlign.Center);
            Row.width('100%');
            Row.padding({ left: 6, right: 6 });
        }, Row);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i48.instrumentFunction(101);
            Image.create(icon);
            Image.width(20);
            Image.height(20);
            Image.fillColor(SettingsTheme.statusText(this.isDark, tone));
            Image.draggable(false);
        }, Image);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i48.instrumentFunction(102);
            Text.create(text);
            Text.fontSize(15);
            Text.fontWeight(FontWeight.Bold);
            Text.fontColor(SettingsTheme.statusText(this.isDark, tone));
            Text.maxLines(1);
            Text.textOverflow({ overflow: TextOverflow.Ellipsis });
        }, Text);
        Text.pop();
        Row.pop();
        Button.pop();
    }
    private buildDeviceActions(parent = null) {
        bjccovmshb1i48.instrumentFunction(103);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i48.instrumentFunction(104);
            GridRow.create({
                columns: 12,
                gutter: { x: 12, y: 8 },
                breakpoints: { value: ['600vp'], reference: BreakpointsReference.ComponentSize }
            });
            GridRow.width('100%');
            GridRow.onBreakpointChange((breakpoint: string) => {
                bjccovmshb1i48.instrumentFunction(105);
                bjccovmshb1i48.instrumentRegion(105, 1);
                this.formBreakpoint = breakpoint;
            });
        }, GridRow);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i48.instrumentFunction(106);
            GridCol.create({ span: { xs: 12, sm: 3 } });
        }, GridCol);
        this.buildFieldLabel.bind(this)(HomeText.DEVICE_ACTIONS);
        GridCol.pop();
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i48.instrumentFunction(107);
            GridCol.create({ span: { xs: 12, sm: 9 } });
        }, GridCol);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i48.instrumentFunction(108);
            GridRow.create({ columns: 12, gutter: { x: 12, y: 8 } });
            GridRow.width('100%');
        }, GridRow);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i48.instrumentFunction(109);
            GridCol.create({ span: 6 });
        }, GridCol);
        this.buildActionButton.bind(this)('delete', HomeText.DELETE_ACTION, HomeResources.TRASH_ICON, 'danger', () => {
            bjccovmshb1i48.instrumentFunction(110);
            bjccovmshb1i48.instrumentRegion(110, 1);
            this.onDeleteProfile();
        });
        GridCol.pop();
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i48.instrumentFunction(111);
            GridCol.create({ span: 6 });
        }, GridCol);
        this.buildActionButton.bind(this)('clearPassword', HomeText.CLEAR_PASSWORD_ACTION, HomeResources.LOCK_ICON, 'warning', () => {
            bjccovmshb1i48.instrumentFunction(112);
            bjccovmshb1i48.instrumentRegion(112, 1);
            this.onClearPassword();
        });
        GridCol.pop();
        GridRow.pop();
        GridCol.pop();
        GridRow.pop();
    }
    private buildConnectionFormFields(parent = null) {
        bjccovmshb1i48.instrumentFunction(113);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i48.instrumentFunction(114);
            GridRow.create({
                columns: 12,
                gutter: { x: 24, y: 12 },
                breakpoints: { value: ['600vp'], reference: BreakpointsReference.ComponentSize }
            });
            GridRow.width('100%');
            GridRow.padding({ top: 2, bottom: 6 });
            GridRow.onBreakpointChange((breakpoint: string) => {
                bjccovmshb1i48.instrumentFunction(115);
                bjccovmshb1i48.instrumentRegion(115, 1);
                this.formBreakpoint = breakpoint;
            });
        }, GridRow);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i48.instrumentFunction(116);
            GridCol.create({ span: { xs: 12, sm: 3 } });
        }, GridCol);
        this.buildFieldLabel.bind(this)('Windows host');
        GridCol.pop();
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i48.instrumentFunction(117);
            GridCol.create({ span: { xs: 12, sm: 9 } });
        }, GridCol);
        this.buildHostInput.bind(this)();
        GridCol.pop();
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i48.instrumentFunction(118);
            GridCol.create({ span: { xs: 12, sm: 3 } });
        }, GridCol);
        this.buildFieldLabel.bind(this)('Port');
        GridCol.pop();
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i48.instrumentFunction(119);
            GridCol.create({ span: { xs: 12, sm: 9 } });
        }, GridCol);
        this.buildPortInput.bind(this)();
        GridCol.pop();
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i48.instrumentFunction(120);
            GridCol.create({ span: { xs: 12, sm: 3 } });
        }, GridCol);
        this.buildFieldLabel.bind(this)(HomeText.WINDOWS_USERNAME_LABEL);
        GridCol.pop();
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i48.instrumentFunction(121);
            GridCol.create({ span: { xs: 12, sm: 9 } });
        }, GridCol);
        this.buildUsernameInput.bind(this)();
        GridCol.pop();
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i48.instrumentFunction(122);
            GridCol.create({ span: { xs: 12, sm: 3 } });
        }, GridCol);
        this.buildFieldLabel.bind(this)('Password');
        GridCol.pop();
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i48.instrumentFunction(123);
            GridCol.create({ span: { xs: 12, sm: 9 } });
        }, GridCol);
        this.buildPasswordInput.bind(this)();
        GridCol.pop();
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i48.instrumentFunction(124);
            GridCol.create({ span: { xs: 12, sm: 3 } });
        }, GridCol);
        this.buildFieldLabel.bind(this)(HomeText.REMEMBER_PASSWORD);
        GridCol.pop();
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i48.instrumentFunction(125);
            GridCol.create({ span: { xs: 12, sm: 9 } });
        }, GridCol);
        this.buildRememberControl.bind(this)();
        GridCol.pop();
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i48.instrumentFunction(126);
            GridCol.create({ span: { xs: 12, sm: 9 }, offset: { xs: 0, sm: 3 } });
        }, GridCol);
        this.buildConnectButton.bind(this)();
        GridCol.pop();
        GridRow.pop();
    }
    initialRender() {
        bjccovmshb1i48.instrumentFunction(127);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i48.instrumentFunction(128);
            Column.create({ space: 12 });
            Column.width('100%');
            Column.padding(this.layoutMode === LayoutMode.EXPANDED ? (bjccovmshb1i48.instrumentBranch(128, 0, true), { left: 34, right: 34, top: 22, bottom: 20 }) : (bjccovmshb1i48.instrumentBranch(128, 0, false), { left: 20, right: 20, top: 16, bottom: 20 }));
            Column.alignItems(HorizontalAlign.Start);
            Column.backgroundColor(HomeTheme.panelBackground(this.isDark));
            Column.borderRadius(8);
            Column.border({
                width: 1,
                color: HomeTheme.mutedBorderColor(this.isDark)
            });
        }, Column);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i48.instrumentFunction(129);
            Text.create(HomeText.CONNECTION_DETAILS_TITLE);
            Text.fontSize(24);
            Text.fontWeight(FontWeight.Bold);
            Text.fontColor(SettingsTheme.primaryText(this.isDark));
            Text.width('100%');
            Text.maxLines(2);
        }, Text);
        Text.pop();
        this.buildConnectionFormFields.bind(this)();
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i48.instrumentFunction(130);
            Divider.create();
            Divider.height(1);
            Divider.color(HomeTheme.mutedBorderColor(this.isDark));
            Divider.margin({ top: 2, bottom: 0 });
        }, Divider);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i48.instrumentFunction(131);
            If.create();
            if (this.hasSelectedProfile()) {
                bjccovmshb1i48.instrumentBranch(131, 0, true);
                bjccovmshb1i48.instrumentRegion(131, 1);
                this.ifElseBranchUpdateFunction(0, () => {
                    bjccovmshb1i48.instrumentFunction(132);
                    this.buildDeviceActions.bind(this)();
                });
            }
            else {
                bjccovmshb1i48.instrumentBranch(131, 0, false);
                this.ifElseBranchUpdateFunction(1, () => {
                });
            }
        }, If);
        If.pop();
        Column.pop();
    }
    rerender() {
        this.updateDirtyElements();
    }
}
