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
let bjccovmshb1i8y = new BjcCov({ version: "bjc v1.0.0", versionCode: 10000, path: "common/src/main/ets/components/settings/BasicSettingsPage.ets", hash: "304fa9da05f3f5864a9954a81fc9401788d4a745e7c0a686862350ed8c2ab02c", lineCnt: 234, count: 0, projectPath: "", functions: { 0: { name: "anonymous_1", count: 0, regions: { 0: { startLoc: { line: 16, col: 11 }, endLoc: { line: 16, col: 21 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 0 }, 1: { name: "anonymous_2", count: 0, regions: { 0: { startLoc: { line: 18, col: 17 }, endLoc: { line: 18, col: 55 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 1 }, 2: { name: "anonymous_3", count: 0, regions: { 0: { startLoc: { line: 16, col: 24 }, endLoc: { line: 17, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 2 }, 3: { name: "anonymous_4", count: 0, regions: { 0: { startLoc: { line: 18, col: 58 }, endLoc: { line: 19, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 3 }, 4: { name: "anonymous_5", count: 0, regions: { 0: { startLoc: { line: 16, col: 11 }, endLoc: { line: 16, col: 21 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 4 }, 5: { name: "anonymous_6", count: 0, regions: { 0: { startLoc: { line: 18, col: 17 }, endLoc: { line: 18, col: 55 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 5 }, 6: { name: "localMode", count: 0, regions: { 0: { startLoc: { line: 20, col: 18 }, endLoc: { line: 20, col: 27 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 6 }, 7: { name: "localMode", count: 0, regions: { 0: { startLoc: { line: 20, col: 18 }, endLoc: { line: 20, col: 51 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 7 }, 8: { name: "systemDark", count: 0, regions: { 0: { startLoc: { line: 21, col: 46 }, endLoc: { line: 21, col: 56 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 8 }, 9: { name: "systemDark", count: 0, regions: { 0: { startLoc: { line: 21, col: 46 }, endLoc: { line: 21, col: 65 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 9 }, 10: { name: "interfaceName", count: 0, regions: { 0: { startLoc: { line: 22, col: 18 }, endLoc: { line: 22, col: 31 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 10 }, 11: { name: "interfaceName", count: 0, regions: { 0: { startLoc: { line: 22, col: 18 }, endLoc: { line: 22, col: 39 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 11 }, 12: { name: "ipAddresses", count: 0, regions: { 0: { startLoc: { line: 23, col: 18 }, endLoc: { line: 23, col: 29 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 12 }, 13: { name: "ipAddresses", count: 0, regions: { 0: { startLoc: { line: 23, col: 18 }, endLoc: { line: 23, col: 39 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 13 }, 14: { name: "networkCardHovered", count: 0, regions: { 0: { startLoc: { line: 24, col: 18 }, endLoc: { line: 24, col: 36 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 14 }, 15: { name: "networkCardHovered", count: 0, regions: { 0: { startLoc: { line: 24, col: 18 }, endLoc: { line: 24, col: 45 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 15 }, 16: { name: "networkCardPressed", count: 0, regions: { 0: { startLoc: { line: 25, col: 18 }, endLoc: { line: 25, col: 36 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 16 }, 17: { name: "networkCardPressed", count: 0, regions: { 0: { startLoc: { line: 25, col: 18 }, endLoc: { line: 25, col: 45 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 17 }, 18: { name: "aboutToAppear", count: 0, regions: { 0: { startLoc: { line: 27, col: 3 }, endLoc: { line: 30, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 28, col: 5 }, endLoc: { line: 30, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 18 }, 19: { name: "resolveVisualDark", count: 0, regions: { 0: { startLoc: { line: 32, col: 3 }, endLoc: { line: 40, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 33, col: 26 }, endLoc: { line: 35, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 36, col: 27 }, endLoc: { line: 38, col: 6 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 39, col: 5 }, endLoc: { line: 40, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 33, col: 9 }, endLoc: { line: 33, col: 24 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 36, col: 9 }, endLoc: { line: 36, col: 25 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 19 }, 20: { name: "isVisualDark", count: 0, regions: { 0: { startLoc: { line: 42, col: 3 }, endLoc: { line: 44, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 43, col: 5 }, endLoc: { line: 44, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 20 }, 21: { name: "applyColorMode", count: 0, regions: { 0: { startLoc: { line: 46, col: 3 }, endLoc: { line: 54, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 49, col: 71 }, endLoc: { line: 52, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 53, col: 5 }, endLoc: { line: 54, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 49, col: 9 }, endLoc: { line: 49, col: 69 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 21 }, 22: { name: "setNetworkCardHovered", count: 0, regions: { 0: { startLoc: { line: 56, col: 3 }, endLoc: { line: 60, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 57, col: 5 }, endLoc: { line: 60, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 22 }, 23: { name: "anonymous_7", count: 0, regions: { 0: { startLoc: { line: 57, col: 48 }, endLoc: { line: 59, col: 6 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 58, col: 7 }, endLoc: { line: 59, col: 6 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 23 }, 24: { name: "refreshIp", count: 0, regions: { 0: { startLoc: { line: 62, col: 3 }, endLoc: { line: 90, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 66, col: 9 }, endLoc: { line: 87, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 87, col: 7 }, endLoc: { line: 89, col: 6 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 68, col: 48 }, endLoc: { line: 71, col: 8 }, count: 0, ignored: 0 }, 4: { startLoc: { line: 77, col: 7 }, endLoc: { line: 84, col: 8 }, count: 0, ignored: 0 }, 5: { startLoc: { line: 80, col: 77 }, endLoc: { line: 82, col: 10 }, count: 0, ignored: 0 }, 6: { startLoc: { line: 86, col: 7 }, endLoc: { line: 87, col: 6 }, count: 0, ignored: 0 }, 7: { startLoc: { line: 83, col: 9 }, endLoc: { line: 84, col: 8 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 68, col: 11 }, endLoc: { line: 68, col: 46 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 80, col: 13 }, endLoc: { line: 80, col: 75 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 2: { startLoc: { line: 86, col: 26 }, endLoc: { line: 86, col: 89 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 24 }, 25: { name: "modeOption", count: 0, regions: { 0: { startLoc: { line: 92, col: 3 }, endLoc: { line: 106, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 25 }, 26: { name: "anonymous_8", count: 0, regions: { 0: { startLoc: { line: 95, col: 5 }, endLoc: { line: 101, col: 34 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 26 }, 27: { name: "anonymous_9", count: 0, regions: { 0: { startLoc: { line: 102, col: 16 }, endLoc: { line: 104, col: 8 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 103, col: 9 }, endLoc: { line: 104, col: 8 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 27 }, 28: { name: "networkInfoCard", count: 0, regions: { 0: { startLoc: { line: 108, col: 3 }, endLoc: { line: 189, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 183, col: 9 }, endLoc: { line: 184, col: 8 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 28 }, 29: { name: "anonymous_11", count: 0, regions: { 0: { startLoc: { line: 110, col: 5 }, endLoc: { line: 188, col: 6 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 163, col: 22 }, endLoc: { line: 164, col: 113 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 171, col: 21 }, endLoc: { line: 171, col: 53 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 2: { startLoc: { line: 173, col: 10 }, endLoc: { line: 173, col: 78 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 3: { startLoc: { line: 173, col: 43 }, endLoc: { line: 173, col: 78 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 4: { startLoc: { line: 174, col: 10 }, endLoc: { line: 174, col: 78 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 5: { startLoc: { line: 174, col: 43 }, endLoc: { line: 174, col: 78 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 29 }, 30: { name: "anonymous_12", count: 0, regions: { 0: { startLoc: { line: 177, col: 14 }, endLoc: { line: 179, col: 6 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 178, col: 7 }, endLoc: { line: 179, col: 6 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 30 }, 31: { name: "anonymous_13", count: 0, regions: { 0: { startLoc: { line: 180, col: 14 }, endLoc: { line: 188, col: 6 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 181, col: 42 }, endLoc: { line: 184, col: 8 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 185, col: 75 }, endLoc: { line: 187, col: 8 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 181, col: 11 }, endLoc: { line: 181, col: 40 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 185, col: 11 }, endLoc: { line: 185, col: 73 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 31 }, 32: { name: "anonymous_14", count: 0, regions: { 0: { startLoc: { line: 111, col: 7 }, endLoc: { line: 142, col: 29 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 32 }, 33: { name: "anonymous_15", count: 0, regions: { 0: { startLoc: { line: 112, col: 9 }, endLoc: { line: 126, col: 24 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 33 }, 34: { name: "anonymous_16", count: 0, regions: { 0: { startLoc: { line: 113, col: 11 }, endLoc: { line: 117, col: 34 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 34 }, 35: { name: "anonymous_17", count: 0, regions: { 0: { startLoc: { line: 118, col: 11 }, endLoc: { line: 124, col: 26 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 35 }, 36: { name: "anonymous_18", count: 0, regions: { 0: { startLoc: { line: 128, col: 9 }, endLoc: { line: 138, col: 12 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 36 }, 37: { name: "anonymous_19", count: 0, regions: { 0: { startLoc: { line: 136, col: 20 }, endLoc: { line: 138, col: 12 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 137, col: 13 }, endLoc: { line: 138, col: 12 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 37 }, 38: { name: "anonymous_20", count: 0, regions: { 0: { startLoc: { line: 144, col: 7 }, endLoc: { line: 148, col: 30 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 38 }, 39: { name: "anonymous_21", count: 0, regions: { 0: { startLoc: { line: 150, col: 7 }, endLoc: { line: 158, col: 38 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 39 }, 40: { name: "anonymous_22", count: 0, regions: { 0: { startLoc: { line: 151, col: 9 }, endLoc: { line: 151, col: 13 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 40 }, 41: { name: "anonymous_23", count: 0, regions: { 0: { startLoc: { line: 151, col: 9 }, endLoc: { line: 157, col: 60 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 41 }, 42: { name: "anonymous_24", count: 0, regions: { 0: { startLoc: { line: 158, col: 10 }, endLoc: { line: 158, col: 38 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 42 }, 43: { name: "initialRender", count: 0, regions: { 0: { startLoc: { line: 191, col: 3 }, endLoc: { line: 232, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 43 }, 44: { name: "anonymous_25", count: 0, regions: { 0: { startLoc: { line: 192, col: 5 }, endLoc: { line: 231, col: 71 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 44 }, 45: { name: "anonymous_26", count: 0, regions: { 0: { startLoc: { line: 193, col: 7 }, endLoc: { line: 196, col: 36 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 45 }, 46: { name: "anonymous_28", count: 0, regions: { 0: { startLoc: { line: 200, col: 7 }, endLoc: { line: 227, col: 22 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 46 }, 47: { name: "anonymous_29", count: 0, regions: { 0: { startLoc: { line: 201, col: 9 }, endLoc: { line: 225, col: 22 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 47 }, 48: { name: "anonymous_30", count: 0, regions: { 0: { startLoc: { line: 202, col: 11 }, endLoc: { line: 205, col: 40 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 48 }, 49: { name: "anonymous_32", count: 0, regions: { 0: { startLoc: { line: 215, col: 11 }, endLoc: { line: 218, col: 40 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 49 } }, exeLine: { 0: 1, 1: 2, 2: 3, 3: 4, 4: 5, 5: 6, 6: 7, 7: 8, 8: 9, 9: 10, 10: 11, 11: 12, 12: 15, 13: 16, 14: 18, 15: 20, 16: 21, 17: 22, 18: 23, 19: 24, 20: 25, 21: 27, 22: 28, 23: 29, 24: 32, 25: 33, 26: 34, 27: 36, 28: 37, 29: 39, 30: 42, 31: 43, 32: 46, 33: 47, 34: 48, 35: 49, 36: 50, 37: 51, 38: 53, 39: 56, 40: 57, 41: 58, 42: 62, 43: 63, 44: 64, 45: 66, 46: 67, 47: 68, 48: 69, 49: 70, 50: 73, 51: 74, 52: 76, 53: 77, 54: 78, 55: 79, 56: 80, 57: 81, 58: 83, 59: 86, 60: 87, 61: 88, 62: 93, 63: 94, 64: 95, 65: 96, 66: 97, 67: 98, 68: 99, 69: 100, 70: 101, 71: 102, 72: 103, 73: 109, 74: 110, 75: 111, 76: 112, 77: 113, 78: 114, 79: 115, 80: 116, 81: 117, 82: 118, 83: 119, 84: 120, 85: 121, 86: 122, 87: 123, 88: 124, 89: 126, 90: 128, 91: 129, 92: 130, 93: 131, 94: 132, 95: 133, 96: 134, 97: 135, 98: 136, 99: 137, 100: 140, 101: 141, 102: 142, 103: 144, 104: 145, 105: 146, 106: 147, 107: 148, 108: 150, 109: 151, 110: 152, 111: 153, 112: 154, 113: 155, 114: 156, 115: 157, 116: 158, 117: 160, 118: 161, 119: 162, 120: 163, 121: 164, 122: 165, 123: 166, 124: 167, 125: 168, 126: 170, 127: 171, 128: 172, 129: 173, 130: 174, 131: 176, 132: 177, 133: 178, 134: 180, 135: 181, 136: 182, 137: 183, 138: 185, 139: 186, 140: 191, 141: 192, 142: 193, 143: 194, 144: 195, 145: 196, 146: 197, 147: 200, 148: 201, 149: 202, 150: 203, 151: 204, 152: 205, 153: 208, 154: 209, 155: 210, 156: 211, 157: 212, 158: 213, 159: 215, 160: 216, 161: 217, 162: 218, 163: 221, 164: 223, 165: 224, 166: 225, 167: 227, 168: 229, 169: 230, 170: 231 } });
if (!("finalizeConstruction" in ViewPU.prototype)) {
    Reflect.set(ViewPU.prototype, "finalizeConstruction", () => { });
}
interface BasicSettingsPage_Params {
    onBack?: () => void;
    onModeChange?: (mode: SettingsAppearanceMode) => void;
    localMode?: SettingsAppearanceMode;
    systemDark?: boolean;
    interfaceName?: string;
    ipAddresses?: string[];
    networkCardHovered?: boolean;
    networkCardPressed?: boolean;
}
import connection from "@ohos:net.connection";
import { SettingsAccent, SettingsListItem, SettingsPageHeader, SettingsResources, SettingsSectionTitle, SettingsTheme } from "@normalized:N&&&common/src/main/ets/components/settings/SettingsUi&";
import type { SettingsAccentName, SettingsAppearanceMode } from "@normalized:N&&&common/src/main/ets/components/settings/SettingsUi&";
import { SettingsText } from "@normalized:N&&&common/src/main/ets/components/settings/SettingsConstants&";
export class BasicSettingsPage extends ViewPU {
    constructor(parent, params, __localStorage, elmtId = -1, paramsLambda = undefined, extraInfo) {
        super(parent, __localStorage, elmtId, extraInfo);
        if (typeof paramsLambda === "function") {
            this.paramsGenerator_ = paramsLambda;
        }
        this.onBack = () => {
            bjccovmshb1i8y.instrumentFunction(2);
        };
        this.onModeChange = (_mode: SettingsAppearanceMode) => {
            bjccovmshb1i8y.instrumentFunction(3);
        };
        this.__localMode = new ObservedPropertySimplePU(SettingsTheme.getStoredAppearanceMode(), this, "localMode");
        this.__systemDark = this.createStorageLink('settingsSystemDark', false, "systemDark");
        this.__interfaceName = new ObservedPropertySimplePU(SettingsText.EMPTY_VALUE, this, "interfaceName");
        this.__ipAddresses = new ObservedPropertyObjectPU([SettingsText.NETWORK_LOADING], this, "ipAddresses");
        this.__networkCardHovered = new ObservedPropertySimplePU(false, this, "networkCardHovered");
        this.__networkCardPressed = new ObservedPropertySimplePU(false, this, "networkCardPressed");
        this.setInitiallyProvidedValue(params);
        this.finalizeConstruction();
    }
    setInitiallyProvidedValue(params: BasicSettingsPage_Params) {
        if (params.onBack !== undefined) {
            this.onBack = params.onBack;
        }
        if (params.onModeChange !== undefined) {
            this.onModeChange = params.onModeChange;
        }
        if (params.localMode !== undefined) {
            this.localMode = params.localMode;
        }
        if (params.interfaceName !== undefined) {
            this.interfaceName = params.interfaceName;
        }
        if (params.ipAddresses !== undefined) {
            this.ipAddresses = params.ipAddresses;
        }
        if (params.networkCardHovered !== undefined) {
            this.networkCardHovered = params.networkCardHovered;
        }
        if (params.networkCardPressed !== undefined) {
            this.networkCardPressed = params.networkCardPressed;
        }
    }
    updateStateVars(params: BasicSettingsPage_Params) {
    }
    purgeVariableDependenciesOnElmtId(rmElmtId) {
        this.__localMode.purgeDependencyOnElmtId(rmElmtId);
        this.__systemDark.purgeDependencyOnElmtId(rmElmtId);
        this.__interfaceName.purgeDependencyOnElmtId(rmElmtId);
        this.__ipAddresses.purgeDependencyOnElmtId(rmElmtId);
        this.__networkCardHovered.purgeDependencyOnElmtId(rmElmtId);
        this.__networkCardPressed.purgeDependencyOnElmtId(rmElmtId);
    }
    aboutToBeDeleted() {
        this.__localMode.aboutToBeDeleted();
        this.__systemDark.aboutToBeDeleted();
        this.__interfaceName.aboutToBeDeleted();
        this.__ipAddresses.aboutToBeDeleted();
        this.__networkCardHovered.aboutToBeDeleted();
        this.__networkCardPressed.aboutToBeDeleted();
        SubscriberManager.Get().delete(this.id__());
        this.aboutToBeDeletedInternal();
    }
    private onBack: () => void;
    private onModeChange: (mode: SettingsAppearanceMode) => void;
    private __localMode: ObservedPropertySimplePU<SettingsAppearanceMode>;
    get localMode() {
        bjccovmshb1i8y.instrumentFunction(6);
        return this.__localMode.get();
    }
    set localMode(newValue: SettingsAppearanceMode) {
        bjccovmshb1i8y.instrumentFunction(7);
        this.__localMode.set(newValue);
    }
    private __systemDark: ObservedPropertyAbstractPU<boolean>;
    get systemDark() {
        bjccovmshb1i8y.instrumentFunction(8);
        return this.__systemDark.get();
    }
    set systemDark(newValue: boolean) {
        bjccovmshb1i8y.instrumentFunction(9);
        this.__systemDark.set(newValue);
    }
    private __interfaceName: ObservedPropertySimplePU<string>;
    get interfaceName() {
        bjccovmshb1i8y.instrumentFunction(10);
        return this.__interfaceName.get();
    }
    set interfaceName(newValue: string) {
        bjccovmshb1i8y.instrumentFunction(11);
        this.__interfaceName.set(newValue);
    }
    private __ipAddresses: ObservedPropertyObjectPU<string[]>;
    get ipAddresses() {
        bjccovmshb1i8y.instrumentFunction(12);
        return this.__ipAddresses.get();
    }
    set ipAddresses(newValue: string[]) {
        bjccovmshb1i8y.instrumentFunction(13);
        this.__ipAddresses.set(newValue);
    }
    private __networkCardHovered: ObservedPropertySimplePU<boolean>;
    get networkCardHovered() {
        bjccovmshb1i8y.instrumentFunction(14);
        return this.__networkCardHovered.get();
    }
    set networkCardHovered(newValue: boolean) {
        bjccovmshb1i8y.instrumentFunction(15);
        this.__networkCardHovered.set(newValue);
    }
    private __networkCardPressed: ObservedPropertySimplePU<boolean>;
    get networkCardPressed() {
        bjccovmshb1i8y.instrumentFunction(16);
        return this.__networkCardPressed.get();
    }
    set networkCardPressed(newValue: boolean) {
        bjccovmshb1i8y.instrumentFunction(17);
        this.__networkCardPressed.set(newValue);
    }
    aboutToAppear(): void {
        bjccovmshb1i8y.instrumentFunction(18);
        bjccovmshb1i8y.instrumentRegion(18, 1);
        this.localMode = SettingsTheme.getStoredAppearanceMode();
        this.refreshIp();
    }
    private resolveVisualDark(mode: SettingsAppearanceMode): boolean {
        bjccovmshb1i8y.instrumentFunction(19);
        if (mode === 'dark') {
            bjccovmshb1i8y.instrumentBranch(19, 0, true);
            bjccovmshb1i8y.instrumentRegion(19, 1);
            return true;
        }
        else {
            bjccovmshb1i8y.instrumentBranch(19, 0, false);
        }
        if (mode === 'light') {
            bjccovmshb1i8y.instrumentBranch(19, 1, true);
            bjccovmshb1i8y.instrumentRegion(19, 2);
            return false;
        }
        else {
            bjccovmshb1i8y.instrumentBranch(19, 1, false);
        }
        bjccovmshb1i8y.instrumentRegion(19, 3);
        return this.systemDark;
    }
    private isVisualDark(): boolean {
        bjccovmshb1i8y.instrumentFunction(20);
        bjccovmshb1i8y.instrumentRegion(20, 1);
        return this.resolveVisualDark(this.localMode);
    }
    private applyColorMode(mode: SettingsAppearanceMode): void {
        bjccovmshb1i8y.instrumentFunction(21);
        const previousMode = this.localMode;
        this.localMode = mode;
        if (SettingsTheme.applyAppearanceMode(this.getUIContext(), mode)) {
            bjccovmshb1i8y.instrumentBranch(21, 0, true);
            bjccovmshb1i8y.instrumentRegion(21, 1);
            this.onModeChange(mode);
            return;
        }
        else {
            bjccovmshb1i8y.instrumentBranch(21, 0, false);
        }
        bjccovmshb1i8y.instrumentRegion(21, 2);
        this.localMode = previousMode;
    }
    private setNetworkCardHovered(hovered: boolean): void {
        bjccovmshb1i8y.instrumentFunction(22);
        bjccovmshb1i8y.instrumentRegion(22, 1);
        SettingsTheme.animate(this.getUIContext(), () => {
            bjccovmshb1i8y.instrumentFunction(23);
            bjccovmshb1i8y.instrumentRegion(23, 1);
            this.networkCardHovered = hovered;
        });
    }
    private async refreshIp(): Promise<void> {
        bjccovmshb1i8y.instrumentFunction(24);
        this.interfaceName = SettingsText.EMPTY_VALUE;
        this.ipAddresses = [SettingsText.NETWORK_LOADING];
        try {
            bjccovmshb1i8y.instrumentRegion(24, 1);
            const netHandle = await connection.getDefaultNet();
            if (!netHandle || netHandle.netId === 0) {
                bjccovmshb1i8y.instrumentBranch(24, 0, true);
                bjccovmshb1i8y.instrumentRegion(24, 3);
                this.ipAddresses = [SettingsText.NETWORK_EMPTY];
                return;
            }
            else {
                bjccovmshb1i8y.instrumentBranch(24, 0, false);
            }
            const properties = await connection.getConnectionProperties(netHandle);
            this.interfaceName = properties.interfaceName || SettingsText.EMPTY_VALUE;
            const addresses: string[] = [];
            for (let i = 0; i < properties.linkAddresses.length; i++) {
                bjccovmshb1i8y.instrumentRegion(24, 4);
                const linkAddress = properties.linkAddresses[i];
                const address = linkAddress.address.address;
                if (!address || address === '::1' || address.indexOf('127.') === 0) {
                    bjccovmshb1i8y.instrumentBranch(24, 1, true);
                    bjccovmshb1i8y.instrumentRegion(24, 5);
                    continue;
                }
                else {
                    bjccovmshb1i8y.instrumentBranch(24, 1, false);
                }
                bjccovmshb1i8y.instrumentRegion(24, 7);
                addresses.push(`${address}/${linkAddress.prefixLength}`);
            }
            bjccovmshb1i8y.instrumentRegion(24, 6);
            this.ipAddresses = addresses.length > 0 ? (bjccovmshb1i8y.instrumentBranch(24, 2, true), addresses) : (bjccovmshb1i8y.instrumentBranch(24, 2, false), [SettingsText.NETWORK_NO_IP]);
        }
        catch (error) {
            bjccovmshb1i8y.instrumentRegion(24, 2);
            this.ipAddresses = [`${SettingsText.NETWORK_READ_FAILED}: ${JSON.stringify(error)}`];
        }
    }
    private modeOption(mode: SettingsAppearanceMode, title: string, description: string, iconResource: Resource, accentName: SettingsAccentName, parent = null) {
        bjccovmshb1i8y.instrumentFunction(25);
        {
            this.observeComponentCreation2((elmtId, isInitialRender) => {
                bjccovmshb1i8y.instrumentFunction(26);
                if (isInitialRender) {
                    let componentCall = new SettingsListItem(this, {
                        title: title,
                        description: description,
                        iconResource: iconResource,
                        accentName: accentName,
                        selected: this.localMode === mode,
                        isDark: this.isVisualDark(),
                        onPress: () => {
                            bjccovmshb1i8y.instrumentFunction(27);
                            bjccovmshb1i8y.instrumentRegion(27, 1);
                            this.applyColorMode(mode);
                        }
                    }, undefined, elmtId, () => { }, { page: "common/src/main/ets/components/settings/BasicSettingsPage.ets", line: 95, col: 5 });
                    ViewPU.create(componentCall);
                    let paramsLambda = () => {
                        return {
                            title: title,
                            description: description,
                            iconResource: iconResource,
                            accentName: accentName,
                            selected: this.localMode === mode,
                            isDark: this.isVisualDark(),
                            onPress: () => {
                                this.applyColorMode(mode);
                            }
                        };
                    };
                    componentCall.paramsGenerator_ = paramsLambda;
                }
                else {
                    this.updateStateVarsOfChildByElmtId(elmtId, {
                        title: title,
                        description: description,
                        iconResource: iconResource,
                        accentName: accentName,
                        selected: this.localMode === mode,
                        isDark: this.isVisualDark()
                    });
                }
            }, { name: "SettingsListItem" });
        }
    }
    private networkInfoCard(parent = null) {
        bjccovmshb1i8y.instrumentFunction(28);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i8y.instrumentFunction(29);
            Column.create();
            Column.alignItems(HorizontalAlign.Start);
            Column.width('100%');
            Column.padding(16);
            Column.backgroundColor(this.networkCardHovered || this.networkCardPressed ? (bjccovmshb1i8y.instrumentBranch(29, 0, true), SettingsTheme.cardHoverBackground(this.isVisualDark())) : (bjccovmshb1i8y.instrumentBranch(29, 0, false), SettingsTheme.cardBackground(this.isVisualDark())));
            Column.borderRadius(SettingsTheme.CARD_RADIUS);
            Column.border({
                width: 1,
                color: SettingsTheme.borderColor(this.isVisualDark())
            });
            Column.shadow(SettingsTheme.shadow(this.isVisualDark(), this.networkCardHovered));
            Column.translate({ y: this.networkCardHovered ? (bjccovmshb1i8y.instrumentBranch(29, 1, true), -5) : (bjccovmshb1i8y.instrumentBranch(29, 1, false), 0) });
            Column.scale({
                x: this.networkCardPressed ? (bjccovmshb1i8y.instrumentBranch(29, 2, true), 0.99) : (bjccovmshb1i8y.instrumentBranch(29, 2, false), this.networkCardHovered ? (bjccovmshb1i8y.instrumentBranch(29, 3, true), 1.012) : (bjccovmshb1i8y.instrumentBranch(29, 3, false), 1)),
                y: this.networkCardPressed ? (bjccovmshb1i8y.instrumentBranch(29, 4, true), 0.99) : (bjccovmshb1i8y.instrumentBranch(29, 4, false), this.networkCardHovered ? (bjccovmshb1i8y.instrumentBranch(29, 5, true), 1.012) : (bjccovmshb1i8y.instrumentBranch(29, 5, false), 1))
            });
            Column.margin({ bottom: 10 });
            Column.onHover((isHover: boolean) => {
                bjccovmshb1i8y.instrumentFunction(30);
                bjccovmshb1i8y.instrumentRegion(30, 1);
                this.setNetworkCardHovered(isHover);
            });
            Column.onTouch((event: TouchEvent) => {
                bjccovmshb1i8y.instrumentFunction(31);
                if (event.type === TouchType.Down) {
                    bjccovmshb1i8y.instrumentBranch(31, 0, true);
                    bjccovmshb1i8y.instrumentRegion(31, 1);
                    this.networkCardPressed = true;
                    bjccovmshb1i8y.instrumentRegion(28, 1);
                    return;
                }
                else {
                    bjccovmshb1i8y.instrumentBranch(31, 0, false);
                }
                if (event.type === TouchType.Up || event.type === TouchType.Cancel) {
                    bjccovmshb1i8y.instrumentBranch(31, 1, true);
                    bjccovmshb1i8y.instrumentRegion(31, 2);
                    this.networkCardPressed = false;
                }
                else {
                    bjccovmshb1i8y.instrumentBranch(31, 1, false);
                }
            });
        }, Column);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i8y.instrumentFunction(32);
            Row.create();
            Row.alignItems(VerticalAlign.Center);
            Row.width('100%');
            Row.margin({ bottom: 14 });
        }, Row);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i8y.instrumentFunction(33);
            Column.create();
            Column.layoutWeight(1);
        }, Column);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i8y.instrumentFunction(34);
            Text.create(SettingsText.NETWORK_NAME_LABEL);
            Text.fontSize(12);
            Text.fontColor(SettingsTheme.mutedText(this.isVisualDark()));
            Text.width('100%');
            Text.margin({ bottom: 4 });
        }, Text);
        Text.pop();
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i8y.instrumentFunction(35);
            Text.create(this.interfaceName);
            Text.fontSize(16);
            Text.fontWeight(FontWeight.Medium);
            Text.fontColor(SettingsTheme.primaryText(this.isVisualDark()));
            Text.maxLines(1);
            Text.textOverflow({ overflow: TextOverflow.Ellipsis });
            Text.width('100%');
        }, Text);
        Text.pop();
        Column.pop();
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i8y.instrumentFunction(36);
            Button.createWithLabel(SettingsText.NETWORK_REFRESH_ACTION);
            Button.type(ButtonType.Normal);
            Button.constraintSize({ minHeight: 48 });
            Button.borderRadius(SettingsTheme.BUTTON_RADIUS);
            Button.backgroundColor(SettingsTheme.subtleButton(this.isVisualDark(), false, false));
            Button.fontColor(SettingsTheme.primaryText(this.isVisualDark()));
            Button.fontSize(12);
            Button.stateEffect(false);
            Button.onClick(() => {
                bjccovmshb1i8y.instrumentFunction(37);
                bjccovmshb1i8y.instrumentRegion(37, 1);
                this.refreshIp();
            });
        }, Button);
        Button.pop();
        Row.pop();
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i8y.instrumentFunction(38);
            Text.create(SettingsText.NETWORK_IP_LABEL);
            Text.fontSize(12);
            Text.fontColor(SettingsTheme.mutedText(this.isVisualDark()));
            Text.width('100%');
            Text.margin({ bottom: 8 });
        }, Text);
        Text.pop();
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i8y.instrumentFunction(39);
            ForEach.create();
            const forEachItemGenFunction = _item => {
                bjccovmshb1i8y.instrumentFunction(40);
                const address = _item;
                this.observeComponentCreation2((elmtId, isInitialRender) => {
                    bjccovmshb1i8y.instrumentFunction(41);
                    Text.create(address);
                    Text.fontSize(15);
                    Text.fontColor(SettingsTheme.secondaryText(this.isVisualDark()));
                    Text.width('100%');
                    Text.margin({ bottom: 8 });
                    Text.maxLines(2);
                    Text.textOverflow({ overflow: TextOverflow.Ellipsis });
                }, Text);
                Text.pop();
            };
            this.forEachUpdateFunction(elmtId, this.ipAddresses, forEachItemGenFunction, (address: string) => { bjccovmshb1i8y.instrumentFunction(42); return address; }, false, false);
        }, ForEach);
        ForEach.pop();
        Column.pop();
    }
    initialRender() {
        bjccovmshb1i8y.instrumentFunction(43);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i8y.instrumentFunction(44);
            Column.create();
            Column.width('100%');
            Column.height('100%');
            Column.backgroundColor(SettingsTheme.pageBackground(this.isVisualDark()));
        }, Column);
        {
            this.observeComponentCreation2((elmtId, isInitialRender) => {
                bjccovmshb1i8y.instrumentFunction(45);
                if (isInitialRender) {
                    let componentCall = new SettingsPageHeader(this, {
                        title: SettingsText.BASIC_SETTINGS_TITLE,
                        subtitle: SettingsText.BASIC_SETTINGS_SUBTITLE,
                        isDark: this.isVisualDark(),
                        onBack: this.onBack
                    }, undefined, elmtId, () => { }, { page: "common/src/main/ets/components/settings/BasicSettingsPage.ets", line: 193, col: 7 });
                    ViewPU.create(componentCall);
                    let paramsLambda = () => {
                        return {
                            title: SettingsText.BASIC_SETTINGS_TITLE,
                            subtitle: SettingsText.BASIC_SETTINGS_SUBTITLE,
                            isDark: this.isVisualDark(),
                            onBack: this.onBack
                        };
                    };
                    componentCall.paramsGenerator_ = paramsLambda;
                }
                else {
                    this.updateStateVarsOfChildByElmtId(elmtId, {
                        title: SettingsText.BASIC_SETTINGS_TITLE,
                        subtitle: SettingsText.BASIC_SETTINGS_SUBTITLE,
                        isDark: this.isVisualDark()
                    });
                }
            }, { name: "SettingsPageHeader" });
        }
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i8y.instrumentFunction(46);
            Scroll.create();
            Scroll.layoutWeight(1);
        }, Scroll);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i8y.instrumentFunction(47);
            Column.create();
            Column.alignItems(HorizontalAlign.Start);
            Column.padding({ left: 20, right: 20, bottom: 24 });
            Column.width('100%');
        }, Column);
        {
            this.observeComponentCreation2((elmtId, isInitialRender) => {
                bjccovmshb1i8y.instrumentFunction(48);
                if (isInitialRender) {
                    let componentCall = new SettingsSectionTitle(this, {
                        title: SettingsText.BASIC_APPEARANCE_SECTION,
                        subtitle: SettingsText.APPEARANCE_SECTION_MODE_DESC,
                        isDark: this.isVisualDark()
                    }, undefined, elmtId, () => { }, { page: "common/src/main/ets/components/settings/BasicSettingsPage.ets", line: 202, col: 11 });
                    ViewPU.create(componentCall);
                    let paramsLambda = () => {
                        return {
                            title: SettingsText.BASIC_APPEARANCE_SECTION,
                            subtitle: SettingsText.APPEARANCE_SECTION_MODE_DESC,
                            isDark: this.isVisualDark()
                        };
                    };
                    componentCall.paramsGenerator_ = paramsLambda;
                }
                else {
                    this.updateStateVarsOfChildByElmtId(elmtId, {
                        title: SettingsText.BASIC_APPEARANCE_SECTION,
                        subtitle: SettingsText.APPEARANCE_SECTION_MODE_DESC,
                        isDark: this.isVisualDark()
                    });
                }
            }, { name: "SettingsSectionTitle" });
        }
        this.modeOption.bind(this)('system', SettingsText.FOLLOW_SYSTEM_TITLE, SettingsText.FOLLOW_SYSTEM_DESC, SettingsResources.SYSTEM_ICON, SettingsAccent.CYAN);
        this.modeOption.bind(this)('light', SettingsText.LIGHT_MODE_TITLE, SettingsText.LIGHT_MODE_DESC, SettingsResources.SUN_ICON, SettingsAccent.ORANGE);
        this.modeOption.bind(this)('dark', SettingsText.DARK_MODE_TITLE, SettingsText.DARK_MODE_DESC, SettingsResources.MOON_ICON, SettingsAccent.INDIGO);
        {
            this.observeComponentCreation2((elmtId, isInitialRender) => {
                bjccovmshb1i8y.instrumentFunction(49);
                if (isInitialRender) {
                    let componentCall = new SettingsSectionTitle(this, {
                        title: SettingsText.BASIC_NETWORK_SECTION,
                        subtitle: SettingsText.NETWORK_SECTION_DETAIL_DESC,
                        isDark: this.isVisualDark()
                    }, undefined, elmtId, () => { }, { page: "common/src/main/ets/components/settings/BasicSettingsPage.ets", line: 215, col: 11 });
                    ViewPU.create(componentCall);
                    let paramsLambda = () => {
                        return {
                            title: SettingsText.BASIC_NETWORK_SECTION,
                            subtitle: SettingsText.NETWORK_SECTION_DETAIL_DESC,
                            isDark: this.isVisualDark()
                        };
                    };
                    componentCall.paramsGenerator_ = paramsLambda;
                }
                else {
                    this.updateStateVarsOfChildByElmtId(elmtId, {
                        title: SettingsText.BASIC_NETWORK_SECTION,
                        subtitle: SettingsText.NETWORK_SECTION_DETAIL_DESC,
                        isDark: this.isVisualDark()
                    });
                }
            }, { name: "SettingsSectionTitle" });
        }
        this.networkInfoCard.bind(this)();
        Column.pop();
        Scroll.pop();
        Column.pop();
    }
    rerender() {
        this.updateDirtyElements();
    }
}
