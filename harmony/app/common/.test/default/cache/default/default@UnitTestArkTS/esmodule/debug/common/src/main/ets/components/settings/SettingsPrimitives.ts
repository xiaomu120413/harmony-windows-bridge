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
let bjccovmshb1ibh = new BjcCov({ version: "bjc v1.0.0", versionCode: 10000, path: "common/src/main/ets/components/settings/SettingsPrimitives.ets", hash: "9b4aa882205f64af0f689fff4725618f80ce309082f4394b4a0de0111aca49c1", lineCnt: 422, count: 0, projectPath: "", functions: { 0: { name: "anonymous_1", count: 0, regions: { 0: { startLoc: { line: 237, col: 12 }, endLoc: { line: 237, col: 22 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 0 }, 1: { name: "anonymous_2", count: 0, regions: { 0: { startLoc: { line: 187, col: 11 }, endLoc: { line: 187, col: 21 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 1 }, 2: { name: "anonymous_3", count: 0, regions: { 0: { startLoc: { line: 140, col: 11 }, endLoc: { line: 140, col: 21 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 2 }, 3: { name: "anonymous_4", count: 0, regions: { 0: { startLoc: { line: 45, col: 12 }, endLoc: { line: 45, col: 22 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 3 }, 4: { name: "setInitiallyProvidedValue", count: 0, regions: { 0: { startLoc: { line: 7, col: 24 }, endLoc: { line: 9, col: 32 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 4 }, 5: { name: "updateStateVars", count: 0, regions: { 0: { startLoc: { line: 7, col: 9 }, endLoc: { line: 9, col: 15 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 5 }, 6: { name: "text", count: 0, regions: { 0: { startLoc: { line: 7, col: 9 }, endLoc: { line: 7, col: 13 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 6 }, 7: { name: "text", count: 0, regions: { 0: { startLoc: { line: 7, col: 9 }, endLoc: { line: 7, col: 21 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 7 }, 8: { name: "tone", count: 0, regions: { 0: { startLoc: { line: 8, col: 9 }, endLoc: { line: 8, col: 13 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 8 }, 9: { name: "tone", count: 0, regions: { 0: { startLoc: { line: 8, col: 9 }, endLoc: { line: 8, col: 33 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 9 }, 10: { name: "isDark", count: 0, regions: { 0: { startLoc: { line: 9, col: 9 }, endLoc: { line: 9, col: 15 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 10 }, 11: { name: "isDark", count: 0, regions: { 0: { startLoc: { line: 9, col: 9 }, endLoc: { line: 9, col: 24 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 11 }, 12: { name: "initialRender", count: 0, regions: { 0: { startLoc: { line: 11, col: 3 }, endLoc: { line: 35, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 12 }, 13: { name: "anonymous_5", count: 0, regions: { 0: { startLoc: { line: 12, col: 5 }, endLoc: { line: 34, col: 6 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 13 }, 14: { name: "anonymous_6", count: 0, regions: { 0: { startLoc: { line: 13, col: 7 }, endLoc: { line: 17, col: 74 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 14 }, 15: { name: "anonymous_7", count: 0, regions: { 0: { startLoc: { line: 19, col: 7 }, endLoc: { line: 24, col: 58 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 15 }, 16: { name: "anonymous_8", count: 0, regions: { 0: { startLoc: { line: 45, col: 25 }, endLoc: { line: 46, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 16 }, 17: { name: "setInitiallyProvidedValue", count: 0, regions: { 0: { startLoc: { line: 40, col: 25 }, endLoc: { line: 44, col: 34 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 17 }, 18: { name: "updateStateVars", count: 0, regions: { 0: { startLoc: { line: 40, col: 9 }, endLoc: { line: 44, col: 17 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 18 }, 19: { name: "title", count: 0, regions: { 0: { startLoc: { line: 40, col: 9 }, endLoc: { line: 40, col: 14 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 19 }, 20: { name: "title", count: 0, regions: { 0: { startLoc: { line: 40, col: 9 }, endLoc: { line: 40, col: 22 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 20 }, 21: { name: "iconResource", count: 0, regions: { 0: { startLoc: { line: 41, col: 9 }, endLoc: { line: 41, col: 21 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 21 }, 22: { name: "iconResource", count: 0, regions: { 0: { startLoc: { line: 41, col: 9 }, endLoc: { line: 41, col: 31 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 22 }, 23: { name: "accentName", count: 0, regions: { 0: { startLoc: { line: 42, col: 9 }, endLoc: { line: 42, col: 19 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 23 }, 24: { name: "accentName", count: 0, regions: { 0: { startLoc: { line: 42, col: 9 }, endLoc: { line: 42, col: 39 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 24 }, 25: { name: "isDark", count: 0, regions: { 0: { startLoc: { line: 43, col: 9 }, endLoc: { line: 43, col: 15 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 25 }, 26: { name: "isDark", count: 0, regions: { 0: { startLoc: { line: 43, col: 9 }, endLoc: { line: 43, col: 24 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 26 }, 27: { name: "selected", count: 0, regions: { 0: { startLoc: { line: 44, col: 9 }, endLoc: { line: 44, col: 17 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 27 }, 28: { name: "selected", count: 0, regions: { 0: { startLoc: { line: 44, col: 9 }, endLoc: { line: 44, col: 26 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 28 }, 29: { name: "anonymous_9", count: 0, regions: { 0: { startLoc: { line: 45, col: 12 }, endLoc: { line: 45, col: 22 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 29 }, 30: { name: "hovered", count: 0, regions: { 0: { startLoc: { line: 47, col: 18 }, endLoc: { line: 47, col: 25 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 30 }, 31: { name: "hovered", count: 0, regions: { 0: { startLoc: { line: 47, col: 18 }, endLoc: { line: 47, col: 34 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 31 }, 32: { name: "pressed", count: 0, regions: { 0: { startLoc: { line: 48, col: 18 }, endLoc: { line: 48, col: 25 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 32 }, 33: { name: "pressed", count: 0, regions: { 0: { startLoc: { line: 48, col: 18 }, endLoc: { line: 48, col: 34 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 33 }, 34: { name: "setHovered", count: 0, regions: { 0: { startLoc: { line: 50, col: 3 }, endLoc: { line: 52, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 51, col: 5 }, endLoc: { line: 52, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 34 }, 35: { name: "initialRender", count: 0, regions: { 0: { startLoc: { line: 54, col: 3 }, endLoc: { line: 101, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 91, col: 9 }, endLoc: { line: 92, col: 8 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 99, col: 7 }, endLoc: { line: 100, col: 6 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 35 }, 36: { name: "anonymous_10", count: 0, regions: { 0: { startLoc: { line: 55, col: 5 }, endLoc: { line: 100, col: 6 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 36 }, 37: { name: "anonymous_11", count: 0, regions: { 0: { startLoc: { line: 85, col: 14 }, endLoc: { line: 87, col: 6 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 86, col: 7 }, endLoc: { line: 87, col: 6 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 37 }, 38: { name: "anonymous_12", count: 0, regions: { 0: { startLoc: { line: 88, col: 14 }, endLoc: { line: 96, col: 6 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 89, col: 42 }, endLoc: { line: 92, col: 8 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 93, col: 75 }, endLoc: { line: 95, col: 8 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 89, col: 11 }, endLoc: { line: 89, col: 40 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 93, col: 11 }, endLoc: { line: 93, col: 73 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 38 }, 39: { name: "anonymous_13", count: 0, regions: { 0: { startLoc: { line: 97, col: 14 }, endLoc: { line: 100, col: 6 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 39 }, 40: { name: "anonymous_14", count: 0, regions: { 0: { startLoc: { line: 56, col: 7 }, endLoc: { line: 69, col: 69 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 68, col: 24 }, endLoc: { line: 69, col: 69 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 40 }, 41: { name: "anonymous_15", count: 0, regions: { 0: { startLoc: { line: 57, col: 9 }, endLoc: { line: 61, col: 27 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 60, col: 22 }, endLoc: { line: 60, col: 107 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 41 }, 42: { name: "anonymous_16", count: 0, regions: { 0: { startLoc: { line: 71, col: 7 }, endLoc: { line: 77, col: 24 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 73, col: 21 }, endLoc: { line: 73, col: 72 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 42 }, 43: { name: "setInitiallyProvidedValue", count: 0, regions: { 0: { startLoc: { line: 106, col: 25 }, endLoc: { line: 109, col: 45 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 43 }, 44: { name: "updateStateVars", count: 0, regions: { 0: { startLoc: { line: 106, col: 9 }, endLoc: { line: 109, col: 13 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 44 }, 45: { name: "label", count: 0, regions: { 0: { startLoc: { line: 106, col: 9 }, endLoc: { line: 106, col: 14 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 45 }, 46: { name: "label", count: 0, regions: { 0: { startLoc: { line: 106, col: 9 }, endLoc: { line: 106, col: 22 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 46 }, 47: { name: "value", count: 0, regions: { 0: { startLoc: { line: 107, col: 9 }, endLoc: { line: 107, col: 14 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 47 }, 48: { name: "value", count: 0, regions: { 0: { startLoc: { line: 107, col: 9 }, endLoc: { line: 107, col: 22 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 48 }, 49: { name: "isDark", count: 0, regions: { 0: { startLoc: { line: 108, col: 9 }, endLoc: { line: 108, col: 15 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 49 }, 50: { name: "isDark", count: 0, regions: { 0: { startLoc: { line: 108, col: 9 }, endLoc: { line: 108, col: 24 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 50 }, 51: { name: "tone", count: 0, regions: { 0: { startLoc: { line: 109, col: 9 }, endLoc: { line: 109, col: 13 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 51 }, 52: { name: "tone", count: 0, regions: { 0: { startLoc: { line: 109, col: 9 }, endLoc: { line: 109, col: 33 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 52 }, 53: { name: "initialRender", count: 0, regions: { 0: { startLoc: { line: 111, col: 3 }, endLoc: { line: 134, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 53 }, 54: { name: "anonymous_17", count: 0, regions: { 0: { startLoc: { line: 112, col: 5 }, endLoc: { line: 133, col: 35 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 54 }, 55: { name: "anonymous_18", count: 0, regions: { 0: { startLoc: { line: 113, col: 7 }, endLoc: { line: 118, col: 24 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 55 }, 56: { name: "anonymous_19", count: 0, regions: { 0: { startLoc: { line: 120, col: 7 }, endLoc: { line: 128, col: 29 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 123, col: 20 }, endLoc: { line: 124, col: 59 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 56 }, 57: { name: "anonymous_20", count: 0, regions: { 0: { startLoc: { line: 140, col: 24 }, endLoc: { line: 141, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 57 }, 58: { name: "setInitiallyProvidedValue", count: 0, regions: { 0: { startLoc: { line: 139, col: 27 }, endLoc: { line: 139, col: 32 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 58 }, 59: { name: "updateStateVars", count: 0, regions: { 0: { startLoc: { line: 139, col: 9 }, endLoc: { line: 139, col: 15 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 59 }, 60: { name: "isDark", count: 0, regions: { 0: { startLoc: { line: 139, col: 9 }, endLoc: { line: 139, col: 15 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 60 }, 61: { name: "isDark", count: 0, regions: { 0: { startLoc: { line: 139, col: 9 }, endLoc: { line: 139, col: 24 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 61 }, 62: { name: "anonymous_21", count: 0, regions: { 0: { startLoc: { line: 140, col: 11 }, endLoc: { line: 140, col: 21 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 62 }, 63: { name: "hovered", count: 0, regions: { 0: { startLoc: { line: 142, col: 18 }, endLoc: { line: 142, col: 25 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 63 }, 64: { name: "hovered", count: 0, regions: { 0: { startLoc: { line: 142, col: 18 }, endLoc: { line: 142, col: 34 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 64 }, 65: { name: "pressed", count: 0, regions: { 0: { startLoc: { line: 143, col: 18 }, endLoc: { line: 143, col: 25 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 65 }, 66: { name: "pressed", count: 0, regions: { 0: { startLoc: { line: 143, col: 18 }, endLoc: { line: 143, col: 34 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 66 }, 67: { name: "setHovered", count: 0, regions: { 0: { startLoc: { line: 145, col: 3 }, endLoc: { line: 147, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 146, col: 5 }, endLoc: { line: 147, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 67 }, 68: { name: "initialRender", count: 0, regions: { 0: { startLoc: { line: 149, col: 3 }, endLoc: { line: 179, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 169, col: 9 }, endLoc: { line: 170, col: 8 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 177, col: 7 }, endLoc: { line: 178, col: 6 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 68 }, 69: { name: "anonymous_22", count: 0, regions: { 0: { startLoc: { line: 150, col: 5 }, endLoc: { line: 178, col: 6 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 69 }, 70: { name: "anonymous_23", count: 0, regions: { 0: { startLoc: { line: 163, col: 14 }, endLoc: { line: 165, col: 6 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 164, col: 7 }, endLoc: { line: 165, col: 6 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 70 }, 71: { name: "anonymous_24", count: 0, regions: { 0: { startLoc: { line: 166, col: 14 }, endLoc: { line: 174, col: 6 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 167, col: 42 }, endLoc: { line: 170, col: 8 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 171, col: 75 }, endLoc: { line: 173, col: 8 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 167, col: 11 }, endLoc: { line: 167, col: 40 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 171, col: 11 }, endLoc: { line: 171, col: 73 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 71 }, 72: { name: "anonymous_25", count: 0, regions: { 0: { startLoc: { line: 175, col: 14 }, endLoc: { line: 178, col: 6 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 72 }, 73: { name: "anonymous_26", count: 0, regions: { 0: { startLoc: { line: 151, col: 7 }, endLoc: { line: 155, col: 25 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 73 }, 74: { name: "anonymous_27", count: 0, regions: { 0: { startLoc: { line: 187, col: 24 }, endLoc: { line: 188, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 74 }, 75: { name: "setInitiallyProvidedValue", count: 0, regions: { 0: { startLoc: { line: 184, col: 25 }, endLoc: { line: 186, col: 32 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 75 }, 76: { name: "updateStateVars", count: 0, regions: { 0: { startLoc: { line: 184, col: 9 }, endLoc: { line: 186, col: 15 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 76 }, 77: { name: "title", count: 0, regions: { 0: { startLoc: { line: 184, col: 9 }, endLoc: { line: 184, col: 14 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 77 }, 78: { name: "title", count: 0, regions: { 0: { startLoc: { line: 184, col: 9 }, endLoc: { line: 184, col: 22 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 78 }, 79: { name: "subtitle", count: 0, regions: { 0: { startLoc: { line: 185, col: 9 }, endLoc: { line: 185, col: 17 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 79 }, 80: { name: "subtitle", count: 0, regions: { 0: { startLoc: { line: 185, col: 9 }, endLoc: { line: 185, col: 25 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 80 }, 81: { name: "isDark", count: 0, regions: { 0: { startLoc: { line: 186, col: 9 }, endLoc: { line: 186, col: 15 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 81 }, 82: { name: "isDark", count: 0, regions: { 0: { startLoc: { line: 186, col: 9 }, endLoc: { line: 186, col: 24 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 82 }, 83: { name: "anonymous_28", count: 0, regions: { 0: { startLoc: { line: 187, col: 11 }, endLoc: { line: 187, col: 21 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 83 }, 84: { name: "initialRender", count: 0, regions: { 0: { startLoc: { line: 190, col: 3 }, endLoc: { line: 225, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 84 }, 85: { name: "anonymous_29", count: 0, regions: { 0: { startLoc: { line: 191, col: 5 }, endLoc: { line: 224, col: 58 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 85 }, 86: { name: "anonymous_30", count: 0, regions: { 0: { startLoc: { line: 192, col: 7 }, endLoc: { line: 220, col: 36 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 86 }, 87: { name: "anonymous_31", count: 0, regions: { 0: { startLoc: { line: 193, col: 9 }, endLoc: { line: 194, col: 30 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 87 }, 88: { name: "anonymous_33", count: 0, regions: { 0: { startLoc: { line: 198, col: 9 }, endLoc: { line: 217, col: 24 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 88 }, 89: { name: "anonymous_34", count: 0, regions: { 0: { startLoc: { line: 199, col: 11 }, endLoc: { line: 205, col: 26 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 89 }, 90: { name: "anonymous_35", count: 0, regions: { 0: { startLoc: { line: 207, col: 11 }, endLoc: { line: 214, col: 12 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 207, col: 41 }, endLoc: { line: 214, col: 12 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 207, col: 15 }, endLoc: { line: 207, col: 39 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 90 }, 91: { name: "anonymous_36", count: 0, regions: { 0: { startLoc: { line: 208, col: 13 }, endLoc: { line: 208, col: 17 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 91 }, 92: { name: "anonymous_37", count: 0, regions: { 0: { startLoc: { line: 208, col: 13 }, endLoc: { line: 213, col: 28 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 92 }, 93: { name: "anonymous_39", count: 0, regions: { 0: { startLoc: { line: 237, col: 25 }, endLoc: { line: 238, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 93 }, 94: { name: "setInitiallyProvidedValue", count: 0, regions: { 0: { startLoc: { line: 230, col: 25 }, endLoc: { line: 236, col: 34 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 94 }, 95: { name: "updateStateVars", count: 0, regions: { 0: { startLoc: { line: 230, col: 9 }, endLoc: { line: 236, col: 17 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 95 }, 96: { name: "title", count: 0, regions: { 0: { startLoc: { line: 230, col: 9 }, endLoc: { line: 230, col: 14 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 96 }, 97: { name: "title", count: 0, regions: { 0: { startLoc: { line: 230, col: 9 }, endLoc: { line: 230, col: 22 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 97 }, 98: { name: "description", count: 0, regions: { 0: { startLoc: { line: 231, col: 9 }, endLoc: { line: 231, col: 20 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 98 }, 99: { name: "description", count: 0, regions: { 0: { startLoc: { line: 231, col: 9 }, endLoc: { line: 231, col: 28 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 99 }, 100: { name: "iconResource", count: 0, regions: { 0: { startLoc: { line: 232, col: 9 }, endLoc: { line: 232, col: 21 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 100 }, 101: { name: "iconResource", count: 0, regions: { 0: { startLoc: { line: 232, col: 9 }, endLoc: { line: 232, col: 31 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 101 }, 102: { name: "accentName", count: 0, regions: { 0: { startLoc: { line: 233, col: 9 }, endLoc: { line: 233, col: 19 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 102 }, 103: { name: "accentName", count: 0, regions: { 0: { startLoc: { line: 233, col: 9 }, endLoc: { line: 233, col: 39 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 103 }, 104: { name: "value", count: 0, regions: { 0: { startLoc: { line: 234, col: 9 }, endLoc: { line: 234, col: 14 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 104 }, 105: { name: "value", count: 0, regions: { 0: { startLoc: { line: 234, col: 9 }, endLoc: { line: 234, col: 22 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 105 }, 106: { name: "isDark", count: 0, regions: { 0: { startLoc: { line: 235, col: 9 }, endLoc: { line: 235, col: 15 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 106 }, 107: { name: "isDark", count: 0, regions: { 0: { startLoc: { line: 235, col: 9 }, endLoc: { line: 235, col: 24 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 107 }, 108: { name: "selected", count: 0, regions: { 0: { startLoc: { line: 236, col: 9 }, endLoc: { line: 236, col: 17 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 108 }, 109: { name: "selected", count: 0, regions: { 0: { startLoc: { line: 236, col: 9 }, endLoc: { line: 236, col: 26 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 109 }, 110: { name: "anonymous_40", count: 0, regions: { 0: { startLoc: { line: 237, col: 12 }, endLoc: { line: 237, col: 22 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 110 }, 111: { name: "hovered", count: 0, regions: { 0: { startLoc: { line: 239, col: 18 }, endLoc: { line: 239, col: 25 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 111 }, 112: { name: "hovered", count: 0, regions: { 0: { startLoc: { line: 239, col: 18 }, endLoc: { line: 239, col: 34 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 112 }, 113: { name: "pressed", count: 0, regions: { 0: { startLoc: { line: 240, col: 18 }, endLoc: { line: 240, col: 25 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 113 }, 114: { name: "pressed", count: 0, regions: { 0: { startLoc: { line: 240, col: 18 }, endLoc: { line: 240, col: 34 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 114 }, 115: { name: "setHovered", count: 0, regions: { 0: { startLoc: { line: 242, col: 3 }, endLoc: { line: 246, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 243, col: 5 }, endLoc: { line: 246, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 115 }, 116: { name: "anonymous_41", count: 0, regions: { 0: { startLoc: { line: 243, col: 48 }, endLoc: { line: 245, col: 6 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 244, col: 7 }, endLoc: { line: 245, col: 6 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 116 }, 117: { name: "initialRender", count: 0, regions: { 0: { startLoc: { line: 248, col: 3 }, endLoc: { line: 333, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 323, col: 9 }, endLoc: { line: 324, col: 8 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 331, col: 7 }, endLoc: { line: 332, col: 6 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 117 }, 118: { name: "anonymous_42", count: 0, regions: { 0: { startLoc: { line: 249, col: 5 }, endLoc: { line: 332, col: 6 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 302, col: 22 }, endLoc: { line: 303, col: 48 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 307, col: 14 }, endLoc: { line: 308, col: 47 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 2: { startLoc: { line: 311, col: 21 }, endLoc: { line: 311, col: 42 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 3: { startLoc: { line: 313, col: 10 }, endLoc: { line: 313, col: 56 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 4: { startLoc: { line: 313, col: 32 }, endLoc: { line: 313, col: 56 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 5: { startLoc: { line: 314, col: 10 }, endLoc: { line: 314, col: 56 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 6: { startLoc: { line: 314, col: 32 }, endLoc: { line: 314, col: 56 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 118 }, 119: { name: "anonymous_43", count: 0, regions: { 0: { startLoc: { line: 317, col: 14 }, endLoc: { line: 319, col: 6 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 318, col: 7 }, endLoc: { line: 319, col: 6 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 119 }, 120: { name: "anonymous_44", count: 0, regions: { 0: { startLoc: { line: 320, col: 14 }, endLoc: { line: 328, col: 6 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 321, col: 42 }, endLoc: { line: 324, col: 8 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 325, col: 75 }, endLoc: { line: 327, col: 8 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 321, col: 11 }, endLoc: { line: 321, col: 40 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 325, col: 11 }, endLoc: { line: 325, col: 73 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 120 }, 121: { name: "anonymous_45", count: 0, regions: { 0: { startLoc: { line: 329, col: 14 }, endLoc: { line: 332, col: 6 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 121 }, 122: { name: "anonymous_46", count: 0, regions: { 0: { startLoc: { line: 250, col: 7 }, endLoc: { line: 264, col: 72 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 262, col: 24 }, endLoc: { line: 264, col: 72 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 263, col: 10 }, endLoc: { line: 264, col: 71 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 122 }, 123: { name: "anonymous_47", count: 0, regions: { 0: { startLoc: { line: 251, col: 9 }, endLoc: { line: 255, col: 27 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 254, col: 22 }, endLoc: { line: 254, col: 107 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 123 }, 124: { name: "anonymous_48", count: 0, regions: { 0: { startLoc: { line: 266, col: 7 }, endLoc: { line: 283, col: 22 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 124 }, 125: { name: "anonymous_49", count: 0, regions: { 0: { startLoc: { line: 267, col: 9 }, endLoc: { line: 273, col: 24 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 269, col: 23 }, endLoc: { line: 269, col: 74 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 125 }, 126: { name: "anonymous_50", count: 0, regions: { 0: { startLoc: { line: 274, col: 9 }, endLoc: { line: 280, col: 24 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 126 }, 127: { name: "anonymous_51", count: 0, regions: { 0: { startLoc: { line: 285, col: 7 }, endLoc: { line: 297, col: 8 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 285, col: 26 }, endLoc: { line: 292, col: 8 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 292, col: 14 }, endLoc: { line: 297, col: 8 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 292, col: 41 }, endLoc: { line: 297, col: 8 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 285, col: 11 }, endLoc: { line: 285, col: 24 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 292, col: 18 }, endLoc: { line: 292, col: 39 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 127 }, 128: { name: "anonymous_52", count: 0, regions: { 0: { startLoc: { line: 286, col: 9 }, endLoc: { line: 291, col: 27 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 128 }, 129: { name: "anonymous_53", count: 0, regions: { 0: { startLoc: { line: 286, col: 9 }, endLoc: { line: 291, col: 27 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 129 }, 130: { name: "anonymous_54", count: 0, regions: { 0: { startLoc: { line: 293, col: 9 }, endLoc: { line: 293, col: 13 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 130 }, 131: { name: "anonymous_55", count: 0, regions: { 0: { startLoc: { line: 293, col: 9 }, endLoc: { line: 296, col: 30 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 131 }, 132: { name: "setInitiallyProvidedValue", count: 0, regions: { 0: { startLoc: { line: 338, col: 25 }, endLoc: { line: 340, col: 32 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 132 }, 133: { name: "updateStateVars", count: 0, regions: { 0: { startLoc: { line: 338, col: 9 }, endLoc: { line: 340, col: 15 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 133 }, 134: { name: "title", count: 0, regions: { 0: { startLoc: { line: 338, col: 9 }, endLoc: { line: 338, col: 14 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 134 }, 135: { name: "title", count: 0, regions: { 0: { startLoc: { line: 338, col: 9 }, endLoc: { line: 338, col: 22 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 135 }, 136: { name: "subtitle", count: 0, regions: { 0: { startLoc: { line: 339, col: 9 }, endLoc: { line: 339, col: 17 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 136 }, 137: { name: "subtitle", count: 0, regions: { 0: { startLoc: { line: 339, col: 9 }, endLoc: { line: 339, col: 25 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 137 }, 138: { name: "isDark", count: 0, regions: { 0: { startLoc: { line: 340, col: 9 }, endLoc: { line: 340, col: 15 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 138 }, 139: { name: "isDark", count: 0, regions: { 0: { startLoc: { line: 340, col: 9 }, endLoc: { line: 340, col: 24 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 139 }, 140: { name: "initialRender", count: 0, regions: { 0: { startLoc: { line: 342, col: 3 }, endLoc: { line: 360, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 140 }, 141: { name: "anonymous_57", count: 0, regions: { 0: { startLoc: { line: 343, col: 5 }, endLoc: { line: 359, col: 35 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 141 }, 142: { name: "anonymous_58", count: 0, regions: { 0: { startLoc: { line: 344, col: 7 }, endLoc: { line: 348, col: 22 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 142 }, 143: { name: "anonymous_59", count: 0, regions: { 0: { startLoc: { line: 349, col: 7 }, endLoc: { line: 355, col: 8 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 349, col: 37 }, endLoc: { line: 355, col: 8 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 349, col: 11 }, endLoc: { line: 349, col: 35 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 143 }, 144: { name: "anonymous_60", count: 0, regions: { 0: { startLoc: { line: 350, col: 9 }, endLoc: { line: 350, col: 13 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 144 }, 145: { name: "anonymous_61", count: 0, regions: { 0: { startLoc: { line: 350, col: 9 }, endLoc: { line: 354, col: 25 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 145 }, 146: { name: "setInitiallyProvidedValue", count: 0, regions: { 0: { startLoc: { line: 365, col: 25 }, endLoc: { line: 367, col: 32 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 146 }, 147: { name: "updateStateVars", count: 0, regions: { 0: { startLoc: { line: 365, col: 9 }, endLoc: { line: 367, col: 15 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 147 }, 148: { name: "title", count: 0, regions: { 0: { startLoc: { line: 365, col: 9 }, endLoc: { line: 365, col: 14 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 148 }, 149: { name: "title", count: 0, regions: { 0: { startLoc: { line: 365, col: 9 }, endLoc: { line: 365, col: 22 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 149 }, 150: { name: "body", count: 0, regions: { 0: { startLoc: { line: 366, col: 9 }, endLoc: { line: 366, col: 13 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 150 }, 151: { name: "body", count: 0, regions: { 0: { startLoc: { line: 366, col: 9 }, endLoc: { line: 366, col: 21 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 151 }, 152: { name: "isDark", count: 0, regions: { 0: { startLoc: { line: 367, col: 9 }, endLoc: { line: 367, col: 15 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 152 }, 153: { name: "isDark", count: 0, regions: { 0: { startLoc: { line: 367, col: 9 }, endLoc: { line: 367, col: 24 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 153 }, 154: { name: "hovered", count: 0, regions: { 0: { startLoc: { line: 368, col: 18 }, endLoc: { line: 368, col: 25 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 154 }, 155: { name: "hovered", count: 0, regions: { 0: { startLoc: { line: 368, col: 18 }, endLoc: { line: 368, col: 34 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 155 }, 156: { name: "pressed", count: 0, regions: { 0: { startLoc: { line: 369, col: 18 }, endLoc: { line: 369, col: 25 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 156 }, 157: { name: "pressed", count: 0, regions: { 0: { startLoc: { line: 369, col: 18 }, endLoc: { line: 369, col: 34 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 157 }, 158: { name: "setHovered", count: 0, regions: { 0: { startLoc: { line: 371, col: 3 }, endLoc: { line: 375, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 372, col: 5 }, endLoc: { line: 375, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 158 }, 159: { name: "anonymous_63", count: 0, regions: { 0: { startLoc: { line: 372, col: 48 }, endLoc: { line: 374, col: 6 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 373, col: 7 }, endLoc: { line: 374, col: 6 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 159 }, 160: { name: "initialRender", count: 0, regions: { 0: { startLoc: { line: 377, col: 3 }, endLoc: { line: 420, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 414, col: 9 }, endLoc: { line: 415, col: 8 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 160 }, 161: { name: "anonymous_64", count: 0, regions: { 0: { startLoc: { line: 378, col: 5 }, endLoc: { line: 419, col: 6 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 394, col: 22 }, endLoc: { line: 395, col: 48 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 402, col: 21 }, endLoc: { line: 402, col: 42 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 2: { startLoc: { line: 404, col: 10 }, endLoc: { line: 404, col: 56 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 3: { startLoc: { line: 404, col: 32 }, endLoc: { line: 404, col: 56 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 4: { startLoc: { line: 405, col: 10 }, endLoc: { line: 405, col: 56 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 5: { startLoc: { line: 405, col: 32 }, endLoc: { line: 405, col: 56 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 161 }, 162: { name: "anonymous_65", count: 0, regions: { 0: { startLoc: { line: 408, col: 14 }, endLoc: { line: 410, col: 6 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 409, col: 7 }, endLoc: { line: 410, col: 6 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 162 }, 163: { name: "anonymous_66", count: 0, regions: { 0: { startLoc: { line: 411, col: 14 }, endLoc: { line: 419, col: 6 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 412, col: 42 }, endLoc: { line: 415, col: 8 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 416, col: 75 }, endLoc: { line: 418, col: 8 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 412, col: 11 }, endLoc: { line: 412, col: 40 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 416, col: 11 }, endLoc: { line: 416, col: 73 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 163 }, 164: { name: "anonymous_67", count: 0, regions: { 0: { startLoc: { line: 379, col: 7 }, endLoc: { line: 384, col: 30 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 164 }, 165: { name: "anonymous_68", count: 0, regions: { 0: { startLoc: { line: 385, col: 7 }, endLoc: { line: 389, col: 22 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 165 } }, exeLine: { 0: 1, 1: 2, 2: 3, 3: 6, 4: 7, 5: 8, 6: 9, 7: 11, 8: 12, 9: 13, 10: 14, 11: 15, 12: 16, 13: 17, 14: 19, 15: 20, 16: 21, 17: 22, 18: 23, 19: 24, 20: 26, 21: 27, 22: 28, 23: 29, 24: 30, 25: 31, 26: 32, 27: 33, 28: 39, 29: 40, 30: 41, 31: 42, 32: 43, 33: 44, 34: 45, 35: 47, 36: 48, 37: 50, 38: 51, 39: 54, 40: 55, 41: 56, 42: 57, 43: 58, 44: 59, 45: 60, 46: 61, 47: 63, 48: 64, 49: 65, 50: 66, 51: 67, 52: 68, 53: 69, 54: 71, 55: 72, 56: 73, 57: 74, 58: 75, 59: 76, 60: 77, 61: 79, 62: 80, 63: 81, 64: 82, 65: 83, 66: 84, 67: 85, 68: 86, 69: 88, 70: 89, 71: 90, 72: 91, 73: 93, 74: 94, 75: 97, 76: 98, 77: 99, 78: 105, 79: 106, 80: 107, 81: 108, 82: 109, 83: 111, 84: 112, 85: 113, 86: 114, 87: 115, 88: 116, 89: 117, 90: 118, 91: 120, 92: 121, 93: 122, 94: 123, 95: 124, 96: 125, 97: 126, 98: 127, 99: 128, 100: 130, 101: 131, 102: 132, 103: 133, 104: 138, 105: 139, 106: 140, 107: 142, 108: 143, 109: 145, 110: 146, 111: 149, 112: 150, 113: 151, 114: 152, 115: 153, 116: 154, 117: 155, 118: 157, 119: 158, 120: 159, 121: 160, 122: 161, 123: 162, 124: 163, 125: 164, 126: 166, 127: 167, 128: 168, 129: 169, 130: 171, 131: 172, 132: 175, 133: 176, 134: 177, 135: 183, 136: 184, 137: 185, 138: 186, 139: 187, 140: 190, 141: 191, 142: 192, 143: 193, 144: 194, 145: 195, 146: 198, 147: 199, 148: 200, 149: 201, 150: 202, 151: 203, 152: 204, 153: 205, 154: 207, 155: 208, 156: 209, 157: 210, 158: 211, 159: 212, 160: 213, 161: 216, 162: 217, 163: 219, 164: 220, 165: 222, 166: 223, 167: 224, 168: 229, 169: 230, 170: 231, 171: 232, 172: 233, 173: 234, 174: 235, 175: 236, 176: 237, 177: 239, 178: 240, 179: 242, 180: 243, 181: 244, 182: 248, 183: 249, 184: 250, 185: 251, 186: 252, 187: 253, 188: 254, 189: 255, 190: 257, 191: 258, 192: 259, 193: 260, 194: 261, 195: 262, 196: 263, 197: 264, 198: 266, 199: 267, 200: 268, 201: 269, 202: 270, 203: 271, 204: 272, 205: 273, 206: 274, 207: 275, 208: 276, 209: 277, 210: 278, 211: 279, 212: 280, 213: 282, 214: 283, 215: 285, 216: 286, 217: 287, 218: 288, 219: 289, 220: 290, 221: 291, 222: 292, 223: 293, 224: 294, 225: 295, 226: 296, 227: 299, 228: 300, 229: 301, 230: 302, 231: 303, 232: 304, 233: 305, 234: 306, 235: 307, 236: 308, 237: 310, 238: 311, 239: 312, 240: 313, 241: 314, 242: 316, 243: 317, 244: 318, 245: 320, 246: 321, 247: 322, 248: 323, 249: 325, 250: 326, 251: 329, 252: 330, 253: 331, 254: 337, 255: 338, 256: 339, 257: 340, 258: 342, 259: 343, 260: 344, 261: 345, 262: 346, 263: 347, 264: 348, 265: 349, 266: 350, 267: 351, 268: 352, 269: 353, 270: 354, 271: 357, 272: 358, 273: 359, 274: 364, 275: 365, 276: 366, 277: 367, 278: 368, 279: 369, 280: 371, 281: 372, 282: 373, 283: 377, 284: 378, 285: 379, 286: 380, 287: 381, 288: 382, 289: 383, 290: 384, 291: 385, 292: 386, 293: 387, 294: 388, 295: 389, 296: 391, 297: 392, 298: 393, 299: 394, 300: 395, 301: 396, 302: 397, 303: 398, 304: 399, 305: 401, 306: 402, 307: 403, 308: 404, 309: 405, 310: 407, 311: 408, 312: 409, 313: 411, 314: 412, 315: 413, 316: 414, 317: 416, 318: 417 } });
if (!("finalizeConstruction" in ViewPU.prototype)) {
    Reflect.set(ViewPU.prototype, "finalizeConstruction", () => { });
}
interface SettingsInfoCard_Params {
    title?: string;
    body?: string;
    isDark?: boolean;
    hovered?: boolean;
    pressed?: boolean;
}
interface SettingsSectionTitle_Params {
    title?: string;
    subtitle?: string;
    isDark?: boolean;
}
interface SettingsListItem_Params {
    title?: string;
    description?: string;
    iconResource?: Resource;
    accentName?: SettingsAccentName;
    value?: string;
    isDark?: boolean;
    selected?: boolean;
    onPress?: () => void;
    hovered?: boolean;
    pressed?: boolean;
}
interface SettingsPageHeader_Params {
    title?: string;
    subtitle?: string;
    isDark?: boolean;
    onBack?: () => void;
}
interface SettingsBackButton_Params {
    isDark?: boolean;
    onBack?: () => void;
    hovered?: boolean;
    pressed?: boolean;
}
interface SettingsKeyValueRow_Params {
    label?: string;
    value?: string;
    isDark?: boolean;
    tone?: SettingsStatusTone;
}
interface SettingsDesktopNavItem_Params {
    title?: string;
    iconResource?: Resource;
    accentName?: SettingsAccentName;
    isDark?: boolean;
    selected?: boolean;
    onPress?: () => void;
    hovered?: boolean;
    pressed?: boolean;
}
interface SettingsStatusChip_Params {
    text?: string;
    tone?: SettingsStatusTone;
    isDark?: boolean;
}
import { SettingsAccent, SettingsResources } from "@normalized:N&&&common/src/main/ets/components/settings/SettingsResources&";
import { SettingsTheme } from "@normalized:N&&&common/src/main/ets/components/settings/SettingsTheme&";
import type { SettingsAccentName, SettingsStatusTone } from './SettingsTypes';
export class SettingsStatusChip extends ViewPU {
    constructor(parent, params, __localStorage, elmtId = -1, paramsLambda = undefined, extraInfo) {
        super(parent, __localStorage, elmtId, extraInfo);
        if (typeof paramsLambda === "function") {
            this.paramsGenerator_ = paramsLambda;
        }
        this.__text = new SynchedPropertySimpleOneWayPU(params.text, this, "text");
        this.__tone = new SynchedPropertySimpleOneWayPU(params.tone, this, "tone");
        this.__isDark = new SynchedPropertySimpleOneWayPU(params.isDark, this, "isDark");
        this.setInitiallyProvidedValue(params);
        this.finalizeConstruction();
    }
    setInitiallyProvidedValue(params: SettingsStatusChip_Params) {
        bjccovmshb1ibh.instrumentFunction(4);
        if (params.text === undefined) {
            this.__text.set('');
        }
        else {
        }
        if (params.tone === undefined) {
            this.__tone.set('neutral');
        }
        else {
        }
        if (params.isDark === undefined) {
            this.__isDark.set(false);
        }
        else {
        }
    }
    updateStateVars(params: SettingsStatusChip_Params) {
        bjccovmshb1ibh.instrumentFunction(5);
        this.__text.reset(params.text);
        this.__tone.reset(params.tone);
        this.__isDark.reset(params.isDark);
    }
    purgeVariableDependenciesOnElmtId(rmElmtId) {
        this.__text.purgeDependencyOnElmtId(rmElmtId);
        this.__tone.purgeDependencyOnElmtId(rmElmtId);
        this.__isDark.purgeDependencyOnElmtId(rmElmtId);
    }
    aboutToBeDeleted() {
        this.__text.aboutToBeDeleted();
        this.__tone.aboutToBeDeleted();
        this.__isDark.aboutToBeDeleted();
        SubscriberManager.Get().delete(this.id__());
        this.aboutToBeDeletedInternal();
    }
    private __text: SynchedPropertySimpleOneWayPU<string>;
    get text() {
        bjccovmshb1ibh.instrumentFunction(6);
        return this.__text.get();
    }
    set text(newValue: string) {
        bjccovmshb1ibh.instrumentFunction(7);
        this.__text.set(newValue);
    }
    private __tone: SynchedPropertySimpleOneWayPU<SettingsStatusTone>;
    get tone() {
        bjccovmshb1ibh.instrumentFunction(8);
        return this.__tone.get();
    }
    set tone(newValue: SettingsStatusTone) {
        bjccovmshb1ibh.instrumentFunction(9);
        this.__tone.set(newValue);
    }
    private __isDark: SynchedPropertySimpleOneWayPU<boolean>;
    get isDark() {
        bjccovmshb1ibh.instrumentFunction(10);
        return this.__isDark.get();
    }
    set isDark(newValue: boolean) {
        bjccovmshb1ibh.instrumentFunction(11);
        this.__isDark.set(newValue);
    }
    initialRender() {
        bjccovmshb1ibh.instrumentFunction(12);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1ibh.instrumentFunction(13);
            Row.create({ space: 6 });
            Row.constraintSize({ minHeight: 28 });
            Row.padding({ left: 10, right: 10, top: 4, bottom: 4 });
            Row.alignItems(VerticalAlign.Center);
            Row.backgroundColor(SettingsTheme.statusBackground(this.isDark, this.tone));
            Row.borderRadius(14);
            Row.border({
                width: 1,
                color: SettingsTheme.statusBorder(this.isDark, this.tone)
            });
        }, Row);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1ibh.instrumentFunction(14);
            Column.create();
            Column.width(7);
            Column.height(7);
            Column.borderRadius(3.5);
            Column.backgroundColor(SettingsTheme.statusText(this.isDark, this.tone));
        }, Column);
        Column.pop();
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1ibh.instrumentFunction(15);
            Text.create(this.text);
            Text.fontSize(11);
            Text.fontWeight(FontWeight.Bold);
            Text.fontColor(SettingsTheme.statusText(this.isDark, this.tone));
            Text.maxLines(2);
            Text.textOverflow({ overflow: TextOverflow.Ellipsis });
        }, Text);
        Text.pop();
        Row.pop();
    }
    rerender() {
        this.updateDirtyElements();
    }
}
export class SettingsDesktopNavItem extends ViewPU {
    constructor(parent, params, __localStorage, elmtId = -1, paramsLambda = undefined, extraInfo) {
        super(parent, __localStorage, elmtId, extraInfo);
        if (typeof paramsLambda === "function") {
            this.paramsGenerator_ = paramsLambda;
        }
        this.__title = new SynchedPropertySimpleOneWayPU(params.title, this, "title");
        this.__iconResource = new SynchedPropertyObjectOneWayPU(params.iconResource, this, "iconResource");
        this.__accentName = new SynchedPropertySimpleOneWayPU(params.accentName, this, "accentName");
        this.__isDark = new SynchedPropertySimpleOneWayPU(params.isDark, this, "isDark");
        this.__selected = new SynchedPropertySimpleOneWayPU(params.selected, this, "selected");
        this.onPress = () => {
            bjccovmshb1ibh.instrumentFunction(16);
        };
        this.__hovered = new ObservedPropertySimplePU(false, this, "hovered");
        this.__pressed = new ObservedPropertySimplePU(false, this, "pressed");
        this.setInitiallyProvidedValue(params);
        this.finalizeConstruction();
    }
    setInitiallyProvidedValue(params: SettingsDesktopNavItem_Params) {
        bjccovmshb1ibh.instrumentFunction(17);
        if (params.title === undefined) {
            this.__title.set('');
        }
        else {
        }
        if (params.iconResource === undefined) {
            this.__iconResource.set(SettingsResources.SETTINGS_ICON);
        }
        else {
        }
        if (params.accentName === undefined) {
            this.__accentName.set(SettingsAccent.BLUE);
        }
        else {
        }
        if (params.isDark === undefined) {
            this.__isDark.set(false);
        }
        else {
        }
        if (params.selected === undefined) {
            this.__selected.set(false);
        }
        else {
        }
        if (params.onPress !== undefined) {
            this.onPress = params.onPress;
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
    updateStateVars(params: SettingsDesktopNavItem_Params) {
        bjccovmshb1ibh.instrumentFunction(18);
        this.__title.reset(params.title);
        this.__iconResource.reset(params.iconResource);
        this.__accentName.reset(params.accentName);
        this.__isDark.reset(params.isDark);
        this.__selected.reset(params.selected);
    }
    purgeVariableDependenciesOnElmtId(rmElmtId) {
        this.__title.purgeDependencyOnElmtId(rmElmtId);
        this.__iconResource.purgeDependencyOnElmtId(rmElmtId);
        this.__accentName.purgeDependencyOnElmtId(rmElmtId);
        this.__isDark.purgeDependencyOnElmtId(rmElmtId);
        this.__selected.purgeDependencyOnElmtId(rmElmtId);
        this.__hovered.purgeDependencyOnElmtId(rmElmtId);
        this.__pressed.purgeDependencyOnElmtId(rmElmtId);
    }
    aboutToBeDeleted() {
        this.__title.aboutToBeDeleted();
        this.__iconResource.aboutToBeDeleted();
        this.__accentName.aboutToBeDeleted();
        this.__isDark.aboutToBeDeleted();
        this.__selected.aboutToBeDeleted();
        this.__hovered.aboutToBeDeleted();
        this.__pressed.aboutToBeDeleted();
        SubscriberManager.Get().delete(this.id__());
        this.aboutToBeDeletedInternal();
    }
    private __title: SynchedPropertySimpleOneWayPU<string>;
    get title() {
        bjccovmshb1ibh.instrumentFunction(19);
        return this.__title.get();
    }
    set title(newValue: string) {
        bjccovmshb1ibh.instrumentFunction(20);
        this.__title.set(newValue);
    }
    private __iconResource: SynchedPropertySimpleOneWayPU<Resource>;
    get iconResource() {
        bjccovmshb1ibh.instrumentFunction(21);
        return this.__iconResource.get();
    }
    set iconResource(newValue: Resource) {
        bjccovmshb1ibh.instrumentFunction(22);
        this.__iconResource.set(newValue);
    }
    private __accentName: SynchedPropertySimpleOneWayPU<SettingsAccentName>;
    get accentName() {
        bjccovmshb1ibh.instrumentFunction(23);
        return this.__accentName.get();
    }
    set accentName(newValue: SettingsAccentName) {
        bjccovmshb1ibh.instrumentFunction(24);
        this.__accentName.set(newValue);
    }
    private __isDark: SynchedPropertySimpleOneWayPU<boolean>;
    get isDark() {
        bjccovmshb1ibh.instrumentFunction(25);
        return this.__isDark.get();
    }
    set isDark(newValue: boolean) {
        bjccovmshb1ibh.instrumentFunction(26);
        this.__isDark.set(newValue);
    }
    private __selected: SynchedPropertySimpleOneWayPU<boolean>;
    get selected() {
        bjccovmshb1ibh.instrumentFunction(27);
        return this.__selected.get();
    }
    set selected(newValue: boolean) {
        bjccovmshb1ibh.instrumentFunction(28);
        this.__selected.set(newValue);
    }
    private onPress: () => void;
    private __hovered: ObservedPropertySimplePU<boolean>;
    get hovered() {
        bjccovmshb1ibh.instrumentFunction(30);
        return this.__hovered.get();
    }
    set hovered(newValue: boolean) {
        bjccovmshb1ibh.instrumentFunction(31);
        this.__hovered.set(newValue);
    }
    private __pressed: ObservedPropertySimplePU<boolean>;
    get pressed() {
        bjccovmshb1ibh.instrumentFunction(32);
        return this.__pressed.get();
    }
    set pressed(newValue: boolean) {
        bjccovmshb1ibh.instrumentFunction(33);
        this.__pressed.set(newValue);
    }
    private setHovered(hovered: boolean): void {
        bjccovmshb1ibh.instrumentFunction(34);
        bjccovmshb1ibh.instrumentRegion(34, 1);
        this.hovered = hovered;
    }
    initialRender() {
        bjccovmshb1ibh.instrumentFunction(35);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1ibh.instrumentFunction(36);
            Row.create({ space: 10 });
            Row.alignItems(VerticalAlign.Center);
            Row.width('100%');
            Row.constraintSize({ minHeight: 48 });
            Row.padding({ left: 10, right: 10 });
            Row.backgroundColor(SettingsTheme.transparentButton(this.selected || this.hovered, this.pressed, this.isDark));
            Row.borderRadius(12);
            Row.onHover((isHover: boolean) => {
                bjccovmshb1ibh.instrumentFunction(37);
                bjccovmshb1ibh.instrumentRegion(37, 1);
                this.setHovered(isHover);
            });
            Row.onTouch((event: TouchEvent) => {
                bjccovmshb1ibh.instrumentFunction(38);
                if (event.type === TouchType.Down) {
                    bjccovmshb1ibh.instrumentBranch(38, 0, true);
                    bjccovmshb1ibh.instrumentRegion(38, 1);
                    this.pressed = true;
                    bjccovmshb1ibh.instrumentRegion(35, 1);
                    return;
                }
                else {
                    bjccovmshb1ibh.instrumentBranch(38, 0, false);
                }
                if (event.type === TouchType.Up || event.type === TouchType.Cancel) {
                    bjccovmshb1ibh.instrumentBranch(38, 1, true);
                    bjccovmshb1ibh.instrumentRegion(38, 2);
                    this.pressed = false;
                }
                else {
                    bjccovmshb1ibh.instrumentBranch(38, 1, false);
                }
            });
            Row.onClick(() => {
                bjccovmshb1ibh.instrumentFunction(39);
                this.pressed = false;
                bjccovmshb1ibh.instrumentRegion(35, 2);
                this.onPress();
            });
        }, Row);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1ibh.instrumentFunction(40);
            Column.create();
            Column.width(32);
            Column.height(32);
            Column.justifyContent(FlexAlign.Center);
            Column.alignItems(HorizontalAlign.Center);
            Column.borderRadius(10);
            Column.backgroundColor(this.selected ? (bjccovmshb1ibh.instrumentBranch(40, 0, true), SettingsTheme.accentColor(this.isDark, this.accentName)) : (bjccovmshb1ibh.instrumentBranch(40, 0, false), SettingsTheme.accentBackground(this.isDark, this.accentName)));
        }, Column);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1ibh.instrumentFunction(41);
            Image.create(this.iconResource);
            Image.width(17);
            Image.height(17);
            Image.fillColor(this.selected ? (bjccovmshb1ibh.instrumentBranch(41, 0, true), Color.White) : (bjccovmshb1ibh.instrumentBranch(41, 0, false), SettingsTheme.accentColor(this.isDark, this.accentName)));
            Image.draggable(false);
        }, Image);
        Column.pop();
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1ibh.instrumentFunction(42);
            Text.create(this.title);
            Text.fontSize(14);
            Text.fontWeight(this.selected ? (bjccovmshb1ibh.instrumentBranch(42, 0, true), FontWeight.Bold) : (bjccovmshb1ibh.instrumentBranch(42, 0, false), FontWeight.Medium));
            Text.fontColor(SettingsTheme.primaryText(this.isDark));
            Text.maxLines(1);
            Text.textOverflow({ overflow: TextOverflow.Ellipsis });
            Text.layoutWeight(1);
        }, Text);
        Text.pop();
        Row.pop();
    }
    rerender() {
        this.updateDirtyElements();
    }
}
export class SettingsKeyValueRow extends ViewPU {
    constructor(parent, params, __localStorage, elmtId = -1, paramsLambda = undefined, extraInfo) {
        super(parent, __localStorage, elmtId, extraInfo);
        if (typeof paramsLambda === "function") {
            this.paramsGenerator_ = paramsLambda;
        }
        this.__label = new SynchedPropertySimpleOneWayPU(params.label, this, "label");
        this.__value = new SynchedPropertySimpleOneWayPU(params.value, this, "value");
        this.__isDark = new SynchedPropertySimpleOneWayPU(params.isDark, this, "isDark");
        this.__tone = new SynchedPropertySimpleOneWayPU(params.tone, this, "tone");
        this.setInitiallyProvidedValue(params);
        this.finalizeConstruction();
    }
    setInitiallyProvidedValue(params: SettingsKeyValueRow_Params) {
        bjccovmshb1ibh.instrumentFunction(43);
        if (params.label === undefined) {
            this.__label.set('');
        }
        else {
        }
        if (params.value === undefined) {
            this.__value.set('');
        }
        else {
        }
        if (params.isDark === undefined) {
            this.__isDark.set(false);
        }
        else {
        }
        if (params.tone === undefined) {
            this.__tone.set('neutral');
        }
        else {
        }
    }
    updateStateVars(params: SettingsKeyValueRow_Params) {
        bjccovmshb1ibh.instrumentFunction(44);
        this.__label.reset(params.label);
        this.__value.reset(params.value);
        this.__isDark.reset(params.isDark);
        this.__tone.reset(params.tone);
    }
    purgeVariableDependenciesOnElmtId(rmElmtId) {
        this.__label.purgeDependencyOnElmtId(rmElmtId);
        this.__value.purgeDependencyOnElmtId(rmElmtId);
        this.__isDark.purgeDependencyOnElmtId(rmElmtId);
        this.__tone.purgeDependencyOnElmtId(rmElmtId);
    }
    aboutToBeDeleted() {
        this.__label.aboutToBeDeleted();
        this.__value.aboutToBeDeleted();
        this.__isDark.aboutToBeDeleted();
        this.__tone.aboutToBeDeleted();
        SubscriberManager.Get().delete(this.id__());
        this.aboutToBeDeletedInternal();
    }
    private __label: SynchedPropertySimpleOneWayPU<string>;
    get label() {
        bjccovmshb1ibh.instrumentFunction(45);
        return this.__label.get();
    }
    set label(newValue: string) {
        bjccovmshb1ibh.instrumentFunction(46);
        this.__label.set(newValue);
    }
    private __value: SynchedPropertySimpleOneWayPU<string>;
    get value() {
        bjccovmshb1ibh.instrumentFunction(47);
        return this.__value.get();
    }
    set value(newValue: string) {
        bjccovmshb1ibh.instrumentFunction(48);
        this.__value.set(newValue);
    }
    private __isDark: SynchedPropertySimpleOneWayPU<boolean>;
    get isDark() {
        bjccovmshb1ibh.instrumentFunction(49);
        return this.__isDark.get();
    }
    set isDark(newValue: boolean) {
        bjccovmshb1ibh.instrumentFunction(50);
        this.__isDark.set(newValue);
    }
    private __tone: SynchedPropertySimpleOneWayPU<SettingsStatusTone>;
    get tone() {
        bjccovmshb1ibh.instrumentFunction(51);
        return this.__tone.get();
    }
    set tone(newValue: SettingsStatusTone) {
        bjccovmshb1ibh.instrumentFunction(52);
        this.__tone.set(newValue);
    }
    initialRender() {
        bjccovmshb1ibh.instrumentFunction(53);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1ibh.instrumentFunction(54);
            Row.create();
            Row.alignItems(VerticalAlign.Center);
            Row.width('100%');
            Row.constraintSize({ minHeight: 32 });
            Row.padding({ top: 4, bottom: 4 });
        }, Row);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1ibh.instrumentFunction(55);
            Text.create(this.label);
            Text.fontSize(12);
            Text.fontColor(SettingsTheme.mutedText(this.isDark));
            Text.maxLines(2);
            Text.textOverflow({ overflow: TextOverflow.Ellipsis });
            Text.layoutWeight(1);
        }, Text);
        Text.pop();
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1ibh.instrumentFunction(56);
            Text.create(this.value);
            Text.fontSize(13);
            Text.fontWeight(FontWeight.Medium);
            Text.fontColor(this.tone === 'neutral' ? (bjccovmshb1ibh.instrumentBranch(56, 0, true), SettingsTheme.primaryText(this.isDark)) : (bjccovmshb1ibh.instrumentBranch(56, 0, false), SettingsTheme.statusText(this.isDark, this.tone)));
            Text.maxLines(2);
            Text.textOverflow({ overflow: TextOverflow.Ellipsis });
            Text.textAlign(TextAlign.End);
            Text.margin({ left: 12 });
        }, Text);
        Text.pop();
        Row.pop();
    }
    rerender() {
        this.updateDirtyElements();
    }
}
export class SettingsBackButton extends ViewPU {
    constructor(parent, params, __localStorage, elmtId = -1, paramsLambda = undefined, extraInfo) {
        super(parent, __localStorage, elmtId, extraInfo);
        if (typeof paramsLambda === "function") {
            this.paramsGenerator_ = paramsLambda;
        }
        this.__isDark = new SynchedPropertySimpleOneWayPU(params.isDark, this, "isDark");
        this.onBack = () => {
            bjccovmshb1ibh.instrumentFunction(57);
        };
        this.__hovered = new ObservedPropertySimplePU(false, this, "hovered");
        this.__pressed = new ObservedPropertySimplePU(false, this, "pressed");
        this.setInitiallyProvidedValue(params);
        this.finalizeConstruction();
    }
    setInitiallyProvidedValue(params: SettingsBackButton_Params) {
        bjccovmshb1ibh.instrumentFunction(58);
        if (params.isDark === undefined) {
            this.__isDark.set(false);
        }
        else {
        }
        if (params.onBack !== undefined) {
            this.onBack = params.onBack;
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
    updateStateVars(params: SettingsBackButton_Params) {
        bjccovmshb1ibh.instrumentFunction(59);
        this.__isDark.reset(params.isDark);
    }
    purgeVariableDependenciesOnElmtId(rmElmtId) {
        this.__isDark.purgeDependencyOnElmtId(rmElmtId);
        this.__hovered.purgeDependencyOnElmtId(rmElmtId);
        this.__pressed.purgeDependencyOnElmtId(rmElmtId);
    }
    aboutToBeDeleted() {
        this.__isDark.aboutToBeDeleted();
        this.__hovered.aboutToBeDeleted();
        this.__pressed.aboutToBeDeleted();
        SubscriberManager.Get().delete(this.id__());
        this.aboutToBeDeletedInternal();
    }
    private __isDark: SynchedPropertySimpleOneWayPU<boolean>;
    get isDark() {
        bjccovmshb1ibh.instrumentFunction(60);
        return this.__isDark.get();
    }
    set isDark(newValue: boolean) {
        bjccovmshb1ibh.instrumentFunction(61);
        this.__isDark.set(newValue);
    }
    private onBack: () => void;
    private __hovered: ObservedPropertySimplePU<boolean>;
    get hovered() {
        bjccovmshb1ibh.instrumentFunction(63);
        return this.__hovered.get();
    }
    set hovered(newValue: boolean) {
        bjccovmshb1ibh.instrumentFunction(64);
        this.__hovered.set(newValue);
    }
    private __pressed: ObservedPropertySimplePU<boolean>;
    get pressed() {
        bjccovmshb1ibh.instrumentFunction(65);
        return this.__pressed.get();
    }
    set pressed(newValue: boolean) {
        bjccovmshb1ibh.instrumentFunction(66);
        this.__pressed.set(newValue);
    }
    private setHovered(hovered: boolean): void {
        bjccovmshb1ibh.instrumentFunction(67);
        bjccovmshb1ibh.instrumentRegion(67, 1);
        this.hovered = hovered;
    }
    initialRender() {
        bjccovmshb1ibh.instrumentFunction(68);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1ibh.instrumentFunction(69);
            Button.createWithChild();
            Button.type(ButtonType.Normal);
            Button.width(48);
            Button.height(48);
            Button.borderRadius(SettingsTheme.BUTTON_RADIUS);
            Button.backgroundColor(SettingsTheme.transparentButton(this.hovered, this.pressed, this.isDark));
            Button.stateEffect(false);
            Button.onHover((isHover: boolean) => {
                bjccovmshb1ibh.instrumentFunction(70);
                bjccovmshb1ibh.instrumentRegion(70, 1);
                this.setHovered(isHover);
            });
            Button.onTouch((event: TouchEvent) => {
                bjccovmshb1ibh.instrumentFunction(71);
                if (event.type === TouchType.Down) {
                    bjccovmshb1ibh.instrumentBranch(71, 0, true);
                    bjccovmshb1ibh.instrumentRegion(71, 1);
                    this.pressed = true;
                    bjccovmshb1ibh.instrumentRegion(68, 1);
                    return;
                }
                else {
                    bjccovmshb1ibh.instrumentBranch(71, 0, false);
                }
                if (event.type === TouchType.Up || event.type === TouchType.Cancel) {
                    bjccovmshb1ibh.instrumentBranch(71, 1, true);
                    bjccovmshb1ibh.instrumentRegion(71, 2);
                    this.pressed = false;
                }
                else {
                    bjccovmshb1ibh.instrumentBranch(71, 1, false);
                }
            });
            Button.onClick(() => {
                bjccovmshb1ibh.instrumentFunction(72);
                this.pressed = false;
                bjccovmshb1ibh.instrumentRegion(68, 2);
                this.onBack();
            });
        }, Button);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1ibh.instrumentFunction(73);
            Image.create(SettingsResources.ARROW_LEFT_ICON);
            Image.width(18);
            Image.height(18);
            Image.fillColor(SettingsTheme.primaryText(this.isDark));
            Image.draggable(false);
        }, Image);
        Button.pop();
    }
    rerender() {
        this.updateDirtyElements();
    }
}
export class SettingsPageHeader extends ViewPU {
    constructor(parent, params, __localStorage, elmtId = -1, paramsLambda = undefined, extraInfo) {
        super(parent, __localStorage, elmtId, extraInfo);
        if (typeof paramsLambda === "function") {
            this.paramsGenerator_ = paramsLambda;
        }
        this.__title = new SynchedPropertySimpleOneWayPU(params.title, this, "title");
        this.__subtitle = new SynchedPropertySimpleOneWayPU(params.subtitle, this, "subtitle");
        this.__isDark = new SynchedPropertySimpleOneWayPU(params.isDark, this, "isDark");
        this.onBack = () => {
            bjccovmshb1ibh.instrumentFunction(74);
        };
        this.setInitiallyProvidedValue(params);
        this.finalizeConstruction();
    }
    setInitiallyProvidedValue(params: SettingsPageHeader_Params) {
        bjccovmshb1ibh.instrumentFunction(75);
        if (params.title === undefined) {
            this.__title.set('');
        }
        else {
        }
        if (params.subtitle === undefined) {
            this.__subtitle.set('');
        }
        else {
        }
        if (params.isDark === undefined) {
            this.__isDark.set(false);
        }
        else {
        }
        if (params.onBack !== undefined) {
            this.onBack = params.onBack;
        }
        else {
        }
    }
    updateStateVars(params: SettingsPageHeader_Params) {
        bjccovmshb1ibh.instrumentFunction(76);
        this.__title.reset(params.title);
        this.__subtitle.reset(params.subtitle);
        this.__isDark.reset(params.isDark);
    }
    purgeVariableDependenciesOnElmtId(rmElmtId) {
        this.__title.purgeDependencyOnElmtId(rmElmtId);
        this.__subtitle.purgeDependencyOnElmtId(rmElmtId);
        this.__isDark.purgeDependencyOnElmtId(rmElmtId);
    }
    aboutToBeDeleted() {
        this.__title.aboutToBeDeleted();
        this.__subtitle.aboutToBeDeleted();
        this.__isDark.aboutToBeDeleted();
        SubscriberManager.Get().delete(this.id__());
        this.aboutToBeDeletedInternal();
    }
    private __title: SynchedPropertySimpleOneWayPU<string>;
    get title() {
        bjccovmshb1ibh.instrumentFunction(77);
        return this.__title.get();
    }
    set title(newValue: string) {
        bjccovmshb1ibh.instrumentFunction(78);
        this.__title.set(newValue);
    }
    private __subtitle: SynchedPropertySimpleOneWayPU<string>;
    get subtitle() {
        bjccovmshb1ibh.instrumentFunction(79);
        return this.__subtitle.get();
    }
    set subtitle(newValue: string) {
        bjccovmshb1ibh.instrumentFunction(80);
        this.__subtitle.set(newValue);
    }
    private __isDark: SynchedPropertySimpleOneWayPU<boolean>;
    get isDark() {
        bjccovmshb1ibh.instrumentFunction(81);
        return this.__isDark.get();
    }
    set isDark(newValue: boolean) {
        bjccovmshb1ibh.instrumentFunction(82);
        this.__isDark.set(newValue);
    }
    private onBack: () => void;
    initialRender() {
        bjccovmshb1ibh.instrumentFunction(84);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1ibh.instrumentFunction(85);
            Row.create();
            Row.width('100%');
            Row.alignItems(VerticalAlign.Top);
            Row.padding({ left: 16, right: 20, top: 16, bottom: 14 });
        }, Row);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1ibh.instrumentFunction(86);
            Row.create({ space: 8 });
            Row.layoutWeight(1);
            Row.alignItems(VerticalAlign.Top);
        }, Row);
        {
            this.observeComponentCreation2((elmtId, isInitialRender) => {
                bjccovmshb1ibh.instrumentFunction(87);
                if (isInitialRender) {
                    let componentCall = new SettingsBackButton(this, {
                        isDark: this.isDark,
                        onBack: this.onBack
                    }, undefined, elmtId, () => { }, { page: "common/src/main/ets/components/settings/SettingsPrimitives.ets", line: 193, col: 9 });
                    ViewPU.create(componentCall);
                    let paramsLambda = () => {
                        return {
                            isDark: this.isDark,
                            onBack: this.onBack
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
            bjccovmshb1ibh.instrumentFunction(88);
            Column.create({ space: 4 });
            Column.alignItems(HorizontalAlign.Start);
            Column.layoutWeight(1);
        }, Column);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1ibh.instrumentFunction(89);
            Text.create(this.title);
            Text.fontSize(24);
            Text.fontWeight(FontWeight.Bold);
            Text.fontColor(SettingsTheme.primaryText(this.isDark));
            Text.maxLines(1);
            Text.textOverflow({ overflow: TextOverflow.Ellipsis });
            Text.width('100%');
        }, Text);
        Text.pop();
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1ibh.instrumentFunction(90);
            If.create();
            if (this.subtitle.length > 0) {
                bjccovmshb1ibh.instrumentBranch(90, 0, true);
                bjccovmshb1ibh.instrumentRegion(90, 1);
                this.ifElseBranchUpdateFunction(0, () => {
                    bjccovmshb1ibh.instrumentFunction(91);
                    this.observeComponentCreation2((elmtId, isInitialRender) => {
                        bjccovmshb1ibh.instrumentFunction(92);
                        Text.create(this.subtitle);
                        Text.fontSize(12);
                        Text.fontColor(SettingsTheme.mutedText(this.isDark));
                        Text.maxLines(2);
                        Text.textOverflow({ overflow: TextOverflow.Ellipsis });
                        Text.width('100%');
                    }, Text);
                    Text.pop();
                });
            }
            else {
                bjccovmshb1ibh.instrumentBranch(90, 0, false);
                this.ifElseBranchUpdateFunction(1, () => {
                });
            }
        }, If);
        If.pop();
        Column.pop();
        Row.pop();
        Row.pop();
    }
    rerender() {
        this.updateDirtyElements();
    }
}
export class SettingsListItem extends ViewPU {
    constructor(parent, params, __localStorage, elmtId = -1, paramsLambda = undefined, extraInfo) {
        super(parent, __localStorage, elmtId, extraInfo);
        if (typeof paramsLambda === "function") {
            this.paramsGenerator_ = paramsLambda;
        }
        this.__title = new SynchedPropertySimpleOneWayPU(params.title, this, "title");
        this.__description = new SynchedPropertySimpleOneWayPU(params.description, this, "description");
        this.__iconResource = new SynchedPropertyObjectOneWayPU(params.iconResource, this, "iconResource");
        this.__accentName = new SynchedPropertySimpleOneWayPU(params.accentName, this, "accentName");
        this.__value = new SynchedPropertySimpleOneWayPU(params.value, this, "value");
        this.__isDark = new SynchedPropertySimpleOneWayPU(params.isDark, this, "isDark");
        this.__selected = new SynchedPropertySimpleOneWayPU(params.selected, this, "selected");
        this.onPress = () => {
            bjccovmshb1ibh.instrumentFunction(93);
        };
        this.__hovered = new ObservedPropertySimplePU(false, this, "hovered");
        this.__pressed = new ObservedPropertySimplePU(false, this, "pressed");
        this.setInitiallyProvidedValue(params);
        this.finalizeConstruction();
    }
    setInitiallyProvidedValue(params: SettingsListItem_Params) {
        bjccovmshb1ibh.instrumentFunction(94);
        if (params.title === undefined) {
            this.__title.set('');
        }
        else {
        }
        if (params.description === undefined) {
            this.__description.set('');
        }
        else {
        }
        if (params.iconResource === undefined) {
            this.__iconResource.set(SettingsResources.SETTINGS_ICON);
        }
        else {
        }
        if (params.accentName === undefined) {
            this.__accentName.set(SettingsAccent.BLUE);
        }
        else {
        }
        if (params.value === undefined) {
            this.__value.set('');
        }
        else {
        }
        if (params.isDark === undefined) {
            this.__isDark.set(false);
        }
        else {
        }
        if (params.selected === undefined) {
            this.__selected.set(false);
        }
        else {
        }
        if (params.onPress !== undefined) {
            this.onPress = params.onPress;
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
    updateStateVars(params: SettingsListItem_Params) {
        bjccovmshb1ibh.instrumentFunction(95);
        this.__title.reset(params.title);
        this.__description.reset(params.description);
        this.__iconResource.reset(params.iconResource);
        this.__accentName.reset(params.accentName);
        this.__value.reset(params.value);
        this.__isDark.reset(params.isDark);
        this.__selected.reset(params.selected);
    }
    purgeVariableDependenciesOnElmtId(rmElmtId) {
        this.__title.purgeDependencyOnElmtId(rmElmtId);
        this.__description.purgeDependencyOnElmtId(rmElmtId);
        this.__iconResource.purgeDependencyOnElmtId(rmElmtId);
        this.__accentName.purgeDependencyOnElmtId(rmElmtId);
        this.__value.purgeDependencyOnElmtId(rmElmtId);
        this.__isDark.purgeDependencyOnElmtId(rmElmtId);
        this.__selected.purgeDependencyOnElmtId(rmElmtId);
        this.__hovered.purgeDependencyOnElmtId(rmElmtId);
        this.__pressed.purgeDependencyOnElmtId(rmElmtId);
    }
    aboutToBeDeleted() {
        this.__title.aboutToBeDeleted();
        this.__description.aboutToBeDeleted();
        this.__iconResource.aboutToBeDeleted();
        this.__accentName.aboutToBeDeleted();
        this.__value.aboutToBeDeleted();
        this.__isDark.aboutToBeDeleted();
        this.__selected.aboutToBeDeleted();
        this.__hovered.aboutToBeDeleted();
        this.__pressed.aboutToBeDeleted();
        SubscriberManager.Get().delete(this.id__());
        this.aboutToBeDeletedInternal();
    }
    private __title: SynchedPropertySimpleOneWayPU<string>;
    get title() {
        bjccovmshb1ibh.instrumentFunction(96);
        return this.__title.get();
    }
    set title(newValue: string) {
        bjccovmshb1ibh.instrumentFunction(97);
        this.__title.set(newValue);
    }
    private __description: SynchedPropertySimpleOneWayPU<string>;
    get description() {
        bjccovmshb1ibh.instrumentFunction(98);
        return this.__description.get();
    }
    set description(newValue: string) {
        bjccovmshb1ibh.instrumentFunction(99);
        this.__description.set(newValue);
    }
    private __iconResource: SynchedPropertySimpleOneWayPU<Resource>;
    get iconResource() {
        bjccovmshb1ibh.instrumentFunction(100);
        return this.__iconResource.get();
    }
    set iconResource(newValue: Resource) {
        bjccovmshb1ibh.instrumentFunction(101);
        this.__iconResource.set(newValue);
    }
    private __accentName: SynchedPropertySimpleOneWayPU<SettingsAccentName>;
    get accentName() {
        bjccovmshb1ibh.instrumentFunction(102);
        return this.__accentName.get();
    }
    set accentName(newValue: SettingsAccentName) {
        bjccovmshb1ibh.instrumentFunction(103);
        this.__accentName.set(newValue);
    }
    private __value: SynchedPropertySimpleOneWayPU<string>;
    get value() {
        bjccovmshb1ibh.instrumentFunction(104);
        return this.__value.get();
    }
    set value(newValue: string) {
        bjccovmshb1ibh.instrumentFunction(105);
        this.__value.set(newValue);
    }
    private __isDark: SynchedPropertySimpleOneWayPU<boolean>;
    get isDark() {
        bjccovmshb1ibh.instrumentFunction(106);
        return this.__isDark.get();
    }
    set isDark(newValue: boolean) {
        bjccovmshb1ibh.instrumentFunction(107);
        this.__isDark.set(newValue);
    }
    private __selected: SynchedPropertySimpleOneWayPU<boolean>;
    get selected() {
        bjccovmshb1ibh.instrumentFunction(108);
        return this.__selected.get();
    }
    set selected(newValue: boolean) {
        bjccovmshb1ibh.instrumentFunction(109);
        this.__selected.set(newValue);
    }
    private onPress: () => void;
    private __hovered: ObservedPropertySimplePU<boolean>;
    get hovered() {
        bjccovmshb1ibh.instrumentFunction(111);
        return this.__hovered.get();
    }
    set hovered(newValue: boolean) {
        bjccovmshb1ibh.instrumentFunction(112);
        this.__hovered.set(newValue);
    }
    private __pressed: ObservedPropertySimplePU<boolean>;
    get pressed() {
        bjccovmshb1ibh.instrumentFunction(113);
        return this.__pressed.get();
    }
    set pressed(newValue: boolean) {
        bjccovmshb1ibh.instrumentFunction(114);
        this.__pressed.set(newValue);
    }
    private setHovered(hovered: boolean): void {
        bjccovmshb1ibh.instrumentFunction(115);
        bjccovmshb1ibh.instrumentRegion(115, 1);
        SettingsTheme.animate(this.getUIContext(), () => {
            bjccovmshb1ibh.instrumentFunction(116);
            bjccovmshb1ibh.instrumentRegion(116, 1);
            this.hovered = hovered;
        });
    }
    initialRender() {
        bjccovmshb1ibh.instrumentFunction(117);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1ibh.instrumentFunction(118);
            Row.create({ space: 14 });
            Row.alignItems(VerticalAlign.Center);
            Row.width('100%');
            Row.padding({ left: 14, right: 14, top: 12, bottom: 12 });
            Row.backgroundColor(this.hovered || this.pressed ? (bjccovmshb1ibh.instrumentBranch(118, 0, true), SettingsTheme.cardHoverBackground(this.isDark)) : (bjccovmshb1ibh.instrumentBranch(118, 0, false), SettingsTheme.cardBackground(this.isDark)));
            Row.borderRadius(SettingsTheme.CARD_RADIUS);
            Row.border({
                width: 1,
                color: this.selected ? (bjccovmshb1ibh.instrumentBranch(118, 1, true), SettingsTheme.accentColor(this.isDark, this.accentName)) : (bjccovmshb1ibh.instrumentBranch(118, 1, false), SettingsTheme.borderColor(this.isDark))
            });
            Row.shadow(SettingsTheme.shadow(this.isDark, this.hovered));
            Row.translate({ y: this.hovered ? (bjccovmshb1ibh.instrumentBranch(118, 2, true), -5) : (bjccovmshb1ibh.instrumentBranch(118, 2, false), 0) });
            Row.scale({
                x: this.pressed ? (bjccovmshb1ibh.instrumentBranch(118, 3, true), 0.99) : (bjccovmshb1ibh.instrumentBranch(118, 3, false), this.hovered ? (bjccovmshb1ibh.instrumentBranch(118, 4, true), 1.012) : (bjccovmshb1ibh.instrumentBranch(118, 4, false), 1)),
                y: this.pressed ? (bjccovmshb1ibh.instrumentBranch(118, 5, true), 0.99) : (bjccovmshb1ibh.instrumentBranch(118, 5, false), this.hovered ? (bjccovmshb1ibh.instrumentBranch(118, 6, true), 1.012) : (bjccovmshb1ibh.instrumentBranch(118, 6, false), 1))
            });
            Row.margin({ bottom: 10 });
            Row.onHover((isHover: boolean) => {
                bjccovmshb1ibh.instrumentFunction(119);
                bjccovmshb1ibh.instrumentRegion(119, 1);
                this.setHovered(isHover);
            });
            Row.onTouch((event: TouchEvent) => {
                bjccovmshb1ibh.instrumentFunction(120);
                if (event.type === TouchType.Down) {
                    bjccovmshb1ibh.instrumentBranch(120, 0, true);
                    bjccovmshb1ibh.instrumentRegion(120, 1);
                    this.pressed = true;
                    bjccovmshb1ibh.instrumentRegion(117, 1);
                    return;
                }
                else {
                    bjccovmshb1ibh.instrumentBranch(120, 0, false);
                }
                if (event.type === TouchType.Up || event.type === TouchType.Cancel) {
                    bjccovmshb1ibh.instrumentBranch(120, 1, true);
                    bjccovmshb1ibh.instrumentRegion(120, 2);
                    this.pressed = false;
                }
                else {
                    bjccovmshb1ibh.instrumentBranch(120, 1, false);
                }
            });
            Row.onClick(() => {
                bjccovmshb1ibh.instrumentFunction(121);
                this.pressed = false;
                bjccovmshb1ibh.instrumentRegion(117, 2);
                this.onPress();
            });
        }, Row);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1ibh.instrumentFunction(122);
            Column.create();
            Column.width(SettingsTheme.ICON_TILE_SIZE);
            Column.height(SettingsTheme.ICON_TILE_SIZE);
            Column.justifyContent(FlexAlign.Center);
            Column.alignItems(HorizontalAlign.Center);
            Column.borderRadius(SettingsTheme.BUTTON_RADIUS);
            Column.backgroundColor(this.selected ? (bjccovmshb1ibh.instrumentBranch(122, 0, true), SettingsTheme.accentColor(this.isDark, this.accentName)) : (bjccovmshb1ibh.instrumentBranch(122, 0, false), (this.hovered || this.pressed ? (bjccovmshb1ibh.instrumentBranch(122, 1, true), SettingsTheme.accentHoverBackground(this.isDark, this.accentName)) : (bjccovmshb1ibh.instrumentBranch(122, 1, false), SettingsTheme.accentBackground(this.isDark, this.accentName)))));
        }, Column);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1ibh.instrumentFunction(123);
            Image.create(this.iconResource);
            Image.width(SettingsTheme.ICON_SIZE);
            Image.height(SettingsTheme.ICON_SIZE);
            Image.fillColor(this.selected ? (bjccovmshb1ibh.instrumentBranch(123, 0, true), Color.White) : (bjccovmshb1ibh.instrumentBranch(123, 0, false), SettingsTheme.accentColor(this.isDark, this.accentName)));
            Image.draggable(false);
        }, Image);
        Column.pop();
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1ibh.instrumentFunction(124);
            Column.create({ space: 5 });
            Column.alignItems(HorizontalAlign.Start);
            Column.layoutWeight(1);
        }, Column);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1ibh.instrumentFunction(125);
            Text.create(this.title);
            Text.fontSize(15);
            Text.fontWeight(this.selected ? (bjccovmshb1ibh.instrumentBranch(125, 0, true), FontWeight.Bold) : (bjccovmshb1ibh.instrumentBranch(125, 0, false), FontWeight.Medium));
            Text.fontColor(SettingsTheme.primaryText(this.isDark));
            Text.maxLines(1);
            Text.textOverflow({ overflow: TextOverflow.Ellipsis });
            Text.width('100%');
        }, Text);
        Text.pop();
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1ibh.instrumentFunction(126);
            Text.create(this.description);
            Text.fontSize(13);
            Text.fontColor(SettingsTheme.mutedText(this.isDark));
            Text.maxLines(2);
            Text.lineHeight(19);
            Text.textOverflow({ overflow: TextOverflow.Ellipsis });
            Text.width('100%');
        }, Text);
        Text.pop();
        Column.pop();
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1ibh.instrumentFunction(127);
            If.create();
            if (this.selected) {
                bjccovmshb1ibh.instrumentBranch(127, 0, true);
                bjccovmshb1ibh.instrumentRegion(127, 1);
                this.ifElseBranchUpdateFunction(0, () => {
                    bjccovmshb1ibh.instrumentFunction(128);
                    this.observeComponentCreation2((elmtId, isInitialRender) => {
                        bjccovmshb1ibh.instrumentFunction(129);
                        Image.create(SettingsResources.CHECK_ICON);
                        Image.width(18);
                        Image.height(18);
                        Image.fillColor(SettingsTheme.accentColor(this.isDark, this.accentName));
                        Image.margin({ left: 6 });
                        Image.draggable(false);
                    }, Image);
                });
            }
            else {
                bjccovmshb1ibh.instrumentBranch(127, 0, false);
                bjccovmshb1ibh.instrumentRegion(127, 2);
                if (this.value.length > 0) {
                    bjccovmshb1ibh.instrumentBranch(127, 1, true);
                    bjccovmshb1ibh.instrumentRegion(127, 3);
                    this.ifElseBranchUpdateFunction(1, () => {
                        bjccovmshb1ibh.instrumentFunction(130);
                        this.observeComponentCreation2((elmtId, isInitialRender) => {
                            bjccovmshb1ibh.instrumentFunction(131);
                            Text.create(this.value);
                            Text.fontSize(12);
                            Text.fontColor(SettingsTheme.mutedText(this.isDark));
                            Text.margin({ left: 6 });
                        }, Text);
                        Text.pop();
                    });
                }
                else {
                    bjccovmshb1ibh.instrumentBranch(127, 1, false);
                    this.ifElseBranchUpdateFunction(2, () => {
                    });
                }
            }
        }, If);
        If.pop();
        Row.pop();
    }
    rerender() {
        this.updateDirtyElements();
    }
}
export class SettingsSectionTitle extends ViewPU {
    constructor(parent, params, __localStorage, elmtId = -1, paramsLambda = undefined, extraInfo) {
        super(parent, __localStorage, elmtId, extraInfo);
        if (typeof paramsLambda === "function") {
            this.paramsGenerator_ = paramsLambda;
        }
        this.__title = new SynchedPropertySimpleOneWayPU(params.title, this, "title");
        this.__subtitle = new SynchedPropertySimpleOneWayPU(params.subtitle, this, "subtitle");
        this.__isDark = new SynchedPropertySimpleOneWayPU(params.isDark, this, "isDark");
        this.setInitiallyProvidedValue(params);
        this.finalizeConstruction();
    }
    setInitiallyProvidedValue(params: SettingsSectionTitle_Params) {
        bjccovmshb1ibh.instrumentFunction(132);
        if (params.title === undefined) {
            this.__title.set('');
        }
        else {
        }
        if (params.subtitle === undefined) {
            this.__subtitle.set('');
        }
        else {
        }
        if (params.isDark === undefined) {
            this.__isDark.set(false);
        }
        else {
        }
    }
    updateStateVars(params: SettingsSectionTitle_Params) {
        bjccovmshb1ibh.instrumentFunction(133);
        this.__title.reset(params.title);
        this.__subtitle.reset(params.subtitle);
        this.__isDark.reset(params.isDark);
    }
    purgeVariableDependenciesOnElmtId(rmElmtId) {
        this.__title.purgeDependencyOnElmtId(rmElmtId);
        this.__subtitle.purgeDependencyOnElmtId(rmElmtId);
        this.__isDark.purgeDependencyOnElmtId(rmElmtId);
    }
    aboutToBeDeleted() {
        this.__title.aboutToBeDeleted();
        this.__subtitle.aboutToBeDeleted();
        this.__isDark.aboutToBeDeleted();
        SubscriberManager.Get().delete(this.id__());
        this.aboutToBeDeletedInternal();
    }
    private __title: SynchedPropertySimpleOneWayPU<string>;
    get title() {
        bjccovmshb1ibh.instrumentFunction(134);
        return this.__title.get();
    }
    set title(newValue: string) {
        bjccovmshb1ibh.instrumentFunction(135);
        this.__title.set(newValue);
    }
    private __subtitle: SynchedPropertySimpleOneWayPU<string>;
    get subtitle() {
        bjccovmshb1ibh.instrumentFunction(136);
        return this.__subtitle.get();
    }
    set subtitle(newValue: string) {
        bjccovmshb1ibh.instrumentFunction(137);
        this.__subtitle.set(newValue);
    }
    private __isDark: SynchedPropertySimpleOneWayPU<boolean>;
    get isDark() {
        bjccovmshb1ibh.instrumentFunction(138);
        return this.__isDark.get();
    }
    set isDark(newValue: boolean) {
        bjccovmshb1ibh.instrumentFunction(139);
        this.__isDark.set(newValue);
    }
    initialRender() {
        bjccovmshb1ibh.instrumentFunction(140);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1ibh.instrumentFunction(141);
            Column.create({ space: 4 });
            Column.alignItems(HorizontalAlign.Start);
            Column.width('100%');
            Column.margin({ top: 6, bottom: 10 });
        }, Column);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1ibh.instrumentFunction(142);
            Text.create(this.title);
            Text.fontSize(13);
            Text.fontWeight(FontWeight.Bold);
            Text.fontColor(SettingsTheme.primaryText(this.isDark));
            Text.width('100%');
        }, Text);
        Text.pop();
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1ibh.instrumentFunction(143);
            If.create();
            if (this.subtitle.length > 0) {
                bjccovmshb1ibh.instrumentBranch(143, 0, true);
                bjccovmshb1ibh.instrumentRegion(143, 1);
                this.ifElseBranchUpdateFunction(0, () => {
                    bjccovmshb1ibh.instrumentFunction(144);
                    this.observeComponentCreation2((elmtId, isInitialRender) => {
                        bjccovmshb1ibh.instrumentFunction(145);
                        Text.create(this.subtitle);
                        Text.fontSize(12);
                        Text.fontColor(SettingsTheme.mutedText(this.isDark));
                        Text.width('100%');
                        Text.lineHeight(18);
                    }, Text);
                    Text.pop();
                });
            }
            else {
                bjccovmshb1ibh.instrumentBranch(143, 0, false);
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
export class SettingsInfoCard extends ViewPU {
    constructor(parent, params, __localStorage, elmtId = -1, paramsLambda = undefined, extraInfo) {
        super(parent, __localStorage, elmtId, extraInfo);
        if (typeof paramsLambda === "function") {
            this.paramsGenerator_ = paramsLambda;
        }
        this.__title = new SynchedPropertySimpleOneWayPU(params.title, this, "title");
        this.__body = new SynchedPropertySimpleOneWayPU(params.body, this, "body");
        this.__isDark = new SynchedPropertySimpleOneWayPU(params.isDark, this, "isDark");
        this.__hovered = new ObservedPropertySimplePU(false, this, "hovered");
        this.__pressed = new ObservedPropertySimplePU(false, this, "pressed");
        this.setInitiallyProvidedValue(params);
        this.finalizeConstruction();
    }
    setInitiallyProvidedValue(params: SettingsInfoCard_Params) {
        bjccovmshb1ibh.instrumentFunction(146);
        if (params.title === undefined) {
            this.__title.set('');
        }
        else {
        }
        if (params.body === undefined) {
            this.__body.set('');
        }
        else {
        }
        if (params.isDark === undefined) {
            this.__isDark.set(false);
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
    updateStateVars(params: SettingsInfoCard_Params) {
        bjccovmshb1ibh.instrumentFunction(147);
        this.__title.reset(params.title);
        this.__body.reset(params.body);
        this.__isDark.reset(params.isDark);
    }
    purgeVariableDependenciesOnElmtId(rmElmtId) {
        this.__title.purgeDependencyOnElmtId(rmElmtId);
        this.__body.purgeDependencyOnElmtId(rmElmtId);
        this.__isDark.purgeDependencyOnElmtId(rmElmtId);
        this.__hovered.purgeDependencyOnElmtId(rmElmtId);
        this.__pressed.purgeDependencyOnElmtId(rmElmtId);
    }
    aboutToBeDeleted() {
        this.__title.aboutToBeDeleted();
        this.__body.aboutToBeDeleted();
        this.__isDark.aboutToBeDeleted();
        this.__hovered.aboutToBeDeleted();
        this.__pressed.aboutToBeDeleted();
        SubscriberManager.Get().delete(this.id__());
        this.aboutToBeDeletedInternal();
    }
    private __title: SynchedPropertySimpleOneWayPU<string>;
    get title() {
        bjccovmshb1ibh.instrumentFunction(148);
        return this.__title.get();
    }
    set title(newValue: string) {
        bjccovmshb1ibh.instrumentFunction(149);
        this.__title.set(newValue);
    }
    private __body: SynchedPropertySimpleOneWayPU<string>;
    get body() {
        bjccovmshb1ibh.instrumentFunction(150);
        return this.__body.get();
    }
    set body(newValue: string) {
        bjccovmshb1ibh.instrumentFunction(151);
        this.__body.set(newValue);
    }
    private __isDark: SynchedPropertySimpleOneWayPU<boolean>;
    get isDark() {
        bjccovmshb1ibh.instrumentFunction(152);
        return this.__isDark.get();
    }
    set isDark(newValue: boolean) {
        bjccovmshb1ibh.instrumentFunction(153);
        this.__isDark.set(newValue);
    }
    private __hovered: ObservedPropertySimplePU<boolean>;
    get hovered() {
        bjccovmshb1ibh.instrumentFunction(154);
        return this.__hovered.get();
    }
    set hovered(newValue: boolean) {
        bjccovmshb1ibh.instrumentFunction(155);
        this.__hovered.set(newValue);
    }
    private __pressed: ObservedPropertySimplePU<boolean>;
    get pressed() {
        bjccovmshb1ibh.instrumentFunction(156);
        return this.__pressed.get();
    }
    set pressed(newValue: boolean) {
        bjccovmshb1ibh.instrumentFunction(157);
        this.__pressed.set(newValue);
    }
    private setHovered(hovered: boolean): void {
        bjccovmshb1ibh.instrumentFunction(158);
        bjccovmshb1ibh.instrumentRegion(158, 1);
        SettingsTheme.animate(this.getUIContext(), () => {
            bjccovmshb1ibh.instrumentFunction(159);
            bjccovmshb1ibh.instrumentRegion(159, 1);
            this.hovered = hovered;
        });
    }
    initialRender() {
        bjccovmshb1ibh.instrumentFunction(160);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1ibh.instrumentFunction(161);
            Column.create();
            Column.alignItems(HorizontalAlign.Start);
            Column.width('100%');
            Column.padding(16);
            Column.backgroundColor(this.hovered || this.pressed ? (bjccovmshb1ibh.instrumentBranch(161, 0, true), SettingsTheme.cardHoverBackground(this.isDark)) : (bjccovmshb1ibh.instrumentBranch(161, 0, false), SettingsTheme.cardBackground(this.isDark)));
            Column.borderRadius(SettingsTheme.CARD_RADIUS);
            Column.border({
                width: 1,
                color: SettingsTheme.borderColor(this.isDark)
            });
            Column.shadow(SettingsTheme.shadow(this.isDark, this.hovered));
            Column.translate({ y: this.hovered ? (bjccovmshb1ibh.instrumentBranch(161, 1, true), -5) : (bjccovmshb1ibh.instrumentBranch(161, 1, false), 0) });
            Column.scale({
                x: this.pressed ? (bjccovmshb1ibh.instrumentBranch(161, 2, true), 0.99) : (bjccovmshb1ibh.instrumentBranch(161, 2, false), this.hovered ? (bjccovmshb1ibh.instrumentBranch(161, 3, true), 1.012) : (bjccovmshb1ibh.instrumentBranch(161, 3, false), 1)),
                y: this.pressed ? (bjccovmshb1ibh.instrumentBranch(161, 4, true), 0.99) : (bjccovmshb1ibh.instrumentBranch(161, 4, false), this.hovered ? (bjccovmshb1ibh.instrumentBranch(161, 5, true), 1.012) : (bjccovmshb1ibh.instrumentBranch(161, 5, false), 1))
            });
            Column.margin({ bottom: 10 });
            Column.onHover((isHover: boolean) => {
                bjccovmshb1ibh.instrumentFunction(162);
                bjccovmshb1ibh.instrumentRegion(162, 1);
                this.setHovered(isHover);
            });
            Column.onTouch((event: TouchEvent) => {
                bjccovmshb1ibh.instrumentFunction(163);
                if (event.type === TouchType.Down) {
                    bjccovmshb1ibh.instrumentBranch(163, 0, true);
                    bjccovmshb1ibh.instrumentRegion(163, 1);
                    this.pressed = true;
                    bjccovmshb1ibh.instrumentRegion(160, 1);
                    return;
                }
                else {
                    bjccovmshb1ibh.instrumentBranch(163, 0, false);
                }
                if (event.type === TouchType.Up || event.type === TouchType.Cancel) {
                    bjccovmshb1ibh.instrumentBranch(163, 1, true);
                    bjccovmshb1ibh.instrumentRegion(163, 2);
                    this.pressed = false;
                }
                else {
                    bjccovmshb1ibh.instrumentBranch(163, 1, false);
                }
            });
        }, Column);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1ibh.instrumentFunction(164);
            Text.create(this.title);
            Text.fontSize(15);
            Text.fontWeight(FontWeight.Medium);
            Text.fontColor(SettingsTheme.primaryText(this.isDark));
            Text.width('100%');
            Text.margin({ bottom: 8 });
        }, Text);
        Text.pop();
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1ibh.instrumentFunction(165);
            Text.create(this.body);
            Text.fontSize(14);
            Text.fontColor(SettingsTheme.secondaryText(this.isDark));
            Text.lineHeight(22);
            Text.width('100%');
        }, Text);
        Text.pop();
        Column.pop();
    }
    rerender() {
        this.updateDirtyElements();
    }
}
