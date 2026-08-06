import type common from "@ohos:app.ability.common";
import preferences from "@ohos:data.preferences";
import util from "@ohos:util";
import asset from "@ohos:security.asset";
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
let bjccovmshb1i31 = new BjcCov({ version: "bjc v1.0.0", versionCode: 10000, path: "common/src/main/ets/rdp/WindowsConnectionStore.ets", hash: "bc00b6496c8c5866a0c714743950da25d14b3c6709857a2312ecad35115c42b9", lineCnt: 449, count: 0, projectPath: "", functions: { 0: { name: "WindowsConnectionStore.constructor", count: 0, regions: { 0: { startLoc: { line: 49, col: 3 }, endLoc: { line: 51, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 50, col: 5 }, endLoc: { line: 51, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 0 }, 1: { name: "WindowsConnectionStore.loadSnapshot", count: 0, regions: { 0: { startLoc: { line: 53, col: 3 }, endLoc: { line: 63, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 54, col: 5 }, endLoc: { line: 63, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 57, col: 30 }, endLoc: { line: 57, col: 112 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 1 }, 2: { name: "WindowsConnectionStore.loadPassword", count: 0, regions: { 0: { startLoc: { line: 65, col: 3 }, endLoc: { line: 67, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 66, col: 5 }, endLoc: { line: 67, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 2 }, 3: { name: "WindowsConnectionStore.saveProfile", count: 0, regions: { 0: { startLoc: { line: 69, col: 3 }, endLoc: { line: 88, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 73, col: 28 }, endLoc: { line: 75, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 77, col: 5 }, endLoc: { line: 88, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 73, col: 9 }, endLoc: { line: 73, col: 26 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 80, col: 21 }, endLoc: { line: 82, col: 99 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 3 }, 4: { name: "WindowsConnectionStore.deleteProfile", count: 0, regions: { 0: { startLoc: { line: 90, col: 3 }, endLoc: { line: 101, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 93, col: 26 }, endLoc: { line: 95, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 97, col: 5 }, endLoc: { line: 101, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 93, col: 9 }, endLoc: { line: 93, col: 24 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 4 }, 5: { name: "anonymous_0", count: 0, regions: { 0: { startLoc: { line: 97, col: 42 }, endLoc: { line: 97, col: 114 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 5 }, 6: { name: "WindowsConnectionStore.clearPassword", count: 0, regions: { 0: { startLoc: { line: 103, col: 3 }, endLoc: { line: 116, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 106, col: 26 }, endLoc: { line: 108, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 110, col: 5 }, endLoc: { line: 116, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 106, col: 9 }, endLoc: { line: 106, col: 24 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 6 }, 7: { name: "WindowsConnectionStore.getPreferences", count: 0, regions: { 0: { startLoc: { line: 118, col: 3 }, endLoc: { line: 130, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 119, col: 41 }, endLoc: { line: 121, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 123, col: 9 }, endLoc: { line: 126, col: 6 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 126, col: 7 }, endLoc: { line: 129, col: 6 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 119, col: 9 }, endLoc: { line: 119, col: 39 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 7 }, 8: { name: "WindowsConnectionStore.loadProfiles", count: 0, regions: { 0: { startLoc: { line: 132, col: 3 }, endLoc: { line: 147, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 134, col: 25 }, endLoc: { line: 136, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 140, col: 5 }, endLoc: { line: 145, col: 6 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 142, col: 29 }, endLoc: { line: 144, col: 8 }, count: 0, ignored: 0 }, 4: { startLoc: { line: 146, col: 5 }, endLoc: { line: 147, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 134, col: 9 }, endLoc: { line: 134, col: 23 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 142, col: 11 }, endLoc: { line: 142, col: 27 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 8 }, 9: { name: "WindowsConnectionStore.loadProfileIds", count: 0, regions: { 0: { startLoc: { line: 149, col: 3 }, endLoc: { line: 164, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 150, col: 9 }, endLoc: { line: 160, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 160, col: 7 }, endLoc: { line: 163, col: 6 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 152, col: 36 }, endLoc: { line: 154, col: 8 }, count: 0, ignored: 0 }, 4: { startLoc: { line: 156, col: 35 }, endLoc: { line: 158, col: 8 }, count: 0, ignored: 0 }, 5: { startLoc: { line: 159, col: 7 }, endLoc: { line: 160, col: 6 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 152, col: 11 }, endLoc: { line: 152, col: 34 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 156, col: 11 }, endLoc: { line: 156, col: 33 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 9 }, 10: { name: "anonymous_1", count: 0, regions: { 0: { startLoc: { line: 159, col: 28 }, endLoc: { line: 159, col: 113 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 10 }, 11: { name: "WindowsConnectionStore.loadProfile", count: 0, regions: { 0: { startLoc: { line: 166, col: 3 }, endLoc: { line: 177, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 167, col: 9 }, endLoc: { line: 173, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 173, col: 7 }, endLoc: { line: 176, col: 6 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 169, col: 56 }, endLoc: { line: 171, col: 8 }, count: 0, ignored: 0 }, 4: { startLoc: { line: 172, col: 7 }, endLoc: { line: 173, col: 6 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 169, col: 11 }, endLoc: { line: 169, col: 54 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 11 }, 12: { name: "WindowsConnectionStore.resolveSelectedProfileId", count: 0, regions: { 0: { startLoc: { line: 179, col: 3 }, endLoc: { line: 196, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 180, col: 32 }, endLoc: { line: 182, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 185, col: 25 }, endLoc: { line: 194, col: 6 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 186, col: 11 }, endLoc: { line: 191, col: 8 }, count: 0, ignored: 0 }, 4: { startLoc: { line: 191, col: 9 }, endLoc: { line: 193, col: 8 }, count: 0, ignored: 0 }, 5: { startLoc: { line: 188, col: 104 }, endLoc: { line: 190, col: 10 }, count: 0, ignored: 0 }, 6: { startLoc: { line: 195, col: 5 }, endLoc: { line: 196, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 180, col: 9 }, endLoc: { line: 180, col: 30 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 185, col: 9 }, endLoc: { line: 185, col: 23 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 2: { startLoc: { line: 188, col: 13 }, endLoc: { line: 188, col: 102 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 12 }, 13: { name: "WindowsConnectionStore.persistProfiles", count: 0, regions: { 0: { startLoc: { line: 198, col: 3 }, endLoc: { line: 226, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 200, col: 25 }, endLoc: { line: 202, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 208, col: 9 }, endLoc: { line: 223, col: 6 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 223, col: 7 }, endLoc: { line: 225, col: 6 }, count: 0, ignored: 0 }, 4: { startLoc: { line: 215, col: 7 }, endLoc: { line: 217, col: 8 }, count: 0, ignored: 0 }, 5: { startLoc: { line: 218, col: 7 }, endLoc: { line: 221, col: 8 }, count: 0, ignored: 0 }, 6: { startLoc: { line: 222, col: 7 }, endLoc: { line: 223, col: 6 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 200, col: 9 }, endLoc: { line: 200, col: 23 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 212, col: 9 }, endLoc: { line: 213, col: 84 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 13 }, 14: { name: "anonymous_2", count: 0, regions: { 0: { startLoc: { line: 209, col: 45 }, endLoc: { line: 209, col: 102 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 14 }, 15: { name: "WindowsConnectionStore.deleteProfileKey", count: 0, regions: { 0: { startLoc: { line: 228, col: 3 }, endLoc: { line: 239, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 230, col: 25 }, endLoc: { line: 232, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 233, col: 9 }, endLoc: { line: 236, col: 6 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 236, col: 7 }, endLoc: { line: 238, col: 6 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 230, col: 9 }, endLoc: { line: 230, col: 23 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 15 }, 16: { name: "WindowsConnectionStore.applyPasswordPreference", count: 0, regions: { 0: { startLoc: { line: 241, col: 3 }, endLoc: { line: 259, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 244, col: 28 }, endLoc: { line: 249, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 251, col: 30 }, endLoc: { line: 256, col: 6 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 258, col: 5 }, endLoc: { line: 259, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 244, col: 9 }, endLoc: { line: 244, col: 26 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 245, col: 33 }, endLoc: { line: 245, col: 105 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 2: { startLoc: { line: 251, col: 9 }, endLoc: { line: 251, col: 28 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 3: { startLoc: { line: 254, col: 31 }, endLoc: { line: 254, col: 57 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 16 }, 17: { name: "WindowsConnectionStore.savePassword", count: 0, regions: { 0: { startLoc: { line: 261, col: 3 }, endLoc: { line: 276, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 262, col: 9 }, endLoc: { line: 272, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 272, col: 7 }, endLoc: { line: 275, col: 6 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 17 }, 18: { name: "WindowsConnectionStore.loadPasswordForProfile", count: 0, regions: { 0: { startLoc: { line: 278, col: 3 }, endLoc: { line: 297, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 279, col: 74 }, endLoc: { line: 281, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 283, col: 9 }, endLoc: { line: 293, col: 6 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 293, col: 7 }, endLoc: { line: 296, col: 6 }, count: 0, ignored: 0 }, 4: { startLoc: { line: 288, col: 32 }, endLoc: { line: 290, col: 8 }, count: 0, ignored: 0 }, 5: { startLoc: { line: 291, col: 7 }, endLoc: { line: 293, col: 6 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 279, col: 9 }, endLoc: { line: 279, col: 72 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 288, col: 11 }, endLoc: { line: 288, col: 30 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 2: { startLoc: { line: 292, col: 14 }, endLoc: { line: 292, col: 94 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 18 }, 19: { name: "WindowsConnectionStore.removePassword", count: 0, regions: { 0: { startLoc: { line: 299, col: 3 }, endLoc: { line: 311, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 300, col: 29 }, endLoc: { line: 302, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 304, col: 9 }, endLoc: { line: 308, col: 6 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 308, col: 7 }, endLoc: { line: 310, col: 6 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 300, col: 9 }, endLoc: { line: 300, col: 27 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 19 }, 20: { name: "WindowsConnectionStore.createProfile", count: 0, regions: { 0: { startLoc: { line: 313, col: 3 }, endLoc: { line: 329, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 315, col: 5 }, endLoc: { line: 329, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 20 }, 21: { name: "WindowsConnectionStore.updatedProfile", count: 0, regions: { 0: { startLoc: { line: 331, col: 3 }, endLoc: { line: 341, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 333, col: 5 }, endLoc: { line: 341, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 333, col: 20 }, endLoc: { line: 333, col: 111 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 21 }, 22: { name: "WindowsConnectionStore.sanitizeProfile", count: 0, regions: { 0: { startLoc: { line: 343, col: 3 }, endLoc: { line: 368, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 344, col: 89 }, endLoc: { line: 346, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 350, col: 55 }, endLoc: { line: 352, col: 6 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 354, col: 5 }, endLoc: { line: 368, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 344, col: 9 }, endLoc: { line: 344, col: 87 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 349, col: 22 }, endLoc: { line: 349, col: 89 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 2: { startLoc: { line: 350, col: 9 }, endLoc: { line: 350, col: 53 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 3: { startLoc: { line: 356, col: 13 }, endLoc: { line: 357, col: 74 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 4: { startLoc: { line: 361, col: 19 }, endLoc: { line: 361, col: 120 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 5: { startLoc: { line: 363, col: 22 }, endLoc: { line: 363, col: 92 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 22 }, 23: { name: "WindowsConnectionStore.findProfileForSave", count: 0, regions: { 0: { startLoc: { line: 370, col: 3 }, endLoc: { line: 385, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 373, col: 28 }, endLoc: { line: 375, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 376, col: 5 }, endLoc: { line: 383, col: 6 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 377, col: 92 }, endLoc: { line: 379, col: 8 }, count: 0, ignored: 0 }, 4: { startLoc: { line: 380, col: 115 }, endLoc: { line: 382, col: 8 }, count: 0, ignored: 0 }, 5: { startLoc: { line: 384, col: 5 }, endLoc: { line: 385, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 373, col: 9 }, endLoc: { line: 373, col: 26 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 377, col: 11 }, endLoc: { line: 377, col: 90 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 2: { startLoc: { line: 380, col: 11 }, endLoc: { line: 380, col: 113 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 23 }, 24: { name: "WindowsConnectionStore.findProfileById", count: 0, regions: { 0: { startLoc: { line: 387, col: 3 }, endLoc: { line: 394, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 388, col: 5 }, endLoc: { line: 392, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 389, col: 37 }, endLoc: { line: 391, col: 8 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 393, col: 5 }, endLoc: { line: 394, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 389, col: 11 }, endLoc: { line: 389, col: 35 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 24 }, 25: { name: "WindowsConnectionStore.upsertProfile", count: 0, regions: { 0: { startLoc: { line: 396, col: 3 }, endLoc: { line: 401, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 398, col: 5 }, endLoc: { line: 401, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 25 }, 26: { name: "anonymous_3", count: 0, regions: { 0: { startLoc: { line: 398, col: 42 }, endLoc: { line: 398, col: 119 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 26 }, 27: { name: "WindowsConnectionStore.sortProfiles", count: 0, regions: { 0: { startLoc: { line: 403, col: 3 }, endLoc: { line: 410, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 404, col: 5 }, endLoc: { line: 410, col: 4 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 408, col: 7 }, endLoc: { line: 409, col: 6 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 27 }, 28: { name: "anonymous_4", count: 0, regions: { 0: { startLoc: { line: 404, col: 26 }, endLoc: { line: 409, col: 6 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 405, col: 59 }, endLoc: { line: 407, col: 8 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 405, col: 11 }, endLoc: { line: 405, col: 57 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 28 }, 29: { name: "WindowsConnectionStore.firstProfileId", count: 0, regions: { 0: { startLoc: { line: 412, col: 3 }, endLoc: { line: 414, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 413, col: 5 }, endLoc: { line: 414, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 413, col: 12 }, endLoc: { line: 413, col: 53 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 29 }, 30: { name: "WindowsConnectionStore.normalizePort", count: 0, regions: { 0: { startLoc: { line: 416, col: 3 }, endLoc: { line: 419, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 417, col: 5 }, endLoc: { line: 419, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 417, col: 19 }, endLoc: { line: 417, col: 62 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 418, col: 12 }, endLoc: { line: 418, col: 55 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 30 }, 31: { name: "WindowsConnectionStore.profileName", count: 0, regions: { 0: { startLoc: { line: 421, col: 3 }, endLoc: { line: 423, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 422, col: 5 }, endLoc: { line: 423, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 422, col: 12 }, endLoc: { line: 422, col: 64 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 31 }, 32: { name: "WindowsConnectionStore.profileKey", count: 0, regions: { 0: { startLoc: { line: 425, col: 3 }, endLoc: { line: 427, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 426, col: 5 }, endLoc: { line: 427, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 32 }, 33: { name: "WindowsConnectionStore.passwordAlias", count: 0, regions: { 0: { startLoc: { line: 429, col: 3 }, endLoc: { line: 431, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 430, col: 5 }, endLoc: { line: 431, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 33 }, 34: { name: "WindowsConnectionStore.newProfileId", count: 0, regions: { 0: { startLoc: { line: 433, col: 3 }, endLoc: { line: 435, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 434, col: 5 }, endLoc: { line: 435, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 34 }, 35: { name: "WindowsConnectionStore.safeTimestamp", count: 0, regions: { 0: { startLoc: { line: 437, col: 3 }, endLoc: { line: 439, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 438, col: 5 }, endLoc: { line: 439, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 438, col: 12 }, endLoc: { line: 438, col: 97 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 35 }, 36: { name: "WindowsConnectionStore.stringToBytes", count: 0, regions: { 0: { startLoc: { line: 441, col: 3 }, endLoc: { line: 443, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 442, col: 5 }, endLoc: { line: 443, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 36 }, 37: { name: "WindowsConnectionStore.bytesToString", count: 0, regions: { 0: { startLoc: { line: 445, col: 3 }, endLoc: { line: 447, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 446, col: 5 }, endLoc: { line: 447, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 37 } }, exeLine: { 0: 1, 1: 2, 2: 3, 3: 4, 4: 5, 5: 7, 6: 8, 7: 9, 8: 10, 9: 11, 10: 12, 11: 13, 12: 15, 13: 16, 14: 17, 15: 18, 16: 19, 17: 20, 18: 21, 19: 22, 20: 23, 21: 24, 22: 25, 23: 26, 24: 29, 25: 30, 26: 31, 27: 32, 28: 33, 29: 34, 30: 35, 31: 36, 32: 39, 33: 40, 34: 41, 35: 42, 36: 45, 37: 46, 38: 47, 39: 49, 40: 50, 41: 53, 42: 54, 43: 55, 44: 56, 45: 57, 46: 58, 47: 59, 48: 60, 49: 61, 50: 65, 51: 66, 52: 69, 53: 70, 54: 71, 55: 72, 56: 73, 57: 74, 58: 77, 59: 78, 60: 79, 61: 80, 62: 81, 63: 82, 64: 84, 65: 85, 66: 86, 67: 87, 68: 90, 69: 91, 70: 92, 71: 93, 72: 94, 73: 97, 74: 98, 75: 99, 76: 100, 77: 103, 78: 104, 79: 105, 80: 106, 81: 107, 82: 110, 83: 111, 84: 112, 85: 113, 86: 114, 87: 115, 88: 118, 89: 119, 90: 120, 91: 123, 92: 124, 93: 125, 94: 126, 95: 127, 96: 128, 97: 132, 98: 133, 99: 134, 100: 135, 101: 138, 102: 139, 103: 140, 104: 141, 105: 142, 106: 143, 107: 146, 108: 149, 109: 150, 110: 151, 111: 152, 112: 153, 113: 155, 114: 156, 115: 157, 116: 159, 117: 160, 118: 161, 119: 162, 120: 166, 121: 167, 122: 168, 123: 169, 124: 170, 125: 172, 126: 173, 127: 174, 128: 175, 129: 179, 130: 180, 131: 181, 132: 184, 133: 185, 134: 186, 135: 187, 136: 188, 137: 189, 138: 191, 139: 192, 140: 195, 141: 198, 142: 199, 143: 200, 144: 201, 145: 204, 146: 205, 147: 206, 148: 208, 149: 209, 150: 210, 151: 211, 152: 212, 153: 213, 154: 215, 155: 216, 156: 218, 157: 219, 158: 220, 159: 222, 160: 223, 161: 224, 162: 228, 163: 229, 164: 230, 165: 231, 166: 233, 167: 234, 168: 235, 169: 236, 170: 237, 171: 241, 172: 242, 173: 243, 174: 244, 175: 245, 176: 246, 177: 247, 178: 248, 179: 251, 180: 252, 181: 253, 182: 254, 183: 255, 184: 258, 185: 261, 186: 262, 187: 263, 188: 264, 189: 265, 190: 266, 191: 267, 192: 268, 193: 269, 194: 270, 195: 271, 196: 272, 197: 273, 198: 274, 199: 278, 200: 279, 201: 280, 202: 283, 203: 284, 204: 285, 205: 286, 206: 287, 207: 288, 208: 289, 209: 291, 210: 292, 211: 293, 212: 294, 213: 295, 214: 299, 215: 300, 216: 301, 217: 304, 218: 305, 219: 306, 220: 307, 221: 308, 222: 309, 223: 313, 224: 314, 225: 315, 226: 316, 227: 317, 228: 318, 229: 319, 230: 320, 231: 321, 232: 322, 233: 323, 234: 324, 235: 325, 236: 326, 237: 327, 238: 331, 239: 332, 240: 333, 241: 334, 242: 335, 243: 336, 244: 337, 245: 338, 246: 339, 247: 340, 248: 343, 249: 344, 250: 345, 251: 348, 252: 349, 253: 350, 254: 351, 255: 354, 256: 355, 257: 356, 258: 357, 259: 358, 260: 359, 261: 360, 262: 361, 263: 362, 264: 363, 265: 364, 266: 365, 267: 366, 268: 370, 269: 371, 270: 372, 271: 373, 272: 374, 273: 376, 274: 377, 275: 378, 276: 380, 277: 381, 278: 384, 279: 387, 280: 388, 281: 389, 282: 390, 283: 393, 284: 396, 285: 397, 286: 398, 287: 399, 288: 400, 289: 403, 290: 404, 291: 405, 292: 406, 293: 408, 294: 412, 295: 413, 296: 416, 297: 417, 298: 418, 299: 421, 300: 422, 301: 425, 302: 426, 303: 429, 304: 430, 305: 433, 306: 434, 307: 437, 308: 438, 309: 441, 310: 442, 311: 445, 312: 446 } });
const STORE_NAME = 'windowsConnectionProfiles';
const KEY_PROFILE_IDS = 'profileIds';
const KEY_DEFAULT_PROFILE_ID = 'defaultProfileId';
const KEY_PROFILE_PREFIX = 'profile.';
const PASSWORD_ALIAS_PREFIX = 'muhub.rdp.password.';
const DEFAULT_RDP_PORT = '3389';
const MAX_PROFILES = 20;
export interface WindowsConnectionProfile {
    id: string;
    name: string;
    host: string;
    port: string;
    username: string;
    certPolicy: string;
    rememberPassword: boolean;
    passwordAlias: string;
    createdAt: number;
    updatedAt: number;
    lastConnectedAt: number;
}
export interface WindowsConnectionSaveInput {
    profileId: string;
    host: string;
    port: string;
    username: string;
    password: string;
    certPolicy: string;
    rememberPassword: boolean;
}
export interface WindowsConnectionSnapshot {
    profiles: WindowsConnectionProfile[];
    selectedProfileId: string;
    selectedPassword: string;
}
export class WindowsConnectionStore {
    private readonly context: common.Context;
    private preferencesCache: preferences.Preferences | null = null;
    constructor(context: common.Context) {
        bjccovmshb1i31.instrumentFunction(0);
        bjccovmshb1i31.instrumentRegion(0, 1);
        this.context = context;
    }
    async loadSnapshot(): Promise<WindowsConnectionSnapshot> {
        bjccovmshb1i31.instrumentFunction(1);
        bjccovmshb1i31.instrumentRegion(1, 1);
        const profiles = await this.loadProfiles();
        const selectedProfileId = await this.resolveSelectedProfileId(profiles);
        const selectedProfile = WindowsConnectionStore.findProfileById(profiles, selectedProfileId);
        const selectedPassword = selectedProfile === null ? (bjccovmshb1i31.instrumentBranch(1, 0, true), '') : (bjccovmshb1i31.instrumentBranch(1, 0, false), await this.loadPasswordForProfile(selectedProfile));
        return {
            profiles: profiles,
            selectedProfileId: selectedProfileId,
            selectedPassword: selectedPassword
        };
    }
    async loadPassword(profile: WindowsConnectionProfile): Promise<string> {
        bjccovmshb1i31.instrumentFunction(2);
        bjccovmshb1i31.instrumentRegion(2, 1);
        return this.loadPasswordForProfile(profile);
    }
    async saveProfile(input: WindowsConnectionSaveInput): Promise<WindowsConnectionProfile | null> {
        bjccovmshb1i31.instrumentFunction(3);
        const host = input.host.trim();
        const port = WindowsConnectionStore.normalizePort(input.port);
        const username = input.username.trim();
        if (host.length === 0) {
            bjccovmshb1i31.instrumentBranch(3, 0, true);
            bjccovmshb1i31.instrumentRegion(3, 1);
            return null;
        }
        else {
            bjccovmshb1i31.instrumentBranch(3, 0, false);
        }
        bjccovmshb1i31.instrumentRegion(3, 2);
        const now = Date.now();
        const profiles = await this.loadProfiles();
        const existing = WindowsConnectionStore.findProfileForSave(profiles, input.profileId, host, port, username);
        const profile = existing === null ? (bjccovmshb1i31.instrumentBranch(3, 1, true), WindowsConnectionStore.createProfile(host, port, username, input.certPolicy, now)) : (bjccovmshb1i31.instrumentBranch(3, 1, false), WindowsConnectionStore.updatedProfile(existing, host, port, username, input.certPolicy, now));
        await this.applyPasswordPreference(profile, input.rememberPassword, input.password);
        const nextProfiles = WindowsConnectionStore.upsertProfile(profiles, profile);
        await this.persistProfiles(nextProfiles, profile.id);
        return profile;
    }
    async deleteProfile(profileId: string): Promise<WindowsConnectionSnapshot> {
        bjccovmshb1i31.instrumentFunction(4);
        const profiles = await this.loadProfiles();
        const target = WindowsConnectionStore.findProfileById(profiles, profileId);
        if (target !== null) {
            bjccovmshb1i31.instrumentBranch(4, 0, true);
            bjccovmshb1i31.instrumentRegion(4, 1);
            await this.removePassword(target.passwordAlias);
        }
        else {
            bjccovmshb1i31.instrumentBranch(4, 0, false);
        }
        bjccovmshb1i31.instrumentRegion(4, 2);
        const nextProfiles = profiles.filter((profile: WindowsConnectionProfile): boolean => { bjccovmshb1i31.instrumentFunction(5); return profile.id !== profileId; });
        await this.persistProfiles(nextProfiles, WindowsConnectionStore.firstProfileId(nextProfiles));
        await this.deleteProfileKey(profileId);
        return this.loadSnapshot();
    }
    async clearPassword(profileId: string): Promise<WindowsConnectionSnapshot> {
        bjccovmshb1i31.instrumentFunction(6);
        const profiles = await this.loadProfiles();
        const target = WindowsConnectionStore.findProfileById(profiles, profileId);
        if (target === null) {
            bjccovmshb1i31.instrumentBranch(6, 0, true);
            bjccovmshb1i31.instrumentRegion(6, 1);
            return this.loadSnapshot();
        }
        else {
            bjccovmshb1i31.instrumentBranch(6, 0, false);
        }
        bjccovmshb1i31.instrumentRegion(6, 2);
        await this.removePassword(target.passwordAlias);
        target.rememberPassword = false;
        target.passwordAlias = '';
        target.updatedAt = Date.now();
        await this.persistProfiles(WindowsConnectionStore.upsertProfile(profiles, target), target.id);
        return this.loadSnapshot();
    }
    private async getPreferences(): Promise<preferences.Preferences | null> {
        bjccovmshb1i31.instrumentFunction(7);
        if (this.preferencesCache !== null) {
            bjccovmshb1i31.instrumentBranch(7, 0, true);
            bjccovmshb1i31.instrumentRegion(7, 1);
            return this.preferencesCache;
        }
        else {
            bjccovmshb1i31.instrumentBranch(7, 0, false);
        }
        try {
            bjccovmshb1i31.instrumentRegion(7, 2);
            this.preferencesCache = await preferences.getPreferences(this.context, STORE_NAME);
            return this.preferencesCache;
        }
        catch (error) {
            bjccovmshb1i31.instrumentRegion(7, 3);
            RdpLogger.error(`windows connection preferences open failed: ${JSON.stringify(error)}`);
            return null;
        }
    }
    private async loadProfiles(): Promise<WindowsConnectionProfile[]> {
        bjccovmshb1i31.instrumentFunction(8);
        const store = await this.getPreferences();
        if (store === null) {
            bjccovmshb1i31.instrumentBranch(8, 0, true);
            bjccovmshb1i31.instrumentRegion(8, 1);
            return [];
        }
        else {
            bjccovmshb1i31.instrumentBranch(8, 0, false);
        }
        const profileIds = await this.loadProfileIds(store);
        const profiles: WindowsConnectionProfile[] = [];
        for (const profileId of profileIds) {
            bjccovmshb1i31.instrumentRegion(8, 2);
            const profile = await this.loadProfile(store, profileId);
            if (profile !== null) {
                bjccovmshb1i31.instrumentBranch(8, 1, true);
                bjccovmshb1i31.instrumentRegion(8, 3);
                profiles.push(profile);
            }
            else {
                bjccovmshb1i31.instrumentBranch(8, 1, false);
            }
        }
        bjccovmshb1i31.instrumentRegion(8, 4);
        return WindowsConnectionStore.sortProfiles(profiles);
    }
    private async loadProfileIds(store: preferences.Preferences): Promise<string[]> {
        bjccovmshb1i31.instrumentFunction(9);
        try {
            bjccovmshb1i31.instrumentRegion(9, 1);
            const raw = await store.get(KEY_PROFILE_IDS, '[]');
            if (typeof raw !== 'string') {
                bjccovmshb1i31.instrumentBranch(9, 0, true);
                bjccovmshb1i31.instrumentRegion(9, 3);
                return [];
            }
            else {
                bjccovmshb1i31.instrumentBranch(9, 0, false);
            }
            const parsed = JSON.parse(raw) as string[];
            if (!Array.isArray(parsed)) {
                bjccovmshb1i31.instrumentBranch(9, 1, true);
                bjccovmshb1i31.instrumentRegion(9, 4);
                return [];
            }
            else {
                bjccovmshb1i31.instrumentBranch(9, 1, false);
            }
            bjccovmshb1i31.instrumentRegion(9, 5);
            return parsed.filter((profileId: string): boolean => { bjccovmshb1i31.instrumentFunction(10); return typeof profileId === 'string' && profileId.length > 0; });
        }
        catch (error) {
            bjccovmshb1i31.instrumentRegion(9, 2);
            RdpLogger.warn(`windows connection profile ids read failed: ${JSON.stringify(error)}`);
            return [];
        }
    }
    private async loadProfile(store: preferences.Preferences, profileId: string): Promise<WindowsConnectionProfile | null> {
        bjccovmshb1i31.instrumentFunction(11);
        try {
            bjccovmshb1i31.instrumentRegion(11, 1);
            const raw = await store.get(WindowsConnectionStore.profileKey(profileId), '');
            if (typeof raw !== 'string' || raw.length === 0) {
                bjccovmshb1i31.instrumentBranch(11, 0, true);
                bjccovmshb1i31.instrumentRegion(11, 3);
                return null;
            }
            else {
                bjccovmshb1i31.instrumentBranch(11, 0, false);
            }
            bjccovmshb1i31.instrumentRegion(11, 4);
            return WindowsConnectionStore.sanitizeProfile(JSON.parse(raw) as WindowsConnectionProfile);
        }
        catch (error) {
            bjccovmshb1i31.instrumentRegion(11, 2);
            RdpLogger.warn(`windows connection profile read failed: id=${profileId}, error=${JSON.stringify(error)}`);
            return null;
        }
    }
    private async resolveSelectedProfileId(profiles: WindowsConnectionProfile[]): Promise<string> {
        bjccovmshb1i31.instrumentFunction(12);
        if (profiles.length === 0) {
            bjccovmshb1i31.instrumentBranch(12, 0, true);
            bjccovmshb1i31.instrumentRegion(12, 1);
            return '';
        }
        else {
            bjccovmshb1i31.instrumentBranch(12, 0, false);
        }
        const store = await this.getPreferences();
        if (store !== null) {
            bjccovmshb1i31.instrumentBranch(12, 1, true);
            bjccovmshb1i31.instrumentRegion(12, 2);
            try {
                bjccovmshb1i31.instrumentRegion(12, 3);
                const raw = await store.get(KEY_DEFAULT_PROFILE_ID, '');
                if (typeof raw === 'string' && WindowsConnectionStore.findProfileById(profiles, raw) !== null) {
                    bjccovmshb1i31.instrumentBranch(12, 2, true);
                    bjccovmshb1i31.instrumentRegion(12, 5);
                    return raw;
                }
                else {
                    bjccovmshb1i31.instrumentBranch(12, 2, false);
                }
            }
            catch (error) {
                bjccovmshb1i31.instrumentRegion(12, 4);
                RdpLogger.warn(`windows connection selected profile read failed: ${JSON.stringify(error)}`);
            }
        }
        else {
            bjccovmshb1i31.instrumentBranch(12, 1, false);
        }
        bjccovmshb1i31.instrumentRegion(12, 6);
        return profiles[0].id;
    }
    private async persistProfiles(profiles: WindowsConnectionProfile[], selectedProfileId: string): Promise<void> {
        bjccovmshb1i31.instrumentFunction(13);
        const store = await this.getPreferences();
        if (store === null) {
            bjccovmshb1i31.instrumentBranch(13, 0, true);
            bjccovmshb1i31.instrumentRegion(13, 1);
            return;
        }
        else {
            bjccovmshb1i31.instrumentBranch(13, 0, false);
        }
        const sortedProfiles = WindowsConnectionStore.sortProfiles(profiles);
        const profilesToKeep = sortedProfiles.slice(0, MAX_PROFILES);
        const removedProfiles = sortedProfiles.slice(MAX_PROFILES);
        try {
            bjccovmshb1i31.instrumentRegion(13, 2);
            const profileIds = profilesToKeep.map((profile: WindowsConnectionProfile): string => { bjccovmshb1i31.instrumentFunction(14); return profile.id; });
            await store.put(KEY_PROFILE_IDS, JSON.stringify(profileIds));
            await store.put(KEY_DEFAULT_PROFILE_ID, WindowsConnectionStore.findProfileById(profilesToKeep, selectedProfileId) === null ? (bjccovmshb1i31.instrumentBranch(13, 1, true), WindowsConnectionStore.firstProfileId(profilesToKeep)) : (bjccovmshb1i31.instrumentBranch(13, 1, false), selectedProfileId));
            for (const profile of profilesToKeep) {
                bjccovmshb1i31.instrumentRegion(13, 4);
                await store.put(WindowsConnectionStore.profileKey(profile.id), JSON.stringify(profile));
            }
            for (const profile of removedProfiles) {
                bjccovmshb1i31.instrumentRegion(13, 5);
                await store.delete(WindowsConnectionStore.profileKey(profile.id));
                await this.removePassword(profile.passwordAlias);
            }
            bjccovmshb1i31.instrumentRegion(13, 6);
            await store.flush();
        }
        catch (error) {
            bjccovmshb1i31.instrumentRegion(13, 3);
            RdpLogger.error(`windows connection profiles persist failed: ${JSON.stringify(error)}`);
        }
    }
    private async deleteProfileKey(profileId: string): Promise<void> {
        bjccovmshb1i31.instrumentFunction(15);
        const store = await this.getPreferences();
        if (store === null) {
            bjccovmshb1i31.instrumentBranch(15, 0, true);
            bjccovmshb1i31.instrumentRegion(15, 1);
            return;
        }
        else {
            bjccovmshb1i31.instrumentBranch(15, 0, false);
        }
        try {
            bjccovmshb1i31.instrumentRegion(15, 2);
            await store.delete(WindowsConnectionStore.profileKey(profileId));
            await store.flush();
        }
        catch (error) {
            bjccovmshb1i31.instrumentRegion(15, 3);
            RdpLogger.warn(`windows connection profile key delete failed: id=${profileId}, error=${JSON.stringify(error)}`);
        }
    }
    private async applyPasswordPreference(profile: WindowsConnectionProfile, rememberPassword: boolean, password: string): Promise<void> {
        bjccovmshb1i31.instrumentFunction(16);
        const passwordAlias = WindowsConnectionStore.passwordAlias(profile.id);
        if (!rememberPassword) {
            bjccovmshb1i31.instrumentBranch(16, 0, true);
            bjccovmshb1i31.instrumentRegion(16, 1);
            await this.removePassword(profile.passwordAlias.length > 0 ? (bjccovmshb1i31.instrumentBranch(16, 1, true), profile.passwordAlias) : (bjccovmshb1i31.instrumentBranch(16, 1, false), passwordAlias));
            profile.rememberPassword = false;
            profile.passwordAlias = '';
            return;
        }
        else {
            bjccovmshb1i31.instrumentBranch(16, 0, false);
        }
        if (password.length > 0) {
            bjccovmshb1i31.instrumentBranch(16, 2, true);
            bjccovmshb1i31.instrumentRegion(16, 2);
            const saved = await this.savePassword(passwordAlias, password);
            profile.rememberPassword = saved;
            profile.passwordAlias = saved ? (bjccovmshb1i31.instrumentBranch(16, 3, true), passwordAlias) : (bjccovmshb1i31.instrumentBranch(16, 3, false), '');
            return;
        }
        else {
            bjccovmshb1i31.instrumentBranch(16, 2, false);
        }
        bjccovmshb1i31.instrumentRegion(16, 3);
        profile.rememberPassword = profile.passwordAlias.length > 0;
    }
    private async savePassword(alias: string, password: string): Promise<boolean> {
        bjccovmshb1i31.instrumentFunction(17);
        try {
            bjccovmshb1i31.instrumentRegion(17, 1);
            const attributes: asset.AssetMap = new Map();
            attributes.set(asset.Tag.ALIAS, WindowsConnectionStore.stringToBytes(alias));
            attributes.set(asset.Tag.SECRET, WindowsConnectionStore.stringToBytes(password));
            attributes.set(asset.Tag.ACCESSIBILITY, asset.Accessibility.DEVICE_FIRST_UNLOCKED);
            attributes.set(asset.Tag.AUTH_TYPE, asset.AuthType.NONE);
            attributes.set(asset.Tag.SYNC_TYPE, asset.SyncType.NEVER);
            attributes.set(asset.Tag.CONFLICT_RESOLUTION, asset.ConflictResolution.OVERWRITE);
            await asset.add(attributes);
            return true;
        }
        catch (error) {
            bjccovmshb1i31.instrumentRegion(17, 2);
            RdpLogger.warn(`windows connection password save failed: alias=${alias}, error=${JSON.stringify(error)}`);
            return false;
        }
    }
    private async loadPasswordForProfile(profile: WindowsConnectionProfile): Promise<string> {
        bjccovmshb1i31.instrumentFunction(18);
        if (!profile.rememberPassword || profile.passwordAlias.length === 0) {
            bjccovmshb1i31.instrumentBranch(18, 0, true);
            bjccovmshb1i31.instrumentRegion(18, 1);
            return '';
        }
        else {
            bjccovmshb1i31.instrumentBranch(18, 0, false);
        }
        try {
            bjccovmshb1i31.instrumentRegion(18, 2);
            const query: asset.AssetMap = new Map();
            query.set(asset.Tag.ALIAS, WindowsConnectionStore.stringToBytes(profile.passwordAlias));
            query.set(asset.Tag.RETURN_TYPE, asset.ReturnType.ALL);
            const result = await asset.query(query);
            if (result.length === 0) {
                bjccovmshb1i31.instrumentBranch(18, 1, true);
                bjccovmshb1i31.instrumentRegion(18, 4);
                return '';
            }
            else {
                bjccovmshb1i31.instrumentBranch(18, 1, false);
            }
            bjccovmshb1i31.instrumentRegion(18, 5);
            const secret = result[0].get(asset.Tag.SECRET);
            return secret instanceof Uint8Array ? (bjccovmshb1i31.instrumentBranch(18, 2, true), WindowsConnectionStore.bytesToString(secret)) : (bjccovmshb1i31.instrumentBranch(18, 2, false), '');
        }
        catch (error) {
            bjccovmshb1i31.instrumentRegion(18, 3);
            RdpLogger.warn(`windows connection password load failed: alias=${profile.passwordAlias}, error=${JSON.stringify(error)}`);
            return '';
        }
    }
    private async removePassword(alias: string): Promise<void> {
        bjccovmshb1i31.instrumentFunction(19);
        if (alias.length === 0) {
            bjccovmshb1i31.instrumentBranch(19, 0, true);
            bjccovmshb1i31.instrumentRegion(19, 1);
            return;
        }
        else {
            bjccovmshb1i31.instrumentBranch(19, 0, false);
        }
        try {
            bjccovmshb1i31.instrumentRegion(19, 2);
            const query: asset.AssetMap = new Map();
            query.set(asset.Tag.ALIAS, WindowsConnectionStore.stringToBytes(alias));
            await asset.remove(query);
        }
        catch (error) {
            bjccovmshb1i31.instrumentRegion(19, 3);
            RdpLogger.warn(`windows connection password remove skipped: alias=${alias}, error=${JSON.stringify(error)}`);
        }
    }
    private static createProfile(host: string, port: string, username: string, certPolicy: string, now: number): WindowsConnectionProfile {
        bjccovmshb1i31.instrumentFunction(20);
        bjccovmshb1i31.instrumentRegion(20, 1);
        const id = WindowsConnectionStore.newProfileId();
        return {
            id: id,
            name: WindowsConnectionStore.profileName(host, username),
            host: host,
            port: port,
            username: username,
            certPolicy: certPolicy,
            rememberPassword: false,
            passwordAlias: '',
            createdAt: now,
            updatedAt: now,
            lastConnectedAt: now
        };
    }
    private static updatedProfile(profile: WindowsConnectionProfile, host: string, port: string, username: string, certPolicy: string, now: number): WindowsConnectionProfile {
        bjccovmshb1i31.instrumentFunction(21);
        bjccovmshb1i31.instrumentRegion(21, 1);
        profile.name = profile.name.length > 0 ? (bjccovmshb1i31.instrumentBranch(21, 0, true), profile.name) : (bjccovmshb1i31.instrumentBranch(21, 0, false), WindowsConnectionStore.profileName(host, username));
        profile.host = host;
        profile.port = port;
        profile.username = username;
        profile.certPolicy = certPolicy;
        profile.updatedAt = now;
        profile.lastConnectedAt = now;
        return profile;
    }
    private static sanitizeProfile(profile: WindowsConnectionProfile): WindowsConnectionProfile | null {
        bjccovmshb1i31.instrumentFunction(22);
        if (!profile || typeof profile.id !== 'string' || typeof profile.host !== 'string') {
            bjccovmshb1i31.instrumentBranch(22, 0, true);
            bjccovmshb1i31.instrumentRegion(22, 1);
            return null;
        }
        else {
            bjccovmshb1i31.instrumentBranch(22, 0, false);
        }
        const host = profile.host.trim();
        const username = typeof profile.username === 'string' ? (bjccovmshb1i31.instrumentBranch(22, 1, true), profile.username.trim()) : (bjccovmshb1i31.instrumentBranch(22, 1, false), '');
        if (profile.id.length === 0 || host.length === 0) {
            bjccovmshb1i31.instrumentBranch(22, 2, true);
            bjccovmshb1i31.instrumentRegion(22, 2);
            return null;
        }
        else {
            bjccovmshb1i31.instrumentBranch(22, 2, false);
        }
        bjccovmshb1i31.instrumentRegion(22, 3);
        return {
            id: profile.id,
            name: typeof profile.name === 'string' && profile.name.length > 0 ? (bjccovmshb1i31.instrumentBranch(22, 3, true), profile.name) : (bjccovmshb1i31.instrumentBranch(22, 3, false), WindowsConnectionStore.profileName(host, username)),
            host: host,
            port: WindowsConnectionStore.normalizePort(profile.port),
            username: username,
            certPolicy: typeof profile.certPolicy === 'string' && profile.certPolicy.length > 0 ? (bjccovmshb1i31.instrumentBranch(22, 4, true), profile.certPolicy) : (bjccovmshb1i31.instrumentBranch(22, 4, false), 'tofu'),
            rememberPassword: profile.rememberPassword === true,
            passwordAlias: typeof profile.passwordAlias === 'string' ? (bjccovmshb1i31.instrumentBranch(22, 5, true), profile.passwordAlias) : (bjccovmshb1i31.instrumentBranch(22, 5, false), ''),
            createdAt: WindowsConnectionStore.safeTimestamp(profile.createdAt),
            updatedAt: WindowsConnectionStore.safeTimestamp(profile.updatedAt),
            lastConnectedAt: WindowsConnectionStore.safeTimestamp(profile.lastConnectedAt)
        };
    }
    private static findProfileForSave(profiles: WindowsConnectionProfile[], profileId: string, host: string, port: string, username: string): WindowsConnectionProfile | null {
        bjccovmshb1i31.instrumentFunction(23);
        const selected = WindowsConnectionStore.findProfileById(profiles, profileId);
        if (selected !== null) {
            bjccovmshb1i31.instrumentBranch(23, 0, true);
            bjccovmshb1i31.instrumentRegion(23, 1);
            return selected;
        }
        else {
            bjccovmshb1i31.instrumentBranch(23, 0, false);
        }
        for (const profile of profiles) {
            bjccovmshb1i31.instrumentRegion(23, 2);
            if (profile.host === host && profile.port === port && profile.username === username) {
                bjccovmshb1i31.instrumentBranch(23, 1, true);
                bjccovmshb1i31.instrumentRegion(23, 3);
                return profile;
            }
            else {
                bjccovmshb1i31.instrumentBranch(23, 1, false);
            }
            if (username.length > 0 && profile.host === host && profile.port === port && profile.username.length === 0) {
                bjccovmshb1i31.instrumentBranch(23, 2, true);
                bjccovmshb1i31.instrumentRegion(23, 4);
                return profile;
            }
            else {
                bjccovmshb1i31.instrumentBranch(23, 2, false);
            }
        }
        bjccovmshb1i31.instrumentRegion(23, 5);
        return null;
    }
    private static findProfileById(profiles: WindowsConnectionProfile[], profileId: string): WindowsConnectionProfile | null {
        bjccovmshb1i31.instrumentFunction(24);
        for (const profile of profiles) {
            bjccovmshb1i31.instrumentRegion(24, 1);
            if (profile.id === profileId) {
                bjccovmshb1i31.instrumentBranch(24, 0, true);
                bjccovmshb1i31.instrumentRegion(24, 2);
                return profile;
            }
            else {
                bjccovmshb1i31.instrumentBranch(24, 0, false);
            }
        }
        bjccovmshb1i31.instrumentRegion(24, 3);
        return null;
    }
    private static upsertProfile(profiles: WindowsConnectionProfile[], nextProfile: WindowsConnectionProfile): WindowsConnectionProfile[] {
        bjccovmshb1i31.instrumentFunction(25);
        bjccovmshb1i31.instrumentRegion(25, 1);
        const nextProfiles = profiles.filter((profile: WindowsConnectionProfile): boolean => { bjccovmshb1i31.instrumentFunction(26); return profile.id !== nextProfile.id; });
        nextProfiles.push(nextProfile);
        return nextProfiles;
    }
    private static sortProfiles(profiles: WindowsConnectionProfile[]): WindowsConnectionProfile[] {
        bjccovmshb1i31.instrumentFunction(27);
        bjccovmshb1i31.instrumentRegion(27, 1);
        return profiles.sort((left: WindowsConnectionProfile, right: WindowsConnectionProfile): number => {
            bjccovmshb1i31.instrumentFunction(28);
            if (right.lastConnectedAt !== left.lastConnectedAt) {
                bjccovmshb1i31.instrumentBranch(28, 0, true);
                bjccovmshb1i31.instrumentRegion(28, 1);
                return right.lastConnectedAt - left.lastConnectedAt;
            }
            else {
                bjccovmshb1i31.instrumentBranch(28, 0, false);
            }
            bjccovmshb1i31.instrumentRegion(27, 2);
            return right.updatedAt - left.updatedAt;
        });
    }
    private static firstProfileId(profiles: WindowsConnectionProfile[]): string {
        bjccovmshb1i31.instrumentFunction(29);
        bjccovmshb1i31.instrumentRegion(29, 1);
        return profiles.length > 0 ? (bjccovmshb1i31.instrumentBranch(29, 0, true), profiles[0].id) : (bjccovmshb1i31.instrumentBranch(29, 0, false), '');
    }
    private static normalizePort(port: string): string {
        bjccovmshb1i31.instrumentFunction(30);
        bjccovmshb1i31.instrumentRegion(30, 1);
        const value = typeof port === 'string' ? (bjccovmshb1i31.instrumentBranch(30, 0, true), port.trim()) : (bjccovmshb1i31.instrumentBranch(30, 0, false), '');
        return value.length > 0 ? (bjccovmshb1i31.instrumentBranch(30, 1, true), value) : (bjccovmshb1i31.instrumentBranch(30, 1, false), DEFAULT_RDP_PORT);
    }
    private static profileName(host: string, username: string): string {
        bjccovmshb1i31.instrumentFunction(31);
        bjccovmshb1i31.instrumentRegion(31, 1);
        return username.length > 0 ? (bjccovmshb1i31.instrumentBranch(31, 0, true), `${username} @ ${host}`) : (bjccovmshb1i31.instrumentBranch(31, 0, false), host);
    }
    private static profileKey(profileId: string): string {
        bjccovmshb1i31.instrumentFunction(32);
        bjccovmshb1i31.instrumentRegion(32, 1);
        return `${KEY_PROFILE_PREFIX}${profileId}`;
    }
    private static passwordAlias(profileId: string): string {
        bjccovmshb1i31.instrumentFunction(33);
        bjccovmshb1i31.instrumentRegion(33, 1);
        return `${PASSWORD_ALIAS_PREFIX}${profileId}`;
    }
    private static newProfileId(): string {
        bjccovmshb1i31.instrumentFunction(34);
        bjccovmshb1i31.instrumentRegion(34, 1);
        return `${Date.now()}-${Math.floor(Math.random() * 1000000)}`;
    }
    private static safeTimestamp(value: number): number {
        bjccovmshb1i31.instrumentFunction(35);
        bjccovmshb1i31.instrumentRegion(35, 1);
        return typeof value === 'number' && Number.isFinite(value) && value > 0 ? (bjccovmshb1i31.instrumentBranch(35, 0, true), value) : (bjccovmshb1i31.instrumentBranch(35, 0, false), Date.now());
    }
    private static stringToBytes(value: string): Uint8Array {
        bjccovmshb1i31.instrumentFunction(36);
        bjccovmshb1i31.instrumentRegion(36, 1);
        return new util.TextEncoder().encodeInto(value);
    }
    private static bytesToString(value: Uint8Array): string {
        bjccovmshb1i31.instrumentFunction(37);
        bjccovmshb1i31.instrumentRegion(37, 1);
        return new util.TextDecoder().decodeToString(value);
    }
}
