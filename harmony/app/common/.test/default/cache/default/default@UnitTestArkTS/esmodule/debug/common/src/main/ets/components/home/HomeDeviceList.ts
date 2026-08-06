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
let bjccovmshb1i5l = new BjcCov({ version: "bjc v1.0.0", versionCode: 10000, path: "common/src/main/ets/components/home/HomeDeviceList.ets", hash: "e6942b6a732d694840ddf2dc5e728736bab51d31a12cc605b8c13d6de92aa3c0", lineCnt: 340, count: 0, projectPath: "", functions: { 0: { name: "anonymous_1", count: 0, regions: { 0: { startLoc: { line: 14, col: 20 }, endLoc: { line: 14, col: 47 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 0 }, 1: { name: "anonymous_2", count: 0, regions: { 0: { startLoc: { line: 16, col: 17 }, endLoc: { line: 16, col: 27 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 1 }, 2: { name: "anonymous_3", count: 0, regions: { 0: { startLoc: { line: 14, col: 50 }, endLoc: { line: 15, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 2 }, 3: { name: "anonymous_4", count: 0, regions: { 0: { startLoc: { line: 16, col: 30 }, endLoc: { line: 17, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 3 }, 4: { name: "setInitiallyProvidedValue", count: 0, regions: { 0: { startLoc: { line: 10, col: 34 }, endLoc: { line: 13, col: 32 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 4 }, 5: { name: "updateStateVars", count: 0, regions: { 0: { startLoc: { line: 10, col: 9 }, endLoc: { line: 13, col: 15 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 5 }, 6: { name: "layoutMode", count: 0, regions: { 0: { startLoc: { line: 10, col: 9 }, endLoc: { line: 10, col: 19 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 6 }, 7: { name: "layoutMode", count: 0, regions: { 0: { startLoc: { line: 10, col: 9 }, endLoc: { line: 10, col: 31 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 7 }, 8: { name: "connectionProfiles", count: 0, regions: { 0: { startLoc: { line: 11, col: 9 }, endLoc: { line: 11, col: 27 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 8 }, 9: { name: "connectionProfiles", count: 0, regions: { 0: { startLoc: { line: 11, col: 9 }, endLoc: { line: 11, col: 55 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 9 }, 10: { name: "selectedConnectionProfileId", count: 0, regions: { 0: { startLoc: { line: 12, col: 9 }, endLoc: { line: 12, col: 36 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 10 }, 11: { name: "selectedConnectionProfileId", count: 0, regions: { 0: { startLoc: { line: 12, col: 9 }, endLoc: { line: 12, col: 44 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 11 }, 12: { name: "isDark", count: 0, regions: { 0: { startLoc: { line: 13, col: 9 }, endLoc: { line: 13, col: 15 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 12 }, 13: { name: "isDark", count: 0, regions: { 0: { startLoc: { line: 13, col: 9 }, endLoc: { line: 13, col: 24 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 13 }, 14: { name: "anonymous_5", count: 0, regions: { 0: { startLoc: { line: 14, col: 20 }, endLoc: { line: 14, col: 47 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 14 }, 15: { name: "anonymous_6", count: 0, regions: { 0: { startLoc: { line: 16, col: 17 }, endLoc: { line: 16, col: 27 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 15 }, 16: { name: "profileSearchText", count: 0, regions: { 0: { startLoc: { line: 18, col: 18 }, endLoc: { line: 18, col: 35 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 16 }, 17: { name: "profileSearchText", count: 0, regions: { 0: { startLoc: { line: 18, col: 18 }, endLoc: { line: 18, col: 43 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 17 }, 18: { name: "hoveredProfileId", count: 0, regions: { 0: { startLoc: { line: 19, col: 18 }, endLoc: { line: 19, col: 34 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 18 }, 19: { name: "hoveredProfileId", count: 0, regions: { 0: { startLoc: { line: 19, col: 18 }, endLoc: { line: 19, col: 42 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 19 }, 20: { name: "pressedProfileId", count: 0, regions: { 0: { startLoc: { line: 20, col: 18 }, endLoc: { line: 20, col: 34 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 20 }, 21: { name: "pressedProfileId", count: 0, regions: { 0: { startLoc: { line: 20, col: 18 }, endLoc: { line: 20, col: 42 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 21 }, 22: { name: "newDeviceHovered", count: 0, regions: { 0: { startLoc: { line: 21, col: 18 }, endLoc: { line: 21, col: 34 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 22 }, 23: { name: "newDeviceHovered", count: 0, regions: { 0: { startLoc: { line: 21, col: 18 }, endLoc: { line: 21, col: 43 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 23 }, 24: { name: "newDevicePressed", count: 0, regions: { 0: { startLoc: { line: 22, col: 18 }, endLoc: { line: 22, col: 34 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 24 }, 25: { name: "newDevicePressed", count: 0, regions: { 0: { startLoc: { line: 22, col: 18 }, endLoc: { line: 22, col: 43 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 25 }, 26: { name: "hasConnectionProfiles", count: 0, regions: { 0: { startLoc: { line: 24, col: 3 }, endLoc: { line: 26, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 25, col: 5 }, endLoc: { line: 26, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 26 }, 27: { name: "isSelectedProfile", count: 0, regions: { 0: { startLoc: { line: 28, col: 3 }, endLoc: { line: 30, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 29, col: 5 }, endLoc: { line: 30, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 27 }, 28: { name: "profileName", count: 0, regions: { 0: { startLoc: { line: 32, col: 3 }, endLoc: { line: 36, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 33, col: 5 }, endLoc: { line: 36, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 33, col: 18 }, endLoc: { line: 34, col: 62 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 34, col: 8 }, endLoc: { line: 34, col: 61 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 2: { startLoc: { line: 35, col: 12 }, endLoc: { line: 35, col: 67 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 28 }, 29: { name: "profileAddress", count: 0, regions: { 0: { startLoc: { line: 38, col: 3 }, endLoc: { line: 40, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 39, col: 5 }, endLoc: { line: 40, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 29 }, 30: { name: "normalizedSearchText", count: 0, regions: { 0: { startLoc: { line: 42, col: 3 }, endLoc: { line: 44, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 43, col: 5 }, endLoc: { line: 44, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 30 }, 31: { name: "profileVisible", count: 0, regions: { 0: { startLoc: { line: 46, col: 3 }, endLoc: { line: 53, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 48, col: 30 }, endLoc: { line: 50, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 51, col: 5 }, endLoc: { line: 53, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 48, col: 9 }, endLoc: { line: 48, col: 28 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 31 }, 32: { name: "visibleProfileCount", count: 0, regions: { 0: { startLoc: { line: 55, col: 3 }, endLoc: { line: 63, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 57, col: 5 }, endLoc: { line: 61, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 58, col: 41 }, endLoc: { line: 60, col: 8 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 62, col: 5 }, endLoc: { line: 63, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 58, col: 11 }, endLoc: { line: 58, col: 39 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 32 }, 33: { name: "setHoveredProfile", count: 0, regions: { 0: { startLoc: { line: 65, col: 3 }, endLoc: { line: 69, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 66, col: 5 }, endLoc: { line: 69, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 33 }, 34: { name: "anonymous_7", count: 0, regions: { 0: { startLoc: { line: 66, col: 48 }, endLoc: { line: 68, col: 6 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 67, col: 7 }, endLoc: { line: 68, col: 6 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 67, col: 31 }, endLoc: { line: 67, col: 55 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 34 }, 35: { name: "buildNewDeviceButton", count: 0, regions: { 0: { startLoc: { line: 71, col: 3 }, endLoc: { line: 108, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 98, col: 9 }, endLoc: { line: 99, col: 8 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 106, col: 7 }, endLoc: { line: 107, col: 6 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 35 }, 36: { name: "anonymous_8", count: 0, regions: { 0: { startLoc: { line: 73, col: 5 }, endLoc: { line: 107, col: 6 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 91, col: 17 }, endLoc: { line: 91, col: 49 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 91, col: 54 }, endLoc: { line: 91, col: 86 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 36 }, 37: { name: "anonymous_9", count: 0, regions: { 0: { startLoc: { line: 92, col: 14 }, endLoc: { line: 94, col: 6 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 93, col: 7 }, endLoc: { line: 94, col: 6 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 37 }, 38: { name: "anonymous_10", count: 0, regions: { 0: { startLoc: { line: 95, col: 14 }, endLoc: { line: 103, col: 6 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 96, col: 42 }, endLoc: { line: 99, col: 8 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 100, col: 75 }, endLoc: { line: 102, col: 8 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 96, col: 11 }, endLoc: { line: 96, col: 40 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 100, col: 11 }, endLoc: { line: 100, col: 73 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 38 }, 39: { name: "anonymous_11", count: 0, regions: { 0: { startLoc: { line: 104, col: 14 }, endLoc: { line: 107, col: 6 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 39 }, 40: { name: "anonymous_12", count: 0, regions: { 0: { startLoc: { line: 74, col: 7 }, endLoc: { line: 78, col: 25 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 40 }, 41: { name: "anonymous_13", count: 0, regions: { 0: { startLoc: { line: 79, col: 7 }, endLoc: { line: 83, col: 20 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 41 }, 42: { name: "buildSearchBox", count: 0, regions: { 0: { startLoc: { line: 110, col: 3 }, endLoc: { line: 141, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 42 }, 43: { name: "anonymous_14", count: 0, regions: { 0: { startLoc: { line: 112, col: 5 }, endLoc: { line: 140, col: 6 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 43 }, 44: { name: "anonymous_15", count: 0, regions: { 0: { startLoc: { line: 113, col: 7 }, endLoc: { line: 117, col: 25 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 44 }, 45: { name: "anonymous_16", count: 0, regions: { 0: { startLoc: { line: 118, col: 7 }, endLoc: { line: 128, col: 10 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 45 }, 46: { name: "anonymous_17", count: 0, regions: { 0: { startLoc: { line: 126, col: 19 }, endLoc: { line: 128, col: 10 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 127, col: 11 }, endLoc: { line: 128, col: 10 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 46 }, 47: { name: "buildDeviceTag", count: 0, regions: { 0: { startLoc: { line: 143, col: 3 }, endLoc: { line: 154, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 47 }, 48: { name: "anonymous_18", count: 0, regions: { 0: { startLoc: { line: 145, col: 5 }, endLoc: { line: 153, col: 18 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 48 }, 49: { name: "buildDeviceIcon", count: 0, regions: { 0: { startLoc: { line: 156, col: 3 }, endLoc: { line: 172, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 49 }, 50: { name: "anonymous_19", count: 0, regions: { 0: { startLoc: { line: 158, col: 5 }, endLoc: { line: 171, col: 66 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 170, col: 22 }, endLoc: { line: 171, col: 66 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 50 }, 51: { name: "anonymous_20", count: 0, regions: { 0: { startLoc: { line: 159, col: 7 }, endLoc: { line: 163, col: 25 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 51 }, 52: { name: "buildDeviceCard", count: 0, regions: { 0: { startLoc: { line: 174, col: 3 }, endLoc: { line: 256, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 246, col: 9 }, endLoc: { line: 247, col: 8 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 254, col: 7 }, endLoc: { line: 255, col: 6 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 52 }, 53: { name: "anonymous_21", count: 0, regions: { 0: { startLoc: { line: 176, col: 5 }, endLoc: { line: 255, col: 6 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 218, col: 22 }, endLoc: { line: 222, col: 46 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 225, col: 14 }, endLoc: { line: 225, col: 53 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 2: { startLoc: { line: 226, col: 14 }, endLoc: { line: 228, col: 51 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 3: { startLoc: { line: 227, col: 10 }, endLoc: { line: 228, col: 50 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 4: { startLoc: { line: 231, col: 15 }, endLoc: { line: 231, col: 59 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 5: { startLoc: { line: 232, col: 14 }, endLoc: { line: 232, col: 53 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 6: { startLoc: { line: 234, col: 16 }, endLoc: { line: 234, col: 60 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 7: { startLoc: { line: 237, col: 10 }, endLoc: { line: 237, col: 57 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 8: { startLoc: { line: 238, col: 10 }, endLoc: { line: 238, col: 57 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 53 }, 54: { name: "anonymous_22", count: 0, regions: { 0: { startLoc: { line: 240, col: 14 }, endLoc: { line: 242, col: 6 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 241, col: 7 }, endLoc: { line: 242, col: 6 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 54 }, 55: { name: "anonymous_23", count: 0, regions: { 0: { startLoc: { line: 243, col: 14 }, endLoc: { line: 251, col: 6 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 244, col: 42 }, endLoc: { line: 247, col: 8 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 248, col: 75 }, endLoc: { line: 250, col: 8 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 244, col: 11 }, endLoc: { line: 244, col: 40 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 248, col: 11 }, endLoc: { line: 248, col: 73 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 55 }, 56: { name: "anonymous_24", count: 0, regions: { 0: { startLoc: { line: 252, col: 14 }, endLoc: { line: 255, col: 6 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 56 }, 57: { name: "anonymous_25", count: 0, regions: { 0: { startLoc: { line: 179, col: 7 }, endLoc: { line: 205, col: 40 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 57 }, 58: { name: "anonymous_26", count: 0, regions: { 0: { startLoc: { line: 180, col: 9 }, endLoc: { line: 186, col: 24 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 58 }, 59: { name: "anonymous_27", count: 0, regions: { 0: { startLoc: { line: 187, col: 9 }, endLoc: { line: 192, col: 24 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 59 }, 60: { name: "anonymous_28", count: 0, regions: { 0: { startLoc: { line: 193, col: 9 }, endLoc: { line: 202, col: 41 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 60 }, 61: { name: "anonymous_29", count: 0, regions: { 0: { startLoc: { line: 194, col: 11 }, endLoc: { line: 196, col: 12 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 194, col: 48 }, endLoc: { line: 196, col: 12 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 194, col: 15 }, endLoc: { line: 194, col: 46 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 61 }, 62: { name: "anonymous_30", count: 0, regions: { 0: { startLoc: { line: 195, col: 13 }, endLoc: { line: 195, col: 60 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 62 }, 63: { name: "anonymous_32", count: 0, regions: { 0: { startLoc: { line: 197, col: 11 }, endLoc: { line: 199, col: 12 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 197, col: 41 }, endLoc: { line: 199, col: 12 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 197, col: 15 }, endLoc: { line: 197, col: 39 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 63 }, 64: { name: "anonymous_33", count: 0, regions: { 0: { startLoc: { line: 198, col: 13 }, endLoc: { line: 198, col: 68 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 64 }, 65: { name: "anonymous_35", count: 0, regions: { 0: { startLoc: { line: 207, col: 7 }, endLoc: { line: 212, col: 25 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 210, col: 20 }, endLoc: { line: 211, col: 47 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 65 }, 66: { name: "buildDeviceEmptyState", count: 0, regions: { 0: { startLoc: { line: 258, col: 3 }, endLoc: { line: 283, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 66 }, 67: { name: "anonymous_36", count: 0, regions: { 0: { startLoc: { line: 260, col: 5 }, endLoc: { line: 282, col: 6 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 67 }, 68: { name: "anonymous_37", count: 0, regions: { 0: { startLoc: { line: 261, col: 7 }, endLoc: { line: 266, col: 58 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 68 }, 69: { name: "anonymous_38", count: 0, regions: { 0: { startLoc: { line: 267, col: 7 }, endLoc: { line: 271, col: 58 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 69 }, 70: { name: "initialRender", count: 0, regions: { 0: { startLoc: { line: 285, col: 3 }, endLoc: { line: 338, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 70 }, 71: { name: "anonymous_39", count: 0, regions: { 0: { startLoc: { line: 286, col: 5 }, endLoc: { line: 337, col: 6 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 328, col: 12 }, endLoc: { line: 328, col: 67 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 71 }, 72: { name: "anonymous_40", count: 0, regions: { 0: { startLoc: { line: 287, col: 7 }, endLoc: { line: 298, col: 39 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 72 }, 73: { name: "anonymous_41", count: 0, regions: { 0: { startLoc: { line: 288, col: 9 }, endLoc: { line: 293, col: 26 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 73 }, 74: { name: "anonymous_42", count: 0, regions: { 0: { startLoc: { line: 302, col: 7 }, endLoc: { line: 326, col: 24 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 74 }, 75: { name: "anonymous_43", count: 0, regions: { 0: { startLoc: { line: 303, col: 9 }, endLoc: { line: 321, col: 41 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 75 }, 76: { name: "anonymous_44", count: 0, regions: { 0: { startLoc: { line: 304, col: 11 }, endLoc: { line: 318, col: 45 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 76 }, 77: { name: "anonymous_45", count: 0, regions: { 0: { startLoc: { line: 305, col: 13 }, endLoc: { line: 315, col: 14 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 305, col: 48 }, endLoc: { line: 307, col: 14 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 307, col: 20 }, endLoc: { line: 315, col: 14 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 307, col: 58 }, endLoc: { line: 309, col: 14 }, count: 0, ignored: 0 }, 4: { startLoc: { line: 309, col: 20 }, endLoc: { line: 315, col: 14 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 305, col: 17 }, endLoc: { line: 305, col: 46 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 307, col: 24 }, endLoc: { line: 307, col: 56 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 77 }, 78: { name: "anonymous_46", count: 0, regions: { 0: { startLoc: { line: 306, col: 15 }, endLoc: { line: 306, col: 98 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 78 }, 79: { name: "anonymous_47", count: 0, regions: { 0: { startLoc: { line: 308, col: 15 }, endLoc: { line: 308, col: 112 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 79 }, 80: { name: "anonymous_48", count: 0, regions: { 0: { startLoc: { line: 310, col: 15 }, endLoc: { line: 310, col: 22 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 80 }, 81: { name: "anonymous_49", count: 0, regions: { 0: { startLoc: { line: 310, col: 15 }, endLoc: { line: 314, col: 67 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 81 }, 82: { name: "anonymous_50", count: 0, regions: { 0: { startLoc: { line: 311, col: 17 }, endLoc: { line: 313, col: 18 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 82 }, 83: { name: "anonymous_51", count: 0, regions: { 0: { startLoc: { line: 311, col: 17 }, endLoc: { line: 313, col: 18 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 311, col: 51 }, endLoc: { line: 313, col: 18 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 311, col: 21 }, endLoc: { line: 311, col: 49 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 83 }, 84: { name: "anonymous_52", count: 0, regions: { 0: { startLoc: { line: 312, col: 19 }, endLoc: { line: 312, col: 48 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 84 }, 85: { name: "anonymous_54", count: 0, regions: { 0: { startLoc: { line: 314, col: 18 }, endLoc: { line: 314, col: 67 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 85 } }, exeLine: { 0: 1, 1: 2, 2: 3, 3: 4, 4: 5, 5: 6, 6: 9, 7: 10, 8: 11, 9: 12, 10: 13, 11: 14, 12: 16, 13: 18, 14: 19, 15: 20, 16: 21, 17: 22, 18: 24, 19: 25, 20: 28, 21: 29, 22: 32, 23: 33, 24: 34, 25: 35, 26: 38, 27: 39, 28: 42, 29: 43, 30: 46, 31: 47, 32: 48, 33: 49, 34: 51, 35: 52, 36: 55, 37: 56, 38: 57, 39: 58, 40: 59, 41: 62, 42: 65, 43: 66, 44: 67, 45: 72, 46: 73, 47: 74, 48: 75, 49: 76, 50: 77, 51: 78, 52: 79, 53: 80, 54: 81, 55: 82, 56: 83, 57: 85, 58: 86, 59: 87, 60: 88, 61: 89, 62: 90, 63: 91, 64: 92, 65: 93, 66: 95, 67: 96, 68: 97, 69: 98, 70: 100, 71: 101, 72: 104, 73: 105, 74: 106, 75: 111, 76: 112, 77: 113, 78: 114, 79: 115, 80: 116, 81: 117, 82: 118, 83: 119, 84: 120, 85: 121, 86: 122, 87: 123, 88: 124, 89: 125, 90: 126, 91: 127, 92: 130, 93: 131, 94: 132, 95: 133, 96: 134, 97: 135, 98: 136, 99: 137, 100: 138, 101: 139, 102: 144, 103: 145, 104: 146, 105: 147, 106: 148, 107: 149, 108: 150, 109: 151, 110: 152, 111: 153, 112: 157, 113: 158, 114: 159, 115: 160, 116: 161, 117: 162, 118: 163, 119: 165, 120: 166, 121: 167, 122: 168, 123: 169, 124: 170, 125: 171, 126: 175, 127: 176, 128: 177, 129: 179, 130: 180, 131: 181, 132: 182, 133: 183, 134: 184, 135: 185, 136: 186, 137: 187, 138: 188, 139: 189, 140: 190, 141: 191, 142: 192, 143: 193, 144: 194, 145: 195, 146: 197, 147: 198, 148: 201, 149: 202, 150: 204, 151: 205, 152: 207, 153: 208, 154: 209, 155: 210, 156: 211, 157: 212, 158: 214, 159: 215, 160: 216, 161: 217, 162: 218, 163: 219, 164: 220, 165: 221, 166: 222, 167: 223, 168: 224, 169: 225, 170: 226, 171: 227, 172: 228, 173: 230, 174: 231, 175: 232, 176: 233, 177: 234, 178: 236, 179: 237, 180: 238, 181: 240, 182: 241, 183: 243, 184: 244, 185: 245, 186: 246, 187: 248, 188: 249, 189: 252, 190: 253, 191: 254, 192: 259, 193: 260, 194: 261, 195: 262, 196: 263, 197: 264, 198: 265, 199: 266, 200: 267, 201: 268, 202: 269, 203: 270, 204: 271, 205: 273, 206: 274, 207: 275, 208: 276, 209: 277, 210: 278, 211: 279, 212: 280, 213: 281, 214: 285, 215: 286, 216: 287, 217: 288, 218: 289, 219: 290, 220: 291, 221: 292, 222: 293, 223: 295, 224: 297, 225: 298, 226: 300, 227: 302, 228: 303, 229: 304, 230: 305, 231: 306, 232: 307, 233: 308, 234: 309, 235: 310, 236: 311, 237: 312, 238: 314, 239: 317, 240: 318, 241: 320, 242: 321, 243: 323, 244: 324, 245: 325, 246: 326, 247: 328, 248: 329, 249: 330, 250: 331, 251: 332, 252: 333, 253: 334, 254: 335, 255: 336 } });
if (!("finalizeConstruction" in ViewPU.prototype)) {
    Reflect.set(ViewPU.prototype, "finalizeConstruction", () => { });
}
interface HomeDeviceList_Params {
    layoutMode?: LayoutMode;
    connectionProfiles?: WindowsConnectionProfile[];
    selectedConnectionProfileId?: string;
    isDark?: boolean;
    onProfileSelect?: (profileId: string) => void;
    onNewProfile?: () => void;
    profileSearchText?: string;
    hoveredProfileId?: string;
    pressedProfileId?: string;
    newDeviceHovered?: boolean;
    newDevicePressed?: boolean;
}
import { LayoutMode } from "@normalized:N&&&common/src/main/ets/adaptive/WindowLayoutPolicy&";
import { SettingsAccent, SettingsTheme } from "@normalized:N&&&common/src/main/ets/components/settings/SettingsUi&";
import type { WindowsConnectionProfile } from '../../rdp/WindowsConnectionStore';
import { HomeResources } from "@normalized:N&&&common/src/main/ets/components/home/HomeResources&";
import { HomeText } from "@normalized:N&&&common/src/main/ets/components/home/HomeText&";
import { HomeTheme } from "@normalized:N&&&common/src/main/ets/components/home/HomeTheme&";
export class HomeDeviceList extends ViewPU {
    constructor(parent, params, __localStorage, elmtId = -1, paramsLambda = undefined, extraInfo) {
        super(parent, __localStorage, elmtId, extraInfo);
        if (typeof paramsLambda === "function") {
            this.paramsGenerator_ = paramsLambda;
        }
        this.__layoutMode = new SynchedPropertySimpleOneWayPU(params.layoutMode, this, "layoutMode");
        this.__connectionProfiles = new SynchedPropertyObjectOneWayPU(params.connectionProfiles, this, "connectionProfiles");
        this.__selectedConnectionProfileId = new SynchedPropertySimpleOneWayPU(params.selectedConnectionProfileId, this, "selectedConnectionProfileId");
        this.__isDark = new SynchedPropertySimpleOneWayPU(params.isDark, this, "isDark");
        this.onProfileSelect = (_profileId: string) => {
            bjccovmshb1i5l.instrumentFunction(2);
        };
        this.onNewProfile = () => {
            bjccovmshb1i5l.instrumentFunction(3);
        };
        this.__profileSearchText = new ObservedPropertySimplePU('', this, "profileSearchText");
        this.__hoveredProfileId = new ObservedPropertySimplePU('', this, "hoveredProfileId");
        this.__pressedProfileId = new ObservedPropertySimplePU('', this, "pressedProfileId");
        this.__newDeviceHovered = new ObservedPropertySimplePU(false, this, "newDeviceHovered");
        this.__newDevicePressed = new ObservedPropertySimplePU(false, this, "newDevicePressed");
        this.setInitiallyProvidedValue(params);
        this.finalizeConstruction();
    }
    setInitiallyProvidedValue(params: HomeDeviceList_Params) {
        bjccovmshb1i5l.instrumentFunction(4);
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
        if (params.isDark === undefined) {
            this.__isDark.set(false);
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
        if (params.profileSearchText !== undefined) {
            this.profileSearchText = params.profileSearchText;
        }
        else {
        }
        if (params.hoveredProfileId !== undefined) {
            this.hoveredProfileId = params.hoveredProfileId;
        }
        else {
        }
        if (params.pressedProfileId !== undefined) {
            this.pressedProfileId = params.pressedProfileId;
        }
        else {
        }
        if (params.newDeviceHovered !== undefined) {
            this.newDeviceHovered = params.newDeviceHovered;
        }
        else {
        }
        if (params.newDevicePressed !== undefined) {
            this.newDevicePressed = params.newDevicePressed;
        }
        else {
        }
    }
    updateStateVars(params: HomeDeviceList_Params) {
        bjccovmshb1i5l.instrumentFunction(5);
        this.__layoutMode.reset(params.layoutMode);
        this.__connectionProfiles.reset(params.connectionProfiles);
        this.__selectedConnectionProfileId.reset(params.selectedConnectionProfileId);
        this.__isDark.reset(params.isDark);
    }
    purgeVariableDependenciesOnElmtId(rmElmtId) {
        this.__layoutMode.purgeDependencyOnElmtId(rmElmtId);
        this.__connectionProfiles.purgeDependencyOnElmtId(rmElmtId);
        this.__selectedConnectionProfileId.purgeDependencyOnElmtId(rmElmtId);
        this.__isDark.purgeDependencyOnElmtId(rmElmtId);
        this.__profileSearchText.purgeDependencyOnElmtId(rmElmtId);
        this.__hoveredProfileId.purgeDependencyOnElmtId(rmElmtId);
        this.__pressedProfileId.purgeDependencyOnElmtId(rmElmtId);
        this.__newDeviceHovered.purgeDependencyOnElmtId(rmElmtId);
        this.__newDevicePressed.purgeDependencyOnElmtId(rmElmtId);
    }
    aboutToBeDeleted() {
        this.__layoutMode.aboutToBeDeleted();
        this.__connectionProfiles.aboutToBeDeleted();
        this.__selectedConnectionProfileId.aboutToBeDeleted();
        this.__isDark.aboutToBeDeleted();
        this.__profileSearchText.aboutToBeDeleted();
        this.__hoveredProfileId.aboutToBeDeleted();
        this.__pressedProfileId.aboutToBeDeleted();
        this.__newDeviceHovered.aboutToBeDeleted();
        this.__newDevicePressed.aboutToBeDeleted();
        SubscriberManager.Get().delete(this.id__());
        this.aboutToBeDeletedInternal();
    }
    private __layoutMode: SynchedPropertySimpleOneWayPU<LayoutMode>;
    get layoutMode() {
        bjccovmshb1i5l.instrumentFunction(6);
        return this.__layoutMode.get();
    }
    set layoutMode(newValue: LayoutMode) {
        bjccovmshb1i5l.instrumentFunction(7);
        this.__layoutMode.set(newValue);
    }
    private __connectionProfiles: SynchedPropertySimpleOneWayPU<WindowsConnectionProfile[]>;
    get connectionProfiles() {
        bjccovmshb1i5l.instrumentFunction(8);
        return this.__connectionProfiles.get();
    }
    set connectionProfiles(newValue: WindowsConnectionProfile[]) {
        bjccovmshb1i5l.instrumentFunction(9);
        this.__connectionProfiles.set(newValue);
    }
    private __selectedConnectionProfileId: SynchedPropertySimpleOneWayPU<string>;
    get selectedConnectionProfileId() {
        bjccovmshb1i5l.instrumentFunction(10);
        return this.__selectedConnectionProfileId.get();
    }
    set selectedConnectionProfileId(newValue: string) {
        bjccovmshb1i5l.instrumentFunction(11);
        this.__selectedConnectionProfileId.set(newValue);
    }
    private __isDark: SynchedPropertySimpleOneWayPU<boolean>;
    get isDark() {
        bjccovmshb1i5l.instrumentFunction(12);
        return this.__isDark.get();
    }
    set isDark(newValue: boolean) {
        bjccovmshb1i5l.instrumentFunction(13);
        this.__isDark.set(newValue);
    }
    private onProfileSelect: (profileId: string) => void;
    private onNewProfile: () => void;
    private __profileSearchText: ObservedPropertySimplePU<string>;
    get profileSearchText() {
        bjccovmshb1i5l.instrumentFunction(16);
        return this.__profileSearchText.get();
    }
    set profileSearchText(newValue: string) {
        bjccovmshb1i5l.instrumentFunction(17);
        this.__profileSearchText.set(newValue);
    }
    private __hoveredProfileId: ObservedPropertySimplePU<string>;
    get hoveredProfileId() {
        bjccovmshb1i5l.instrumentFunction(18);
        return this.__hoveredProfileId.get();
    }
    set hoveredProfileId(newValue: string) {
        bjccovmshb1i5l.instrumentFunction(19);
        this.__hoveredProfileId.set(newValue);
    }
    private __pressedProfileId: ObservedPropertySimplePU<string>;
    get pressedProfileId() {
        bjccovmshb1i5l.instrumentFunction(20);
        return this.__pressedProfileId.get();
    }
    set pressedProfileId(newValue: string) {
        bjccovmshb1i5l.instrumentFunction(21);
        this.__pressedProfileId.set(newValue);
    }
    private __newDeviceHovered: ObservedPropertySimplePU<boolean>;
    get newDeviceHovered() {
        bjccovmshb1i5l.instrumentFunction(22);
        return this.__newDeviceHovered.get();
    }
    set newDeviceHovered(newValue: boolean) {
        bjccovmshb1i5l.instrumentFunction(23);
        this.__newDeviceHovered.set(newValue);
    }
    private __newDevicePressed: ObservedPropertySimplePU<boolean>;
    get newDevicePressed() {
        bjccovmshb1i5l.instrumentFunction(24);
        return this.__newDevicePressed.get();
    }
    set newDevicePressed(newValue: boolean) {
        bjccovmshb1i5l.instrumentFunction(25);
        this.__newDevicePressed.set(newValue);
    }
    private hasConnectionProfiles(): boolean {
        bjccovmshb1i5l.instrumentFunction(26);
        bjccovmshb1i5l.instrumentRegion(26, 1);
        return this.connectionProfiles.length > 0;
    }
    private isSelectedProfile(profile: WindowsConnectionProfile): boolean {
        bjccovmshb1i5l.instrumentFunction(27);
        bjccovmshb1i5l.instrumentRegion(27, 1);
        return profile.id === this.selectedConnectionProfileId;
    }
    private profileName(profile: WindowsConnectionProfile): string {
        bjccovmshb1i5l.instrumentFunction(28);
        bjccovmshb1i5l.instrumentRegion(28, 1);
        const name = profile.username.length > 0 ? (bjccovmshb1i5l.instrumentBranch(28, 0, true), profile.username) : (bjccovmshb1i5l.instrumentBranch(28, 0, false), (profile.name.length > 0 ? (bjccovmshb1i5l.instrumentBranch(28, 1, true), profile.name) : (bjccovmshb1i5l.instrumentBranch(28, 1, false), profile.host)));
        return name.length > 20 ? (bjccovmshb1i5l.instrumentBranch(28, 2, true), `${name.substring(0, 20)}...`) : (bjccovmshb1i5l.instrumentBranch(28, 2, false), name);
    }
    private profileAddress(profile: WindowsConnectionProfile): string {
        bjccovmshb1i5l.instrumentFunction(29);
        bjccovmshb1i5l.instrumentRegion(29, 1);
        return `${profile.host}:${profile.port}`;
    }
    private normalizedSearchText(): string {
        bjccovmshb1i5l.instrumentFunction(30);
        bjccovmshb1i5l.instrumentRegion(30, 1);
        return this.profileSearchText.trim().toLowerCase();
    }
    private profileVisible(profile: WindowsConnectionProfile): boolean {
        bjccovmshb1i5l.instrumentFunction(31);
        const search = this.normalizedSearchText();
        if (search.length === 0) {
            bjccovmshb1i5l.instrumentBranch(31, 0, true);
            bjccovmshb1i5l.instrumentRegion(31, 1);
            return true;
        }
        else {
            bjccovmshb1i5l.instrumentBranch(31, 0, false);
        }
        bjccovmshb1i5l.instrumentRegion(31, 2);
        const haystack = `${profile.name} ${profile.host} ${profile.port} ${profile.username}`.toLowerCase();
        return haystack.indexOf(search) >= 0;
    }
    private visibleProfileCount(): number {
        bjccovmshb1i5l.instrumentFunction(32);
        let count = 0;
        for (const profile of this.connectionProfiles) {
            bjccovmshb1i5l.instrumentRegion(32, 1);
            if (this.profileVisible(profile)) {
                bjccovmshb1i5l.instrumentBranch(32, 0, true);
                bjccovmshb1i5l.instrumentRegion(32, 2);
                count++;
            }
            else {
                bjccovmshb1i5l.instrumentBranch(32, 0, false);
            }
        }
        bjccovmshb1i5l.instrumentRegion(32, 3);
        return count;
    }
    private setHoveredProfile(profileId: string, hovered: boolean): void {
        bjccovmshb1i5l.instrumentFunction(33);
        bjccovmshb1i5l.instrumentRegion(33, 1);
        SettingsTheme.animate(this.getUIContext(), () => {
            bjccovmshb1i5l.instrumentFunction(34);
            bjccovmshb1i5l.instrumentRegion(34, 1);
            this.hoveredProfileId = hovered ? (bjccovmshb1i5l.instrumentBranch(34, 0, true), profileId) : (bjccovmshb1i5l.instrumentBranch(34, 0, false), '');
        });
    }
    private buildNewDeviceButton(parent = null) {
        bjccovmshb1i5l.instrumentFunction(35);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i5l.instrumentFunction(36);
            Row.create({ space: 8 });
            Row.padding({ left: 16, right: 16 });
            Row.constraintSize({ minWidth: 136, minHeight: 48 });
            Row.justifyContent(FlexAlign.Center);
            Row.alignItems(VerticalAlign.Center);
            Row.borderRadius(8);
            Row.backgroundColor(HomeTheme.primaryButtonBackground(this.isDark, this.newDeviceHovered, this.newDevicePressed));
            Row.scale({ x: this.newDevicePressed ? (bjccovmshb1i5l.instrumentBranch(36, 0, true), 0.98) : (bjccovmshb1i5l.instrumentBranch(36, 0, false), 1), y: this.newDevicePressed ? (bjccovmshb1i5l.instrumentBranch(36, 1, true), 0.98) : (bjccovmshb1i5l.instrumentBranch(36, 1, false), 1) });
            Row.onHover((hovered: boolean) => {
                bjccovmshb1i5l.instrumentFunction(37);
                bjccovmshb1i5l.instrumentRegion(37, 1);
                this.newDeviceHovered = hovered;
            });
            Row.onTouch((event: TouchEvent) => {
                bjccovmshb1i5l.instrumentFunction(38);
                if (event.type === TouchType.Down) {
                    bjccovmshb1i5l.instrumentBranch(38, 0, true);
                    bjccovmshb1i5l.instrumentRegion(38, 1);
                    this.newDevicePressed = true;
                    bjccovmshb1i5l.instrumentRegion(35, 1);
                    return;
                }
                else {
                    bjccovmshb1i5l.instrumentBranch(38, 0, false);
                }
                if (event.type === TouchType.Up || event.type === TouchType.Cancel) {
                    bjccovmshb1i5l.instrumentBranch(38, 1, true);
                    bjccovmshb1i5l.instrumentRegion(38, 2);
                    this.newDevicePressed = false;
                }
                else {
                    bjccovmshb1i5l.instrumentBranch(38, 1, false);
                }
            });
            Row.onClick(() => {
                bjccovmshb1i5l.instrumentFunction(39);
                this.newDevicePressed = false;
                bjccovmshb1i5l.instrumentRegion(35, 2);
                this.onNewProfile();
            });
        }, Row);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i5l.instrumentFunction(40);
            Image.create(HomeResources.ADD_DEVICE_ICON);
            Image.width(20);
            Image.height(20);
            Image.fillColor(Color.White);
            Image.draggable(false);
        }, Image);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i5l.instrumentFunction(41);
            Text.create(HomeText.NEW_DEVICE_ACTION);
            Text.fontSize(14);
            Text.fontColor(Color.White);
            Text.fontWeight(FontWeight.Bold);
            Text.maxLines(1);
        }, Text);
        Text.pop();
        Row.pop();
    }
    private buildSearchBox(parent = null) {
        bjccovmshb1i5l.instrumentFunction(42);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i5l.instrumentFunction(43);
            Row.create({ space: 12 });
            Row.width('94%');
            Row.height(58);
            Row.padding({ left: 16, right: 10 });
            Row.alignItems(VerticalAlign.Center);
            Row.backgroundColor(HomeTheme.searchBackground(this.isDark));
            Row.borderRadius(8);
            Row.border({
                width: 1,
                color: HomeTheme.searchBorderColor(this.isDark),
                style: BorderStyle.Solid
            });
        }, Row);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i5l.instrumentFunction(44);
            Image.create(HomeResources.SEARCH_ICON);
            Image.width(22);
            Image.height(22);
            Image.fillColor(SettingsTheme.accentColor(this.isDark, SettingsAccent.BLUE));
            Image.draggable(false);
        }, Image);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i5l.instrumentFunction(45);
            TextInput.create({ placeholder: HomeText.DEVICE_SEARCH_PLACEHOLDER, text: this.profileSearchText });
            TextInput.height(48);
            TextInput.layoutWeight(1);
            TextInput.fontSize(15);
            TextInput.fontColor(SettingsTheme.primaryText(this.isDark));
            TextInput.placeholderColor(SettingsTheme.mutedText(this.isDark));
            TextInput.backgroundColor('rgba(0,0,0,0)');
            TextInput.borderRadius(0);
            TextInput.onChange((value: string) => {
                bjccovmshb1i5l.instrumentFunction(46);
                bjccovmshb1i5l.instrumentRegion(46, 1);
                this.profileSearchText = value;
            });
        }, TextInput);
        Row.pop();
    }
    private buildDeviceTag(text: string, parent = null) {
        bjccovmshb1i5l.instrumentFunction(47);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i5l.instrumentFunction(48);
            Text.create(text);
            Text.fontSize(10);
            Text.fontWeight(FontWeight.Medium);
            Text.fontColor(SettingsTheme.statusText(this.isDark, 'ok'));
            Text.constraintSize({ minHeight: 18 });
            Text.padding({ left: 6, right: 6 });
            Text.backgroundColor(SettingsTheme.statusBackground(this.isDark, 'ok'));
            Text.borderRadius(6);
            Text.maxLines(1);
        }, Text);
        Text.pop();
    }
    private buildDeviceIcon(selected: boolean, parent = null) {
        bjccovmshb1i5l.instrumentFunction(49);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i5l.instrumentFunction(50);
            Column.create();
            Column.width(36);
            Column.height(36);
            Column.borderRadius(18);
            Column.justifyContent(FlexAlign.Center);
            Column.alignItems(HorizontalAlign.Center);
            Column.backgroundColor(selected ? (bjccovmshb1i5l.instrumentBranch(50, 0, true), SettingsTheme.accentColor(this.isDark, SettingsAccent.BLUE)) : (bjccovmshb1i5l.instrumentBranch(50, 0, false), SettingsTheme.accentColor(this.isDark, SettingsAccent.CYAN)));
        }, Column);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i5l.instrumentFunction(51);
            Image.create(HomeResources.MONITOR_ICON);
            Image.width(20);
            Image.height(20);
            Image.fillColor(Color.White);
            Image.draggable(false);
        }, Image);
        Column.pop();
    }
    private buildDeviceCard(profile: WindowsConnectionProfile, parent = null) {
        bjccovmshb1i5l.instrumentFunction(52);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i5l.instrumentFunction(53);
            Row.create({ space: 8 });
            Row.width('94%');
            Row.constraintSize({ minHeight: 72 });
            Row.padding({ left: 10, right: 8, top: 8, bottom: 8 });
            Row.alignItems(VerticalAlign.Center);
            Row.backgroundColor(this.isSelectedProfile(profile) ? (bjccovmshb1i5l.instrumentBranch(53, 0, true), HomeTheme.activePanelBackground(this.isDark, this.hoveredProfileId === profile.id, this.pressedProfileId === profile.id)) : (bjccovmshb1i5l.instrumentBranch(53, 0, false), HomeTheme.inactivePanelBackground(this.isDark, this.hoveredProfileId === profile.id, this.pressedProfileId === profile.id)));
            Row.borderRadius(8);
            Row.border({
                width: this.isSelectedProfile(profile) ? (bjccovmshb1i5l.instrumentBranch(53, 1, true), 2) : (bjccovmshb1i5l.instrumentBranch(53, 1, false), 1),
                color: this.isSelectedProfile(profile) ? (bjccovmshb1i5l.instrumentBranch(53, 2, true), SettingsTheme.accentColor(this.isDark, SettingsAccent.BLUE)) : (bjccovmshb1i5l.instrumentBranch(53, 2, false), (this.hoveredProfileId === profile.id ? (bjccovmshb1i5l.instrumentBranch(53, 3, true), HomeTheme.searchBorderColor(this.isDark)) : (bjccovmshb1i5l.instrumentBranch(53, 3, false), HomeTheme.mutedBorderColor(this.isDark))))
            });
            Row.shadow({
                radius: this.hoveredProfileId === profile.id ? (bjccovmshb1i5l.instrumentBranch(53, 4, true), 8) : (bjccovmshb1i5l.instrumentBranch(53, 4, false), 0),
                color: this.isDark ? (bjccovmshb1i5l.instrumentBranch(53, 5, true), '#26000000') : (bjccovmshb1i5l.instrumentBranch(53, 5, false), '#10111827'),
                offsetX: 0,
                offsetY: this.hoveredProfileId === profile.id ? (bjccovmshb1i5l.instrumentBranch(53, 6, true), 4) : (bjccovmshb1i5l.instrumentBranch(53, 6, false), 0)
            });
            Row.scale({
                x: this.pressedProfileId === profile.id ? (bjccovmshb1i5l.instrumentBranch(53, 7, true), 0.99) : (bjccovmshb1i5l.instrumentBranch(53, 7, false), 1),
                y: this.pressedProfileId === profile.id ? (bjccovmshb1i5l.instrumentBranch(53, 8, true), 0.99) : (bjccovmshb1i5l.instrumentBranch(53, 8, false), 1)
            });
            Row.onHover((hovered: boolean) => {
                bjccovmshb1i5l.instrumentFunction(54);
                bjccovmshb1i5l.instrumentRegion(54, 1);
                this.setHoveredProfile(profile.id, hovered);
            });
            Row.onTouch((event: TouchEvent) => {
                bjccovmshb1i5l.instrumentFunction(55);
                if (event.type === TouchType.Down) {
                    bjccovmshb1i5l.instrumentBranch(55, 0, true);
                    bjccovmshb1i5l.instrumentRegion(55, 1);
                    this.pressedProfileId = profile.id;
                    bjccovmshb1i5l.instrumentRegion(52, 1);
                    return;
                }
                else {
                    bjccovmshb1i5l.instrumentBranch(55, 0, false);
                }
                if (event.type === TouchType.Up || event.type === TouchType.Cancel) {
                    bjccovmshb1i5l.instrumentBranch(55, 1, true);
                    bjccovmshb1i5l.instrumentRegion(55, 2);
                    this.pressedProfileId = '';
                }
                else {
                    bjccovmshb1i5l.instrumentBranch(55, 1, false);
                }
            });
            Row.onClick(() => {
                bjccovmshb1i5l.instrumentFunction(56);
                this.pressedProfileId = '';
                bjccovmshb1i5l.instrumentRegion(52, 2);
                this.onProfileSelect(profile.id);
            });
        }, Row);
        this.buildDeviceIcon.bind(this)(this.isSelectedProfile(profile));
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i5l.instrumentFunction(57);
            Column.create({ space: 3 });
            Column.layoutWeight(1);
            Column.alignItems(HorizontalAlign.Start);
        }, Column);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i5l.instrumentFunction(58);
            Text.create(this.profileName(profile));
            Text.fontSize(14);
            Text.fontWeight(FontWeight.Bold);
            Text.fontColor(SettingsTheme.primaryText(this.isDark));
            Text.maxLines(1);
            Text.textOverflow({ overflow: TextOverflow.Ellipsis });
            Text.width('100%');
        }, Text);
        Text.pop();
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i5l.instrumentFunction(59);
            Text.create(this.profileAddress(profile));
            Text.fontSize(12);
            Text.fontColor(SettingsTheme.secondaryText(this.isDark));
            Text.maxLines(1);
            Text.textOverflow({ overflow: TextOverflow.Ellipsis });
            Text.width('100%');
        }, Text);
        Text.pop();
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i5l.instrumentFunction(60);
            Row.create({ space: 6 });
            Row.constraintSize({ minHeight: 18 });
            Row.alignItems(VerticalAlign.Center);
        }, Row);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i5l.instrumentFunction(61);
            If.create();
            if (this.isSelectedProfile(profile)) {
                bjccovmshb1i5l.instrumentBranch(61, 0, true);
                bjccovmshb1i5l.instrumentRegion(61, 1);
                this.ifElseBranchUpdateFunction(0, () => {
                    bjccovmshb1i5l.instrumentFunction(62);
                    this.buildDeviceTag.bind(this)(HomeText.DEVICE_TAG_RECENT);
                });
            }
            else {
                bjccovmshb1i5l.instrumentBranch(61, 0, false);
                this.ifElseBranchUpdateFunction(1, () => {
                });
            }
        }, If);
        If.pop();
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i5l.instrumentFunction(63);
            If.create();
            if (profile.rememberPassword) {
                bjccovmshb1i5l.instrumentBranch(63, 0, true);
                bjccovmshb1i5l.instrumentRegion(63, 1);
                this.ifElseBranchUpdateFunction(0, () => {
                    bjccovmshb1i5l.instrumentFunction(64);
                    this.buildDeviceTag.bind(this)(HomeText.DEVICE_TAG_PASSWORD_SAVED);
                });
            }
            else {
                bjccovmshb1i5l.instrumentBranch(63, 0, false);
                this.ifElseBranchUpdateFunction(1, () => {
                });
            }
        }, If);
        If.pop();
        Row.pop();
        Column.pop();
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i5l.instrumentFunction(65);
            Image.create(HomeResources.CHEVRON_RIGHT_ICON);
            Image.width(18);
            Image.height(18);
            Image.fillColor(this.isSelectedProfile(profile) ? (bjccovmshb1i5l.instrumentBranch(65, 0, true), SettingsTheme.accentColor(this.isDark, SettingsAccent.BLUE)) : (bjccovmshb1i5l.instrumentBranch(65, 0, false), SettingsTheme.mutedText(this.isDark)));
            Image.draggable(false);
        }, Image);
        Row.pop();
    }
    private buildDeviceEmptyState(title: string, body: string, parent = null) {
        bjccovmshb1i5l.instrumentFunction(66);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i5l.instrumentFunction(67);
            Column.create({ space: 8 });
            Column.width('94%');
            Column.constraintSize({ minHeight: 90 });
            Column.padding({ left: 12, right: 12, top: 12, bottom: 12 });
            Column.justifyContent(FlexAlign.Center);
            Column.alignItems(HorizontalAlign.Center);
            Column.borderRadius(8);
            Column.border({
                width: 1,
                color: HomeTheme.mutedBorderColor(this.isDark)
            });
        }, Column);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i5l.instrumentFunction(68);
            Text.create(title);
            Text.fontSize(16);
            Text.fontWeight(FontWeight.Bold);
            Text.fontColor(SettingsTheme.primaryText(this.isDark));
            Text.maxLines(2);
            Text.textOverflow({ overflow: TextOverflow.Ellipsis });
        }, Text);
        Text.pop();
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i5l.instrumentFunction(69);
            Text.create(body);
            Text.fontSize(13);
            Text.fontColor(SettingsTheme.mutedText(this.isDark));
            Text.maxLines(2);
            Text.textOverflow({ overflow: TextOverflow.Ellipsis });
        }, Text);
        Text.pop();
        Column.pop();
    }
    initialRender() {
        bjccovmshb1i5l.instrumentFunction(70);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i5l.instrumentFunction(71);
            Column.create({ space: 20 });
            Column.width(this.layoutMode === LayoutMode.COMPACT ? (bjccovmshb1i5l.instrumentBranch(71, 0, true), '100%') : (bjccovmshb1i5l.instrumentBranch(71, 0, false), '31%'));
            Column.height('100%');
            Column.padding({ left: 8, right: 8, top: 24, bottom: 24 });
            Column.alignItems(HorizontalAlign.Center);
            Column.backgroundColor(HomeTheme.panelBackground(this.isDark));
            Column.borderRadius(8);
            Column.border({
                width: 1,
                color: HomeTheme.mutedBorderColor(this.isDark)
            });
        }, Column);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i5l.instrumentFunction(72);
            Row.create();
            Row.width('94%');
            Row.alignItems(VerticalAlign.Center);
        }, Row);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i5l.instrumentFunction(73);
            Text.create(HomeText.DEVICE_LIST_TITLE);
            Text.fontSize(24);
            Text.fontWeight(FontWeight.Bold);
            Text.fontColor(SettingsTheme.primaryText(this.isDark));
            Text.maxLines(1);
            Text.layoutWeight(1);
        }, Text);
        Text.pop();
        this.buildNewDeviceButton.bind(this)();
        Row.pop();
        this.buildSearchBox.bind(this)();
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i5l.instrumentFunction(74);
            Scroll.create();
            Scroll.width('100%');
            Scroll.layoutWeight(1);
            Scroll.scrollBar(BarState.Auto);
            Scroll.scrollBarWidth(4);
        }, Scroll);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i5l.instrumentFunction(75);
            Row.create();
            Row.width('100%');
            Row.justifyContent(FlexAlign.Center);
        }, Row);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i5l.instrumentFunction(76);
            Column.create({ space: 10 });
            Column.width('100%');
            Column.alignItems(HorizontalAlign.Center);
        }, Column);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i5l.instrumentFunction(77);
            If.create();
            if (!this.hasConnectionProfiles()) {
                bjccovmshb1i5l.instrumentBranch(77, 0, true);
                bjccovmshb1i5l.instrumentRegion(77, 1);
                this.ifElseBranchUpdateFunction(0, () => {
                    bjccovmshb1i5l.instrumentFunction(78);
                    this.buildDeviceEmptyState.bind(this)(HomeText.DEVICE_EMPTY_TITLE, HomeText.DEVICE_EMPTY_BODY);
                });
            }
            else {
                bjccovmshb1i5l.instrumentBranch(77, 0, false);
                bjccovmshb1i5l.instrumentRegion(77, 2);
                if (this.visibleProfileCount() === 0) {
                    bjccovmshb1i5l.instrumentBranch(77, 1, true);
                    bjccovmshb1i5l.instrumentRegion(77, 3);
                    this.ifElseBranchUpdateFunction(1, () => {
                        bjccovmshb1i5l.instrumentFunction(79);
                        this.buildDeviceEmptyState.bind(this)(HomeText.DEVICE_SEARCH_EMPTY_TITLE, HomeText.DEVICE_SEARCH_EMPTY_BODY);
                    });
                }
                else {
                    bjccovmshb1i5l.instrumentBranch(77, 1, false);
                    bjccovmshb1i5l.instrumentRegion(77, 4);
                    this.ifElseBranchUpdateFunction(2, () => {
                        bjccovmshb1i5l.instrumentFunction(80);
                        this.observeComponentCreation2((elmtId, isInitialRender) => {
                            bjccovmshb1i5l.instrumentFunction(81);
                            ForEach.create();
                            const forEachItemGenFunction = _item => {
                                bjccovmshb1i5l.instrumentFunction(82);
                                const profile = _item;
                                this.observeComponentCreation2((elmtId, isInitialRender) => {
                                    bjccovmshb1i5l.instrumentFunction(83);
                                    If.create();
                                    if (this.profileVisible(profile)) {
                                        bjccovmshb1i5l.instrumentBranch(83, 0, true);
                                        bjccovmshb1i5l.instrumentRegion(83, 1);
                                        this.ifElseBranchUpdateFunction(0, () => {
                                            bjccovmshb1i5l.instrumentFunction(84);
                                            this.buildDeviceCard.bind(this)(profile);
                                        });
                                    }
                                    else {
                                        bjccovmshb1i5l.instrumentBranch(83, 0, false);
                                        this.ifElseBranchUpdateFunction(1, () => {
                                        });
                                    }
                                }, If);
                                If.pop();
                            };
                            this.forEachUpdateFunction(elmtId, this.connectionProfiles, forEachItemGenFunction, (profile: WindowsConnectionProfile) => { bjccovmshb1i5l.instrumentFunction(85); return profile.id; }, false, false);
                        }, ForEach);
                        ForEach.pop();
                    });
                }
            }
        }, If);
        If.pop();
        Column.pop();
        Row.pop();
        Scroll.pop();
        Column.pop();
    }
    rerender() {
        this.updateDirtyElements();
    }
}
