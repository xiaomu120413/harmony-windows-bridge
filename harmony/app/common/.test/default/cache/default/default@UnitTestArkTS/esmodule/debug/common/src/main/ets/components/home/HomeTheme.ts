import { SettingsAccent, SettingsTheme } from "@normalized:N&&&common/src/main/ets/components/settings/SettingsUi&";
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
let bjccovmshb1i8s = new BjcCov({ version: "bjc v1.0.0", versionCode: 10000, path: "common/src/main/ets/components/home/HomeTheme.ets", hash: "d0fdab3c49eaa1c24b7ba22b41ab83170c0f27720747301c26a57f5ca0b312cb", lineCnt: 72, count: 0, projectPath: "", functions: { 0: { name: "HomeTheme.panelBackground", count: 0, regions: { 0: { startLoc: { line: 4, col: 3 }, endLoc: { line: 6, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 5, col: 5 }, endLoc: { line: 6, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 0 }, 1: { name: "HomeTheme.appBackground", count: 0, regions: { 0: { startLoc: { line: 8, col: 3 }, endLoc: { line: 10, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 9, col: 5 }, endLoc: { line: 10, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 9, col: 12 }, endLoc: { line: 9, col: 42 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 1 }, 2: { name: "HomeTheme.fieldBackground", count: 0, regions: { 0: { startLoc: { line: 12, col: 3 }, endLoc: { line: 14, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 13, col: 5 }, endLoc: { line: 14, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 13, col: 12 }, endLoc: { line: 13, col: 42 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 2 }, 3: { name: "HomeTheme.searchBackground", count: 0, regions: { 0: { startLoc: { line: 16, col: 3 }, endLoc: { line: 18, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 17, col: 5 }, endLoc: { line: 18, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 17, col: 12 }, endLoc: { line: 17, col: 42 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 3 }, 4: { name: "HomeTheme.borderColor", count: 0, regions: { 0: { startLoc: { line: 20, col: 3 }, endLoc: { line: 22, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 21, col: 5 }, endLoc: { line: 22, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 21, col: 12 }, endLoc: { line: 21, col: 111 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 4 }, 5: { name: "HomeTheme.mutedBorderColor", count: 0, regions: { 0: { startLoc: { line: 24, col: 3 }, endLoc: { line: 26, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 25, col: 5 }, endLoc: { line: 26, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 25, col: 12 }, endLoc: { line: 25, col: 42 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 5 }, 6: { name: "HomeTheme.searchBorderColor", count: 0, regions: { 0: { startLoc: { line: 28, col: 3 }, endLoc: { line: 30, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 29, col: 5 }, endLoc: { line: 30, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 29, col: 12 }, endLoc: { line: 29, col: 42 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 6 }, 7: { name: "HomeTheme.primaryButtonBackground", count: 0, regions: { 0: { startLoc: { line: 32, col: 3 }, endLoc: { line: 40, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 33, col: 18 }, endLoc: { line: 35, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 36, col: 18 }, endLoc: { line: 38, col: 6 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 39, col: 5 }, endLoc: { line: 40, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 33, col: 9 }, endLoc: { line: 33, col: 16 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 34, col: 14 }, endLoc: { line: 34, col: 44 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 2: { startLoc: { line: 36, col: 9 }, endLoc: { line: 36, col: 16 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 3: { startLoc: { line: 37, col: 14 }, endLoc: { line: 37, col: 44 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 7 }, 8: { name: "HomeTheme.neutralButtonBackground", count: 0, regions: { 0: { startLoc: { line: 42, col: 3 }, endLoc: { line: 50, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 43, col: 18 }, endLoc: { line: 45, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 46, col: 18 }, endLoc: { line: 48, col: 6 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 49, col: 5 }, endLoc: { line: 50, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 43, col: 9 }, endLoc: { line: 43, col: 16 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 44, col: 14 }, endLoc: { line: 44, col: 44 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 2: { startLoc: { line: 46, col: 9 }, endLoc: { line: 46, col: 16 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 3: { startLoc: { line: 47, col: 14 }, endLoc: { line: 47, col: 44 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 4: { startLoc: { line: 49, col: 12 }, endLoc: { line: 49, col: 42 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 8 }, 9: { name: "HomeTheme.activePanelBackground", count: 0, regions: { 0: { startLoc: { line: 52, col: 3 }, endLoc: { line: 60, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 53, col: 18 }, endLoc: { line: 55, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 56, col: 18 }, endLoc: { line: 58, col: 6 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 59, col: 5 }, endLoc: { line: 60, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 53, col: 9 }, endLoc: { line: 53, col: 16 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 54, col: 14 }, endLoc: { line: 54, col: 44 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 2: { startLoc: { line: 56, col: 9 }, endLoc: { line: 56, col: 16 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 3: { startLoc: { line: 57, col: 14 }, endLoc: { line: 57, col: 44 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 4: { startLoc: { line: 59, col: 12 }, endLoc: { line: 59, col: 42 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 9 }, 10: { name: "HomeTheme.inactivePanelBackground", count: 0, regions: { 0: { startLoc: { line: 62, col: 3 }, endLoc: { line: 70, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 63, col: 18 }, endLoc: { line: 65, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 66, col: 18 }, endLoc: { line: 68, col: 6 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 69, col: 5 }, endLoc: { line: 70, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 63, col: 9 }, endLoc: { line: 63, col: 16 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 64, col: 14 }, endLoc: { line: 64, col: 44 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 2: { startLoc: { line: 66, col: 9 }, endLoc: { line: 66, col: 16 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 3: { startLoc: { line: 67, col: 14 }, endLoc: { line: 67, col: 44 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 10 } }, exeLine: { 0: 1, 1: 3, 2: 4, 3: 5, 4: 8, 5: 9, 6: 12, 7: 13, 8: 16, 9: 17, 10: 20, 11: 21, 12: 24, 13: 25, 14: 28, 15: 29, 16: 32, 17: 33, 18: 34, 19: 36, 20: 37, 21: 39, 22: 42, 23: 43, 24: 44, 25: 46, 26: 47, 27: 49, 28: 52, 29: 53, 30: 54, 31: 56, 32: 57, 33: 59, 34: 62, 35: 63, 36: 64, 37: 66, 38: 67, 39: 69 } });
export class HomeTheme {
    static panelBackground(isDark: boolean): string {
        bjccovmshb1i8s.instrumentFunction(0);
        bjccovmshb1i8s.instrumentRegion(0, 1);
        return SettingsTheme.cardBackground(isDark);
    }
    static appBackground(isDark: boolean): string {
        bjccovmshb1i8s.instrumentFunction(1);
        bjccovmshb1i8s.instrumentRegion(1, 1);
        return isDark ? (bjccovmshb1i8s.instrumentBranch(1, 0, true), '#101114') : (bjccovmshb1i8s.instrumentBranch(1, 0, false), '#F5F7FA');
    }
    static fieldBackground(isDark: boolean): string {
        bjccovmshb1i8s.instrumentFunction(2);
        bjccovmshb1i8s.instrumentRegion(2, 1);
        return isDark ? (bjccovmshb1i8s.instrumentBranch(2, 0, true), '#15171C') : (bjccovmshb1i8s.instrumentBranch(2, 0, false), '#FFFFFF');
    }
    static searchBackground(isDark: boolean): string {
        bjccovmshb1i8s.instrumentFunction(3);
        bjccovmshb1i8s.instrumentRegion(3, 1);
        return isDark ? (bjccovmshb1i8s.instrumentBranch(3, 0, true), '#10243A') : (bjccovmshb1i8s.instrumentBranch(3, 0, false), '#F2F8FF');
    }
    static borderColor(isDark: boolean, active: boolean = false): string {
        bjccovmshb1i8s.instrumentFunction(4);
        bjccovmshb1i8s.instrumentRegion(4, 1);
        return active ? (bjccovmshb1i8s.instrumentBranch(4, 0, true), SettingsTheme.accentColor(isDark, SettingsAccent.BLUE)) : (bjccovmshb1i8s.instrumentBranch(4, 0, false), SettingsTheme.borderColor(isDark));
    }
    static mutedBorderColor(isDark: boolean): string {
        bjccovmshb1i8s.instrumentFunction(5);
        bjccovmshb1i8s.instrumentRegion(5, 1);
        return isDark ? (bjccovmshb1i8s.instrumentBranch(5, 0, true), '#303641') : (bjccovmshb1i8s.instrumentBranch(5, 0, false), '#D9DEE7');
    }
    static searchBorderColor(isDark: boolean): string {
        bjccovmshb1i8s.instrumentFunction(6);
        bjccovmshb1i8s.instrumentRegion(6, 1);
        return isDark ? (bjccovmshb1i8s.instrumentBranch(6, 0, true), '#28567F') : (bjccovmshb1i8s.instrumentBranch(6, 0, false), '#C9E4FF');
    }
    static primaryButtonBackground(isDark: boolean, hovered: boolean, pressed: boolean): string {
        bjccovmshb1i8s.instrumentFunction(7);
        if (pressed) {
            bjccovmshb1i8s.instrumentBranch(7, 0, true);
            bjccovmshb1i8s.instrumentRegion(7, 1);
            return isDark ? (bjccovmshb1i8s.instrumentBranch(7, 1, true), '#4E9FFF') : (bjccovmshb1i8s.instrumentBranch(7, 1, false), '#086FD8');
        }
        else {
            bjccovmshb1i8s.instrumentBranch(7, 0, false);
        }
        if (hovered) {
            bjccovmshb1i8s.instrumentBranch(7, 2, true);
            bjccovmshb1i8s.instrumentRegion(7, 2);
            return isDark ? (bjccovmshb1i8s.instrumentBranch(7, 3, true), '#65AEFF') : (bjccovmshb1i8s.instrumentBranch(7, 3, false), '#087AF0');
        }
        else {
            bjccovmshb1i8s.instrumentBranch(7, 2, false);
        }
        bjccovmshb1i8s.instrumentRegion(7, 3);
        return SettingsTheme.accentColor(isDark, SettingsAccent.BLUE);
    }
    static neutralButtonBackground(isDark: boolean, hovered: boolean, pressed: boolean): string {
        bjccovmshb1i8s.instrumentFunction(8);
        if (pressed) {
            bjccovmshb1i8s.instrumentBranch(8, 0, true);
            bjccovmshb1i8s.instrumentRegion(8, 1);
            return isDark ? (bjccovmshb1i8s.instrumentBranch(8, 1, true), '#2B3038') : (bjccovmshb1i8s.instrumentBranch(8, 1, false), '#E6EBF2');
        }
        else {
            bjccovmshb1i8s.instrumentBranch(8, 0, false);
        }
        if (hovered) {
            bjccovmshb1i8s.instrumentBranch(8, 2, true);
            bjccovmshb1i8s.instrumentRegion(8, 2);
            return isDark ? (bjccovmshb1i8s.instrumentBranch(8, 3, true), '#242A33') : (bjccovmshb1i8s.instrumentBranch(8, 3, false), '#EEF2F7');
        }
        else {
            bjccovmshb1i8s.instrumentBranch(8, 2, false);
        }
        bjccovmshb1i8s.instrumentRegion(8, 3);
        return isDark ? (bjccovmshb1i8s.instrumentBranch(8, 4, true), '#1D2128') : (bjccovmshb1i8s.instrumentBranch(8, 4, false), '#F2F4F7');
    }
    static activePanelBackground(isDark: boolean, hovered: boolean, pressed: boolean): string {
        bjccovmshb1i8s.instrumentFunction(9);
        if (pressed) {
            bjccovmshb1i8s.instrumentBranch(9, 0, true);
            bjccovmshb1i8s.instrumentRegion(9, 1);
            return isDark ? (bjccovmshb1i8s.instrumentBranch(9, 1, true), '#162B46') : (bjccovmshb1i8s.instrumentBranch(9, 1, false), '#EDF6FF');
        }
        else {
            bjccovmshb1i8s.instrumentBranch(9, 0, false);
        }
        if (hovered) {
            bjccovmshb1i8s.instrumentBranch(9, 2, true);
            bjccovmshb1i8s.instrumentRegion(9, 2);
            return isDark ? (bjccovmshb1i8s.instrumentBranch(9, 3, true), '#1E3B60') : (bjccovmshb1i8s.instrumentBranch(9, 3, false), '#EAF4FF');
        }
        else {
            bjccovmshb1i8s.instrumentBranch(9, 2, false);
        }
        bjccovmshb1i8s.instrumentRegion(9, 3);
        return isDark ? (bjccovmshb1i8s.instrumentBranch(9, 4, true), '#14263D') : (bjccovmshb1i8s.instrumentBranch(9, 4, false), '#F7FBFF');
    }
    static inactivePanelBackground(isDark: boolean, hovered: boolean, pressed: boolean): string {
        bjccovmshb1i8s.instrumentFunction(10);
        if (pressed) {
            bjccovmshb1i8s.instrumentBranch(10, 0, true);
            bjccovmshb1i8s.instrumentRegion(10, 1);
            return isDark ? (bjccovmshb1i8s.instrumentBranch(10, 1, true), '#20242B') : (bjccovmshb1i8s.instrumentBranch(10, 1, false), '#EEF2F7');
        }
        else {
            bjccovmshb1i8s.instrumentBranch(10, 0, false);
        }
        if (hovered) {
            bjccovmshb1i8s.instrumentBranch(10, 2, true);
            bjccovmshb1i8s.instrumentRegion(10, 2);
            return isDark ? (bjccovmshb1i8s.instrumentBranch(10, 3, true), '#223043') : (bjccovmshb1i8s.instrumentBranch(10, 3, false), '#F2F8FF');
        }
        else {
            bjccovmshb1i8s.instrumentBranch(10, 2, false);
        }
        bjccovmshb1i8s.instrumentRegion(10, 3);
        return HomeTheme.fieldBackground(isDark);
    }
}
