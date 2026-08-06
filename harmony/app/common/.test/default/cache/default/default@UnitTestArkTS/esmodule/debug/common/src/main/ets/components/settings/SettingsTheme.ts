import type common from "@ohos:app.ability.common";
import ConfigurationConstant from "@ohos:app.ability.ConfigurationConstant";
import { SettingsAccent } from "@normalized:N&&&common/src/main/ets/components/settings/SettingsResources&";
import { SETTINGS_STORAGE_APPEARANCE_MODE, SETTINGS_STORAGE_SYSTEM_DARK } from "@normalized:N&&&common/src/main/ets/components/settings/SettingsTypes&";
import type { SettingsAccentName, SettingsAppearanceMode, SettingsStatusTone } from "@normalized:N&&&common/src/main/ets/components/settings/SettingsTypes&";
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
let bjccovmshb1ic5 = new BjcCov({ version: "bjc v1.0.0", versionCode: 10000, path: "common/src/main/ets/components/settings/SettingsTheme.ets", hash: "35c48c7e0af90308ff1253b1edef2b113367028cdb90ed94de1eccbae031a0e5", lineCnt: 305, count: 0, projectPath: "", functions: { 0: { name: "SettingsTheme.normalizeAppearanceMode", count: 0, regions: { 0: { startLoc: { line: 18, col: 3 }, endLoc: { line: 26, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 19, col: 26 }, endLoc: { line: 21, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 22, col: 27 }, endLoc: { line: 24, col: 6 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 25, col: 5 }, endLoc: { line: 26, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 19, col: 9 }, endLoc: { line: 19, col: 24 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 22, col: 9 }, endLoc: { line: 22, col: 25 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 0 }, 1: { name: "SettingsTheme.getStoredAppearanceMode", count: 0, regions: { 0: { startLoc: { line: 28, col: 3 }, endLoc: { line: 30, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 29, col: 5 }, endLoc: { line: 30, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 1 }, 2: { name: "SettingsTheme.detectSystemDark", count: 0, regions: { 0: { startLoc: { line: 32, col: 3 }, endLoc: { line: 41, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 34, col: 72 }, endLoc: { line: 36, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 37, col: 73 }, endLoc: { line: 39, col: 6 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 40, col: 5 }, endLoc: { line: 41, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 34, col: 9 }, endLoc: { line: 34, col: 70 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 37, col: 9 }, endLoc: { line: 37, col: 71 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 2 }, 3: { name: "SettingsTheme.initAppearance", count: 0, regions: { 0: { startLoc: { line: 43, col: 3 }, endLoc: { line: 49, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 44, col: 5 }, endLoc: { line: 49, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 45, col: 24 }, endLoc: { line: 45, col: 60 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 3 }, 4: { name: "SettingsTheme.updateSystemAppearance", count: 0, regions: { 0: { startLoc: { line: 51, col: 3 }, endLoc: { line: 59, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 52, col: 72 }, endLoc: { line: 55, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 56, col: 73 }, endLoc: { line: 58, col: 6 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 52, col: 9 }, endLoc: { line: 52, col: 70 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 56, col: 9 }, endLoc: { line: 56, col: 71 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 4 }, 5: { name: "SettingsTheme.colorModeForAppearance", count: 0, regions: { 0: { startLoc: { line: 61, col: 3 }, endLoc: { line: 69, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 62, col: 26 }, endLoc: { line: 64, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 65, col: 27 }, endLoc: { line: 67, col: 6 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 68, col: 5 }, endLoc: { line: 69, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 62, col: 9 }, endLoc: { line: 62, col: 24 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 65, col: 9 }, endLoc: { line: 65, col: 25 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 5 }, 6: { name: "SettingsTheme.applyAppearanceModeToApplication", count: 0, regions: { 0: { startLoc: { line: 71, col: 3 }, endLoc: { line: 78, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 72, col: 9 }, endLoc: { line: 75, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 75, col: 7 }, endLoc: { line: 77, col: 6 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 6 }, 7: { name: "SettingsTheme.applyAppearanceMode", count: 0, regions: { 0: { startLoc: { line: 80, col: 3 }, endLoc: { line: 89, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 82, col: 23 }, endLoc: { line: 84, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 86, col: 5 }, endLoc: { line: 89, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 82, col: 9 }, endLoc: { line: 82, col: 21 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 7 }, 8: { name: "SettingsTheme.isSystemDark", count: 0, regions: { 0: { startLoc: { line: 91, col: 3 }, endLoc: { line: 104, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 93, col: 23 }, endLoc: { line: 95, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 99, col: 28 }, endLoc: { line: 102, col: 6 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 103, col: 5 }, endLoc: { line: 104, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 93, col: 9 }, endLoc: { line: 93, col: 21 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 99, col: 9 }, endLoc: { line: 99, col: 26 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 8 }, 9: { name: "SettingsTheme.isDark", count: 0, regions: { 0: { startLoc: { line: 106, col: 3 }, endLoc: { line: 114, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 107, col: 26 }, endLoc: { line: 109, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 110, col: 27 }, endLoc: { line: 112, col: 6 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 113, col: 5 }, endLoc: { line: 114, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 107, col: 9 }, endLoc: { line: 107, col: 24 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 110, col: 9 }, endLoc: { line: 110, col: 25 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 9 }, 10: { name: "SettingsTheme.animate", count: 0, regions: { 0: { startLoc: { line: 116, col: 3 }, endLoc: { line: 121, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 117, col: 5 }, endLoc: { line: 121, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 10 }, 11: { name: "anonymous_0", count: 0, regions: { 0: { startLoc: { line: 116, col: 48 }, endLoc: { line: 116, col: 58 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 11 }, 12: { name: "SettingsTheme.pageBackground", count: 0, regions: { 0: { startLoc: { line: 123, col: 3 }, endLoc: { line: 125, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 124, col: 5 }, endLoc: { line: 125, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 124, col: 12 }, endLoc: { line: 124, col: 42 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 12 }, 13: { name: "SettingsTheme.cardBackground", count: 0, regions: { 0: { startLoc: { line: 127, col: 3 }, endLoc: { line: 129, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 128, col: 5 }, endLoc: { line: 129, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 128, col: 12 }, endLoc: { line: 128, col: 42 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 13 }, 14: { name: "SettingsTheme.cardHoverBackground", count: 0, regions: { 0: { startLoc: { line: 131, col: 3 }, endLoc: { line: 133, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 132, col: 5 }, endLoc: { line: 133, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 132, col: 12 }, endLoc: { line: 132, col: 42 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 14 }, 15: { name: "SettingsTheme.primaryText", count: 0, regions: { 0: { startLoc: { line: 135, col: 3 }, endLoc: { line: 137, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 136, col: 5 }, endLoc: { line: 137, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 136, col: 12 }, endLoc: { line: 136, col: 42 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 15 }, 16: { name: "SettingsTheme.secondaryText", count: 0, regions: { 0: { startLoc: { line: 139, col: 3 }, endLoc: { line: 141, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 140, col: 5 }, endLoc: { line: 141, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 140, col: 12 }, endLoc: { line: 140, col: 42 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 16 }, 17: { name: "SettingsTheme.mutedText", count: 0, regions: { 0: { startLoc: { line: 143, col: 3 }, endLoc: { line: 145, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 144, col: 5 }, endLoc: { line: 145, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 144, col: 12 }, endLoc: { line: 144, col: 42 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 17 }, 18: { name: "SettingsTheme.disabledText", count: 0, regions: { 0: { startLoc: { line: 147, col: 3 }, endLoc: { line: 149, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 148, col: 5 }, endLoc: { line: 149, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 148, col: 12 }, endLoc: { line: 148, col: 42 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 18 }, 19: { name: "SettingsTheme.accentColor", count: 0, regions: { 0: { startLoc: { line: 151, col: 3 }, endLoc: { line: 171, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 152, col: 47 }, endLoc: { line: 154, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 155, col: 46 }, endLoc: { line: 157, col: 6 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 158, col: 46 }, endLoc: { line: 160, col: 6 }, count: 0, ignored: 0 }, 4: { startLoc: { line: 161, col: 45 }, endLoc: { line: 163, col: 6 }, count: 0, ignored: 0 }, 5: { startLoc: { line: 164, col: 47 }, endLoc: { line: 166, col: 6 }, count: 0, ignored: 0 }, 6: { startLoc: { line: 167, col: 47 }, endLoc: { line: 169, col: 6 }, count: 0, ignored: 0 }, 7: { startLoc: { line: 170, col: 5 }, endLoc: { line: 171, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 152, col: 9 }, endLoc: { line: 152, col: 45 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 153, col: 14 }, endLoc: { line: 153, col: 44 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 2: { startLoc: { line: 155, col: 9 }, endLoc: { line: 155, col: 44 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 3: { startLoc: { line: 156, col: 14 }, endLoc: { line: 156, col: 44 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 4: { startLoc: { line: 158, col: 9 }, endLoc: { line: 158, col: 44 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 5: { startLoc: { line: 159, col: 14 }, endLoc: { line: 159, col: 44 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 6: { startLoc: { line: 161, col: 9 }, endLoc: { line: 161, col: 43 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 7: { startLoc: { line: 162, col: 14 }, endLoc: { line: 162, col: 44 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 8: { startLoc: { line: 164, col: 9 }, endLoc: { line: 164, col: 45 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 9: { startLoc: { line: 165, col: 14 }, endLoc: { line: 165, col: 44 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 10: { startLoc: { line: 167, col: 9 }, endLoc: { line: 167, col: 45 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 11: { startLoc: { line: 168, col: 14 }, endLoc: { line: 168, col: 44 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 12: { startLoc: { line: 170, col: 12 }, endLoc: { line: 170, col: 42 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 19 }, 20: { name: "SettingsTheme.accentBackground", count: 0, regions: { 0: { startLoc: { line: 173, col: 3 }, endLoc: { line: 193, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 174, col: 47 }, endLoc: { line: 176, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 177, col: 46 }, endLoc: { line: 179, col: 6 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 180, col: 46 }, endLoc: { line: 182, col: 6 }, count: 0, ignored: 0 }, 4: { startLoc: { line: 183, col: 45 }, endLoc: { line: 185, col: 6 }, count: 0, ignored: 0 }, 5: { startLoc: { line: 186, col: 47 }, endLoc: { line: 188, col: 6 }, count: 0, ignored: 0 }, 6: { startLoc: { line: 189, col: 47 }, endLoc: { line: 191, col: 6 }, count: 0, ignored: 0 }, 7: { startLoc: { line: 192, col: 5 }, endLoc: { line: 193, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 174, col: 9 }, endLoc: { line: 174, col: 45 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 175, col: 14 }, endLoc: { line: 175, col: 44 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 2: { startLoc: { line: 177, col: 9 }, endLoc: { line: 177, col: 44 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 3: { startLoc: { line: 178, col: 14 }, endLoc: { line: 178, col: 44 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 4: { startLoc: { line: 180, col: 9 }, endLoc: { line: 180, col: 44 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 5: { startLoc: { line: 181, col: 14 }, endLoc: { line: 181, col: 44 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 6: { startLoc: { line: 183, col: 9 }, endLoc: { line: 183, col: 43 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 7: { startLoc: { line: 184, col: 14 }, endLoc: { line: 184, col: 44 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 8: { startLoc: { line: 186, col: 9 }, endLoc: { line: 186, col: 45 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 9: { startLoc: { line: 187, col: 14 }, endLoc: { line: 187, col: 44 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 10: { startLoc: { line: 189, col: 9 }, endLoc: { line: 189, col: 45 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 11: { startLoc: { line: 190, col: 14 }, endLoc: { line: 190, col: 44 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 12: { startLoc: { line: 192, col: 12 }, endLoc: { line: 192, col: 42 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 20 }, 21: { name: "SettingsTheme.accentHoverBackground", count: 0, regions: { 0: { startLoc: { line: 195, col: 3 }, endLoc: { line: 215, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 196, col: 47 }, endLoc: { line: 198, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 199, col: 46 }, endLoc: { line: 201, col: 6 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 202, col: 46 }, endLoc: { line: 204, col: 6 }, count: 0, ignored: 0 }, 4: { startLoc: { line: 205, col: 45 }, endLoc: { line: 207, col: 6 }, count: 0, ignored: 0 }, 5: { startLoc: { line: 208, col: 47 }, endLoc: { line: 210, col: 6 }, count: 0, ignored: 0 }, 6: { startLoc: { line: 211, col: 47 }, endLoc: { line: 213, col: 6 }, count: 0, ignored: 0 }, 7: { startLoc: { line: 214, col: 5 }, endLoc: { line: 215, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 196, col: 9 }, endLoc: { line: 196, col: 45 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 197, col: 14 }, endLoc: { line: 197, col: 44 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 2: { startLoc: { line: 199, col: 9 }, endLoc: { line: 199, col: 44 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 3: { startLoc: { line: 200, col: 14 }, endLoc: { line: 200, col: 44 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 4: { startLoc: { line: 202, col: 9 }, endLoc: { line: 202, col: 44 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 5: { startLoc: { line: 203, col: 14 }, endLoc: { line: 203, col: 44 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 6: { startLoc: { line: 205, col: 9 }, endLoc: { line: 205, col: 43 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 7: { startLoc: { line: 206, col: 14 }, endLoc: { line: 206, col: 44 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 8: { startLoc: { line: 208, col: 9 }, endLoc: { line: 208, col: 45 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 9: { startLoc: { line: 209, col: 14 }, endLoc: { line: 209, col: 44 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 10: { startLoc: { line: 211, col: 9 }, endLoc: { line: 211, col: 45 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 11: { startLoc: { line: 212, col: 14 }, endLoc: { line: 212, col: 44 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 12: { startLoc: { line: 214, col: 12 }, endLoc: { line: 214, col: 42 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 21 }, 22: { name: "SettingsTheme.borderColor", count: 0, regions: { 0: { startLoc: { line: 217, col: 3 }, endLoc: { line: 222, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 218, col: 17 }, endLoc: { line: 220, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 221, col: 5 }, endLoc: { line: 222, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 218, col: 9 }, endLoc: { line: 218, col: 15 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 221, col: 12 }, endLoc: { line: 221, col: 42 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 22 }, 23: { name: "SettingsTheme.transparentButton", count: 0, regions: { 0: { startLoc: { line: 224, col: 3 }, endLoc: { line: 232, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 225, col: 31 }, endLoc: { line: 227, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 228, col: 18 }, endLoc: { line: 230, col: 6 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 231, col: 5 }, endLoc: { line: 232, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 225, col: 9 }, endLoc: { line: 225, col: 29 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 228, col: 9 }, endLoc: { line: 228, col: 16 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 2: { startLoc: { line: 229, col: 14 }, endLoc: { line: 229, col: 68 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 3: { startLoc: { line: 231, col: 12 }, endLoc: { line: 231, col: 66 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 23 }, 24: { name: "SettingsTheme.subtleButton", count: 0, regions: { 0: { startLoc: { line: 234, col: 3 }, endLoc: { line: 242, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 235, col: 18 }, endLoc: { line: 237, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 238, col: 18 }, endLoc: { line: 240, col: 6 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 241, col: 5 }, endLoc: { line: 242, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 235, col: 9 }, endLoc: { line: 235, col: 16 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 236, col: 14 }, endLoc: { line: 236, col: 44 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 2: { startLoc: { line: 238, col: 9 }, endLoc: { line: 238, col: 16 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 3: { startLoc: { line: 239, col: 14 }, endLoc: { line: 239, col: 44 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 4: { startLoc: { line: 241, col: 12 }, endLoc: { line: 241, col: 42 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 24 }, 25: { name: "SettingsTheme.disabledButton", count: 0, regions: { 0: { startLoc: { line: 244, col: 3 }, endLoc: { line: 246, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 245, col: 5 }, endLoc: { line: 246, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 245, col: 12 }, endLoc: { line: 245, col: 42 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 25 }, 26: { name: "SettingsTheme.shadow", count: 0, regions: { 0: { startLoc: { line: 248, col: 3 }, endLoc: { line: 255, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 249, col: 5 }, endLoc: { line: 255, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 250, col: 15 }, endLoc: { line: 250, col: 31 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 251, col: 14 }, endLoc: { line: 251, col: 100 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 2: { startLoc: { line: 251, col: 24 }, endLoc: { line: 251, col: 59 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 3: { startLoc: { line: 251, col: 64 }, endLoc: { line: 251, col: 99 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 4: { startLoc: { line: 253, col: 16 }, endLoc: { line: 253, col: 31 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 26 }, 27: { name: "SettingsTheme.statusText", count: 0, regions: { 0: { startLoc: { line: 257, col: 3 }, endLoc: { line: 271, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 258, col: 24 }, endLoc: { line: 260, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 261, col: 29 }, endLoc: { line: 263, col: 6 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 264, col: 28 }, endLoc: { line: 266, col: 6 }, count: 0, ignored: 0 }, 4: { startLoc: { line: 267, col: 26 }, endLoc: { line: 269, col: 6 }, count: 0, ignored: 0 }, 5: { startLoc: { line: 270, col: 5 }, endLoc: { line: 271, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 258, col: 9 }, endLoc: { line: 258, col: 22 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 259, col: 14 }, endLoc: { line: 259, col: 44 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 2: { startLoc: { line: 261, col: 9 }, endLoc: { line: 261, col: 27 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 3: { startLoc: { line: 262, col: 14 }, endLoc: { line: 262, col: 44 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 4: { startLoc: { line: 264, col: 9 }, endLoc: { line: 264, col: 26 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 5: { startLoc: { line: 265, col: 14 }, endLoc: { line: 265, col: 44 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 6: { startLoc: { line: 267, col: 9 }, endLoc: { line: 267, col: 24 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 7: { startLoc: { line: 268, col: 14 }, endLoc: { line: 268, col: 44 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 8: { startLoc: { line: 270, col: 12 }, endLoc: { line: 270, col: 42 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 27 }, 28: { name: "SettingsTheme.statusBackground", count: 0, regions: { 0: { startLoc: { line: 273, col: 3 }, endLoc: { line: 287, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 274, col: 24 }, endLoc: { line: 276, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 277, col: 29 }, endLoc: { line: 279, col: 6 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 280, col: 28 }, endLoc: { line: 282, col: 6 }, count: 0, ignored: 0 }, 4: { startLoc: { line: 283, col: 26 }, endLoc: { line: 285, col: 6 }, count: 0, ignored: 0 }, 5: { startLoc: { line: 286, col: 5 }, endLoc: { line: 287, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 274, col: 9 }, endLoc: { line: 274, col: 22 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 275, col: 14 }, endLoc: { line: 275, col: 44 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 2: { startLoc: { line: 277, col: 9 }, endLoc: { line: 277, col: 27 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 3: { startLoc: { line: 278, col: 14 }, endLoc: { line: 278, col: 44 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 4: { startLoc: { line: 280, col: 9 }, endLoc: { line: 280, col: 26 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 5: { startLoc: { line: 281, col: 14 }, endLoc: { line: 281, col: 44 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 6: { startLoc: { line: 283, col: 9 }, endLoc: { line: 283, col: 24 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 7: { startLoc: { line: 284, col: 14 }, endLoc: { line: 284, col: 44 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 8: { startLoc: { line: 286, col: 12 }, endLoc: { line: 286, col: 42 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 28 }, 29: { name: "SettingsTheme.statusBorder", count: 0, regions: { 0: { startLoc: { line: 289, col: 3 }, endLoc: { line: 303, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 290, col: 24 }, endLoc: { line: 292, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 293, col: 29 }, endLoc: { line: 295, col: 6 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 296, col: 28 }, endLoc: { line: 298, col: 6 }, count: 0, ignored: 0 }, 4: { startLoc: { line: 299, col: 26 }, endLoc: { line: 301, col: 6 }, count: 0, ignored: 0 }, 5: { startLoc: { line: 302, col: 5 }, endLoc: { line: 303, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 290, col: 9 }, endLoc: { line: 290, col: 22 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 291, col: 14 }, endLoc: { line: 291, col: 44 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 2: { startLoc: { line: 293, col: 9 }, endLoc: { line: 293, col: 27 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 3: { startLoc: { line: 294, col: 14 }, endLoc: { line: 294, col: 44 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 4: { startLoc: { line: 296, col: 9 }, endLoc: { line: 296, col: 26 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 5: { startLoc: { line: 297, col: 14 }, endLoc: { line: 297, col: 44 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 6: { startLoc: { line: 299, col: 9 }, endLoc: { line: 299, col: 24 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 7: { startLoc: { line: 300, col: 14 }, endLoc: { line: 300, col: 44 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 29 } }, exeLine: { 0: 1, 1: 2, 2: 3, 3: 4, 4: 5, 5: 6, 6: 7, 7: 8, 8: 9, 9: 11, 10: 12, 11: 13, 12: 14, 13: 15, 14: 16, 15: 18, 16: 19, 17: 20, 18: 22, 19: 23, 20: 25, 21: 28, 22: 29, 23: 32, 24: 33, 25: 34, 26: 35, 27: 37, 28: 38, 29: 40, 30: 43, 31: 44, 32: 45, 33: 46, 34: 47, 35: 48, 36: 51, 37: 52, 38: 53, 39: 54, 40: 56, 41: 57, 42: 61, 43: 62, 44: 63, 45: 65, 46: 66, 47: 68, 48: 71, 49: 72, 50: 73, 51: 74, 52: 75, 53: 76, 54: 80, 55: 81, 56: 82, 57: 83, 58: 86, 59: 87, 60: 88, 61: 91, 62: 92, 63: 93, 64: 94, 65: 97, 66: 98, 67: 99, 68: 100, 69: 101, 70: 103, 71: 106, 72: 107, 73: 108, 74: 110, 75: 111, 76: 113, 77: 116, 78: 117, 79: 118, 80: 119, 81: 120, 82: 123, 83: 124, 84: 127, 85: 128, 86: 131, 87: 132, 88: 135, 89: 136, 90: 139, 91: 140, 92: 143, 93: 144, 94: 147, 95: 148, 96: 151, 97: 152, 98: 153, 99: 155, 100: 156, 101: 158, 102: 159, 103: 161, 104: 162, 105: 164, 106: 165, 107: 167, 108: 168, 109: 170, 110: 173, 111: 174, 112: 175, 113: 177, 114: 178, 115: 180, 116: 181, 117: 183, 118: 184, 119: 186, 120: 187, 121: 189, 122: 190, 123: 192, 124: 195, 125: 196, 126: 197, 127: 199, 128: 200, 129: 202, 130: 203, 131: 205, 132: 206, 133: 208, 134: 209, 135: 211, 136: 212, 137: 214, 138: 217, 139: 218, 140: 219, 141: 221, 142: 224, 143: 225, 144: 226, 145: 228, 146: 229, 147: 231, 148: 234, 149: 235, 150: 236, 151: 238, 152: 239, 153: 241, 154: 244, 155: 245, 156: 248, 157: 249, 158: 250, 159: 251, 160: 252, 161: 253, 162: 257, 163: 258, 164: 259, 165: 261, 166: 262, 167: 264, 168: 265, 169: 267, 170: 268, 171: 270, 172: 273, 173: 274, 174: 275, 175: 277, 176: 278, 177: 280, 178: 281, 179: 283, 180: 284, 181: 286, 182: 289, 183: 290, 184: 291, 185: 293, 186: 294, 187: 296, 188: 297, 189: 299, 190: 300, 191: 302 } });
export class SettingsTheme {
    static readonly MOTION_MS: number = 180;
    static readonly CARD_RADIUS: number = 12;
    static readonly BUTTON_RADIUS: number = 12;
    static readonly ICON_TILE_SIZE: number = 38;
    static readonly ICON_SIZE: number = 20;
    static normalizeAppearanceMode(mode: string | undefined): SettingsAppearanceMode {
        bjccovmshb1ic5.instrumentFunction(0);
        if (mode === 'dark') {
            bjccovmshb1ic5.instrumentBranch(0, 0, true);
            bjccovmshb1ic5.instrumentRegion(0, 1);
            return 'dark';
        }
        else {
            bjccovmshb1ic5.instrumentBranch(0, 0, false);
        }
        if (mode === 'light') {
            bjccovmshb1ic5.instrumentBranch(0, 1, true);
            bjccovmshb1ic5.instrumentRegion(0, 2);
            return 'light';
        }
        else {
            bjccovmshb1ic5.instrumentBranch(0, 1, false);
        }
        bjccovmshb1ic5.instrumentRegion(0, 3);
        return 'system';
    }
    static getStoredAppearanceMode(): SettingsAppearanceMode {
        bjccovmshb1ic5.instrumentFunction(1);
        bjccovmshb1ic5.instrumentRegion(1, 1);
        return SettingsTheme.normalizeAppearanceMode(AppStorage.get<string>(SETTINGS_STORAGE_APPEARANCE_MODE));
    }
    static detectSystemDark(context: common.UIAbilityContext): boolean | null {
        bjccovmshb1ic5.instrumentFunction(2);
        const colorMode = context.config?.colorMode;
        if (colorMode === ConfigurationConstant.ColorMode.COLOR_MODE_DARK) {
            bjccovmshb1ic5.instrumentBranch(2, 0, true);
            bjccovmshb1ic5.instrumentRegion(2, 1);
            return true;
        }
        else {
            bjccovmshb1ic5.instrumentBranch(2, 0, false);
        }
        if (colorMode === ConfigurationConstant.ColorMode.COLOR_MODE_LIGHT) {
            bjccovmshb1ic5.instrumentBranch(2, 1, true);
            bjccovmshb1ic5.instrumentRegion(2, 2);
            return false;
        }
        else {
            bjccovmshb1ic5.instrumentBranch(2, 1, false);
        }
        bjccovmshb1ic5.instrumentRegion(2, 3);
        return null;
    }
    static initAppearance(context: common.UIAbilityContext): void {
        bjccovmshb1ic5.instrumentFunction(3);
        bjccovmshb1ic5.instrumentRegion(3, 1);
        const detected = SettingsTheme.detectSystemDark(context);
        const systemDark = detected === null ? (bjccovmshb1ic5.instrumentBranch(3, 0, true), false) : (bjccovmshb1ic5.instrumentBranch(3, 0, false), detected);
        AppStorage.setOrCreate<boolean>(SETTINGS_STORAGE_SYSTEM_DARK, systemDark);
        AppStorage.setOrCreate<SettingsAppearanceMode>(SETTINGS_STORAGE_APPEARANCE_MODE, SettingsTheme.getStoredAppearanceMode());
    }
    static updateSystemAppearance(colorMode: ConfigurationConstant.ColorMode | undefined): void {
        bjccovmshb1ic5.instrumentFunction(4);
        if (colorMode === ConfigurationConstant.ColorMode.COLOR_MODE_DARK) {
            bjccovmshb1ic5.instrumentBranch(4, 0, true);
            bjccovmshb1ic5.instrumentRegion(4, 1);
            AppStorage.setOrCreate<boolean>(SETTINGS_STORAGE_SYSTEM_DARK, true);
            return;
        }
        else {
            bjccovmshb1ic5.instrumentBranch(4, 0, false);
        }
        if (colorMode === ConfigurationConstant.ColorMode.COLOR_MODE_LIGHT) {
            bjccovmshb1ic5.instrumentBranch(4, 1, true);
            bjccovmshb1ic5.instrumentRegion(4, 2);
            AppStorage.setOrCreate<boolean>(SETTINGS_STORAGE_SYSTEM_DARK, false);
        }
        else {
            bjccovmshb1ic5.instrumentBranch(4, 1, false);
        }
    }
    static colorModeForAppearance(mode: SettingsAppearanceMode): ConfigurationConstant.ColorMode {
        bjccovmshb1ic5.instrumentFunction(5);
        if (mode === 'dark') {
            bjccovmshb1ic5.instrumentBranch(5, 0, true);
            bjccovmshb1ic5.instrumentRegion(5, 1);
            return ConfigurationConstant.ColorMode.COLOR_MODE_DARK;
        }
        else {
            bjccovmshb1ic5.instrumentBranch(5, 0, false);
        }
        if (mode === 'light') {
            bjccovmshb1ic5.instrumentBranch(5, 1, true);
            bjccovmshb1ic5.instrumentRegion(5, 2);
            return ConfigurationConstant.ColorMode.COLOR_MODE_LIGHT;
        }
        else {
            bjccovmshb1ic5.instrumentBranch(5, 1, false);
        }
        bjccovmshb1ic5.instrumentRegion(5, 3);
        return ConfigurationConstant.ColorMode.COLOR_MODE_NOT_SET;
    }
    static applyAppearanceModeToApplication(context: common.UIAbilityContext, mode: SettingsAppearanceMode): boolean {
        bjccovmshb1ic5.instrumentFunction(6);
        try {
            bjccovmshb1ic5.instrumentRegion(6, 1);
            context.getApplicationContext().setColorMode(SettingsTheme.colorModeForAppearance(mode));
            return true;
        }
        catch (_error) {
            bjccovmshb1ic5.instrumentRegion(6, 2);
            return false;
        }
    }
    static applyAppearanceMode(uiContext: UIContext, mode: SettingsAppearanceMode): boolean {
        bjccovmshb1ic5.instrumentFunction(7);
        const hostContext = uiContext.getHostContext();
        if (!hostContext) {
            bjccovmshb1ic5.instrumentBranch(7, 0, true);
            bjccovmshb1ic5.instrumentRegion(7, 1);
            return false;
        }
        else {
            bjccovmshb1ic5.instrumentBranch(7, 0, false);
        }
        bjccovmshb1ic5.instrumentRegion(7, 2);
        const abilityContext = hostContext as common.UIAbilityContext;
        AppStorage.setOrCreate<SettingsAppearanceMode>(SETTINGS_STORAGE_APPEARANCE_MODE, mode);
        return SettingsTheme.applyAppearanceModeToApplication(abilityContext, mode);
    }
    static isSystemDark(uiContext: UIContext): boolean {
        bjccovmshb1ic5.instrumentFunction(8);
        const hostContext = uiContext.getHostContext();
        if (!hostContext) {
            bjccovmshb1ic5.instrumentBranch(8, 0, true);
            bjccovmshb1ic5.instrumentRegion(8, 1);
            return AppStorage.get<boolean>(SETTINGS_STORAGE_SYSTEM_DARK) === true;
        }
        else {
            bjccovmshb1ic5.instrumentBranch(8, 0, false);
        }
        const abilityContext = hostContext as common.UIAbilityContext;
        const detected = SettingsTheme.detectSystemDark(abilityContext);
        if (detected !== null) {
            bjccovmshb1ic5.instrumentBranch(8, 1, true);
            bjccovmshb1ic5.instrumentRegion(8, 2);
            AppStorage.setOrCreate<boolean>(SETTINGS_STORAGE_SYSTEM_DARK, detected);
            return detected;
        }
        else {
            bjccovmshb1ic5.instrumentBranch(8, 1, false);
        }
        bjccovmshb1ic5.instrumentRegion(8, 3);
        return AppStorage.get<boolean>(SETTINGS_STORAGE_SYSTEM_DARK) === true;
    }
    static isDark(uiContext: UIContext, mode: SettingsAppearanceMode): boolean {
        bjccovmshb1ic5.instrumentFunction(9);
        if (mode === 'dark') {
            bjccovmshb1ic5.instrumentBranch(9, 0, true);
            bjccovmshb1ic5.instrumentRegion(9, 1);
            return true;
        }
        else {
            bjccovmshb1ic5.instrumentBranch(9, 0, false);
        }
        if (mode === 'light') {
            bjccovmshb1ic5.instrumentBranch(9, 1, true);
            bjccovmshb1ic5.instrumentRegion(9, 2);
            return false;
        }
        else {
            bjccovmshb1ic5.instrumentBranch(9, 1, false);
        }
        bjccovmshb1ic5.instrumentRegion(9, 3);
        return SettingsTheme.isSystemDark(uiContext);
    }
    static animate(uiContext: UIContext, update: () => void): void {
        bjccovmshb1ic5.instrumentFunction(10);
        bjccovmshb1ic5.instrumentRegion(10, 1);
        uiContext.animateTo({
            duration: SettingsTheme.MOTION_MS,
            curve: Curve.EaseOut
        }, update);
    }
    static pageBackground(isDark: boolean): string {
        bjccovmshb1ic5.instrumentFunction(12);
        bjccovmshb1ic5.instrumentRegion(12, 1);
        return isDark ? (bjccovmshb1ic5.instrumentBranch(12, 0, true), '#101114') : (bjccovmshb1ic5.instrumentBranch(12, 0, false), '#F6F7F9');
    }
    static cardBackground(isDark: boolean): string {
        bjccovmshb1ic5.instrumentFunction(13);
        bjccovmshb1ic5.instrumentRegion(13, 1);
        return isDark ? (bjccovmshb1ic5.instrumentBranch(13, 0, true), '#181A1F') : (bjccovmshb1ic5.instrumentBranch(13, 0, false), '#FFFFFF');
    }
    static cardHoverBackground(isDark: boolean): string {
        bjccovmshb1ic5.instrumentFunction(14);
        bjccovmshb1ic5.instrumentRegion(14, 1);
        return isDark ? (bjccovmshb1ic5.instrumentBranch(14, 0, true), '#20232A') : (bjccovmshb1ic5.instrumentBranch(14, 0, false), '#F8FAFC');
    }
    static primaryText(isDark: boolean): string {
        bjccovmshb1ic5.instrumentFunction(15);
        bjccovmshb1ic5.instrumentRegion(15, 1);
        return isDark ? (bjccovmshb1ic5.instrumentBranch(15, 0, true), '#F8FAFC') : (bjccovmshb1ic5.instrumentBranch(15, 0, false), '#101828');
    }
    static secondaryText(isDark: boolean): string {
        bjccovmshb1ic5.instrumentFunction(16);
        bjccovmshb1ic5.instrumentRegion(16, 1);
        return isDark ? (bjccovmshb1ic5.instrumentBranch(16, 0, true), '#D1D5DB') : (bjccovmshb1ic5.instrumentBranch(16, 0, false), '#374151');
    }
    static mutedText(isDark: boolean): string {
        bjccovmshb1ic5.instrumentFunction(17);
        bjccovmshb1ic5.instrumentRegion(17, 1);
        return isDark ? (bjccovmshb1ic5.instrumentBranch(17, 0, true), '#9CA3AF') : (bjccovmshb1ic5.instrumentBranch(17, 0, false), '#6B7280');
    }
    static disabledText(isDark: boolean): string {
        bjccovmshb1ic5.instrumentFunction(18);
        bjccovmshb1ic5.instrumentRegion(18, 1);
        return isDark ? (bjccovmshb1ic5.instrumentBranch(18, 0, true), '#6B7280') : (bjccovmshb1ic5.instrumentBranch(18, 0, false), '#A0A7B2');
    }
    static accentColor(isDark: boolean, accentName: SettingsAccentName): string {
        bjccovmshb1ic5.instrumentFunction(19);
        if (accentName === SettingsAccent.PURPLE) {
            bjccovmshb1ic5.instrumentBranch(19, 0, true);
            bjccovmshb1ic5.instrumentRegion(19, 1);
            return isDark ? (bjccovmshb1ic5.instrumentBranch(19, 1, true), '#C4B5FD') : (bjccovmshb1ic5.instrumentBranch(19, 1, false), '#7C3AED');
        }
        else {
            bjccovmshb1ic5.instrumentBranch(19, 0, false);
        }
        if (accentName === SettingsAccent.GREEN) {
            bjccovmshb1ic5.instrumentBranch(19, 2, true);
            bjccovmshb1ic5.instrumentRegion(19, 2);
            return isDark ? (bjccovmshb1ic5.instrumentBranch(19, 3, true), '#86EFAC') : (bjccovmshb1ic5.instrumentBranch(19, 3, false), '#16A34A');
        }
        else {
            bjccovmshb1ic5.instrumentBranch(19, 2, false);
        }
        if (accentName === SettingsAccent.AMBER) {
            bjccovmshb1ic5.instrumentBranch(19, 4, true);
            bjccovmshb1ic5.instrumentRegion(19, 3);
            return isDark ? (bjccovmshb1ic5.instrumentBranch(19, 5, true), '#FCD34D') : (bjccovmshb1ic5.instrumentBranch(19, 5, false), '#D97706');
        }
        else {
            bjccovmshb1ic5.instrumentBranch(19, 4, false);
        }
        if (accentName === SettingsAccent.CYAN) {
            bjccovmshb1ic5.instrumentBranch(19, 6, true);
            bjccovmshb1ic5.instrumentRegion(19, 4);
            return isDark ? (bjccovmshb1ic5.instrumentBranch(19, 7, true), '#67E8F9') : (bjccovmshb1ic5.instrumentBranch(19, 7, false), '#0891B2');
        }
        else {
            bjccovmshb1ic5.instrumentBranch(19, 6, false);
        }
        if (accentName === SettingsAccent.ORANGE) {
            bjccovmshb1ic5.instrumentBranch(19, 8, true);
            bjccovmshb1ic5.instrumentRegion(19, 5);
            return isDark ? (bjccovmshb1ic5.instrumentBranch(19, 9, true), '#FDBA74') : (bjccovmshb1ic5.instrumentBranch(19, 9, false), '#EA580C');
        }
        else {
            bjccovmshb1ic5.instrumentBranch(19, 8, false);
        }
        if (accentName === SettingsAccent.INDIGO) {
            bjccovmshb1ic5.instrumentBranch(19, 10, true);
            bjccovmshb1ic5.instrumentRegion(19, 6);
            return isDark ? (bjccovmshb1ic5.instrumentBranch(19, 11, true), '#A5B4FC') : (bjccovmshb1ic5.instrumentBranch(19, 11, false), '#4F46E5');
        }
        else {
            bjccovmshb1ic5.instrumentBranch(19, 10, false);
        }
        bjccovmshb1ic5.instrumentRegion(19, 7);
        return isDark ? (bjccovmshb1ic5.instrumentBranch(19, 12, true), '#72B7FF') : (bjccovmshb1ic5.instrumentBranch(19, 12, false), '#0A84FF');
    }
    static accentBackground(isDark: boolean, accentName: SettingsAccentName): string {
        bjccovmshb1ic5.instrumentFunction(20);
        if (accentName === SettingsAccent.PURPLE) {
            bjccovmshb1ic5.instrumentBranch(20, 0, true);
            bjccovmshb1ic5.instrumentRegion(20, 1);
            return isDark ? (bjccovmshb1ic5.instrumentBranch(20, 1, true), '#35245F') : (bjccovmshb1ic5.instrumentBranch(20, 1, false), '#F3E8FF');
        }
        else {
            bjccovmshb1ic5.instrumentBranch(20, 0, false);
        }
        if (accentName === SettingsAccent.GREEN) {
            bjccovmshb1ic5.instrumentBranch(20, 2, true);
            bjccovmshb1ic5.instrumentRegion(20, 2);
            return isDark ? (bjccovmshb1ic5.instrumentBranch(20, 3, true), '#173F2A') : (bjccovmshb1ic5.instrumentBranch(20, 3, false), '#EAF8EF');
        }
        else {
            bjccovmshb1ic5.instrumentBranch(20, 2, false);
        }
        if (accentName === SettingsAccent.AMBER) {
            bjccovmshb1ic5.instrumentBranch(20, 4, true);
            bjccovmshb1ic5.instrumentRegion(20, 3);
            return isDark ? (bjccovmshb1ic5.instrumentBranch(20, 5, true), '#4A3718') : (bjccovmshb1ic5.instrumentBranch(20, 5, false), '#FFF4D6');
        }
        else {
            bjccovmshb1ic5.instrumentBranch(20, 4, false);
        }
        if (accentName === SettingsAccent.CYAN) {
            bjccovmshb1ic5.instrumentBranch(20, 6, true);
            bjccovmshb1ic5.instrumentRegion(20, 4);
            return isDark ? (bjccovmshb1ic5.instrumentBranch(20, 7, true), '#153F47') : (bjccovmshb1ic5.instrumentBranch(20, 7, false), '#E8FAFC');
        }
        else {
            bjccovmshb1ic5.instrumentBranch(20, 6, false);
        }
        if (accentName === SettingsAccent.ORANGE) {
            bjccovmshb1ic5.instrumentBranch(20, 8, true);
            bjccovmshb1ic5.instrumentRegion(20, 5);
            return isDark ? (bjccovmshb1ic5.instrumentBranch(20, 9, true), '#4A2C16') : (bjccovmshb1ic5.instrumentBranch(20, 9, false), '#FFF0E5');
        }
        else {
            bjccovmshb1ic5.instrumentBranch(20, 8, false);
        }
        if (accentName === SettingsAccent.INDIGO) {
            bjccovmshb1ic5.instrumentBranch(20, 10, true);
            bjccovmshb1ic5.instrumentRegion(20, 6);
            return isDark ? (bjccovmshb1ic5.instrumentBranch(20, 11, true), '#272D61') : (bjccovmshb1ic5.instrumentBranch(20, 11, false), '#EEF2FF');
        }
        else {
            bjccovmshb1ic5.instrumentBranch(20, 10, false);
        }
        bjccovmshb1ic5.instrumentRegion(20, 7);
        return isDark ? (bjccovmshb1ic5.instrumentBranch(20, 12, true), '#173A5E') : (bjccovmshb1ic5.instrumentBranch(20, 12, false), '#EAF3FF');
    }
    static accentHoverBackground(isDark: boolean, accentName: SettingsAccentName): string {
        bjccovmshb1ic5.instrumentFunction(21);
        if (accentName === SettingsAccent.PURPLE) {
            bjccovmshb1ic5.instrumentBranch(21, 0, true);
            bjccovmshb1ic5.instrumentRegion(21, 1);
            return isDark ? (bjccovmshb1ic5.instrumentBranch(21, 1, true), '#402A73') : (bjccovmshb1ic5.instrumentBranch(21, 1, false), '#EDE2FF');
        }
        else {
            bjccovmshb1ic5.instrumentBranch(21, 0, false);
        }
        if (accentName === SettingsAccent.GREEN) {
            bjccovmshb1ic5.instrumentBranch(21, 2, true);
            bjccovmshb1ic5.instrumentRegion(21, 2);
            return isDark ? (bjccovmshb1ic5.instrumentBranch(21, 3, true), '#1C4B32') : (bjccovmshb1ic5.instrumentBranch(21, 3, false), '#DDF5E7');
        }
        else {
            bjccovmshb1ic5.instrumentBranch(21, 2, false);
        }
        if (accentName === SettingsAccent.AMBER) {
            bjccovmshb1ic5.instrumentBranch(21, 4, true);
            bjccovmshb1ic5.instrumentRegion(21, 3);
            return isDark ? (bjccovmshb1ic5.instrumentBranch(21, 5, true), '#5A421E') : (bjccovmshb1ic5.instrumentBranch(21, 5, false), '#FFEBC2');
        }
        else {
            bjccovmshb1ic5.instrumentBranch(21, 4, false);
        }
        if (accentName === SettingsAccent.CYAN) {
            bjccovmshb1ic5.instrumentBranch(21, 6, true);
            bjccovmshb1ic5.instrumentRegion(21, 4);
            return isDark ? (bjccovmshb1ic5.instrumentBranch(21, 7, true), '#1A4A53') : (bjccovmshb1ic5.instrumentBranch(21, 7, false), '#DCF7FA');
        }
        else {
            bjccovmshb1ic5.instrumentBranch(21, 6, false);
        }
        if (accentName === SettingsAccent.ORANGE) {
            bjccovmshb1ic5.instrumentBranch(21, 8, true);
            bjccovmshb1ic5.instrumentRegion(21, 5);
            return isDark ? (bjccovmshb1ic5.instrumentBranch(21, 9, true), '#5A341A') : (bjccovmshb1ic5.instrumentBranch(21, 9, false), '#FFE5D4');
        }
        else {
            bjccovmshb1ic5.instrumentBranch(21, 8, false);
        }
        if (accentName === SettingsAccent.INDIGO) {
            bjccovmshb1ic5.instrumentBranch(21, 10, true);
            bjccovmshb1ic5.instrumentRegion(21, 6);
            return isDark ? (bjccovmshb1ic5.instrumentBranch(21, 11, true), '#303574') : (bjccovmshb1ic5.instrumentBranch(21, 11, false), '#E4E9FF');
        }
        else {
            bjccovmshb1ic5.instrumentBranch(21, 10, false);
        }
        bjccovmshb1ic5.instrumentRegion(21, 7);
        return isDark ? (bjccovmshb1ic5.instrumentBranch(21, 12, true), '#1C456E') : (bjccovmshb1ic5.instrumentBranch(21, 12, false), '#DDEEFF');
    }
    static borderColor(isDark: boolean, active: boolean = false): string {
        bjccovmshb1ic5.instrumentFunction(22);
        if (active) {
            bjccovmshb1ic5.instrumentBranch(22, 0, true);
            bjccovmshb1ic5.instrumentRegion(22, 1);
            return '#0A84FF';
        }
        else {
            bjccovmshb1ic5.instrumentBranch(22, 0, false);
        }
        bjccovmshb1ic5.instrumentRegion(22, 2);
        return isDark ? (bjccovmshb1ic5.instrumentBranch(22, 1, true), '#2B3038') : (bjccovmshb1ic5.instrumentBranch(22, 1, false), '#E5E7EB');
    }
    static transparentButton(hovered: boolean, pressed: boolean, isDark: boolean): string {
        bjccovmshb1ic5.instrumentFunction(23);
        if (!hovered && !pressed) {
            bjccovmshb1ic5.instrumentBranch(23, 0, true);
            bjccovmshb1ic5.instrumentRegion(23, 1);
            return 'rgba(0,0,0,0)';
        }
        else {
            bjccovmshb1ic5.instrumentBranch(23, 0, false);
        }
        if (pressed) {
            bjccovmshb1ic5.instrumentBranch(23, 1, true);
            bjccovmshb1ic5.instrumentRegion(23, 2);
            return isDark ? (bjccovmshb1ic5.instrumentBranch(23, 2, true), 'rgba(255,255,255,0.14)') : (bjccovmshb1ic5.instrumentBranch(23, 2, false), 'rgba(0,0,0,0.08)');
        }
        else {
            bjccovmshb1ic5.instrumentBranch(23, 1, false);
        }
        bjccovmshb1ic5.instrumentRegion(23, 3);
        return isDark ? (bjccovmshb1ic5.instrumentBranch(23, 3, true), 'rgba(255,255,255,0.10)') : (bjccovmshb1ic5.instrumentBranch(23, 3, false), 'rgba(0,0,0,0.05)');
    }
    static subtleButton(isDark: boolean, hovered: boolean, pressed: boolean): string {
        bjccovmshb1ic5.instrumentFunction(24);
        if (pressed) {
            bjccovmshb1ic5.instrumentBranch(24, 0, true);
            bjccovmshb1ic5.instrumentRegion(24, 1);
            return isDark ? (bjccovmshb1ic5.instrumentBranch(24, 1, true), '#2A3038') : (bjccovmshb1ic5.instrumentBranch(24, 1, false), '#E5EAF0');
        }
        else {
            bjccovmshb1ic5.instrumentBranch(24, 0, false);
        }
        if (hovered) {
            bjccovmshb1ic5.instrumentBranch(24, 2, true);
            bjccovmshb1ic5.instrumentRegion(24, 2);
            return isDark ? (bjccovmshb1ic5.instrumentBranch(24, 3, true), '#242932') : (bjccovmshb1ic5.instrumentBranch(24, 3, false), '#EEF2F7');
        }
        else {
            bjccovmshb1ic5.instrumentBranch(24, 2, false);
        }
        bjccovmshb1ic5.instrumentRegion(24, 3);
        return isDark ? (bjccovmshb1ic5.instrumentBranch(24, 4, true), '#1D2128') : (bjccovmshb1ic5.instrumentBranch(24, 4, false), '#F2F4F7');
    }
    static disabledButton(isDark: boolean): string {
        bjccovmshb1ic5.instrumentFunction(25);
        bjccovmshb1ic5.instrumentRegion(25, 1);
        return isDark ? (bjccovmshb1ic5.instrumentBranch(25, 0, true), '#1A1D23') : (bjccovmshb1ic5.instrumentBranch(25, 0, false), '#F6F7F9');
    }
    static shadow(isDark: boolean, hovered: boolean): ShadowOptions {
        bjccovmshb1ic5.instrumentFunction(26);
        bjccovmshb1ic5.instrumentRegion(26, 1);
        return {
            radius: hovered ? (bjccovmshb1ic5.instrumentBranch(26, 0, true), 16) : (bjccovmshb1ic5.instrumentBranch(26, 0, false), 6),
            color: isDark ? (bjccovmshb1ic5.instrumentBranch(26, 1, true), (hovered ? (bjccovmshb1ic5.instrumentBranch(26, 2, true), '#44000000') : (bjccovmshb1ic5.instrumentBranch(26, 2, false), '#1A000000'))) : (bjccovmshb1ic5.instrumentBranch(26, 1, false), (hovered ? (bjccovmshb1ic5.instrumentBranch(26, 3, true), '#12111827') : (bjccovmshb1ic5.instrumentBranch(26, 3, false), '#06111827'))),
            offsetX: 0,
            offsetY: hovered ? (bjccovmshb1ic5.instrumentBranch(26, 4, true), 8) : (bjccovmshb1ic5.instrumentBranch(26, 4, false), 2)
        };
    }
    static statusText(isDark: boolean, tone: SettingsStatusTone): string {
        bjccovmshb1ic5.instrumentFunction(27);
        if (tone === 'ok') {
            bjccovmshb1ic5.instrumentBranch(27, 0, true);
            bjccovmshb1ic5.instrumentRegion(27, 1);
            return isDark ? (bjccovmshb1ic5.instrumentBranch(27, 1, true), '#86EFAC') : (bjccovmshb1ic5.instrumentBranch(27, 1, false), '#15803D');
        }
        else {
            bjccovmshb1ic5.instrumentBranch(27, 0, false);
        }
        if (tone === 'warning') {
            bjccovmshb1ic5.instrumentBranch(27, 2, true);
            bjccovmshb1ic5.instrumentRegion(27, 2);
            return isDark ? (bjccovmshb1ic5.instrumentBranch(27, 3, true), '#FCD34D') : (bjccovmshb1ic5.instrumentBranch(27, 3, false), '#B45309');
        }
        else {
            bjccovmshb1ic5.instrumentBranch(27, 2, false);
        }
        if (tone === 'danger') {
            bjccovmshb1ic5.instrumentBranch(27, 4, true);
            bjccovmshb1ic5.instrumentRegion(27, 3);
            return isDark ? (bjccovmshb1ic5.instrumentBranch(27, 5, true), '#FCA5A5') : (bjccovmshb1ic5.instrumentBranch(27, 5, false), '#B91C1C');
        }
        else {
            bjccovmshb1ic5.instrumentBranch(27, 4, false);
        }
        if (tone === 'info') {
            bjccovmshb1ic5.instrumentBranch(27, 6, true);
            bjccovmshb1ic5.instrumentRegion(27, 4);
            return isDark ? (bjccovmshb1ic5.instrumentBranch(27, 7, true), '#93C5FD') : (bjccovmshb1ic5.instrumentBranch(27, 7, false), '#0A84FF');
        }
        else {
            bjccovmshb1ic5.instrumentBranch(27, 6, false);
        }
        bjccovmshb1ic5.instrumentRegion(27, 5);
        return isDark ? (bjccovmshb1ic5.instrumentBranch(27, 8, true), '#D1D5DB') : (bjccovmshb1ic5.instrumentBranch(27, 8, false), '#475467');
    }
    static statusBackground(isDark: boolean, tone: SettingsStatusTone): string {
        bjccovmshb1ic5.instrumentFunction(28);
        if (tone === 'ok') {
            bjccovmshb1ic5.instrumentBranch(28, 0, true);
            bjccovmshb1ic5.instrumentRegion(28, 1);
            return isDark ? (bjccovmshb1ic5.instrumentBranch(28, 1, true), '#173F2A') : (bjccovmshb1ic5.instrumentBranch(28, 1, false), '#EAF8EF');
        }
        else {
            bjccovmshb1ic5.instrumentBranch(28, 0, false);
        }
        if (tone === 'warning') {
            bjccovmshb1ic5.instrumentBranch(28, 2, true);
            bjccovmshb1ic5.instrumentRegion(28, 2);
            return isDark ? (bjccovmshb1ic5.instrumentBranch(28, 3, true), '#4A3718') : (bjccovmshb1ic5.instrumentBranch(28, 3, false), '#FFF4D6');
        }
        else {
            bjccovmshb1ic5.instrumentBranch(28, 2, false);
        }
        if (tone === 'danger') {
            bjccovmshb1ic5.instrumentBranch(28, 4, true);
            bjccovmshb1ic5.instrumentRegion(28, 3);
            return isDark ? (bjccovmshb1ic5.instrumentBranch(28, 5, true), '#4A1F1F') : (bjccovmshb1ic5.instrumentBranch(28, 5, false), '#FEECEC');
        }
        else {
            bjccovmshb1ic5.instrumentBranch(28, 4, false);
        }
        if (tone === 'info') {
            bjccovmshb1ic5.instrumentBranch(28, 6, true);
            bjccovmshb1ic5.instrumentRegion(28, 4);
            return isDark ? (bjccovmshb1ic5.instrumentBranch(28, 7, true), '#173A5E') : (bjccovmshb1ic5.instrumentBranch(28, 7, false), '#EAF3FF');
        }
        else {
            bjccovmshb1ic5.instrumentBranch(28, 6, false);
        }
        bjccovmshb1ic5.instrumentRegion(28, 5);
        return isDark ? (bjccovmshb1ic5.instrumentBranch(28, 8, true), '#242932') : (bjccovmshb1ic5.instrumentBranch(28, 8, false), '#F2F4F7');
    }
    static statusBorder(isDark: boolean, tone: SettingsStatusTone): string {
        bjccovmshb1ic5.instrumentFunction(29);
        if (tone === 'ok') {
            bjccovmshb1ic5.instrumentBranch(29, 0, true);
            bjccovmshb1ic5.instrumentRegion(29, 1);
            return isDark ? (bjccovmshb1ic5.instrumentBranch(29, 1, true), '#24563A') : (bjccovmshb1ic5.instrumentBranch(29, 1, false), '#C8EBD5');
        }
        else {
            bjccovmshb1ic5.instrumentBranch(29, 0, false);
        }
        if (tone === 'warning') {
            bjccovmshb1ic5.instrumentBranch(29, 2, true);
            bjccovmshb1ic5.instrumentRegion(29, 2);
            return isDark ? (bjccovmshb1ic5.instrumentBranch(29, 3, true), '#6B531E') : (bjccovmshb1ic5.instrumentBranch(29, 3, false), '#F6D98B');
        }
        else {
            bjccovmshb1ic5.instrumentBranch(29, 2, false);
        }
        if (tone === 'danger') {
            bjccovmshb1ic5.instrumentBranch(29, 4, true);
            bjccovmshb1ic5.instrumentRegion(29, 3);
            return isDark ? (bjccovmshb1ic5.instrumentBranch(29, 5, true), '#733232') : (bjccovmshb1ic5.instrumentBranch(29, 5, false), '#F5B8B8');
        }
        else {
            bjccovmshb1ic5.instrumentBranch(29, 4, false);
        }
        if (tone === 'info') {
            bjccovmshb1ic5.instrumentBranch(29, 6, true);
            bjccovmshb1ic5.instrumentRegion(29, 4);
            return isDark ? (bjccovmshb1ic5.instrumentBranch(29, 7, true), '#24517C') : (bjccovmshb1ic5.instrumentBranch(29, 7, false), '#B8DBFF');
        }
        else {
            bjccovmshb1ic5.instrumentBranch(29, 6, false);
        }
        bjccovmshb1ic5.instrumentRegion(29, 5);
        return SettingsTheme.borderColor(isDark);
    }
}
