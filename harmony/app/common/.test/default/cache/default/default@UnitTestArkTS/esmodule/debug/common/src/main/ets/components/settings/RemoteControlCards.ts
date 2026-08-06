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
let bjccovmshb1i9e = new BjcCov({ version: "bjc v1.0.0", versionCode: 10000, path: "common/src/main/ets/components/settings/RemoteControlCards.ets", hash: "69205277f46d920425876b0e972340bd7fc8f0bfe5d1e6d08655809dc43f4cf9", lineCnt: 531, count: 0, projectPath: "", functions: { 0: { name: "anonymous_1", count: 0, regions: { 0: { startLoc: { line: 411, col: 14 }, endLoc: { line: 411, col: 24 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 0 }, 1: { name: "anonymous_2", count: 0, regions: { 0: { startLoc: { line: 250, col: 17 }, endLoc: { line: 250, col: 43 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 1 }, 2: { name: "anonymous_3", count: 0, regions: { 0: { startLoc: { line: 252, col: 17 }, endLoc: { line: 252, col: 27 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 2 }, 3: { name: "anonymous_4", count: 0, regions: { 0: { startLoc: { line: 152, col: 31 }, endLoc: { line: 152, col: 41 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 3 }, 4: { name: "anonymous_5", count: 0, regions: { 0: { startLoc: { line: 13, col: 14 }, endLoc: { line: 13, col: 24 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 4 }, 5: { name: "anonymous_6", count: 0, regions: { 0: { startLoc: { line: 15, col: 12 }, endLoc: { line: 15, col: 22 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 5 }, 6: { name: "anonymous_7", count: 0, regions: { 0: { startLoc: { line: 13, col: 27 }, endLoc: { line: 14, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 6 }, 7: { name: "anonymous_8", count: 0, regions: { 0: { startLoc: { line: 15, col: 25 }, endLoc: { line: 16, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 7 }, 8: { name: "setInitiallyProvidedValue", count: 0, regions: { 0: { startLoc: { line: 6, col: 27 }, endLoc: { line: 12, col: 29 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 8 }, 9: { name: "updateStateVars", count: 0, regions: { 0: { startLoc: { line: 6, col: 9 }, endLoc: { line: 12, col: 16 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 9 }, 10: { name: "isDark", count: 0, regions: { 0: { startLoc: { line: 6, col: 9 }, endLoc: { line: 6, col: 15 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 10 }, 11: { name: "isDark", count: 0, regions: { 0: { startLoc: { line: 6, col: 9 }, endLoc: { line: 6, col: 24 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 11 }, 12: { name: "targetSection", count: 0, regions: { 0: { startLoc: { line: 7, col: 9 }, endLoc: { line: 7, col: 22 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 12 }, 13: { name: "targetSection", count: 0, regions: { 0: { startLoc: { line: 7, col: 9 }, endLoc: { line: 7, col: 30 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 13 }, 14: { name: "running", count: 0, regions: { 0: { startLoc: { line: 8, col: 9 }, endLoc: { line: 8, col: 16 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 14 }, 15: { name: "running", count: 0, regions: { 0: { startLoc: { line: 8, col: 9 }, endLoc: { line: 8, col: 25 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 15 }, 16: { name: "busy", count: 0, regions: { 0: { startLoc: { line: 9, col: 9 }, endLoc: { line: 9, col: 13 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 16 }, 17: { name: "busy", count: 0, regions: { 0: { startLoc: { line: 9, col: 9 }, endLoc: { line: 9, col: 22 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 17 }, 18: { name: "state", count: 0, regions: { 0: { startLoc: { line: 10, col: 9 }, endLoc: { line: 10, col: 14 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 18 }, 19: { name: "state", count: 0, regions: { 0: { startLoc: { line: 10, col: 9 }, endLoc: { line: 10, col: 22 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 19 }, 20: { name: "port", count: 0, regions: { 0: { startLoc: { line: 11, col: 9 }, endLoc: { line: 11, col: 13 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 20 }, 21: { name: "port", count: 0, regions: { 0: { startLoc: { line: 11, col: 9 }, endLoc: { line: 11, col: 21 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 21 }, 22: { name: "message", count: 0, regions: { 0: { startLoc: { line: 12, col: 9 }, endLoc: { line: 12, col: 16 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 22 }, 23: { name: "message", count: 0, regions: { 0: { startLoc: { line: 12, col: 9 }, endLoc: { line: 12, col: 24 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 23 }, 24: { name: "anonymous_9", count: 0, regions: { 0: { startLoc: { line: 13, col: 14 }, endLoc: { line: 13, col: 24 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 24 }, 25: { name: "anonymous_10", count: 0, regions: { 0: { startLoc: { line: 15, col: 12 }, endLoc: { line: 15, col: 22 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 25 }, 26: { name: "hovered", count: 0, regions: { 0: { startLoc: { line: 17, col: 18 }, endLoc: { line: 17, col: 25 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 26 }, 27: { name: "hovered", count: 0, regions: { 0: { startLoc: { line: 17, col: 18 }, endLoc: { line: 17, col: 34 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 27 }, 28: { name: "pressed", count: 0, regions: { 0: { startLoc: { line: 18, col: 18 }, endLoc: { line: 18, col: 25 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 28 }, 29: { name: "pressed", count: 0, regions: { 0: { startLoc: { line: 18, col: 18 }, endLoc: { line: 18, col: 34 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 29 }, 30: { name: "isTarget", count: 0, regions: { 0: { startLoc: { line: 20, col: 3 }, endLoc: { line: 22, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 21, col: 5 }, endLoc: { line: 22, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 30 }, 31: { name: "setHovered", count: 0, regions: { 0: { startLoc: { line: 24, col: 3 }, endLoc: { line: 28, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 25, col: 5 }, endLoc: { line: 28, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 31 }, 32: { name: "anonymous_11", count: 0, regions: { 0: { startLoc: { line: 25, col: 48 }, endLoc: { line: 27, col: 6 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 26, col: 7 }, endLoc: { line: 27, col: 6 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 32 }, 33: { name: "statusText", count: 0, regions: { 0: { startLoc: { line: 30, col: 3 }, endLoc: { line: 32, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 31, col: 5 }, endLoc: { line: 32, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 33 }, 34: { name: "actionText", count: 0, regions: { 0: { startLoc: { line: 34, col: 3 }, endLoc: { line: 39, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 35, col: 20 }, endLoc: { line: 37, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 38, col: 5 }, endLoc: { line: 39, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 35, col: 9 }, endLoc: { line: 35, col: 18 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 38, col: 12 }, endLoc: { line: 38, col: 110 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 34 }, 35: { name: "actionDisabled", count: 0, regions: { 0: { startLoc: { line: 41, col: 3 }, endLoc: { line: 43, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 42, col: 5 }, endLoc: { line: 43, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 35 }, 36: { name: "initialRender", count: 0, regions: { 0: { startLoc: { line: 45, col: 3 }, endLoc: { line: 145, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 139, col: 9 }, endLoc: { line: 140, col: 8 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 112, col: 11 }, endLoc: { line: 113, col: 10 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 110, col: 13 }, endLoc: { line: 111, col: 12 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 36 }, 37: { name: "anonymous_12", count: 0, regions: { 0: { startLoc: { line: 46, col: 5 }, endLoc: { line: 144, col: 6 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 118, col: 22 }, endLoc: { line: 119, col: 97 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 123, col: 14 }, endLoc: { line: 124, col: 47 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 2: { startLoc: { line: 127, col: 21 }, endLoc: { line: 127, col: 42 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 3: { startLoc: { line: 129, col: 10 }, endLoc: { line: 129, col: 56 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 4: { startLoc: { line: 129, col: 32 }, endLoc: { line: 129, col: 56 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 5: { startLoc: { line: 130, col: 10 }, endLoc: { line: 130, col: 56 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 6: { startLoc: { line: 130, col: 32 }, endLoc: { line: 130, col: 56 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 37 }, 38: { name: "anonymous_13", count: 0, regions: { 0: { startLoc: { line: 133, col: 14 }, endLoc: { line: 135, col: 6 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 134, col: 7 }, endLoc: { line: 135, col: 6 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 38 }, 39: { name: "anonymous_14", count: 0, regions: { 0: { startLoc: { line: 136, col: 14 }, endLoc: { line: 144, col: 6 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 137, col: 42 }, endLoc: { line: 140, col: 8 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 141, col: 75 }, endLoc: { line: 143, col: 8 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 137, col: 11 }, endLoc: { line: 137, col: 40 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 141, col: 11 }, endLoc: { line: 141, col: 73 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 39 }, 40: { name: "anonymous_15", count: 0, regions: { 0: { startLoc: { line: 47, col: 7 }, endLoc: { line: 61, col: 30 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 60, col: 68 }, endLoc: { line: 61, col: 29 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 40 }, 41: { name: "anonymous_16", count: 0, regions: { 0: { startLoc: { line: 48, col: 9 }, endLoc: { line: 53, col: 27 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 51, col: 61 }, endLoc: { line: 52, col: 33 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 41 }, 42: { name: "anonymous_17", count: 0, regions: { 0: { startLoc: { line: 63, col: 7 }, endLoc: { line: 91, col: 22 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 42 }, 43: { name: "anonymous_18", count: 0, regions: { 0: { startLoc: { line: 64, col: 9 }, endLoc: { line: 68, col: 24 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 43 }, 44: { name: "anonymous_19", count: 0, regions: { 0: { startLoc: { line: 69, col: 9 }, endLoc: { line: 75, col: 60 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 44 }, 45: { name: "anonymous_20", count: 0, regions: { 0: { startLoc: { line: 76, col: 9 }, endLoc: { line: 80, col: 24 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 78, col: 61 }, endLoc: { line: 79, col: 33 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 45 }, 46: { name: "anonymous_21", count: 0, regions: { 0: { startLoc: { line: 81, col: 9 }, endLoc: { line: 89, col: 10 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 81, col: 38 }, endLoc: { line: 89, col: 10 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 81, col: 13 }, endLoc: { line: 81, col: 36 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 46 }, 47: { name: "anonymous_22", count: 0, regions: { 0: { startLoc: { line: 82, col: 11 }, endLoc: { line: 82, col: 15 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 47 }, 48: { name: "anonymous_23", count: 0, regions: { 0: { startLoc: { line: 82, col: 11 }, endLoc: { line: 88, col: 62 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 48 }, 49: { name: "anonymous_25", count: 0, regions: { 0: { startLoc: { line: 93, col: 7 }, endLoc: { line: 113, col: 10 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 97, col: 26 }, endLoc: { line: 98, col: 64 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 99, col: 20 }, endLoc: { line: 100, col: 49 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 49 }, 50: { name: "anonymous_26", count: 0, regions: { 0: { startLoc: { line: 104, col: 18 }, endLoc: { line: 113, col: 10 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 105, col: 38 }, endLoc: { line: 107, col: 12 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 108, col: 29 }, endLoc: { line: 111, col: 12 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 105, col: 15 }, endLoc: { line: 105, col: 36 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 108, col: 15 }, endLoc: { line: 108, col: 27 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 50 }, 51: { name: "anonymous_27", count: 0, regions: { 0: { startLoc: { line: 152, col: 44 }, endLoc: { line: 153, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 51 }, 52: { name: "setInitiallyProvidedValue", count: 0, regions: { 0: { startLoc: { line: 150, col: 27 }, endLoc: { line: 151, col: 35 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 52 }, 53: { name: "updateStateVars", count: 0, regions: { 0: { startLoc: { line: 150, col: 9 }, endLoc: { line: 151, col: 22 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 53 }, 54: { name: "isDark", count: 0, regions: { 0: { startLoc: { line: 150, col: 9 }, endLoc: { line: 150, col: 15 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 54 }, 55: { name: "isDark", count: 0, regions: { 0: { startLoc: { line: 150, col: 9 }, endLoc: { line: 150, col: 24 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 55 }, 56: { name: "targetSection", count: 0, regions: { 0: { startLoc: { line: 151, col: 9 }, endLoc: { line: 151, col: 22 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 56 }, 57: { name: "targetSection", count: 0, regions: { 0: { startLoc: { line: 151, col: 9 }, endLoc: { line: 151, col: 30 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 57 }, 58: { name: "anonymous_28", count: 0, regions: { 0: { startLoc: { line: 152, col: 31 }, endLoc: { line: 152, col: 41 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 58 }, 59: { name: "hovered", count: 0, regions: { 0: { startLoc: { line: 154, col: 18 }, endLoc: { line: 154, col: 25 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 59 }, 60: { name: "hovered", count: 0, regions: { 0: { startLoc: { line: 154, col: 18 }, endLoc: { line: 154, col: 34 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 60 }, 61: { name: "pressed", count: 0, regions: { 0: { startLoc: { line: 155, col: 18 }, endLoc: { line: 155, col: 25 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 61 }, 62: { name: "pressed", count: 0, regions: { 0: { startLoc: { line: 155, col: 18 }, endLoc: { line: 155, col: 34 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 62 }, 63: { name: "isTarget", count: 0, regions: { 0: { startLoc: { line: 157, col: 3 }, endLoc: { line: 159, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 158, col: 5 }, endLoc: { line: 159, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 63 }, 64: { name: "setHovered", count: 0, regions: { 0: { startLoc: { line: 161, col: 3 }, endLoc: { line: 165, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 162, col: 5 }, endLoc: { line: 165, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 64 }, 65: { name: "anonymous_29", count: 0, regions: { 0: { startLoc: { line: 162, col: 48 }, endLoc: { line: 164, col: 6 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 163, col: 7 }, endLoc: { line: 164, col: 6 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 65 }, 66: { name: "initialRender", count: 0, regions: { 0: { startLoc: { line: 167, col: 3 }, endLoc: { line: 241, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 235, col: 9 }, endLoc: { line: 236, col: 8 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 66 }, 67: { name: "anonymous_30", count: 0, regions: { 0: { startLoc: { line: 168, col: 5 }, endLoc: { line: 240, col: 6 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 214, col: 22 }, endLoc: { line: 215, col: 97 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 219, col: 14 }, endLoc: { line: 220, col: 47 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 2: { startLoc: { line: 223, col: 21 }, endLoc: { line: 223, col: 42 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 3: { startLoc: { line: 225, col: 10 }, endLoc: { line: 225, col: 56 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 4: { startLoc: { line: 225, col: 32 }, endLoc: { line: 225, col: 56 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 5: { startLoc: { line: 226, col: 10 }, endLoc: { line: 226, col: 56 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 6: { startLoc: { line: 226, col: 32 }, endLoc: { line: 226, col: 56 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 67 }, 68: { name: "anonymous_31", count: 0, regions: { 0: { startLoc: { line: 229, col: 14 }, endLoc: { line: 231, col: 6 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 230, col: 7 }, endLoc: { line: 231, col: 6 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 68 }, 69: { name: "anonymous_32", count: 0, regions: { 0: { startLoc: { line: 232, col: 14 }, endLoc: { line: 240, col: 6 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 233, col: 42 }, endLoc: { line: 236, col: 8 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 237, col: 75 }, endLoc: { line: 239, col: 8 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 233, col: 11 }, endLoc: { line: 233, col: 40 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 237, col: 11 }, endLoc: { line: 237, col: 73 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 69 }, 70: { name: "anonymous_33", count: 0, regions: { 0: { startLoc: { line: 169, col: 7 }, endLoc: { line: 181, col: 90 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 70 }, 71: { name: "anonymous_34", count: 0, regions: { 0: { startLoc: { line: 170, col: 9 }, endLoc: { line: 174, col: 27 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 71 }, 72: { name: "anonymous_35", count: 0, regions: { 0: { startLoc: { line: 183, col: 7 }, endLoc: { line: 197, col: 22 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 72 }, 73: { name: "anonymous_36", count: 0, regions: { 0: { startLoc: { line: 184, col: 9 }, endLoc: { line: 188, col: 24 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 73 }, 74: { name: "anonymous_37", count: 0, regions: { 0: { startLoc: { line: 189, col: 9 }, endLoc: { line: 195, col: 60 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 74 }, 75: { name: "anonymous_38", count: 0, regions: { 0: { startLoc: { line: 199, col: 7 }, endLoc: { line: 209, col: 10 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 75 }, 76: { name: "anonymous_39", count: 0, regions: { 0: { startLoc: { line: 207, col: 18 }, endLoc: { line: 209, col: 10 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 208, col: 11 }, endLoc: { line: 209, col: 10 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 76 }, 77: { name: "anonymous_40", count: 0, regions: { 0: { startLoc: { line: 250, col: 46 }, endLoc: { line: 251, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 77 }, 78: { name: "anonymous_41", count: 0, regions: { 0: { startLoc: { line: 252, col: 30 }, endLoc: { line: 253, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 78 }, 79: { name: "setInitiallyProvidedValue", count: 0, regions: { 0: { startLoc: { line: 246, col: 27 }, endLoc: { line: 249, col: 38 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 79 }, 80: { name: "updateStateVars", count: 0, regions: { 0: { startLoc: { line: 246, col: 9 }, endLoc: { line: 249, col: 19 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 80 }, 81: { name: "isDark", count: 0, regions: { 0: { startLoc: { line: 246, col: 9 }, endLoc: { line: 246, col: 15 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 81 }, 82: { name: "isDark", count: 0, regions: { 0: { startLoc: { line: 246, col: 9 }, endLoc: { line: 246, col: 24 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 82 }, 83: { name: "targetSection", count: 0, regions: { 0: { startLoc: { line: 247, col: 9 }, endLoc: { line: 247, col: 22 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 83 }, 84: { name: "targetSection", count: 0, regions: { 0: { startLoc: { line: 247, col: 9 }, endLoc: { line: 247, col: 30 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 84 }, 85: { name: "gateEnabled", count: 0, regions: { 0: { startLoc: { line: 248, col: 9 }, endLoc: { line: 248, col: 20 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 85 }, 86: { name: "gateEnabled", count: 0, regions: { 0: { startLoc: { line: 248, col: 9 }, endLoc: { line: 248, col: 29 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 86 }, 87: { name: "accessCode", count: 0, regions: { 0: { startLoc: { line: 249, col: 9 }, endLoc: { line: 249, col: 19 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 87 }, 88: { name: "accessCode", count: 0, regions: { 0: { startLoc: { line: 249, col: 9 }, endLoc: { line: 249, col: 27 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 88 }, 89: { name: "anonymous_42", count: 0, regions: { 0: { startLoc: { line: 250, col: 17 }, endLoc: { line: 250, col: 43 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 89 }, 90: { name: "anonymous_43", count: 0, regions: { 0: { startLoc: { line: 252, col: 17 }, endLoc: { line: 252, col: 27 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 90 }, 91: { name: "hovered", count: 0, regions: { 0: { startLoc: { line: 254, col: 18 }, endLoc: { line: 254, col: 25 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 91 }, 92: { name: "hovered", count: 0, regions: { 0: { startLoc: { line: 254, col: 18 }, endLoc: { line: 254, col: 34 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 92 }, 93: { name: "pressed", count: 0, regions: { 0: { startLoc: { line: 255, col: 18 }, endLoc: { line: 255, col: 25 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 93 }, 94: { name: "pressed", count: 0, regions: { 0: { startLoc: { line: 255, col: 18 }, endLoc: { line: 255, col: 34 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 94 }, 95: { name: "isTarget", count: 0, regions: { 0: { startLoc: { line: 257, col: 3 }, endLoc: { line: 259, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 258, col: 5 }, endLoc: { line: 259, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 95 }, 96: { name: "setHovered", count: 0, regions: { 0: { startLoc: { line: 261, col: 3 }, endLoc: { line: 265, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 262, col: 5 }, endLoc: { line: 265, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 96 }, 97: { name: "anonymous_44", count: 0, regions: { 0: { startLoc: { line: 262, col: 48 }, endLoc: { line: 264, col: 6 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 263, col: 7 }, endLoc: { line: 264, col: 6 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 97 }, 98: { name: "gateStatusText", count: 0, regions: { 0: { startLoc: { line: 267, col: 3 }, endLoc: { line: 269, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 268, col: 5 }, endLoc: { line: 269, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 268, col: 12 }, endLoc: { line: 268, col: 103 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 98 }, 99: { name: "accessCodeText", count: 0, regions: { 0: { startLoc: { line: 271, col: 3 }, endLoc: { line: 273, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 272, col: 5 }, endLoc: { line: 273, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 272, col: 12 }, endLoc: { line: 272, col: 89 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 99 }, 100: { name: "regenerateDisabled", count: 0, regions: { 0: { startLoc: { line: 275, col: 3 }, endLoc: { line: 277, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 276, col: 5 }, endLoc: { line: 277, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 100 }, 101: { name: "initialRender", count: 0, regions: { 0: { startLoc: { line: 279, col: 3 }, endLoc: { line: 393, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 387, col: 9 }, endLoc: { line: 388, col: 8 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 351, col: 13 }, endLoc: { line: 352, col: 12 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 101 }, 102: { name: "anonymous_45", count: 0, regions: { 0: { startLoc: { line: 280, col: 5 }, endLoc: { line: 392, col: 6 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 366, col: 22 }, endLoc: { line: 367, col: 97 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 371, col: 14 }, endLoc: { line: 372, col: 47 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 2: { startLoc: { line: 375, col: 21 }, endLoc: { line: 375, col: 42 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 3: { startLoc: { line: 377, col: 10 }, endLoc: { line: 377, col: 56 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 4: { startLoc: { line: 377, col: 32 }, endLoc: { line: 377, col: 56 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 5: { startLoc: { line: 378, col: 10 }, endLoc: { line: 378, col: 56 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 6: { startLoc: { line: 378, col: 32 }, endLoc: { line: 378, col: 56 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 102 }, 103: { name: "anonymous_46", count: 0, regions: { 0: { startLoc: { line: 381, col: 14 }, endLoc: { line: 383, col: 6 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 382, col: 7 }, endLoc: { line: 383, col: 6 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 103 }, 104: { name: "anonymous_47", count: 0, regions: { 0: { startLoc: { line: 384, col: 14 }, endLoc: { line: 392, col: 6 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 385, col: 42 }, endLoc: { line: 388, col: 8 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 389, col: 75 }, endLoc: { line: 391, col: 8 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 385, col: 11 }, endLoc: { line: 385, col: 40 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 389, col: 11 }, endLoc: { line: 389, col: 73 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 104 }, 105: { name: "anonymous_48", count: 0, regions: { 0: { startLoc: { line: 281, col: 7 }, endLoc: { line: 318, col: 39 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 105 }, 106: { name: "anonymous_49", count: 0, regions: { 0: { startLoc: { line: 282, col: 9 }, endLoc: { line: 294, col: 90 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 106 }, 107: { name: "anonymous_50", count: 0, regions: { 0: { startLoc: { line: 283, col: 11 }, endLoc: { line: 287, col: 29 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 107 }, 108: { name: "anonymous_51", count: 0, regions: { 0: { startLoc: { line: 296, col: 9 }, endLoc: { line: 310, col: 24 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 108 }, 109: { name: "anonymous_52", count: 0, regions: { 0: { startLoc: { line: 297, col: 11 }, endLoc: { line: 301, col: 26 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 109 }, 110: { name: "anonymous_53", count: 0, regions: { 0: { startLoc: { line: 302, col: 11 }, endLoc: { line: 308, col: 62 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 110 }, 111: { name: "anonymous_54", count: 0, regions: { 0: { startLoc: { line: 312, col: 9 }, endLoc: { line: 315, col: 12 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 111 }, 112: { name: "anonymous_55", count: 0, regions: { 0: { startLoc: { line: 313, col: 21 }, endLoc: { line: 315, col: 12 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 314, col: 13 }, endLoc: { line: 315, col: 12 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 112 }, 113: { name: "anonymous_56", count: 0, regions: { 0: { startLoc: { line: 320, col: 7 }, endLoc: { line: 355, col: 20 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 113 }, 114: { name: "anonymous_57", count: 0, regions: { 0: { startLoc: { line: 321, col: 9 }, endLoc: { line: 334, col: 24 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 114 }, 115: { name: "anonymous_58", count: 0, regions: { 0: { startLoc: { line: 322, col: 11 }, endLoc: { line: 325, col: 26 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 115 }, 116: { name: "anonymous_59", count: 0, regions: { 0: { startLoc: { line: 326, col: 11 }, endLoc: { line: 332, col: 62 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 116 }, 117: { name: "anonymous_60", count: 0, regions: { 0: { startLoc: { line: 336, col: 9 }, endLoc: { line: 352, col: 12 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 340, col: 28 }, endLoc: { line: 341, col: 66 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 342, col: 22 }, endLoc: { line: 343, col: 51 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 117 }, 118: { name: "anonymous_61", count: 0, regions: { 0: { startLoc: { line: 347, col: 20 }, endLoc: { line: 352, col: 12 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 348, col: 44 }, endLoc: { line: 350, col: 14 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 348, col: 17 }, endLoc: { line: 348, col: 42 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 118 }, 119: { name: "anonymous_62", count: 0, regions: { 0: { startLoc: { line: 357, col: 7 }, endLoc: { line: 361, col: 22 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 359, col: 59 }, endLoc: { line: 360, col: 31 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 119 }, 120: { name: "anonymous_63", count: 0, regions: { 0: { startLoc: { line: 411, col: 27 }, endLoc: { line: 412, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 120 }, 121: { name: "setInitiallyProvidedValue", count: 0, regions: { 0: { startLoc: { line: 398, col: 27 }, endLoc: { line: 410, col: 30 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 121 }, 122: { name: "updateStateVars", count: 0, regions: { 0: { startLoc: { line: 398, col: 9 }, endLoc: { line: 410, col: 13 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 122 }, 123: { name: "isDark", count: 0, regions: { 0: { startLoc: { line: 398, col: 9 }, endLoc: { line: 398, col: 15 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 123 }, 124: { name: "isDark", count: 0, regions: { 0: { startLoc: { line: 398, col: 9 }, endLoc: { line: 398, col: 24 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 124 }, 125: { name: "targetSection", count: 0, regions: { 0: { startLoc: { line: 399, col: 9 }, endLoc: { line: 399, col: 22 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 125 }, 126: { name: "targetSection", count: 0, regions: { 0: { startLoc: { line: 399, col: 9 }, endLoc: { line: 399, col: 30 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 126 }, 127: { name: "section", count: 0, regions: { 0: { startLoc: { line: 400, col: 9 }, endLoc: { line: 400, col: 16 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 127 }, 128: { name: "section", count: 0, regions: { 0: { startLoc: { line: 400, col: 9 }, endLoc: { line: 400, col: 24 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 128 }, 129: { name: "title", count: 0, regions: { 0: { startLoc: { line: 401, col: 9 }, endLoc: { line: 401, col: 14 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 129 }, 130: { name: "title", count: 0, regions: { 0: { startLoc: { line: 401, col: 9 }, endLoc: { line: 401, col: 22 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 130 }, 131: { name: "description", count: 0, regions: { 0: { startLoc: { line: 402, col: 9 }, endLoc: { line: 402, col: 20 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 131 }, 132: { name: "description", count: 0, regions: { 0: { startLoc: { line: 402, col: 9 }, endLoc: { line: 402, col: 28 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 132 }, 133: { name: "grantedText", count: 0, regions: { 0: { startLoc: { line: 403, col: 9 }, endLoc: { line: 403, col: 20 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 133 }, 134: { name: "grantedText", count: 0, regions: { 0: { startLoc: { line: 403, col: 9 }, endLoc: { line: 403, col: 28 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 134 }, 135: { name: "missingText", count: 0, regions: { 0: { startLoc: { line: 404, col: 9 }, endLoc: { line: 404, col: 20 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 135 }, 136: { name: "missingText", count: 0, regions: { 0: { startLoc: { line: 404, col: 9 }, endLoc: { line: 404, col: 28 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 136 }, 137: { name: "actionTextValue", count: 0, regions: { 0: { startLoc: { line: 405, col: 9 }, endLoc: { line: 405, col: 24 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 137 }, 138: { name: "actionTextValue", count: 0, regions: { 0: { startLoc: { line: 405, col: 9 }, endLoc: { line: 405, col: 32 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 138 }, 139: { name: "busyText", count: 0, regions: { 0: { startLoc: { line: 406, col: 9 }, endLoc: { line: 406, col: 17 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 139 }, 140: { name: "busyText", count: 0, regions: { 0: { startLoc: { line: 406, col: 9 }, endLoc: { line: 406, col: 25 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 140 }, 141: { name: "icon", count: 0, regions: { 0: { startLoc: { line: 407, col: 9 }, endLoc: { line: 407, col: 13 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 141 }, 142: { name: "icon", count: 0, regions: { 0: { startLoc: { line: 407, col: 9 }, endLoc: { line: 407, col: 23 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 142 }, 143: { name: "accent", count: 0, regions: { 0: { startLoc: { line: 408, col: 9 }, endLoc: { line: 408, col: 15 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 143 }, 144: { name: "accent", count: 0, regions: { 0: { startLoc: { line: 408, col: 9 }, endLoc: { line: 408, col: 35 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 144 }, 145: { name: "granted", count: 0, regions: { 0: { startLoc: { line: 409, col: 9 }, endLoc: { line: 409, col: 16 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 145 }, 146: { name: "granted", count: 0, regions: { 0: { startLoc: { line: 409, col: 9 }, endLoc: { line: 409, col: 25 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 146 }, 147: { name: "busy", count: 0, regions: { 0: { startLoc: { line: 410, col: 9 }, endLoc: { line: 410, col: 13 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 147 }, 148: { name: "busy", count: 0, regions: { 0: { startLoc: { line: 410, col: 9 }, endLoc: { line: 410, col: 22 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 148 }, 149: { name: "anonymous_64", count: 0, regions: { 0: { startLoc: { line: 411, col: 14 }, endLoc: { line: 411, col: 24 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 149 }, 150: { name: "hovered", count: 0, regions: { 0: { startLoc: { line: 413, col: 18 }, endLoc: { line: 413, col: 25 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 150 }, 151: { name: "hovered", count: 0, regions: { 0: { startLoc: { line: 413, col: 18 }, endLoc: { line: 413, col: 34 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 151 }, 152: { name: "pressed", count: 0, regions: { 0: { startLoc: { line: 414, col: 18 }, endLoc: { line: 414, col: 25 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 152 }, 153: { name: "pressed", count: 0, regions: { 0: { startLoc: { line: 414, col: 18 }, endLoc: { line: 414, col: 34 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 153 }, 154: { name: "isTarget", count: 0, regions: { 0: { startLoc: { line: 416, col: 3 }, endLoc: { line: 418, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 417, col: 5 }, endLoc: { line: 418, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 154 }, 155: { name: "setHovered", count: 0, regions: { 0: { startLoc: { line: 420, col: 3 }, endLoc: { line: 424, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 421, col: 5 }, endLoc: { line: 424, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 155 }, 156: { name: "anonymous_65", count: 0, regions: { 0: { startLoc: { line: 421, col: 48 }, endLoc: { line: 423, col: 6 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 422, col: 7 }, endLoc: { line: 423, col: 6 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 156 }, 157: { name: "statusText", count: 0, regions: { 0: { startLoc: { line: 426, col: 3 }, endLoc: { line: 431, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 427, col: 20 }, endLoc: { line: 429, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 430, col: 5 }, endLoc: { line: 431, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 427, col: 9 }, endLoc: { line: 427, col: 18 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 430, col: 12 }, endLoc: { line: 430, col: 62 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 157 }, 158: { name: "actionText", count: 0, regions: { 0: { startLoc: { line: 433, col: 3 }, endLoc: { line: 438, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 434, col: 20 }, endLoc: { line: 436, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 437, col: 5 }, endLoc: { line: 438, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 434, col: 9 }, endLoc: { line: 434, col: 18 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 437, col: 12 }, endLoc: { line: 437, col: 66 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 158 }, 159: { name: "actionDisabled", count: 0, regions: { 0: { startLoc: { line: 440, col: 3 }, endLoc: { line: 442, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 441, col: 5 }, endLoc: { line: 442, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 159 }, 160: { name: "initialRender", count: 0, regions: { 0: { startLoc: { line: 444, col: 3 }, endLoc: { line: 529, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 523, col: 9 }, endLoc: { line: 524, col: 8 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 496, col: 11 }, endLoc: { line: 497, col: 10 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 160 }, 161: { name: "anonymous_66", count: 0, regions: { 0: { startLoc: { line: 445, col: 5 }, endLoc: { line: 528, col: 6 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 502, col: 22 }, endLoc: { line: 503, col: 97 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 507, col: 14 }, endLoc: { line: 508, col: 47 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 2: { startLoc: { line: 511, col: 21 }, endLoc: { line: 511, col: 42 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 3: { startLoc: { line: 513, col: 10 }, endLoc: { line: 513, col: 56 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 4: { startLoc: { line: 513, col: 32 }, endLoc: { line: 513, col: 56 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 5: { startLoc: { line: 514, col: 10 }, endLoc: { line: 514, col: 56 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 6: { startLoc: { line: 514, col: 32 }, endLoc: { line: 514, col: 56 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 161 }, 162: { name: "anonymous_67", count: 0, regions: { 0: { startLoc: { line: 517, col: 14 }, endLoc: { line: 519, col: 6 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 518, col: 7 }, endLoc: { line: 519, col: 6 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 162 }, 163: { name: "anonymous_68", count: 0, regions: { 0: { startLoc: { line: 520, col: 14 }, endLoc: { line: 528, col: 6 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 521, col: 42 }, endLoc: { line: 524, col: 8 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 525, col: 75 }, endLoc: { line: 527, col: 8 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 521, col: 11 }, endLoc: { line: 521, col: 40 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 525, col: 11 }, endLoc: { line: 525, col: 73 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 163 }, 164: { name: "anonymous_69", count: 0, regions: { 0: { startLoc: { line: 446, col: 7 }, endLoc: { line: 458, col: 80 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 164 }, 165: { name: "anonymous_70", count: 0, regions: { 0: { startLoc: { line: 447, col: 9 }, endLoc: { line: 451, col: 27 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 165 }, 166: { name: "anonymous_71", count: 0, regions: { 0: { startLoc: { line: 460, col: 7 }, endLoc: { line: 479, col: 22 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 166 }, 167: { name: "anonymous_72", count: 0, regions: { 0: { startLoc: { line: 461, col: 9 }, endLoc: { line: 465, col: 24 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 167 }, 168: { name: "anonymous_73", count: 0, regions: { 0: { startLoc: { line: 466, col: 9 }, endLoc: { line: 472, col: 60 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 168 }, 169: { name: "anonymous_74", count: 0, regions: { 0: { startLoc: { line: 473, col: 9 }, endLoc: { line: 477, col: 24 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 475, col: 61 }, endLoc: { line: 476, col: 33 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 169 }, 170: { name: "anonymous_75", count: 0, regions: { 0: { startLoc: { line: 481, col: 7 }, endLoc: { line: 497, col: 10 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 485, col: 26 }, endLoc: { line: 486, col: 64 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 487, col: 20 }, endLoc: { line: 488, col: 49 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 170 }, 171: { name: "anonymous_76", count: 0, regions: { 0: { startLoc: { line: 492, col: 18 }, endLoc: { line: 497, col: 10 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 493, col: 38 }, endLoc: { line: 495, col: 12 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 493, col: 15 }, endLoc: { line: 493, col: 36 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 171 } }, exeLine: { 0: 1, 1: 2, 2: 5, 3: 6, 4: 7, 5: 8, 6: 9, 7: 10, 8: 11, 9: 12, 10: 13, 11: 15, 12: 17, 13: 18, 14: 20, 15: 21, 16: 24, 17: 25, 18: 26, 19: 30, 20: 31, 21: 34, 22: 35, 23: 36, 24: 38, 25: 41, 26: 42, 27: 45, 28: 46, 29: 47, 30: 48, 31: 49, 32: 50, 33: 51, 34: 52, 35: 53, 36: 55, 37: 56, 38: 57, 39: 58, 40: 59, 41: 60, 42: 61, 43: 63, 44: 64, 45: 65, 46: 66, 47: 67, 48: 68, 49: 69, 50: 70, 51: 71, 52: 72, 53: 73, 54: 74, 55: 75, 56: 76, 57: 77, 58: 78, 59: 79, 60: 80, 61: 81, 62: 82, 63: 83, 64: 84, 65: 85, 66: 86, 67: 87, 68: 88, 69: 91, 70: 93, 71: 94, 72: 95, 73: 96, 74: 97, 75: 98, 76: 99, 77: 100, 78: 101, 79: 102, 80: 103, 81: 104, 82: 105, 83: 106, 84: 108, 85: 109, 86: 110, 87: 112, 88: 115, 89: 116, 90: 117, 91: 118, 92: 119, 93: 120, 94: 121, 95: 122, 96: 123, 97: 124, 98: 126, 99: 127, 100: 128, 101: 129, 102: 130, 103: 132, 104: 133, 105: 134, 106: 136, 107: 137, 108: 138, 109: 139, 110: 141, 111: 142, 112: 149, 113: 150, 114: 151, 115: 152, 116: 154, 117: 155, 118: 157, 119: 158, 120: 161, 121: 162, 122: 163, 123: 167, 124: 168, 125: 169, 126: 170, 127: 171, 128: 172, 129: 173, 130: 174, 131: 176, 132: 177, 133: 178, 134: 179, 135: 180, 136: 181, 137: 183, 138: 184, 139: 185, 140: 186, 141: 187, 142: 188, 143: 189, 144: 190, 145: 191, 146: 192, 147: 193, 148: 194, 149: 195, 150: 197, 151: 199, 152: 200, 153: 201, 154: 202, 155: 203, 156: 204, 157: 205, 158: 206, 159: 207, 160: 208, 161: 211, 162: 212, 163: 213, 164: 214, 165: 215, 166: 216, 167: 217, 168: 218, 169: 219, 170: 220, 171: 222, 172: 223, 173: 224, 174: 225, 175: 226, 176: 228, 177: 229, 178: 230, 179: 232, 180: 233, 181: 234, 182: 235, 183: 237, 184: 238, 185: 245, 186: 246, 187: 247, 188: 248, 189: 249, 190: 250, 191: 252, 192: 254, 193: 255, 194: 257, 195: 258, 196: 261, 197: 262, 198: 263, 199: 267, 200: 268, 201: 271, 202: 272, 203: 275, 204: 276, 205: 279, 206: 280, 207: 281, 208: 282, 209: 283, 210: 284, 211: 285, 212: 286, 213: 287, 214: 289, 215: 290, 216: 291, 217: 292, 218: 293, 219: 294, 220: 296, 221: 297, 222: 298, 223: 299, 224: 300, 225: 301, 226: 302, 227: 303, 228: 304, 229: 305, 230: 306, 231: 307, 232: 308, 233: 310, 234: 312, 235: 313, 236: 314, 237: 317, 238: 318, 239: 320, 240: 321, 241: 322, 242: 323, 243: 324, 244: 325, 245: 326, 246: 327, 247: 328, 248: 329, 249: 330, 250: 331, 251: 332, 252: 334, 253: 336, 254: 337, 255: 338, 256: 339, 257: 340, 258: 341, 259: 342, 260: 343, 261: 344, 262: 345, 263: 346, 264: 347, 265: 348, 266: 349, 267: 351, 268: 354, 269: 355, 270: 357, 271: 358, 272: 359, 273: 360, 274: 361, 275: 363, 276: 364, 277: 365, 278: 366, 279: 367, 280: 368, 281: 369, 282: 370, 283: 371, 284: 372, 285: 374, 286: 375, 287: 376, 288: 377, 289: 378, 290: 380, 291: 381, 292: 382, 293: 384, 294: 385, 295: 386, 296: 387, 297: 389, 298: 390, 299: 397, 300: 398, 301: 399, 302: 400, 303: 401, 304: 402, 305: 403, 306: 404, 307: 405, 308: 406, 309: 407, 310: 408, 311: 409, 312: 410, 313: 411, 314: 413, 315: 414, 316: 416, 317: 417, 318: 420, 319: 421, 320: 422, 321: 426, 322: 427, 323: 428, 324: 430, 325: 433, 326: 434, 327: 435, 328: 437, 329: 440, 330: 441, 331: 444, 332: 445, 333: 446, 334: 447, 335: 448, 336: 449, 337: 450, 338: 451, 339: 453, 340: 454, 341: 455, 342: 456, 343: 457, 344: 458, 345: 460, 346: 461, 347: 462, 348: 463, 349: 464, 350: 465, 351: 466, 352: 467, 353: 468, 354: 469, 355: 470, 356: 471, 357: 472, 358: 473, 359: 474, 360: 475, 361: 476, 362: 477, 363: 479, 364: 481, 365: 482, 366: 483, 367: 484, 368: 485, 369: 486, 370: 487, 371: 488, 372: 489, 373: 490, 374: 491, 375: 492, 376: 493, 377: 494, 378: 496, 379: 499, 380: 500, 381: 501, 382: 502, 383: 503, 384: 504, 385: 505, 386: 506, 387: 507, 388: 508, 389: 510, 390: 511, 391: 512, 392: 513, 393: 514, 394: 516, 395: 517, 396: 518, 397: 520, 398: 521, 399: 522, 400: 523, 401: 525, 402: 526 } });
if (!("finalizeConstruction" in ViewPU.prototype)) {
    Reflect.set(ViewPU.prototype, "finalizeConstruction", () => { });
}
interface RemotePermissionCard_Params {
    isDark?: boolean;
    targetSection?: string;
    section?: string;
    title?: string;
    description?: string;
    grantedText?: string;
    missingText?: string;
    actionTextValue?: string;
    busyText?: string;
    icon?: Resource;
    accent?: SettingsAccentName;
    granted?: boolean;
    busy?: boolean;
    onRequest?: () => void;
    hovered?: boolean;
    pressed?: boolean;
}
interface RemoteAccessCard_Params {
    isDark?: boolean;
    targetSection?: string;
    gateEnabled?: boolean;
    accessCode?: string;
    onGateChange?: (enabled: boolean) => void;
    onRegenerate?: () => void;
    hovered?: boolean;
    pressed?: boolean;
}
interface RemoteFilesCard_Params {
    isDark?: boolean;
    targetSection?: string;
    onOpenRemoteFilesDirectory?: () => void;
    hovered?: boolean;
    pressed?: boolean;
}
interface XrdpServerCard_Params {
    isDark?: boolean;
    targetSection?: string;
    running?: boolean;
    busy?: boolean;
    state?: string;
    port?: number;
    message?: string;
    onRefresh?: () => void;
    onStart?: () => void;
    hovered?: boolean;
    pressed?: boolean;
}
import { SettingsRemoteControlSection, SettingsText } from "@normalized:N&&&common/src/main/ets/components/settings/SettingsConstants&";
import { SettingsAccent, SettingsResources, SettingsTheme } from "@normalized:N&&&common/src/main/ets/components/settings/SettingsUi&";
import type { SettingsAccentName } from "@normalized:N&&&common/src/main/ets/components/settings/SettingsUi&";
export class XrdpServerCard extends ViewPU {
    constructor(parent, params, __localStorage, elmtId = -1, paramsLambda = undefined, extraInfo) {
        super(parent, __localStorage, elmtId, extraInfo);
        if (typeof paramsLambda === "function") {
            this.paramsGenerator_ = paramsLambda;
        }
        this.__isDark = new SynchedPropertySimpleOneWayPU(params.isDark, this, "isDark");
        this.__targetSection = new SynchedPropertySimpleOneWayPU(params.targetSection, this, "targetSection");
        this.__running = new SynchedPropertySimpleOneWayPU(params.running, this, "running");
        this.__busy = new SynchedPropertySimpleOneWayPU(params.busy, this, "busy");
        this.__state = new SynchedPropertySimpleOneWayPU(params.state, this, "state");
        this.__port = new SynchedPropertySimpleOneWayPU(params.port, this, "port");
        this.__message = new SynchedPropertySimpleOneWayPU(params.message, this, "message");
        this.onRefresh = () => {
            bjccovmshb1i9e.instrumentFunction(6);
        };
        this.onStart = () => {
            bjccovmshb1i9e.instrumentFunction(7);
        };
        this.__hovered = new ObservedPropertySimplePU(false, this, "hovered");
        this.__pressed = new ObservedPropertySimplePU(false, this, "pressed");
        this.setInitiallyProvidedValue(params);
        this.finalizeConstruction();
    }
    setInitiallyProvidedValue(params: XrdpServerCard_Params) {
        bjccovmshb1i9e.instrumentFunction(8);
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
        if (params.running === undefined) {
            this.__running.set(false);
        }
        else {
        }
        if (params.busy === undefined) {
            this.__busy.set(false);
        }
        else {
        }
        if (params.state === undefined) {
            this.__state.set('Stopped');
        }
        else {
        }
        if (params.port === undefined) {
            this.__port.set(3390);
        }
        else {
        }
        if (params.message === undefined) {
            this.__message.set('');
        }
        else {
        }
        if (params.onRefresh !== undefined) {
            this.onRefresh = params.onRefresh;
        }
        else {
        }
        if (params.onStart !== undefined) {
            this.onStart = params.onStart;
        }
        else {
        }
        if (params.hovered !== undefined) {
            this.hovered = params.hovered;
        }
        else {
        }
        if (params.pressed !== undefined) {
            this.pressed = params.pressed;
        }
        else {
        }
    }
    updateStateVars(params: XrdpServerCard_Params) {
        bjccovmshb1i9e.instrumentFunction(9);
        this.__isDark.reset(params.isDark);
        this.__targetSection.reset(params.targetSection);
        this.__running.reset(params.running);
        this.__busy.reset(params.busy);
        this.__state.reset(params.state);
        this.__port.reset(params.port);
        this.__message.reset(params.message);
    }
    purgeVariableDependenciesOnElmtId(rmElmtId) {
        this.__isDark.purgeDependencyOnElmtId(rmElmtId);
        this.__targetSection.purgeDependencyOnElmtId(rmElmtId);
        this.__running.purgeDependencyOnElmtId(rmElmtId);
        this.__busy.purgeDependencyOnElmtId(rmElmtId);
        this.__state.purgeDependencyOnElmtId(rmElmtId);
        this.__port.purgeDependencyOnElmtId(rmElmtId);
        this.__message.purgeDependencyOnElmtId(rmElmtId);
        this.__hovered.purgeDependencyOnElmtId(rmElmtId);
        this.__pressed.purgeDependencyOnElmtId(rmElmtId);
    }
    aboutToBeDeleted() {
        this.__isDark.aboutToBeDeleted();
        this.__targetSection.aboutToBeDeleted();
        this.__running.aboutToBeDeleted();
        this.__busy.aboutToBeDeleted();
        this.__state.aboutToBeDeleted();
        this.__port.aboutToBeDeleted();
        this.__message.aboutToBeDeleted();
        this.__hovered.aboutToBeDeleted();
        this.__pressed.aboutToBeDeleted();
        SubscriberManager.Get().delete(this.id__());
        this.aboutToBeDeletedInternal();
    }
    private __isDark: SynchedPropertySimpleOneWayPU<boolean>;
    get isDark() {
        bjccovmshb1i9e.instrumentFunction(10);
        return this.__isDark.get();
    }
    set isDark(newValue: boolean) {
        bjccovmshb1i9e.instrumentFunction(11);
        this.__isDark.set(newValue);
    }
    private __targetSection: SynchedPropertySimpleOneWayPU<string>;
    get targetSection() {
        bjccovmshb1i9e.instrumentFunction(12);
        return this.__targetSection.get();
    }
    set targetSection(newValue: string) {
        bjccovmshb1i9e.instrumentFunction(13);
        this.__targetSection.set(newValue);
    }
    private __running: SynchedPropertySimpleOneWayPU<boolean>;
    get running() {
        bjccovmshb1i9e.instrumentFunction(14);
        return this.__running.get();
    }
    set running(newValue: boolean) {
        bjccovmshb1i9e.instrumentFunction(15);
        this.__running.set(newValue);
    }
    private __busy: SynchedPropertySimpleOneWayPU<boolean>;
    get busy() {
        bjccovmshb1i9e.instrumentFunction(16);
        return this.__busy.get();
    }
    set busy(newValue: boolean) {
        bjccovmshb1i9e.instrumentFunction(17);
        this.__busy.set(newValue);
    }
    private __state: SynchedPropertySimpleOneWayPU<string>;
    get state() {
        bjccovmshb1i9e.instrumentFunction(18);
        return this.__state.get();
    }
    set state(newValue: string) {
        bjccovmshb1i9e.instrumentFunction(19);
        this.__state.set(newValue);
    }
    private __port: SynchedPropertySimpleOneWayPU<number>;
    get port() {
        bjccovmshb1i9e.instrumentFunction(20);
        return this.__port.get();
    }
    set port(newValue: number) {
        bjccovmshb1i9e.instrumentFunction(21);
        this.__port.set(newValue);
    }
    private __message: SynchedPropertySimpleOneWayPU<string>;
    get message() {
        bjccovmshb1i9e.instrumentFunction(22);
        return this.__message.get();
    }
    set message(newValue: string) {
        bjccovmshb1i9e.instrumentFunction(23);
        this.__message.set(newValue);
    }
    private onRefresh: () => void;
    private onStart: () => void;
    private __hovered: ObservedPropertySimplePU<boolean>;
    get hovered() {
        bjccovmshb1i9e.instrumentFunction(26);
        return this.__hovered.get();
    }
    set hovered(newValue: boolean) {
        bjccovmshb1i9e.instrumentFunction(27);
        this.__hovered.set(newValue);
    }
    private __pressed: ObservedPropertySimplePU<boolean>;
    get pressed() {
        bjccovmshb1i9e.instrumentFunction(28);
        return this.__pressed.get();
    }
    set pressed(newValue: boolean) {
        bjccovmshb1i9e.instrumentFunction(29);
        this.__pressed.set(newValue);
    }
    private isTarget(): boolean {
        bjccovmshb1i9e.instrumentFunction(30);
        bjccovmshb1i9e.instrumentRegion(30, 1);
        return this.targetSection === SettingsRemoteControlSection.SERVER;
    }
    private setHovered(hovered: boolean): void {
        bjccovmshb1i9e.instrumentFunction(31);
        bjccovmshb1i9e.instrumentRegion(31, 1);
        SettingsTheme.animate(this.getUIContext(), () => {
            bjccovmshb1i9e.instrumentFunction(32);
            bjccovmshb1i9e.instrumentRegion(32, 1);
            this.hovered = hovered;
        });
    }
    private statusText(): string {
        bjccovmshb1i9e.instrumentFunction(33);
        bjccovmshb1i9e.instrumentRegion(33, 1);
        return SettingsText.remoteServerStatusText(this.running, this.busy, this.state, this.port);
    }
    private actionText(): string {
        bjccovmshb1i9e.instrumentFunction(34);
        if (this.busy) {
            bjccovmshb1i9e.instrumentBranch(34, 0, true);
            bjccovmshb1i9e.instrumentRegion(34, 1);
            return SettingsText.REMOTE_SERVER_BUSY;
        }
        else {
            bjccovmshb1i9e.instrumentBranch(34, 0, false);
        }
        bjccovmshb1i9e.instrumentRegion(34, 2);
        return this.running ? (bjccovmshb1i9e.instrumentBranch(34, 1, true), SettingsText.REMOTE_SERVER_REFRESH_ACTION) : (bjccovmshb1i9e.instrumentBranch(34, 1, false), SettingsText.REMOTE_SERVER_START_ACTION);
    }
    private actionDisabled(): boolean {
        bjccovmshb1i9e.instrumentFunction(35);
        bjccovmshb1i9e.instrumentRegion(35, 1);
        return this.busy;
    }
    initialRender() {
        bjccovmshb1i9e.instrumentFunction(36);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i9e.instrumentFunction(37);
            Row.create({ space: 14 });
            Row.alignItems(VerticalAlign.Center);
            Row.width('100%');
            Row.padding(16);
            Row.backgroundColor(this.hovered || this.pressed || this.isTarget() ? (bjccovmshb1i9e.instrumentBranch(37, 0, true), SettingsTheme.cardHoverBackground(this.isDark)) : (bjccovmshb1i9e.instrumentBranch(37, 0, false), SettingsTheme.cardBackground(this.isDark)));
            Row.borderRadius(SettingsTheme.CARD_RADIUS);
            Row.border({
                width: 1,
                color: this.isTarget() ? (bjccovmshb1i9e.instrumentBranch(37, 1, true), SettingsTheme.accentColor(this.isDark, SettingsAccent.BLUE)) : (bjccovmshb1i9e.instrumentBranch(37, 1, false), SettingsTheme.borderColor(this.isDark))
            });
            Row.shadow(SettingsTheme.shadow(this.isDark, this.hovered || this.isTarget()));
            Row.translate({ y: this.hovered ? (bjccovmshb1i9e.instrumentBranch(37, 2, true), -5) : (bjccovmshb1i9e.instrumentBranch(37, 2, false), 0) });
            Row.scale({
                x: this.pressed ? (bjccovmshb1i9e.instrumentBranch(37, 3, true), 0.99) : (bjccovmshb1i9e.instrumentBranch(37, 3, false), this.hovered ? (bjccovmshb1i9e.instrumentBranch(37, 4, true), 1.012) : (bjccovmshb1i9e.instrumentBranch(37, 4, false), 1)),
                y: this.pressed ? (bjccovmshb1i9e.instrumentBranch(37, 5, true), 0.99) : (bjccovmshb1i9e.instrumentBranch(37, 5, false), this.hovered ? (bjccovmshb1i9e.instrumentBranch(37, 6, true), 1.012) : (bjccovmshb1i9e.instrumentBranch(37, 6, false), 1))
            });
            Row.margin({ bottom: 12 });
            Row.onHover((isHover: boolean) => {
                bjccovmshb1i9e.instrumentFunction(38);
                bjccovmshb1i9e.instrumentRegion(38, 1);
                this.setHovered(isHover);
            });
            Row.onTouch((event: TouchEvent) => {
                bjccovmshb1i9e.instrumentFunction(39);
                if (event.type === TouchType.Down) {
                    bjccovmshb1i9e.instrumentBranch(39, 0, true);
                    bjccovmshb1i9e.instrumentRegion(39, 1);
                    this.pressed = true;
                    bjccovmshb1i9e.instrumentRegion(36, 1);
                    return;
                }
                else {
                    bjccovmshb1i9e.instrumentBranch(39, 0, false);
                }
                if (event.type === TouchType.Up || event.type === TouchType.Cancel) {
                    bjccovmshb1i9e.instrumentBranch(39, 1, true);
                    bjccovmshb1i9e.instrumentRegion(39, 2);
                    this.pressed = false;
                }
                else {
                    bjccovmshb1i9e.instrumentBranch(39, 1, false);
                }
            });
        }, Row);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i9e.instrumentFunction(40);
            Column.create();
            Column.width(SettingsTheme.ICON_TILE_SIZE);
            Column.height(SettingsTheme.ICON_TILE_SIZE);
            Column.justifyContent(FlexAlign.Center);
            Column.alignItems(HorizontalAlign.Center);
            Column.borderRadius(SettingsTheme.BUTTON_RADIUS);
            Column.backgroundColor(SettingsTheme.accentBackground(this.isDark, this.running ? (bjccovmshb1i9e.instrumentBranch(40, 0, true), SettingsAccent.GREEN) : (bjccovmshb1i9e.instrumentBranch(40, 0, false), SettingsAccent.AMBER)));
        }, Column);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i9e.instrumentFunction(41);
            Image.create(SettingsResources.REMOTE_SERVICE_ICON);
            Image.width(SettingsTheme.ICON_SIZE);
            Image.height(SettingsTheme.ICON_SIZE);
            Image.fillColor(SettingsTheme.accentColor(this.isDark, this.running ? (bjccovmshb1i9e.instrumentBranch(41, 0, true), SettingsAccent.GREEN) : (bjccovmshb1i9e.instrumentBranch(41, 0, false), SettingsAccent.AMBER)));
            Image.draggable(false);
        }, Image);
        Column.pop();
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i9e.instrumentFunction(42);
            Column.create({ space: 5 });
            Column.layoutWeight(1);
        }, Column);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i9e.instrumentFunction(43);
            Text.create(SettingsText.REMOTE_SERVER_TITLE);
            Text.fontSize(15);
            Text.fontWeight(FontWeight.Medium);
            Text.fontColor(SettingsTheme.primaryText(this.isDark));
            Text.width('100%');
        }, Text);
        Text.pop();
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i9e.instrumentFunction(44);
            Text.create(SettingsText.REMOTE_SERVER_DESC);
            Text.fontSize(13);
            Text.fontColor(SettingsTheme.mutedText(this.isDark));
            Text.lineHeight(19);
            Text.width('100%');
            Text.maxLines(2);
            Text.textOverflow({ overflow: TextOverflow.Ellipsis });
        }, Text);
        Text.pop();
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i9e.instrumentFunction(45);
            Text.create(this.statusText());
            Text.fontSize(12);
            Text.fontColor(SettingsTheme.accentColor(this.isDark, this.running ? (bjccovmshb1i9e.instrumentBranch(45, 0, true), SettingsAccent.GREEN) : (bjccovmshb1i9e.instrumentBranch(45, 0, false), SettingsAccent.AMBER)));
            Text.width('100%');
        }, Text);
        Text.pop();
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i9e.instrumentFunction(46);
            If.create();
            if (this.message.length > 0) {
                bjccovmshb1i9e.instrumentBranch(46, 0, true);
                bjccovmshb1i9e.instrumentRegion(46, 1);
                this.ifElseBranchUpdateFunction(0, () => {
                    bjccovmshb1i9e.instrumentFunction(47);
                    this.observeComponentCreation2((elmtId, isInitialRender) => {
                        bjccovmshb1i9e.instrumentFunction(48);
                        Text.create(this.message);
                        Text.fontSize(12);
                        Text.fontColor(SettingsTheme.mutedText(this.isDark));
                        Text.lineHeight(18);
                        Text.width('100%');
                        Text.maxLines(2);
                        Text.textOverflow({ overflow: TextOverflow.Ellipsis });
                    }, Text);
                    Text.pop();
                });
            }
            else {
                bjccovmshb1i9e.instrumentBranch(46, 0, false);
                this.ifElseBranchUpdateFunction(1, () => {
                });
            }
        }, If);
        If.pop();
        Column.pop();
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i9e.instrumentFunction(49);
            Button.createWithLabel(this.actionText());
            Button.type(ButtonType.Normal);
            Button.constraintSize({ minHeight: 48 });
            Button.borderRadius(SettingsTheme.BUTTON_RADIUS);
            Button.backgroundColor(this.actionDisabled() ? (bjccovmshb1i9e.instrumentBranch(49, 0, true), SettingsTheme.disabledButton(this.isDark)) : (bjccovmshb1i9e.instrumentBranch(49, 0, false), SettingsTheme.subtleButton(this.isDark, false, false)));
            Button.fontColor(this.actionDisabled() ? (bjccovmshb1i9e.instrumentBranch(49, 1, true), SettingsTheme.disabledText(this.isDark)) : (bjccovmshb1i9e.instrumentBranch(49, 1, false), SettingsTheme.primaryText(this.isDark)));
            Button.fontSize(12);
            Button.stateEffect(false);
            Button.enabled(!this.actionDisabled());
            Button.onClick(() => {
                bjccovmshb1i9e.instrumentFunction(50);
                if (this.actionDisabled()) {
                    bjccovmshb1i9e.instrumentBranch(50, 0, true);
                    bjccovmshb1i9e.instrumentRegion(50, 1);
                    return;
                }
                else {
                    bjccovmshb1i9e.instrumentBranch(50, 0, false);
                }
                if (this.running) {
                    bjccovmshb1i9e.instrumentBranch(50, 1, true);
                    bjccovmshb1i9e.instrumentRegion(50, 2);
                    this.onRefresh();
                    bjccovmshb1i9e.instrumentRegion(36, 3);
                    return;
                }
                else {
                    bjccovmshb1i9e.instrumentBranch(50, 1, false);
                }
                bjccovmshb1i9e.instrumentRegion(36, 2);
                this.onStart();
            });
        }, Button);
        Button.pop();
        Row.pop();
    }
    rerender() {
        this.updateDirtyElements();
    }
}
export class RemoteFilesCard extends ViewPU {
    constructor(parent, params, __localStorage, elmtId = -1, paramsLambda = undefined, extraInfo) {
        super(parent, __localStorage, elmtId, extraInfo);
        if (typeof paramsLambda === "function") {
            this.paramsGenerator_ = paramsLambda;
        }
        this.__isDark = new SynchedPropertySimpleOneWayPU(params.isDark, this, "isDark");
        this.__targetSection = new SynchedPropertySimpleOneWayPU(params.targetSection, this, "targetSection");
        this.onOpenRemoteFilesDirectory = () => {
            bjccovmshb1i9e.instrumentFunction(51);
        };
        this.__hovered = new ObservedPropertySimplePU(false, this, "hovered");
        this.__pressed = new ObservedPropertySimplePU(false, this, "pressed");
        this.setInitiallyProvidedValue(params);
        this.finalizeConstruction();
    }
    setInitiallyProvidedValue(params: RemoteFilesCard_Params) {
        bjccovmshb1i9e.instrumentFunction(52);
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
        if (params.onOpenRemoteFilesDirectory !== undefined) {
            this.onOpenRemoteFilesDirectory = params.onOpenRemoteFilesDirectory;
        }
        else {
        }
        if (params.hovered !== undefined) {
            this.hovered = params.hovered;
        }
        else {
        }
        if (params.pressed !== undefined) {
            this.pressed = params.pressed;
        }
        else {
        }
    }
    updateStateVars(params: RemoteFilesCard_Params) {
        bjccovmshb1i9e.instrumentFunction(53);
        this.__isDark.reset(params.isDark);
        this.__targetSection.reset(params.targetSection);
    }
    purgeVariableDependenciesOnElmtId(rmElmtId) {
        this.__isDark.purgeDependencyOnElmtId(rmElmtId);
        this.__targetSection.purgeDependencyOnElmtId(rmElmtId);
        this.__hovered.purgeDependencyOnElmtId(rmElmtId);
        this.__pressed.purgeDependencyOnElmtId(rmElmtId);
    }
    aboutToBeDeleted() {
        this.__isDark.aboutToBeDeleted();
        this.__targetSection.aboutToBeDeleted();
        this.__hovered.aboutToBeDeleted();
        this.__pressed.aboutToBeDeleted();
        SubscriberManager.Get().delete(this.id__());
        this.aboutToBeDeletedInternal();
    }
    private __isDark: SynchedPropertySimpleOneWayPU<boolean>;
    get isDark() {
        bjccovmshb1i9e.instrumentFunction(54);
        return this.__isDark.get();
    }
    set isDark(newValue: boolean) {
        bjccovmshb1i9e.instrumentFunction(55);
        this.__isDark.set(newValue);
    }
    private __targetSection: SynchedPropertySimpleOneWayPU<string>;
    get targetSection() {
        bjccovmshb1i9e.instrumentFunction(56);
        return this.__targetSection.get();
    }
    set targetSection(newValue: string) {
        bjccovmshb1i9e.instrumentFunction(57);
        this.__targetSection.set(newValue);
    }
    private onOpenRemoteFilesDirectory: () => void;
    private __hovered: ObservedPropertySimplePU<boolean>;
    get hovered() {
        bjccovmshb1i9e.instrumentFunction(59);
        return this.__hovered.get();
    }
    set hovered(newValue: boolean) {
        bjccovmshb1i9e.instrumentFunction(60);
        this.__hovered.set(newValue);
    }
    private __pressed: ObservedPropertySimplePU<boolean>;
    get pressed() {
        bjccovmshb1i9e.instrumentFunction(61);
        return this.__pressed.get();
    }
    set pressed(newValue: boolean) {
        bjccovmshb1i9e.instrumentFunction(62);
        this.__pressed.set(newValue);
    }
    private isTarget(): boolean {
        bjccovmshb1i9e.instrumentFunction(63);
        bjccovmshb1i9e.instrumentRegion(63, 1);
        return this.targetSection === SettingsRemoteControlSection.FILES;
    }
    private setHovered(hovered: boolean): void {
        bjccovmshb1i9e.instrumentFunction(64);
        bjccovmshb1i9e.instrumentRegion(64, 1);
        SettingsTheme.animate(this.getUIContext(), () => {
            bjccovmshb1i9e.instrumentFunction(65);
            bjccovmshb1i9e.instrumentRegion(65, 1);
            this.hovered = hovered;
        });
    }
    initialRender() {
        bjccovmshb1i9e.instrumentFunction(66);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i9e.instrumentFunction(67);
            Row.create({ space: 14 });
            Row.alignItems(VerticalAlign.Center);
            Row.width('100%');
            Row.padding(16);
            Row.backgroundColor(this.hovered || this.pressed || this.isTarget() ? (bjccovmshb1i9e.instrumentBranch(67, 0, true), SettingsTheme.cardHoverBackground(this.isDark)) : (bjccovmshb1i9e.instrumentBranch(67, 0, false), SettingsTheme.cardBackground(this.isDark)));
            Row.borderRadius(SettingsTheme.CARD_RADIUS);
            Row.border({
                width: 1,
                color: this.isTarget() ? (bjccovmshb1i9e.instrumentBranch(67, 1, true), SettingsTheme.accentColor(this.isDark, SettingsAccent.PURPLE)) : (bjccovmshb1i9e.instrumentBranch(67, 1, false), SettingsTheme.borderColor(this.isDark))
            });
            Row.shadow(SettingsTheme.shadow(this.isDark, this.hovered || this.isTarget()));
            Row.translate({ y: this.hovered ? (bjccovmshb1i9e.instrumentBranch(67, 2, true), -5) : (bjccovmshb1i9e.instrumentBranch(67, 2, false), 0) });
            Row.scale({
                x: this.pressed ? (bjccovmshb1i9e.instrumentBranch(67, 3, true), 0.99) : (bjccovmshb1i9e.instrumentBranch(67, 3, false), this.hovered ? (bjccovmshb1i9e.instrumentBranch(67, 4, true), 1.012) : (bjccovmshb1i9e.instrumentBranch(67, 4, false), 1)),
                y: this.pressed ? (bjccovmshb1i9e.instrumentBranch(67, 5, true), 0.99) : (bjccovmshb1i9e.instrumentBranch(67, 5, false), this.hovered ? (bjccovmshb1i9e.instrumentBranch(67, 6, true), 1.012) : (bjccovmshb1i9e.instrumentBranch(67, 6, false), 1))
            });
            Row.margin({ bottom: 12 });
            Row.onHover((isHover: boolean) => {
                bjccovmshb1i9e.instrumentFunction(68);
                bjccovmshb1i9e.instrumentRegion(68, 1);
                this.setHovered(isHover);
            });
            Row.onTouch((event: TouchEvent) => {
                bjccovmshb1i9e.instrumentFunction(69);
                if (event.type === TouchType.Down) {
                    bjccovmshb1i9e.instrumentBranch(69, 0, true);
                    bjccovmshb1i9e.instrumentRegion(69, 1);
                    this.pressed = true;
                    bjccovmshb1i9e.instrumentRegion(66, 1);
                    return;
                }
                else {
                    bjccovmshb1i9e.instrumentBranch(69, 0, false);
                }
                if (event.type === TouchType.Up || event.type === TouchType.Cancel) {
                    bjccovmshb1i9e.instrumentBranch(69, 1, true);
                    bjccovmshb1i9e.instrumentRegion(69, 2);
                    this.pressed = false;
                }
                else {
                    bjccovmshb1i9e.instrumentBranch(69, 1, false);
                }
            });
        }, Row);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i9e.instrumentFunction(70);
            Column.create();
            Column.width(SettingsTheme.ICON_TILE_SIZE);
            Column.height(SettingsTheme.ICON_TILE_SIZE);
            Column.justifyContent(FlexAlign.Center);
            Column.alignItems(HorizontalAlign.Center);
            Column.borderRadius(SettingsTheme.BUTTON_RADIUS);
            Column.backgroundColor(SettingsTheme.accentBackground(this.isDark, SettingsAccent.PURPLE));
        }, Column);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i9e.instrumentFunction(71);
            Image.create(SettingsResources.REMOTE_FILES_ICON);
            Image.width(SettingsTheme.ICON_SIZE);
            Image.height(SettingsTheme.ICON_SIZE);
            Image.fillColor(SettingsTheme.accentColor(this.isDark, SettingsAccent.PURPLE));
            Image.draggable(false);
        }, Image);
        Column.pop();
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i9e.instrumentFunction(72);
            Column.create({ space: 5 });
            Column.layoutWeight(1);
        }, Column);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i9e.instrumentFunction(73);
            Text.create(SettingsText.REMOTE_FILES_FEATURE_TITLE);
            Text.fontSize(15);
            Text.fontWeight(FontWeight.Medium);
            Text.fontColor(SettingsTheme.primaryText(this.isDark));
            Text.width('100%');
        }, Text);
        Text.pop();
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i9e.instrumentFunction(74);
            Text.create(SettingsText.REMOTE_FILES_FEATURE_DESC);
            Text.fontSize(13);
            Text.fontColor(SettingsTheme.mutedText(this.isDark));
            Text.lineHeight(19);
            Text.width('100%');
            Text.maxLines(2);
            Text.textOverflow({ overflow: TextOverflow.Ellipsis });
        }, Text);
        Text.pop();
        Column.pop();
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i9e.instrumentFunction(75);
            Button.createWithLabel(SettingsText.REMOTE_FILES_FEATURE_ACTION);
            Button.type(ButtonType.Normal);
            Button.constraintSize({ minHeight: 48 });
            Button.borderRadius(SettingsTheme.BUTTON_RADIUS);
            Button.backgroundColor(SettingsTheme.subtleButton(this.isDark, false, false));
            Button.fontColor(SettingsTheme.primaryText(this.isDark));
            Button.fontSize(12);
            Button.stateEffect(false);
            Button.onClick(() => {
                bjccovmshb1i9e.instrumentFunction(76);
                bjccovmshb1i9e.instrumentRegion(76, 1);
                this.onOpenRemoteFilesDirectory();
            });
        }, Button);
        Button.pop();
        Row.pop();
    }
    rerender() {
        this.updateDirtyElements();
    }
}
export class RemoteAccessCard extends ViewPU {
    constructor(parent, params, __localStorage, elmtId = -1, paramsLambda = undefined, extraInfo) {
        super(parent, __localStorage, elmtId, extraInfo);
        if (typeof paramsLambda === "function") {
            this.paramsGenerator_ = paramsLambda;
        }
        this.__isDark = new SynchedPropertySimpleOneWayPU(params.isDark, this, "isDark");
        this.__targetSection = new SynchedPropertySimpleOneWayPU(params.targetSection, this, "targetSection");
        this.__gateEnabled = new SynchedPropertySimpleOneWayPU(params.gateEnabled, this, "gateEnabled");
        this.__accessCode = new SynchedPropertySimpleOneWayPU(params.accessCode, this, "accessCode");
        this.onGateChange = (_enabled: boolean) => {
            bjccovmshb1i9e.instrumentFunction(77);
        };
        this.onRegenerate = () => {
            bjccovmshb1i9e.instrumentFunction(78);
        };
        this.__hovered = new ObservedPropertySimplePU(false, this, "hovered");
        this.__pressed = new ObservedPropertySimplePU(false, this, "pressed");
        this.setInitiallyProvidedValue(params);
        this.finalizeConstruction();
    }
    setInitiallyProvidedValue(params: RemoteAccessCard_Params) {
        bjccovmshb1i9e.instrumentFunction(79);
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
        if (params.gateEnabled === undefined) {
            this.__gateEnabled.set(false);
        }
        else {
        }
        if (params.accessCode === undefined) {
            this.__accessCode.set('000000');
        }
        else {
        }
        if (params.onGateChange !== undefined) {
            this.onGateChange = params.onGateChange;
        }
        else {
        }
        if (params.onRegenerate !== undefined) {
            this.onRegenerate = params.onRegenerate;
        }
        else {
        }
        if (params.hovered !== undefined) {
            this.hovered = params.hovered;
        }
        else {
        }
        if (params.pressed !== undefined) {
            this.pressed = params.pressed;
        }
        else {
        }
    }
    updateStateVars(params: RemoteAccessCard_Params) {
        bjccovmshb1i9e.instrumentFunction(80);
        this.__isDark.reset(params.isDark);
        this.__targetSection.reset(params.targetSection);
        this.__gateEnabled.reset(params.gateEnabled);
        this.__accessCode.reset(params.accessCode);
    }
    purgeVariableDependenciesOnElmtId(rmElmtId) {
        this.__isDark.purgeDependencyOnElmtId(rmElmtId);
        this.__targetSection.purgeDependencyOnElmtId(rmElmtId);
        this.__gateEnabled.purgeDependencyOnElmtId(rmElmtId);
        this.__accessCode.purgeDependencyOnElmtId(rmElmtId);
        this.__hovered.purgeDependencyOnElmtId(rmElmtId);
        this.__pressed.purgeDependencyOnElmtId(rmElmtId);
    }
    aboutToBeDeleted() {
        this.__isDark.aboutToBeDeleted();
        this.__targetSection.aboutToBeDeleted();
        this.__gateEnabled.aboutToBeDeleted();
        this.__accessCode.aboutToBeDeleted();
        this.__hovered.aboutToBeDeleted();
        this.__pressed.aboutToBeDeleted();
        SubscriberManager.Get().delete(this.id__());
        this.aboutToBeDeletedInternal();
    }
    private __isDark: SynchedPropertySimpleOneWayPU<boolean>;
    get isDark() {
        bjccovmshb1i9e.instrumentFunction(81);
        return this.__isDark.get();
    }
    set isDark(newValue: boolean) {
        bjccovmshb1i9e.instrumentFunction(82);
        this.__isDark.set(newValue);
    }
    private __targetSection: SynchedPropertySimpleOneWayPU<string>;
    get targetSection() {
        bjccovmshb1i9e.instrumentFunction(83);
        return this.__targetSection.get();
    }
    set targetSection(newValue: string) {
        bjccovmshb1i9e.instrumentFunction(84);
        this.__targetSection.set(newValue);
    }
    private __gateEnabled: SynchedPropertySimpleOneWayPU<boolean>;
    get gateEnabled() {
        bjccovmshb1i9e.instrumentFunction(85);
        return this.__gateEnabled.get();
    }
    set gateEnabled(newValue: boolean) {
        bjccovmshb1i9e.instrumentFunction(86);
        this.__gateEnabled.set(newValue);
    }
    private __accessCode: SynchedPropertySimpleOneWayPU<string>;
    get accessCode() {
        bjccovmshb1i9e.instrumentFunction(87);
        return this.__accessCode.get();
    }
    set accessCode(newValue: string) {
        bjccovmshb1i9e.instrumentFunction(88);
        this.__accessCode.set(newValue);
    }
    private onGateChange: (enabled: boolean) => void;
    private onRegenerate: () => void;
    private __hovered: ObservedPropertySimplePU<boolean>;
    get hovered() {
        bjccovmshb1i9e.instrumentFunction(91);
        return this.__hovered.get();
    }
    set hovered(newValue: boolean) {
        bjccovmshb1i9e.instrumentFunction(92);
        this.__hovered.set(newValue);
    }
    private __pressed: ObservedPropertySimplePU<boolean>;
    get pressed() {
        bjccovmshb1i9e.instrumentFunction(93);
        return this.__pressed.get();
    }
    set pressed(newValue: boolean) {
        bjccovmshb1i9e.instrumentFunction(94);
        this.__pressed.set(newValue);
    }
    private isTarget(): boolean {
        bjccovmshb1i9e.instrumentFunction(95);
        bjccovmshb1i9e.instrumentRegion(95, 1);
        return this.targetSection === SettingsRemoteControlSection.ACCESS;
    }
    private setHovered(hovered: boolean): void {
        bjccovmshb1i9e.instrumentFunction(96);
        bjccovmshb1i9e.instrumentRegion(96, 1);
        SettingsTheme.animate(this.getUIContext(), () => {
            bjccovmshb1i9e.instrumentFunction(97);
            bjccovmshb1i9e.instrumentRegion(97, 1);
            this.hovered = hovered;
        });
    }
    private gateStatusText(): string {
        bjccovmshb1i9e.instrumentFunction(98);
        bjccovmshb1i9e.instrumentRegion(98, 1);
        return this.gateEnabled ? (bjccovmshb1i9e.instrumentBranch(98, 0, true), SettingsText.REMOTE_ACCESS_GATE_ON) : (bjccovmshb1i9e.instrumentBranch(98, 0, false), SettingsText.REMOTE_ACCESS_GATE_OFF);
    }
    private accessCodeText(): string {
        bjccovmshb1i9e.instrumentFunction(99);
        bjccovmshb1i9e.instrumentRegion(99, 1);
        return this.gateEnabled ? (bjccovmshb1i9e.instrumentBranch(99, 0, true), this.accessCode) : (bjccovmshb1i9e.instrumentBranch(99, 0, false), SettingsText.REMOTE_ACCESS_CODE_DISABLED);
    }
    private regenerateDisabled(): boolean {
        bjccovmshb1i9e.instrumentFunction(100);
        bjccovmshb1i9e.instrumentRegion(100, 1);
        return !this.gateEnabled;
    }
    initialRender() {
        bjccovmshb1i9e.instrumentFunction(101);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i9e.instrumentFunction(102);
            Column.create({ space: 14 });
            Column.alignItems(HorizontalAlign.Start);
            Column.width('100%');
            Column.padding(16);
            Column.backgroundColor(this.hovered || this.pressed || this.isTarget() ? (bjccovmshb1i9e.instrumentBranch(102, 0, true), SettingsTheme.cardHoverBackground(this.isDark)) : (bjccovmshb1i9e.instrumentBranch(102, 0, false), SettingsTheme.cardBackground(this.isDark)));
            Column.borderRadius(SettingsTheme.CARD_RADIUS);
            Column.border({
                width: 1,
                color: this.isTarget() ? (bjccovmshb1i9e.instrumentBranch(102, 1, true), SettingsTheme.accentColor(this.isDark, SettingsAccent.BLUE)) : (bjccovmshb1i9e.instrumentBranch(102, 1, false), SettingsTheme.borderColor(this.isDark))
            });
            Column.shadow(SettingsTheme.shadow(this.isDark, this.hovered || this.isTarget()));
            Column.translate({ y: this.hovered ? (bjccovmshb1i9e.instrumentBranch(102, 2, true), -5) : (bjccovmshb1i9e.instrumentBranch(102, 2, false), 0) });
            Column.scale({
                x: this.pressed ? (bjccovmshb1i9e.instrumentBranch(102, 3, true), 0.99) : (bjccovmshb1i9e.instrumentBranch(102, 3, false), this.hovered ? (bjccovmshb1i9e.instrumentBranch(102, 4, true), 1.012) : (bjccovmshb1i9e.instrumentBranch(102, 4, false), 1)),
                y: this.pressed ? (bjccovmshb1i9e.instrumentBranch(102, 5, true), 0.99) : (bjccovmshb1i9e.instrumentBranch(102, 5, false), this.hovered ? (bjccovmshb1i9e.instrumentBranch(102, 6, true), 1.012) : (bjccovmshb1i9e.instrumentBranch(102, 6, false), 1))
            });
            Column.margin({ bottom: 12 });
            Column.onHover((isHover: boolean) => {
                bjccovmshb1i9e.instrumentFunction(103);
                bjccovmshb1i9e.instrumentRegion(103, 1);
                this.setHovered(isHover);
            });
            Column.onTouch((event: TouchEvent) => {
                bjccovmshb1i9e.instrumentFunction(104);
                if (event.type === TouchType.Down) {
                    bjccovmshb1i9e.instrumentBranch(104, 0, true);
                    bjccovmshb1i9e.instrumentRegion(104, 1);
                    this.pressed = true;
                    bjccovmshb1i9e.instrumentRegion(101, 1);
                    return;
                }
                else {
                    bjccovmshb1i9e.instrumentBranch(104, 0, false);
                }
                if (event.type === TouchType.Up || event.type === TouchType.Cancel) {
                    bjccovmshb1i9e.instrumentBranch(104, 1, true);
                    bjccovmshb1i9e.instrumentRegion(104, 2);
                    this.pressed = false;
                }
                else {
                    bjccovmshb1i9e.instrumentBranch(104, 1, false);
                }
            });
        }, Column);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i9e.instrumentFunction(105);
            Row.create({ space: 14 });
            Row.width('100%');
            Row.alignItems(VerticalAlign.Center);
        }, Row);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i9e.instrumentFunction(106);
            Column.create();
            Column.width(SettingsTheme.ICON_TILE_SIZE);
            Column.height(SettingsTheme.ICON_TILE_SIZE);
            Column.justifyContent(FlexAlign.Center);
            Column.alignItems(HorizontalAlign.Center);
            Column.borderRadius(SettingsTheme.BUTTON_RADIUS);
            Column.backgroundColor(SettingsTheme.accentBackground(this.isDark, SettingsAccent.BLUE));
        }, Column);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i9e.instrumentFunction(107);
            Image.create(SettingsResources.REMOTE_ACCESS_ICON);
            Image.width(SettingsTheme.ICON_SIZE);
            Image.height(SettingsTheme.ICON_SIZE);
            Image.fillColor(SettingsTheme.accentColor(this.isDark, SettingsAccent.BLUE));
            Image.draggable(false);
        }, Image);
        Column.pop();
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i9e.instrumentFunction(108);
            Column.create({ space: 5 });
            Column.layoutWeight(1);
        }, Column);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i9e.instrumentFunction(109);
            Text.create(SettingsText.REMOTE_ACCESS_GATE_TITLE);
            Text.fontSize(15);
            Text.fontWeight(FontWeight.Medium);
            Text.fontColor(SettingsTheme.primaryText(this.isDark));
            Text.width('100%');
        }, Text);
        Text.pop();
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i9e.instrumentFunction(110);
            Text.create(SettingsText.REMOTE_ACCESS_GATE_DESC);
            Text.fontSize(13);
            Text.fontColor(SettingsTheme.mutedText(this.isDark));
            Text.lineHeight(19);
            Text.width('100%');
            Text.maxLines(2);
            Text.textOverflow({ overflow: TextOverflow.Ellipsis });
        }, Text);
        Text.pop();
        Column.pop();
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i9e.instrumentFunction(111);
            Toggle.create({ type: ToggleType.Switch, isOn: this.gateEnabled });
            Toggle.onChange((enabled: boolean) => {
                bjccovmshb1i9e.instrumentFunction(112);
                bjccovmshb1i9e.instrumentRegion(112, 1);
                this.onGateChange(enabled);
            });
        }, Toggle);
        Toggle.pop();
        Row.pop();
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i9e.instrumentFunction(113);
            Row.create();
            Row.alignItems(VerticalAlign.Center);
            Row.width('100%');
        }, Row);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i9e.instrumentFunction(114);
            Column.create({ space: 4 });
            Column.layoutWeight(1);
        }, Column);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i9e.instrumentFunction(115);
            Text.create(SettingsText.REMOTE_ACCESS_CODE_LABEL);
            Text.fontSize(12);
            Text.fontColor(SettingsTheme.mutedText(this.isDark));
            Text.width('100%');
        }, Text);
        Text.pop();
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i9e.instrumentFunction(116);
            Text.create(this.accessCodeText());
            Text.fontSize(18);
            Text.fontWeight(FontWeight.Medium);
            Text.fontColor(SettingsTheme.primaryText(this.isDark));
            Text.width('100%');
            Text.maxLines(1);
            Text.textOverflow({ overflow: TextOverflow.Ellipsis });
        }, Text);
        Text.pop();
        Column.pop();
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i9e.instrumentFunction(117);
            Button.createWithLabel(SettingsText.REMOTE_ACCESS_CODE_ACTION);
            Button.type(ButtonType.Normal);
            Button.constraintSize({ minHeight: 48 });
            Button.borderRadius(SettingsTheme.BUTTON_RADIUS);
            Button.backgroundColor(this.regenerateDisabled() ? (bjccovmshb1i9e.instrumentBranch(117, 0, true), SettingsTheme.disabledButton(this.isDark)) : (bjccovmshb1i9e.instrumentBranch(117, 0, false), SettingsTheme.subtleButton(this.isDark, false, false)));
            Button.fontColor(this.regenerateDisabled() ? (bjccovmshb1i9e.instrumentBranch(117, 1, true), SettingsTheme.disabledText(this.isDark)) : (bjccovmshb1i9e.instrumentBranch(117, 1, false), SettingsTheme.primaryText(this.isDark)));
            Button.fontSize(12);
            Button.stateEffect(false);
            Button.enabled(!this.regenerateDisabled());
            Button.onClick(() => {
                bjccovmshb1i9e.instrumentFunction(118);
                if (this.regenerateDisabled()) {
                    bjccovmshb1i9e.instrumentBranch(118, 0, true);
                    bjccovmshb1i9e.instrumentRegion(118, 1);
                    return;
                }
                else {
                    bjccovmshb1i9e.instrumentBranch(118, 0, false);
                }
                bjccovmshb1i9e.instrumentRegion(101, 2);
                this.onRegenerate();
            });
        }, Button);
        Button.pop();
        Row.pop();
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i9e.instrumentFunction(119);
            Text.create(this.gateStatusText());
            Text.fontSize(12);
            Text.fontColor(SettingsTheme.accentColor(this.isDark, this.gateEnabled ? (bjccovmshb1i9e.instrumentBranch(119, 0, true), SettingsAccent.GREEN) : (bjccovmshb1i9e.instrumentBranch(119, 0, false), SettingsAccent.AMBER)));
            Text.width('100%');
        }, Text);
        Text.pop();
        Column.pop();
    }
    rerender() {
        this.updateDirtyElements();
    }
}
export class RemotePermissionCard extends ViewPU {
    constructor(parent, params, __localStorage, elmtId = -1, paramsLambda = undefined, extraInfo) {
        super(parent, __localStorage, elmtId, extraInfo);
        if (typeof paramsLambda === "function") {
            this.paramsGenerator_ = paramsLambda;
        }
        this.__isDark = new SynchedPropertySimpleOneWayPU(params.isDark, this, "isDark");
        this.__targetSection = new SynchedPropertySimpleOneWayPU(params.targetSection, this, "targetSection");
        this.__section = new SynchedPropertySimpleOneWayPU(params.section, this, "section");
        this.__title = new SynchedPropertySimpleOneWayPU(params.title, this, "title");
        this.__description = new SynchedPropertySimpleOneWayPU(params.description, this, "description");
        this.__grantedText = new SynchedPropertySimpleOneWayPU(params.grantedText, this, "grantedText");
        this.__missingText = new SynchedPropertySimpleOneWayPU(params.missingText, this, "missingText");
        this.__actionTextValue = new SynchedPropertySimpleOneWayPU(params.actionTextValue, this, "actionTextValue");
        this.__busyText = new SynchedPropertySimpleOneWayPU(params.busyText, this, "busyText");
        this.__icon = new SynchedPropertyObjectOneWayPU(params.icon, this, "icon");
        this.__accent = new SynchedPropertySimpleOneWayPU(params.accent, this, "accent");
        this.__granted = new SynchedPropertySimpleOneWayPU(params.granted, this, "granted");
        this.__busy = new SynchedPropertySimpleOneWayPU(params.busy, this, "busy");
        this.onRequest = () => {
            bjccovmshb1i9e.instrumentFunction(120);
        };
        this.__hovered = new ObservedPropertySimplePU(false, this, "hovered");
        this.__pressed = new ObservedPropertySimplePU(false, this, "pressed");
        this.setInitiallyProvidedValue(params);
        this.finalizeConstruction();
    }
    setInitiallyProvidedValue(params: RemotePermissionCard_Params) {
        bjccovmshb1i9e.instrumentFunction(121);
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
        if (params.section === undefined) {
            this.__section.set(SettingsRemoteControlSection.SCREEN);
        }
        else {
        }
        if (params.title === undefined) {
            this.__title.set(SettingsText.REMOTE_SCREEN_PERMISSION_TITLE);
        }
        else {
        }
        if (params.description === undefined) {
            this.__description.set(SettingsText.REMOTE_SCREEN_PERMISSION_DESC);
        }
        else {
        }
        if (params.grantedText === undefined) {
            this.__grantedText.set(SettingsText.REMOTE_PERMISSION_GRANTED);
        }
        else {
        }
        if (params.missingText === undefined) {
            this.__missingText.set(SettingsText.REMOTE_PERMISSION_MISSING);
        }
        else {
        }
        if (params.actionTextValue === undefined) {
            this.__actionTextValue.set(SettingsText.REMOTE_PERMISSION_ACTION);
        }
        else {
        }
        if (params.busyText === undefined) {
            this.__busyText.set(SettingsText.REMOTE_PERMISSION_BUSY);
        }
        else {
        }
        if (params.icon === undefined) {
            this.__icon.set(SettingsResources.REMOTE_SCREEN_ICON);
        }
        else {
        }
        if (params.accent === undefined) {
            this.__accent.set(SettingsAccent.CYAN);
        }
        else {
        }
        if (params.granted === undefined) {
            this.__granted.set(false);
        }
        else {
        }
        if (params.busy === undefined) {
            this.__busy.set(false);
        }
        else {
        }
        if (params.onRequest !== undefined) {
            this.onRequest = params.onRequest;
        }
        else {
        }
        if (params.hovered !== undefined) {
            this.hovered = params.hovered;
        }
        else {
        }
        if (params.pressed !== undefined) {
            this.pressed = params.pressed;
        }
        else {
        }
    }
    updateStateVars(params: RemotePermissionCard_Params) {
        bjccovmshb1i9e.instrumentFunction(122);
        this.__isDark.reset(params.isDark);
        this.__targetSection.reset(params.targetSection);
        this.__section.reset(params.section);
        this.__title.reset(params.title);
        this.__description.reset(params.description);
        this.__grantedText.reset(params.grantedText);
        this.__missingText.reset(params.missingText);
        this.__actionTextValue.reset(params.actionTextValue);
        this.__busyText.reset(params.busyText);
        this.__icon.reset(params.icon);
        this.__accent.reset(params.accent);
        this.__granted.reset(params.granted);
        this.__busy.reset(params.busy);
    }
    purgeVariableDependenciesOnElmtId(rmElmtId) {
        this.__isDark.purgeDependencyOnElmtId(rmElmtId);
        this.__targetSection.purgeDependencyOnElmtId(rmElmtId);
        this.__section.purgeDependencyOnElmtId(rmElmtId);
        this.__title.purgeDependencyOnElmtId(rmElmtId);
        this.__description.purgeDependencyOnElmtId(rmElmtId);
        this.__grantedText.purgeDependencyOnElmtId(rmElmtId);
        this.__missingText.purgeDependencyOnElmtId(rmElmtId);
        this.__actionTextValue.purgeDependencyOnElmtId(rmElmtId);
        this.__busyText.purgeDependencyOnElmtId(rmElmtId);
        this.__icon.purgeDependencyOnElmtId(rmElmtId);
        this.__accent.purgeDependencyOnElmtId(rmElmtId);
        this.__granted.purgeDependencyOnElmtId(rmElmtId);
        this.__busy.purgeDependencyOnElmtId(rmElmtId);
        this.__hovered.purgeDependencyOnElmtId(rmElmtId);
        this.__pressed.purgeDependencyOnElmtId(rmElmtId);
    }
    aboutToBeDeleted() {
        this.__isDark.aboutToBeDeleted();
        this.__targetSection.aboutToBeDeleted();
        this.__section.aboutToBeDeleted();
        this.__title.aboutToBeDeleted();
        this.__description.aboutToBeDeleted();
        this.__grantedText.aboutToBeDeleted();
        this.__missingText.aboutToBeDeleted();
        this.__actionTextValue.aboutToBeDeleted();
        this.__busyText.aboutToBeDeleted();
        this.__icon.aboutToBeDeleted();
        this.__accent.aboutToBeDeleted();
        this.__granted.aboutToBeDeleted();
        this.__busy.aboutToBeDeleted();
        this.__hovered.aboutToBeDeleted();
        this.__pressed.aboutToBeDeleted();
        SubscriberManager.Get().delete(this.id__());
        this.aboutToBeDeletedInternal();
    }
    private __isDark: SynchedPropertySimpleOneWayPU<boolean>;
    get isDark() {
        bjccovmshb1i9e.instrumentFunction(123);
        return this.__isDark.get();
    }
    set isDark(newValue: boolean) {
        bjccovmshb1i9e.instrumentFunction(124);
        this.__isDark.set(newValue);
    }
    private __targetSection: SynchedPropertySimpleOneWayPU<string>;
    get targetSection() {
        bjccovmshb1i9e.instrumentFunction(125);
        return this.__targetSection.get();
    }
    set targetSection(newValue: string) {
        bjccovmshb1i9e.instrumentFunction(126);
        this.__targetSection.set(newValue);
    }
    private __section: SynchedPropertySimpleOneWayPU<string>;
    get section() {
        bjccovmshb1i9e.instrumentFunction(127);
        return this.__section.get();
    }
    set section(newValue: string) {
        bjccovmshb1i9e.instrumentFunction(128);
        this.__section.set(newValue);
    }
    private __title: SynchedPropertySimpleOneWayPU<string>;
    get title() {
        bjccovmshb1i9e.instrumentFunction(129);
        return this.__title.get();
    }
    set title(newValue: string) {
        bjccovmshb1i9e.instrumentFunction(130);
        this.__title.set(newValue);
    }
    private __description: SynchedPropertySimpleOneWayPU<string>;
    get description() {
        bjccovmshb1i9e.instrumentFunction(131);
        return this.__description.get();
    }
    set description(newValue: string) {
        bjccovmshb1i9e.instrumentFunction(132);
        this.__description.set(newValue);
    }
    private __grantedText: SynchedPropertySimpleOneWayPU<string>;
    get grantedText() {
        bjccovmshb1i9e.instrumentFunction(133);
        return this.__grantedText.get();
    }
    set grantedText(newValue: string) {
        bjccovmshb1i9e.instrumentFunction(134);
        this.__grantedText.set(newValue);
    }
    private __missingText: SynchedPropertySimpleOneWayPU<string>;
    get missingText() {
        bjccovmshb1i9e.instrumentFunction(135);
        return this.__missingText.get();
    }
    set missingText(newValue: string) {
        bjccovmshb1i9e.instrumentFunction(136);
        this.__missingText.set(newValue);
    }
    private __actionTextValue: SynchedPropertySimpleOneWayPU<string>;
    get actionTextValue() {
        bjccovmshb1i9e.instrumentFunction(137);
        return this.__actionTextValue.get();
    }
    set actionTextValue(newValue: string) {
        bjccovmshb1i9e.instrumentFunction(138);
        this.__actionTextValue.set(newValue);
    }
    private __busyText: SynchedPropertySimpleOneWayPU<string>;
    get busyText() {
        bjccovmshb1i9e.instrumentFunction(139);
        return this.__busyText.get();
    }
    set busyText(newValue: string) {
        bjccovmshb1i9e.instrumentFunction(140);
        this.__busyText.set(newValue);
    }
    private __icon: SynchedPropertySimpleOneWayPU<Resource>;
    get icon() {
        bjccovmshb1i9e.instrumentFunction(141);
        return this.__icon.get();
    }
    set icon(newValue: Resource) {
        bjccovmshb1i9e.instrumentFunction(142);
        this.__icon.set(newValue);
    }
    private __accent: SynchedPropertySimpleOneWayPU<SettingsAccentName>;
    get accent() {
        bjccovmshb1i9e.instrumentFunction(143);
        return this.__accent.get();
    }
    set accent(newValue: SettingsAccentName) {
        bjccovmshb1i9e.instrumentFunction(144);
        this.__accent.set(newValue);
    }
    private __granted: SynchedPropertySimpleOneWayPU<boolean>;
    get granted() {
        bjccovmshb1i9e.instrumentFunction(145);
        return this.__granted.get();
    }
    set granted(newValue: boolean) {
        bjccovmshb1i9e.instrumentFunction(146);
        this.__granted.set(newValue);
    }
    private __busy: SynchedPropertySimpleOneWayPU<boolean>;
    get busy() {
        bjccovmshb1i9e.instrumentFunction(147);
        return this.__busy.get();
    }
    set busy(newValue: boolean) {
        bjccovmshb1i9e.instrumentFunction(148);
        this.__busy.set(newValue);
    }
    private onRequest: () => void;
    private __hovered: ObservedPropertySimplePU<boolean>;
    get hovered() {
        bjccovmshb1i9e.instrumentFunction(150);
        return this.__hovered.get();
    }
    set hovered(newValue: boolean) {
        bjccovmshb1i9e.instrumentFunction(151);
        this.__hovered.set(newValue);
    }
    private __pressed: ObservedPropertySimplePU<boolean>;
    get pressed() {
        bjccovmshb1i9e.instrumentFunction(152);
        return this.__pressed.get();
    }
    set pressed(newValue: boolean) {
        bjccovmshb1i9e.instrumentFunction(153);
        this.__pressed.set(newValue);
    }
    private isTarget(): boolean {
        bjccovmshb1i9e.instrumentFunction(154);
        bjccovmshb1i9e.instrumentRegion(154, 1);
        return this.targetSection === this.section;
    }
    private setHovered(hovered: boolean): void {
        bjccovmshb1i9e.instrumentFunction(155);
        bjccovmshb1i9e.instrumentRegion(155, 1);
        SettingsTheme.animate(this.getUIContext(), () => {
            bjccovmshb1i9e.instrumentFunction(156);
            bjccovmshb1i9e.instrumentRegion(156, 1);
            this.hovered = hovered;
        });
    }
    private statusText(): string {
        bjccovmshb1i9e.instrumentFunction(157);
        if (this.busy) {
            bjccovmshb1i9e.instrumentBranch(157, 0, true);
            bjccovmshb1i9e.instrumentRegion(157, 1);
            return this.busyText;
        }
        else {
            bjccovmshb1i9e.instrumentBranch(157, 0, false);
        }
        bjccovmshb1i9e.instrumentRegion(157, 2);
        return this.granted ? (bjccovmshb1i9e.instrumentBranch(157, 1, true), this.grantedText) : (bjccovmshb1i9e.instrumentBranch(157, 1, false), this.missingText);
    }
    private actionText(): string {
        bjccovmshb1i9e.instrumentFunction(158);
        if (this.busy) {
            bjccovmshb1i9e.instrumentBranch(158, 0, true);
            bjccovmshb1i9e.instrumentRegion(158, 1);
            return this.busyText;
        }
        else {
            bjccovmshb1i9e.instrumentBranch(158, 0, false);
        }
        bjccovmshb1i9e.instrumentRegion(158, 2);
        return this.granted ? (bjccovmshb1i9e.instrumentBranch(158, 1, true), this.grantedText) : (bjccovmshb1i9e.instrumentBranch(158, 1, false), this.actionTextValue);
    }
    private actionDisabled(): boolean {
        bjccovmshb1i9e.instrumentFunction(159);
        bjccovmshb1i9e.instrumentRegion(159, 1);
        return this.busy || this.granted;
    }
    initialRender() {
        bjccovmshb1i9e.instrumentFunction(160);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i9e.instrumentFunction(161);
            Row.create({ space: 14 });
            Row.alignItems(VerticalAlign.Center);
            Row.width('100%');
            Row.padding(16);
            Row.backgroundColor(this.hovered || this.pressed || this.isTarget() ? (bjccovmshb1i9e.instrumentBranch(161, 0, true), SettingsTheme.cardHoverBackground(this.isDark)) : (bjccovmshb1i9e.instrumentBranch(161, 0, false), SettingsTheme.cardBackground(this.isDark)));
            Row.borderRadius(SettingsTheme.CARD_RADIUS);
            Row.border({
                width: 1,
                color: this.isTarget() ? (bjccovmshb1i9e.instrumentBranch(161, 1, true), SettingsTheme.accentColor(this.isDark, this.accent)) : (bjccovmshb1i9e.instrumentBranch(161, 1, false), SettingsTheme.borderColor(this.isDark))
            });
            Row.shadow(SettingsTheme.shadow(this.isDark, this.hovered || this.isTarget()));
            Row.translate({ y: this.hovered ? (bjccovmshb1i9e.instrumentBranch(161, 2, true), -5) : (bjccovmshb1i9e.instrumentBranch(161, 2, false), 0) });
            Row.scale({
                x: this.pressed ? (bjccovmshb1i9e.instrumentBranch(161, 3, true), 0.99) : (bjccovmshb1i9e.instrumentBranch(161, 3, false), this.hovered ? (bjccovmshb1i9e.instrumentBranch(161, 4, true), 1.012) : (bjccovmshb1i9e.instrumentBranch(161, 4, false), 1)),
                y: this.pressed ? (bjccovmshb1i9e.instrumentBranch(161, 5, true), 0.99) : (bjccovmshb1i9e.instrumentBranch(161, 5, false), this.hovered ? (bjccovmshb1i9e.instrumentBranch(161, 6, true), 1.012) : (bjccovmshb1i9e.instrumentBranch(161, 6, false), 1))
            });
            Row.margin({ bottom: 10 });
            Row.onHover((isHover: boolean) => {
                bjccovmshb1i9e.instrumentFunction(162);
                bjccovmshb1i9e.instrumentRegion(162, 1);
                this.setHovered(isHover);
            });
            Row.onTouch((event: TouchEvent) => {
                bjccovmshb1i9e.instrumentFunction(163);
                if (event.type === TouchType.Down) {
                    bjccovmshb1i9e.instrumentBranch(163, 0, true);
                    bjccovmshb1i9e.instrumentRegion(163, 1);
                    this.pressed = true;
                    bjccovmshb1i9e.instrumentRegion(160, 1);
                    return;
                }
                else {
                    bjccovmshb1i9e.instrumentBranch(163, 0, false);
                }
                if (event.type === TouchType.Up || event.type === TouchType.Cancel) {
                    bjccovmshb1i9e.instrumentBranch(163, 1, true);
                    bjccovmshb1i9e.instrumentRegion(163, 2);
                    this.pressed = false;
                }
                else {
                    bjccovmshb1i9e.instrumentBranch(163, 1, false);
                }
            });
        }, Row);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i9e.instrumentFunction(164);
            Column.create();
            Column.width(SettingsTheme.ICON_TILE_SIZE);
            Column.height(SettingsTheme.ICON_TILE_SIZE);
            Column.justifyContent(FlexAlign.Center);
            Column.alignItems(HorizontalAlign.Center);
            Column.borderRadius(SettingsTheme.BUTTON_RADIUS);
            Column.backgroundColor(SettingsTheme.accentBackground(this.isDark, this.accent));
        }, Column);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i9e.instrumentFunction(165);
            Image.create(this.icon);
            Image.width(SettingsTheme.ICON_SIZE);
            Image.height(SettingsTheme.ICON_SIZE);
            Image.fillColor(SettingsTheme.accentColor(this.isDark, this.accent));
            Image.draggable(false);
        }, Image);
        Column.pop();
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i9e.instrumentFunction(166);
            Column.create({ space: 5 });
            Column.layoutWeight(1);
        }, Column);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i9e.instrumentFunction(167);
            Text.create(this.title);
            Text.fontSize(15);
            Text.fontWeight(FontWeight.Medium);
            Text.fontColor(SettingsTheme.primaryText(this.isDark));
            Text.width('100%');
        }, Text);
        Text.pop();
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i9e.instrumentFunction(168);
            Text.create(this.description);
            Text.fontSize(13);
            Text.fontColor(SettingsTheme.mutedText(this.isDark));
            Text.lineHeight(19);
            Text.width('100%');
            Text.maxLines(2);
            Text.textOverflow({ overflow: TextOverflow.Ellipsis });
        }, Text);
        Text.pop();
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i9e.instrumentFunction(169);
            Text.create(this.statusText());
            Text.fontSize(12);
            Text.fontColor(SettingsTheme.accentColor(this.isDark, this.granted ? (bjccovmshb1i9e.instrumentBranch(169, 0, true), SettingsAccent.GREEN) : (bjccovmshb1i9e.instrumentBranch(169, 0, false), SettingsAccent.AMBER)));
            Text.width('100%');
        }, Text);
        Text.pop();
        Column.pop();
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i9e.instrumentFunction(170);
            Button.createWithLabel(this.actionText());
            Button.type(ButtonType.Normal);
            Button.constraintSize({ minHeight: 48 });
            Button.borderRadius(SettingsTheme.BUTTON_RADIUS);
            Button.backgroundColor(this.actionDisabled() ? (bjccovmshb1i9e.instrumentBranch(170, 0, true), SettingsTheme.disabledButton(this.isDark)) : (bjccovmshb1i9e.instrumentBranch(170, 0, false), SettingsTheme.subtleButton(this.isDark, false, false)));
            Button.fontColor(this.actionDisabled() ? (bjccovmshb1i9e.instrumentBranch(170, 1, true), SettingsTheme.disabledText(this.isDark)) : (bjccovmshb1i9e.instrumentBranch(170, 1, false), SettingsTheme.primaryText(this.isDark)));
            Button.fontSize(12);
            Button.stateEffect(false);
            Button.enabled(!this.actionDisabled());
            Button.onClick(() => {
                bjccovmshb1i9e.instrumentFunction(171);
                if (this.actionDisabled()) {
                    bjccovmshb1i9e.instrumentBranch(171, 0, true);
                    bjccovmshb1i9e.instrumentRegion(171, 1);
                    return;
                }
                else {
                    bjccovmshb1i9e.instrumentBranch(171, 0, false);
                }
                bjccovmshb1i9e.instrumentRegion(160, 2);
                this.onRequest();
            });
        }, Button);
        Button.pop();
        Row.pop();
    }
    rerender() {
        this.updateDirtyElements();
    }
}
