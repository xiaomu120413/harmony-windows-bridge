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
let bjccovmshb1i6i = new BjcCov({ version: "bjc v1.0.0", versionCode: 10000, path: "common/src/main/ets/components/home/HomeHeader.ets", hash: "9c5669d875b7a46bd943103b529b82a404cd62219f3759f1412d312b0f78163c", lineCnt: 169, count: 0, projectPath: "", functions: { 0: { name: "anonymous_1", count: 0, regions: { 0: { startLoc: { line: 15, col: 19 }, endLoc: { line: 15, col: 57 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 0 }, 1: { name: "anonymous_2", count: 0, regions: { 0: { startLoc: { line: 15, col: 60 }, endLoc: { line: 16, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 1 }, 2: { name: "setInitiallyProvidedValue", count: 0, regions: { 0: { startLoc: { line: 9, col: 34 }, endLoc: { line: 14, col: 53 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 2 }, 3: { name: "updateStateVars", count: 0, regions: { 0: { startLoc: { line: 9, col: 9 }, endLoc: { line: 14, col: 37 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 3 }, 4: { name: "layoutMode", count: 0, regions: { 0: { startLoc: { line: 9, col: 9 }, endLoc: { line: 9, col: 19 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 4 }, 5: { name: "layoutMode", count: 0, regions: { 0: { startLoc: { line: 9, col: 9 }, endLoc: { line: 9, col: 31 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 5 }, 6: { name: "isDark", count: 0, regions: { 0: { startLoc: { line: 10, col: 9 }, endLoc: { line: 10, col: 15 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 6 }, 7: { name: "isDark", count: 0, regions: { 0: { startLoc: { line: 10, col: 9 }, endLoc: { line: 10, col: 24 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 7 }, 8: { name: "xrdpServerBusy", count: 0, regions: { 0: { startLoc: { line: 11, col: 9 }, endLoc: { line: 11, col: 23 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 8 }, 9: { name: "xrdpServerBusy", count: 0, regions: { 0: { startLoc: { line: 11, col: 9 }, endLoc: { line: 11, col: 32 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 9 }, 10: { name: "xrdpServerState", count: 0, regions: { 0: { startLoc: { line: 12, col: 9 }, endLoc: { line: 12, col: 24 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 10 }, 11: { name: "xrdpServerState", count: 0, regions: { 0: { startLoc: { line: 12, col: 9 }, endLoc: { line: 12, col: 32 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 11 }, 12: { name: "xrdpServerRunning", count: 0, regions: { 0: { startLoc: { line: 13, col: 9 }, endLoc: { line: 13, col: 26 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 12 }, 13: { name: "xrdpServerRunning", count: 0, regions: { 0: { startLoc: { line: 13, col: 9 }, endLoc: { line: 13, col: 35 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 13 }, 14: { name: "remoteControlServerAvailable", count: 0, regions: { 0: { startLoc: { line: 14, col: 9 }, endLoc: { line: 14, col: 37 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 14 }, 15: { name: "remoteControlServerAvailable", count: 0, regions: { 0: { startLoc: { line: 14, col: 9 }, endLoc: { line: 14, col: 46 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 15 }, 16: { name: "anonymous_3", count: 0, regions: { 0: { startLoc: { line: 15, col: 19 }, endLoc: { line: 15, col: 57 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 16 }, 17: { name: "settingsHovered", count: 0, regions: { 0: { startLoc: { line: 17, col: 18 }, endLoc: { line: 17, col: 33 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 17 }, 18: { name: "settingsHovered", count: 0, regions: { 0: { startLoc: { line: 17, col: 18 }, endLoc: { line: 17, col: 42 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 18 }, 19: { name: "settingsPressed", count: 0, regions: { 0: { startLoc: { line: 18, col: 18 }, endLoc: { line: 18, col: 33 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 19 }, 20: { name: "settingsPressed", count: 0, regions: { 0: { startLoc: { line: 18, col: 18 }, endLoc: { line: 18, col: 42 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 20 }, 21: { name: "headerStatusTone", count: 0, regions: { 0: { startLoc: { line: 21, col: 3 }, endLoc: { line: 23, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 22, col: 5 }, endLoc: { line: 23, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 21 }, 22: { name: "headerStatusText", count: 0, regions: { 0: { startLoc: { line: 25, col: 3 }, endLoc: { line: 30, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 26, col: 30 }, endLoc: { line: 28, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 29, col: 5 }, endLoc: { line: 30, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 26, col: 9 }, endLoc: { line: 26, col: 28 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 22 }, 23: { name: "buildHeaderStatus", count: 0, regions: { 0: { startLoc: { line: 32, col: 3 }, endLoc: { line: 47, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 23 }, 24: { name: "anonymous_4", count: 0, regions: { 0: { startLoc: { line: 34, col: 5 }, endLoc: { line: 46, col: 37 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 24 }, 25: { name: "anonymous_5", count: 0, regions: { 0: { startLoc: { line: 35, col: 7 }, endLoc: { line: 39, col: 88 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 25 }, 26: { name: "anonymous_6", count: 0, regions: { 0: { startLoc: { line: 40, col: 7 }, endLoc: { line: 44, col: 20 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 26 }, 27: { name: "buildSettingsButton", count: 0, regions: { 0: { startLoc: { line: 49, col: 3 }, endLoc: { line: 85, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 75, col: 9 }, endLoc: { line: 76, col: 8 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 83, col: 7 }, endLoc: { line: 84, col: 6 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 27 }, 28: { name: "anonymous_7", count: 0, regions: { 0: { startLoc: { line: 51, col: 5 }, endLoc: { line: 84, col: 6 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 68, col: 17 }, endLoc: { line: 68, col: 48 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 68, col: 53 }, endLoc: { line: 68, col: 84 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 28 }, 29: { name: "anonymous_8", count: 0, regions: { 0: { startLoc: { line: 69, col: 14 }, endLoc: { line: 71, col: 6 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 70, col: 7 }, endLoc: { line: 71, col: 6 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 29 }, 30: { name: "anonymous_9", count: 0, regions: { 0: { startLoc: { line: 72, col: 14 }, endLoc: { line: 80, col: 6 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 73, col: 42 }, endLoc: { line: 76, col: 8 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 77, col: 75 }, endLoc: { line: 79, col: 8 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 73, col: 11 }, endLoc: { line: 73, col: 40 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 77, col: 11 }, endLoc: { line: 77, col: 73 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 30 }, 31: { name: "anonymous_10", count: 0, regions: { 0: { startLoc: { line: 81, col: 14 }, endLoc: { line: 84, col: 6 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 31 }, 32: { name: "anonymous_11", count: 0, regions: { 0: { startLoc: { line: 52, col: 7 }, endLoc: { line: 56, col: 25 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 32 }, 33: { name: "anonymous_12", count: 0, regions: { 0: { startLoc: { line: 57, col: 7 }, endLoc: { line: 61, col: 20 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 33 }, 34: { name: "buildExpandedHeader", count: 0, regions: { 0: { startLoc: { line: 87, col: 3 }, endLoc: { line: 125, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 34 }, 35: { name: "anonymous_13", count: 0, regions: { 0: { startLoc: { line: 89, col: 5 }, endLoc: { line: 124, col: 6 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 35 }, 36: { name: "anonymous_14", count: 0, regions: { 0: { startLoc: { line: 90, col: 7 }, endLoc: { line: 91, col: 36 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 36 }, 37: { name: "anonymous_15", count: 0, regions: { 0: { startLoc: { line: 93, col: 7 }, endLoc: { line: 100, col: 24 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 37 }, 38: { name: "anonymous_16", count: 0, regions: { 0: { startLoc: { line: 102, col: 7 }, endLoc: { line: 114, col: 39 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 38 }, 39: { name: "anonymous_17", count: 0, regions: { 0: { startLoc: { line: 103, col: 9 }, endLoc: { line: 109, col: 10 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 103, col: 48 }, endLoc: { line: 109, col: 10 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 103, col: 13 }, endLoc: { line: 103, col: 46 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 39 }, 40: { name: "anonymous_18", count: 0, regions: { 0: { startLoc: { line: 104, col: 11 }, endLoc: { line: 108, col: 54 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 40 }, 41: { name: "anonymous_19", count: 0, regions: { 0: { startLoc: { line: 105, col: 11 }, endLoc: { line: 108, col: 54 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 41 }, 42: { name: "buildCompactHeader", count: 0, regions: { 0: { startLoc: { line: 127, col: 3 }, endLoc: { line: 156, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 42 }, 43: { name: "anonymous_21", count: 0, regions: { 0: { startLoc: { line: 129, col: 5 }, endLoc: { line: 155, col: 6 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 43 }, 44: { name: "anonymous_22", count: 0, regions: { 0: { startLoc: { line: 130, col: 7 }, endLoc: { line: 142, col: 39 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 44 }, 45: { name: "anonymous_23", count: 0, regions: { 0: { startLoc: { line: 131, col: 9 }, endLoc: { line: 137, col: 60 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 45 }, 46: { name: "anonymous_24", count: 0, regions: { 0: { startLoc: { line: 144, col: 7 }, endLoc: { line: 146, col: 8 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 144, col: 46 }, endLoc: { line: 146, col: 8 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 144, col: 11 }, endLoc: { line: 144, col: 44 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 46 }, 47: { name: "anonymous_25", count: 0, regions: { 0: { startLoc: { line: 145, col: 9 }, endLoc: { line: 145, col: 33 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 47 }, 48: { name: "initialRender", count: 0, regions: { 0: { startLoc: { line: 158, col: 3 }, endLoc: { line: 167, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 48 }, 49: { name: "anonymous_27", count: 0, regions: { 0: { startLoc: { line: 159, col: 5 }, endLoc: { line: 166, col: 18 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 49 }, 50: { name: "anonymous_28", count: 0, regions: { 0: { startLoc: { line: 160, col: 7 }, endLoc: { line: 164, col: 8 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 160, col: 51 }, endLoc: { line: 162, col: 8 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 162, col: 14 }, endLoc: { line: 164, col: 8 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 160, col: 11 }, endLoc: { line: 160, col: 49 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 50 }, 51: { name: "anonymous_29", count: 0, regions: { 0: { startLoc: { line: 161, col: 9 }, endLoc: { line: 161, col: 34 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 51 }, 52: { name: "anonymous_30", count: 0, regions: { 0: { startLoc: { line: 163, col: 9 }, endLoc: { line: 163, col: 35 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 52 } }, exeLine: { 0: 1, 1: 2, 2: 3, 3: 4, 4: 5, 5: 8, 6: 9, 7: 10, 8: 11, 9: 12, 10: 13, 11: 14, 12: 15, 13: 17, 14: 18, 15: 19, 16: 21, 17: 22, 18: 25, 19: 26, 20: 27, 21: 29, 22: 33, 23: 34, 24: 35, 25: 36, 26: 37, 27: 38, 28: 39, 29: 40, 30: 41, 31: 42, 32: 43, 33: 44, 34: 46, 35: 50, 36: 51, 37: 52, 38: 53, 39: 54, 40: 55, 41: 56, 42: 57, 43: 58, 44: 59, 45: 60, 46: 61, 47: 63, 48: 64, 49: 65, 50: 66, 51: 67, 52: 68, 53: 69, 54: 70, 55: 72, 56: 73, 57: 74, 58: 75, 59: 77, 60: 78, 61: 81, 62: 82, 63: 83, 64: 88, 65: 89, 66: 90, 67: 91, 68: 93, 69: 94, 70: 95, 71: 96, 72: 97, 73: 98, 74: 99, 75: 100, 76: 102, 77: 103, 78: 104, 79: 105, 80: 106, 81: 107, 82: 108, 83: 110, 84: 112, 85: 113, 86: 114, 87: 116, 88: 117, 89: 118, 90: 119, 91: 120, 92: 121, 93: 122, 94: 123, 95: 128, 96: 129, 97: 130, 98: 131, 99: 132, 100: 133, 101: 134, 102: 135, 103: 136, 104: 137, 105: 139, 106: 141, 107: 142, 108: 144, 109: 145, 110: 148, 111: 149, 112: 150, 113: 151, 114: 152, 115: 153, 116: 154, 117: 158, 118: 159, 119: 160, 120: 161, 121: 162, 122: 163, 123: 166 } });
if (!("finalizeConstruction" in ViewPU.prototype)) {
    Reflect.set(ViewPU.prototype, "finalizeConstruction", () => { });
}
interface HomeHeader_Params {
    layoutMode?: LayoutMode;
    isDark?: boolean;
    xrdpServerBusy?: boolean;
    xrdpServerState?: string;
    xrdpServerRunning?: boolean;
    remoteControlServerAvailable?: boolean;
    onOpenSettings?: (remoteControlSection: string) => void;
    settingsHovered?: boolean;
    settingsPressed?: boolean;
    headerSideWidth?: number;
}
import { LayoutMode } from "@normalized:N&&&common/src/main/ets/adaptive/WindowLayoutPolicy&";
import { SettingsText } from "@normalized:N&&&common/src/main/ets/components/settings/SettingsConstants&";
import { SettingsAccent, SettingsResources, SettingsTheme } from "@normalized:N&&&common/src/main/ets/components/settings/SettingsUi&";
import type { SettingsStatusTone } from "@normalized:N&&&common/src/main/ets/components/settings/SettingsUi&";
import { HomeText } from "@normalized:N&&&common/src/main/ets/components/home/HomeText&";
import { HomeTheme } from "@normalized:N&&&common/src/main/ets/components/home/HomeTheme&";
export class HomeHeader extends ViewPU {
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
        this.__remoteControlServerAvailable = new SynchedPropertySimpleOneWayPU(params.remoteControlServerAvailable, this, "remoteControlServerAvailable");
        this.onOpenSettings = (_remoteControlSection: string) => {
            bjccovmshb1i6i.instrumentFunction(1);
        };
        this.__settingsHovered = new ObservedPropertySimplePU(false, this, "settingsHovered");
        this.__settingsPressed = new ObservedPropertySimplePU(false, this, "settingsPressed");
        this.headerSideWidth = 260;
        this.setInitiallyProvidedValue(params);
        this.finalizeConstruction();
    }
    setInitiallyProvidedValue(params: HomeHeader_Params) {
        bjccovmshb1i6i.instrumentFunction(2);
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
        if (params.settingsHovered !== undefined) {
            this.settingsHovered = params.settingsHovered;
        }
        else {
        }
        if (params.settingsPressed !== undefined) {
            this.settingsPressed = params.settingsPressed;
        }
        else {
        }
        if (params.headerSideWidth !== undefined) {
            this.headerSideWidth = params.headerSideWidth;
        }
        else {
        }
    }
    updateStateVars(params: HomeHeader_Params) {
        bjccovmshb1i6i.instrumentFunction(3);
        this.__layoutMode.reset(params.layoutMode);
        this.__isDark.reset(params.isDark);
        this.__xrdpServerBusy.reset(params.xrdpServerBusy);
        this.__xrdpServerState.reset(params.xrdpServerState);
        this.__xrdpServerRunning.reset(params.xrdpServerRunning);
        this.__remoteControlServerAvailable.reset(params.remoteControlServerAvailable);
    }
    purgeVariableDependenciesOnElmtId(rmElmtId) {
        this.__layoutMode.purgeDependencyOnElmtId(rmElmtId);
        this.__isDark.purgeDependencyOnElmtId(rmElmtId);
        this.__xrdpServerBusy.purgeDependencyOnElmtId(rmElmtId);
        this.__xrdpServerState.purgeDependencyOnElmtId(rmElmtId);
        this.__xrdpServerRunning.purgeDependencyOnElmtId(rmElmtId);
        this.__remoteControlServerAvailable.purgeDependencyOnElmtId(rmElmtId);
        this.__settingsHovered.purgeDependencyOnElmtId(rmElmtId);
        this.__settingsPressed.purgeDependencyOnElmtId(rmElmtId);
    }
    aboutToBeDeleted() {
        this.__layoutMode.aboutToBeDeleted();
        this.__isDark.aboutToBeDeleted();
        this.__xrdpServerBusy.aboutToBeDeleted();
        this.__xrdpServerState.aboutToBeDeleted();
        this.__xrdpServerRunning.aboutToBeDeleted();
        this.__remoteControlServerAvailable.aboutToBeDeleted();
        this.__settingsHovered.aboutToBeDeleted();
        this.__settingsPressed.aboutToBeDeleted();
        SubscriberManager.Get().delete(this.id__());
        this.aboutToBeDeletedInternal();
    }
    private __layoutMode: SynchedPropertySimpleOneWayPU<LayoutMode>;
    get layoutMode() {
        bjccovmshb1i6i.instrumentFunction(4);
        return this.__layoutMode.get();
    }
    set layoutMode(newValue: LayoutMode) {
        bjccovmshb1i6i.instrumentFunction(5);
        this.__layoutMode.set(newValue);
    }
    private __isDark: SynchedPropertySimpleOneWayPU<boolean>;
    get isDark() {
        bjccovmshb1i6i.instrumentFunction(6);
        return this.__isDark.get();
    }
    set isDark(newValue: boolean) {
        bjccovmshb1i6i.instrumentFunction(7);
        this.__isDark.set(newValue);
    }
    private __xrdpServerBusy: SynchedPropertySimpleOneWayPU<boolean>;
    get xrdpServerBusy() {
        bjccovmshb1i6i.instrumentFunction(8);
        return this.__xrdpServerBusy.get();
    }
    set xrdpServerBusy(newValue: boolean) {
        bjccovmshb1i6i.instrumentFunction(9);
        this.__xrdpServerBusy.set(newValue);
    }
    private __xrdpServerState: SynchedPropertySimpleOneWayPU<string>;
    get xrdpServerState() {
        bjccovmshb1i6i.instrumentFunction(10);
        return this.__xrdpServerState.get();
    }
    set xrdpServerState(newValue: string) {
        bjccovmshb1i6i.instrumentFunction(11);
        this.__xrdpServerState.set(newValue);
    }
    private __xrdpServerRunning: SynchedPropertySimpleOneWayPU<boolean>;
    get xrdpServerRunning() {
        bjccovmshb1i6i.instrumentFunction(12);
        return this.__xrdpServerRunning.get();
    }
    set xrdpServerRunning(newValue: boolean) {
        bjccovmshb1i6i.instrumentFunction(13);
        this.__xrdpServerRunning.set(newValue);
    }
    private __remoteControlServerAvailable: SynchedPropertySimpleOneWayPU<boolean>;
    get remoteControlServerAvailable() {
        bjccovmshb1i6i.instrumentFunction(14);
        return this.__remoteControlServerAvailable.get();
    }
    set remoteControlServerAvailable(newValue: boolean) {
        bjccovmshb1i6i.instrumentFunction(15);
        this.__remoteControlServerAvailable.set(newValue);
    }
    private onOpenSettings: (remoteControlSection: string) => void;
    private __settingsHovered: ObservedPropertySimplePU<boolean>;
    get settingsHovered() {
        bjccovmshb1i6i.instrumentFunction(17);
        return this.__settingsHovered.get();
    }
    set settingsHovered(newValue: boolean) {
        bjccovmshb1i6i.instrumentFunction(18);
        this.__settingsHovered.set(newValue);
    }
    private __settingsPressed: ObservedPropertySimplePU<boolean>;
    get settingsPressed() {
        bjccovmshb1i6i.instrumentFunction(19);
        return this.__settingsPressed.get();
    }
    set settingsPressed(newValue: boolean) {
        bjccovmshb1i6i.instrumentFunction(20);
        this.__settingsPressed.set(newValue);
    }
    private readonly headerSideWidth: number;
    private headerStatusTone(): SettingsStatusTone {
        bjccovmshb1i6i.instrumentFunction(21);
        bjccovmshb1i6i.instrumentRegion(21, 1);
        return SettingsText.remoteServerStatusTone(this.xrdpServerRunning, this.xrdpServerBusy, this.xrdpServerState);
    }
    private headerStatusText(): string {
        bjccovmshb1i6i.instrumentFunction(22);
        if (this.xrdpServerBusy) {
            bjccovmshb1i6i.instrumentBranch(22, 0, true);
            bjccovmshb1i6i.instrumentRegion(22, 1);
            return SettingsText.REMOTE_SERVER_BUSY;
        }
        else {
            bjccovmshb1i6i.instrumentBranch(22, 0, false);
        }
        bjccovmshb1i6i.instrumentRegion(22, 2);
        return SettingsText.remoteServerStateLabel(this.xrdpServerRunning, this.xrdpServerState);
    }
    private buildHeaderStatus(parent = null) {
        bjccovmshb1i6i.instrumentFunction(23);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i6i.instrumentFunction(24);
            Row.create({ space: 10 });
            Row.alignItems(VerticalAlign.Center);
        }, Row);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i6i.instrumentFunction(25);
            Column.create();
            Column.width(9);
            Column.height(9);
            Column.borderRadius(4.5);
            Column.backgroundColor(SettingsTheme.statusText(this.isDark, this.headerStatusTone()));
        }, Column);
        Column.pop();
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i6i.instrumentFunction(26);
            Text.create(this.headerStatusText());
            Text.fontSize(15);
            Text.fontWeight(FontWeight.Medium);
            Text.fontColor(SettingsTheme.primaryText(this.isDark));
            Text.maxLines(1);
        }, Text);
        Text.pop();
        Row.pop();
    }
    private buildSettingsButton(parent = null) {
        bjccovmshb1i6i.instrumentFunction(27);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i6i.instrumentFunction(28);
            Row.create({ space: 8 });
            Row.height(48);
            Row.padding({ left: 12, right: 12 });
            Row.alignItems(VerticalAlign.Center);
            Row.borderRadius(8);
            Row.backgroundColor(HomeTheme.neutralButtonBackground(this.isDark, this.settingsHovered, this.settingsPressed));
            Row.scale({ x: this.settingsPressed ? (bjccovmshb1i6i.instrumentBranch(28, 0, true), 0.98) : (bjccovmshb1i6i.instrumentBranch(28, 0, false), 1), y: this.settingsPressed ? (bjccovmshb1i6i.instrumentBranch(28, 1, true), 0.98) : (bjccovmshb1i6i.instrumentBranch(28, 1, false), 1) });
            Row.onHover((hovered: boolean) => {
                bjccovmshb1i6i.instrumentFunction(29);
                bjccovmshb1i6i.instrumentRegion(29, 1);
                this.settingsHovered = hovered;
            });
            Row.onTouch((event: TouchEvent) => {
                bjccovmshb1i6i.instrumentFunction(30);
                if (event.type === TouchType.Down) {
                    bjccovmshb1i6i.instrumentBranch(30, 0, true);
                    bjccovmshb1i6i.instrumentRegion(30, 1);
                    this.settingsPressed = true;
                    bjccovmshb1i6i.instrumentRegion(27, 1);
                    return;
                }
                else {
                    bjccovmshb1i6i.instrumentBranch(30, 0, false);
                }
                if (event.type === TouchType.Up || event.type === TouchType.Cancel) {
                    bjccovmshb1i6i.instrumentBranch(30, 1, true);
                    bjccovmshb1i6i.instrumentRegion(30, 2);
                    this.settingsPressed = false;
                }
                else {
                    bjccovmshb1i6i.instrumentBranch(30, 1, false);
                }
            });
            Row.onClick(() => {
                bjccovmshb1i6i.instrumentFunction(31);
                this.settingsPressed = false;
                bjccovmshb1i6i.instrumentRegion(27, 2);
                this.onOpenSettings('');
            });
        }, Row);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i6i.instrumentFunction(32);
            Image.create(SettingsResources.SETTINGS_ICON);
            Image.width(19);
            Image.height(19);
            Image.fillColor(SettingsTheme.accentColor(this.isDark, SettingsAccent.BLUE));
            Image.draggable(false);
        }, Image);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i6i.instrumentFunction(33);
            Text.create(SettingsText.SETTINGS_TITLE);
            Text.fontSize(14);
            Text.fontWeight(FontWeight.Medium);
            Text.fontColor(SettingsTheme.primaryText(this.isDark));
            Text.maxLines(1);
        }, Text);
        Text.pop();
        Row.pop();
    }
    private buildExpandedHeader(parent = null) {
        bjccovmshb1i6i.instrumentFunction(34);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i6i.instrumentFunction(35);
            Row.create();
            Row.width('100%');
            Row.height(84);
            Row.padding({ left: 32, right: 30 });
            Row.alignItems(VerticalAlign.Center);
            Row.backgroundColor(HomeTheme.panelBackground(this.isDark));
            Row.border({
                width: { bottom: 1 },
                color: HomeTheme.borderColor(this.isDark)
            });
        }, Row);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i6i.instrumentFunction(36);
            Row.create();
            Row.width(this.headerSideWidth);
        }, Row);
        Row.pop();
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i6i.instrumentFunction(37);
            Text.create(HomeText.APP_TAGLINE);
            Text.fontSize(20);
            Text.fontWeight(FontWeight.Bold);
            Text.fontColor(SettingsTheme.primaryText(this.isDark));
            Text.maxLines(1);
            Text.textOverflow({ overflow: TextOverflow.Ellipsis });
            Text.textAlign(TextAlign.Center);
            Text.layoutWeight(1);
        }, Text);
        Text.pop();
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i6i.instrumentFunction(38);
            Row.create({ space: 18 });
            Row.width(this.headerSideWidth);
            Row.justifyContent(FlexAlign.End);
            Row.alignItems(VerticalAlign.Center);
        }, Row);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i6i.instrumentFunction(39);
            If.create();
            if (this.remoteControlServerAvailable) {
                bjccovmshb1i6i.instrumentBranch(39, 0, true);
                bjccovmshb1i6i.instrumentRegion(39, 1);
                this.ifElseBranchUpdateFunction(0, () => {
                    bjccovmshb1i6i.instrumentFunction(40);
                    this.buildHeaderStatus.bind(this)();
                    this.observeComponentCreation2((elmtId, isInitialRender) => {
                        bjccovmshb1i6i.instrumentFunction(41);
                        Divider.create();
                        Divider.vertical(true);
                        Divider.height(26);
                        Divider.color(HomeTheme.borderColor(this.isDark));
                    }, Divider);
                });
            }
            else {
                bjccovmshb1i6i.instrumentBranch(39, 0, false);
                this.ifElseBranchUpdateFunction(1, () => {
                });
            }
        }, If);
        If.pop();
        this.buildSettingsButton.bind(this)();
        Row.pop();
        Row.pop();
    }
    private buildCompactHeader(parent = null) {
        bjccovmshb1i6i.instrumentFunction(42);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i6i.instrumentFunction(43);
            Column.create({ space: 8 });
            Column.width('100%');
            Column.padding({ left: 16, right: 16, top: 10, bottom: 10 });
            Column.alignItems(HorizontalAlign.Start);
            Column.backgroundColor(HomeTheme.panelBackground(this.isDark));
            Column.border({
                width: { bottom: 1 },
                color: HomeTheme.borderColor(this.isDark)
            });
        }, Column);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i6i.instrumentFunction(44);
            Row.create({ space: 12 });
            Row.width('100%');
            Row.alignItems(VerticalAlign.Center);
        }, Row);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i6i.instrumentFunction(45);
            Text.create(HomeText.APP_TAGLINE);
            Text.fontSize(20);
            Text.fontWeight(FontWeight.Bold);
            Text.fontColor(SettingsTheme.primaryText(this.isDark));
            Text.layoutWeight(1);
            Text.maxLines(2);
            Text.textOverflow({ overflow: TextOverflow.Ellipsis });
        }, Text);
        Text.pop();
        this.buildSettingsButton.bind(this)();
        Row.pop();
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i6i.instrumentFunction(46);
            If.create();
            if (this.remoteControlServerAvailable) {
                bjccovmshb1i6i.instrumentBranch(46, 0, true);
                bjccovmshb1i6i.instrumentRegion(46, 1);
                this.ifElseBranchUpdateFunction(0, () => {
                    bjccovmshb1i6i.instrumentFunction(47);
                    this.buildHeaderStatus.bind(this)();
                });
            }
            else {
                bjccovmshb1i6i.instrumentBranch(46, 0, false);
                this.ifElseBranchUpdateFunction(1, () => {
                });
            }
        }, If);
        If.pop();
        Column.pop();
    }
    initialRender() {
        bjccovmshb1i6i.instrumentFunction(48);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i6i.instrumentFunction(49);
            Stack.create();
            Stack.width('100%');
        }, Stack);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i6i.instrumentFunction(50);
            If.create();
            if (this.layoutMode === LayoutMode.COMPACT) {
                bjccovmshb1i6i.instrumentBranch(50, 0, true);
                bjccovmshb1i6i.instrumentRegion(50, 1);
                this.ifElseBranchUpdateFunction(0, () => {
                    bjccovmshb1i6i.instrumentFunction(51);
                    this.buildCompactHeader.bind(this)();
                });
            }
            else {
                bjccovmshb1i6i.instrumentBranch(50, 0, false);
                bjccovmshb1i6i.instrumentRegion(50, 2);
                this.ifElseBranchUpdateFunction(1, () => {
                    bjccovmshb1i6i.instrumentFunction(52);
                    this.buildExpandedHeader.bind(this)();
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
