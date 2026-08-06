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
let bjccovmshb1ia4 = new BjcCov({ version: "bjc v1.0.0", versionCode: 10000, path: "common/src/main/ets/components/settings/ProjectHelpPage.ets", hash: "d2018d295d5b2b9e761ebecfd015f67349bdd647bb8379696c3c79d1c77615fa", lineCnt: 176, count: 0, projectPath: "", functions: { 0: { name: "anonymous_1", count: 0, regions: { 0: { startLoc: { line: 12, col: 11 }, endLoc: { line: 12, col: 21 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 0 }, 1: { name: "anonymous_2", count: 0, regions: { 0: { startLoc: { line: 12, col: 24 }, endLoc: { line: 13, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 1 }, 2: { name: "setInitiallyProvidedValue", count: 0, regions: { 0: { startLoc: { line: 14, col: 27 }, endLoc: { line: 14, col: 32 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 2 }, 3: { name: "updateStateVars", count: 0, regions: { 0: { startLoc: { line: 14, col: 9 }, endLoc: { line: 14, col: 15 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 3 }, 4: { name: "anonymous_3", count: 0, regions: { 0: { startLoc: { line: 12, col: 11 }, endLoc: { line: 12, col: 21 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 4 }, 5: { name: "isDark", count: 0, regions: { 0: { startLoc: { line: 14, col: 9 }, endLoc: { line: 14, col: 15 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 5 }, 6: { name: "isDark", count: 0, regions: { 0: { startLoc: { line: 14, col: 9 }, endLoc: { line: 14, col: 24 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 6 }, 7: { name: "aboutCardHovered", count: 0, regions: { 0: { startLoc: { line: 15, col: 18 }, endLoc: { line: 15, col: 34 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 7 }, 8: { name: "aboutCardHovered", count: 0, regions: { 0: { startLoc: { line: 15, col: 18 }, endLoc: { line: 15, col: 43 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 8 }, 9: { name: "aboutCardPressed", count: 0, regions: { 0: { startLoc: { line: 16, col: 18 }, endLoc: { line: 16, col: 34 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 9 }, 10: { name: "aboutCardPressed", count: 0, regions: { 0: { startLoc: { line: 16, col: 18 }, endLoc: { line: 16, col: 43 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 10 }, 11: { name: "setAboutCardHovered", count: 0, regions: { 0: { startLoc: { line: 18, col: 3 }, endLoc: { line: 22, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 19, col: 5 }, endLoc: { line: 22, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 11 }, 12: { name: "anonymous_4", count: 0, regions: { 0: { startLoc: { line: 19, col: 48 }, endLoc: { line: 21, col: 6 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 20, col: 7 }, endLoc: { line: 21, col: 6 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 12 }, 13: { name: "isUrl", count: 0, regions: { 0: { startLoc: { line: 24, col: 3 }, endLoc: { line: 26, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 25, col: 5 }, endLoc: { line: 26, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 13 }, 14: { name: "aboutInfoRow", count: 0, regions: { 0: { startLoc: { line: 28, col: 3 }, endLoc: { line: 51, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 14 }, 15: { name: "anonymous_5", count: 0, regions: { 0: { startLoc: { line: 30, col: 5 }, endLoc: { line: 50, col: 27 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 15 }, 16: { name: "anonymous_6", count: 0, regions: { 0: { startLoc: { line: 31, col: 7 }, endLoc: { line: 35, col: 30 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 16 }, 17: { name: "anonymous_7", count: 0, regions: { 0: { startLoc: { line: 36, col: 7 }, endLoc: { line: 46, col: 8 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 36, col: 30 }, endLoc: { line: 40, col: 8 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 40, col: 14 }, endLoc: { line: 46, col: 8 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 36, col: 11 }, endLoc: { line: 36, col: 28 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 17 }, 18: { name: "anonymous_8", count: 0, regions: { 0: { startLoc: { line: 37, col: 9 }, endLoc: { line: 37, col: 18 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 18 }, 19: { name: "anonymous_9", count: 0, regions: { 0: { startLoc: { line: 37, col: 9 }, endLoc: { line: 39, col: 24 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 19 }, 20: { name: "anonymous_10", count: 0, regions: { 0: { startLoc: { line: 41, col: 9 }, endLoc: { line: 41, col: 13 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 20 }, 21: { name: "anonymous_11", count: 0, regions: { 0: { startLoc: { line: 41, col: 9 }, endLoc: { line: 45, col: 25 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 21 }, 22: { name: "aboutInfoCard", count: 0, regions: { 0: { startLoc: { line: 53, col: 3 }, endLoc: { line: 90, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 84, col: 9 }, endLoc: { line: 85, col: 8 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 22 }, 23: { name: "anonymous_12", count: 0, regions: { 0: { startLoc: { line: 55, col: 5 }, endLoc: { line: 89, col: 6 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 64, col: 22 }, endLoc: { line: 65, col: 48 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 72, col: 21 }, endLoc: { line: 72, col: 51 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 2: { startLoc: { line: 74, col: 10 }, endLoc: { line: 74, col: 74 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 3: { startLoc: { line: 74, col: 41 }, endLoc: { line: 74, col: 74 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 4: { startLoc: { line: 75, col: 10 }, endLoc: { line: 75, col: 74 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 5: { startLoc: { line: 75, col: 41 }, endLoc: { line: 75, col: 74 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 23 }, 24: { name: "anonymous_13", count: 0, regions: { 0: { startLoc: { line: 78, col: 14 }, endLoc: { line: 80, col: 6 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 79, col: 7 }, endLoc: { line: 80, col: 6 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 24 }, 25: { name: "anonymous_14", count: 0, regions: { 0: { startLoc: { line: 81, col: 14 }, endLoc: { line: 89, col: 6 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 82, col: 42 }, endLoc: { line: 85, col: 8 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 86, col: 75 }, endLoc: { line: 88, col: 8 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 82, col: 11 }, endLoc: { line: 82, col: 40 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 86, col: 11 }, endLoc: { line: 86, col: 73 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 25 }, 26: { name: "initialRender", count: 0, regions: { 0: { startLoc: { line: 92, col: 3 }, endLoc: { line: 174, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 26 }, 27: { name: "anonymous_15", count: 0, regions: { 0: { startLoc: { line: 93, col: 5 }, endLoc: { line: 173, col: 63 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 27 }, 28: { name: "anonymous_16", count: 0, regions: { 0: { startLoc: { line: 94, col: 7 }, endLoc: { line: 97, col: 28 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 28 }, 29: { name: "anonymous_18", count: 0, regions: { 0: { startLoc: { line: 101, col: 7 }, endLoc: { line: 169, col: 22 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 29 }, 30: { name: "anonymous_19", count: 0, regions: { 0: { startLoc: { line: 102, col: 9 }, endLoc: { line: 167, col: 22 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 30 }, 31: { name: "anonymous_20", count: 0, regions: { 0: { startLoc: { line: 103, col: 11 }, endLoc: { line: 106, col: 32 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 31 }, 32: { name: "anonymous_22", count: 0, regions: { 0: { startLoc: { line: 109, col: 11 }, endLoc: { line: 112, col: 32 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 32 }, 33: { name: "anonymous_24", count: 0, regions: { 0: { startLoc: { line: 114, col: 11 }, endLoc: { line: 117, col: 32 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 33 }, 34: { name: "anonymous_26", count: 0, regions: { 0: { startLoc: { line: 119, col: 11 }, endLoc: { line: 122, col: 32 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 34 }, 35: { name: "anonymous_28", count: 0, regions: { 0: { startLoc: { line: 124, col: 11 }, endLoc: { line: 127, col: 32 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 35 }, 36: { name: "anonymous_30", count: 0, regions: { 0: { startLoc: { line: 129, col: 11 }, endLoc: { line: 132, col: 32 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 36 }, 37: { name: "anonymous_32", count: 0, regions: { 0: { startLoc: { line: 134, col: 11 }, endLoc: { line: 137, col: 32 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 37 }, 38: { name: "anonymous_34", count: 0, regions: { 0: { startLoc: { line: 140, col: 11 }, endLoc: { line: 143, col: 32 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 38 }, 39: { name: "anonymous_36", count: 0, regions: { 0: { startLoc: { line: 146, col: 11 }, endLoc: { line: 149, col: 32 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 39 }, 40: { name: "anonymous_38", count: 0, regions: { 0: { startLoc: { line: 151, col: 11 }, endLoc: { line: 154, col: 32 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 40 }, 41: { name: "anonymous_40", count: 0, regions: { 0: { startLoc: { line: 157, col: 11 }, endLoc: { line: 160, col: 32 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 41 } }, exeLine: { 0: 1, 1: 2, 2: 3, 3: 4, 4: 5, 5: 6, 6: 7, 7: 8, 8: 11, 9: 12, 10: 14, 11: 15, 12: 16, 13: 18, 14: 19, 15: 20, 16: 24, 17: 25, 18: 29, 19: 30, 20: 31, 21: 32, 22: 33, 23: 34, 24: 35, 25: 36, 26: 37, 27: 38, 28: 39, 29: 40, 30: 41, 31: 42, 32: 43, 33: 44, 34: 45, 35: 48, 36: 49, 37: 50, 38: 54, 39: 55, 40: 56, 41: 57, 42: 58, 43: 59, 44: 60, 45: 62, 46: 63, 47: 64, 48: 65, 49: 66, 50: 67, 51: 68, 52: 69, 53: 71, 54: 72, 55: 73, 56: 74, 57: 75, 58: 77, 59: 78, 60: 79, 61: 81, 62: 82, 63: 83, 64: 84, 65: 86, 66: 87, 67: 92, 68: 93, 69: 94, 70: 95, 71: 96, 72: 97, 73: 98, 74: 101, 75: 102, 76: 103, 77: 104, 78: 105, 79: 106, 80: 109, 81: 110, 82: 111, 83: 112, 84: 114, 85: 115, 86: 116, 87: 117, 88: 119, 89: 120, 90: 121, 91: 122, 92: 124, 93: 125, 94: 126, 95: 127, 96: 129, 97: 130, 98: 131, 99: 132, 100: 134, 101: 135, 102: 136, 103: 137, 104: 140, 105: 141, 106: 142, 107: 143, 108: 146, 109: 147, 110: 148, 111: 149, 112: 151, 113: 152, 114: 153, 115: 154, 116: 157, 117: 158, 118: 159, 119: 160, 120: 163, 121: 165, 122: 166, 123: 167, 124: 169, 125: 171, 126: 172, 127: 173 } });
if (!("finalizeConstruction" in ViewPU.prototype)) {
    Reflect.set(ViewPU.prototype, "finalizeConstruction", () => { });
}
interface ProjectHelpPage_Params {
    onBack?: () => void;
    isDark?: boolean;
    aboutCardHovered?: boolean;
    aboutCardPressed?: boolean;
}
import { SettingsAccent, SettingsInfoCard, SettingsPageHeader, SettingsSectionTitle, SettingsTheme } from "@normalized:N&&&common/src/main/ets/components/settings/SettingsUi&";
import { SettingsText } from "@normalized:N&&&common/src/main/ets/components/settings/SettingsConstants&";
export class ProjectHelpPage extends ViewPU {
    constructor(parent, params, __localStorage, elmtId = -1, paramsLambda = undefined, extraInfo) {
        super(parent, __localStorage, elmtId, extraInfo);
        if (typeof paramsLambda === "function") {
            this.paramsGenerator_ = paramsLambda;
        }
        this.onBack = () => {
            bjccovmshb1ia4.instrumentFunction(1);
        };
        this.__isDark = new SynchedPropertySimpleOneWayPU(params.isDark, this, "isDark");
        this.__aboutCardHovered = new ObservedPropertySimplePU(false, this, "aboutCardHovered");
        this.__aboutCardPressed = new ObservedPropertySimplePU(false, this, "aboutCardPressed");
        this.setInitiallyProvidedValue(params);
        this.finalizeConstruction();
    }
    setInitiallyProvidedValue(params: ProjectHelpPage_Params) {
        bjccovmshb1ia4.instrumentFunction(2);
        if (params.onBack !== undefined) {
            this.onBack = params.onBack;
        }
        else {
        }
        if (params.isDark === undefined) {
            this.__isDark.set(false);
        }
        else {
        }
        if (params.aboutCardHovered !== undefined) {
            this.aboutCardHovered = params.aboutCardHovered;
        }
        else {
        }
        if (params.aboutCardPressed !== undefined) {
            this.aboutCardPressed = params.aboutCardPressed;
        }
        else {
        }
    }
    updateStateVars(params: ProjectHelpPage_Params) {
        bjccovmshb1ia4.instrumentFunction(3);
        this.__isDark.reset(params.isDark);
    }
    purgeVariableDependenciesOnElmtId(rmElmtId) {
        this.__isDark.purgeDependencyOnElmtId(rmElmtId);
        this.__aboutCardHovered.purgeDependencyOnElmtId(rmElmtId);
        this.__aboutCardPressed.purgeDependencyOnElmtId(rmElmtId);
    }
    aboutToBeDeleted() {
        this.__isDark.aboutToBeDeleted();
        this.__aboutCardHovered.aboutToBeDeleted();
        this.__aboutCardPressed.aboutToBeDeleted();
        SubscriberManager.Get().delete(this.id__());
        this.aboutToBeDeletedInternal();
    }
    private onBack: () => void;
    private __isDark: SynchedPropertySimpleOneWayPU<boolean>;
    get isDark() {
        bjccovmshb1ia4.instrumentFunction(5);
        return this.__isDark.get();
    }
    set isDark(newValue: boolean) {
        bjccovmshb1ia4.instrumentFunction(6);
        this.__isDark.set(newValue);
    }
    private __aboutCardHovered: ObservedPropertySimplePU<boolean>;
    get aboutCardHovered() {
        bjccovmshb1ia4.instrumentFunction(7);
        return this.__aboutCardHovered.get();
    }
    set aboutCardHovered(newValue: boolean) {
        bjccovmshb1ia4.instrumentFunction(8);
        this.__aboutCardHovered.set(newValue);
    }
    private __aboutCardPressed: ObservedPropertySimplePU<boolean>;
    get aboutCardPressed() {
        bjccovmshb1ia4.instrumentFunction(9);
        return this.__aboutCardPressed.get();
    }
    set aboutCardPressed(newValue: boolean) {
        bjccovmshb1ia4.instrumentFunction(10);
        this.__aboutCardPressed.set(newValue);
    }
    private setAboutCardHovered(hovered: boolean): void {
        bjccovmshb1ia4.instrumentFunction(11);
        bjccovmshb1ia4.instrumentRegion(11, 1);
        SettingsTheme.animate(this.getUIContext(), () => {
            bjccovmshb1ia4.instrumentFunction(12);
            bjccovmshb1ia4.instrumentRegion(12, 1);
            this.aboutCardHovered = hovered;
        });
    }
    private isUrl(value: string): boolean {
        bjccovmshb1ia4.instrumentFunction(13);
        bjccovmshb1ia4.instrumentRegion(13, 1);
        return value.startsWith('http://') || value.startsWith('https://');
    }
    private aboutInfoRow(label: string, value: string, parent = null) {
        bjccovmshb1ia4.instrumentFunction(14);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1ia4.instrumentFunction(15);
            Column.create();
            Column.alignItems(HorizontalAlign.Start);
            Column.width('100%');
            Column.margin({ bottom: 12 });
        }, Column);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1ia4.instrumentFunction(16);
            Text.create(label);
            Text.fontSize(12);
            Text.fontColor(SettingsTheme.mutedText(this.isDark));
            Text.width('100%');
            Text.margin({ bottom: 4 });
        }, Text);
        Text.pop();
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1ia4.instrumentFunction(17);
            If.create();
            if (this.isUrl(value)) {
                bjccovmshb1ia4.instrumentBranch(17, 0, true);
                bjccovmshb1ia4.instrumentRegion(17, 1);
                this.ifElseBranchUpdateFunction(0, () => {
                    bjccovmshb1ia4.instrumentFunction(18);
                    this.observeComponentCreation2((elmtId, isInitialRender) => {
                        bjccovmshb1ia4.instrumentFunction(19);
                        Hyperlink.create(value, value);
                        Hyperlink.color(SettingsTheme.accentColor(this.isDark, SettingsAccent.BLUE));
                        Hyperlink.width('100%');
                    }, Hyperlink);
                    Hyperlink.pop();
                });
            }
            else {
                bjccovmshb1ia4.instrumentBranch(17, 0, false);
                bjccovmshb1ia4.instrumentRegion(17, 2);
                this.ifElseBranchUpdateFunction(1, () => {
                    bjccovmshb1ia4.instrumentFunction(20);
                    this.observeComponentCreation2((elmtId, isInitialRender) => {
                        bjccovmshb1ia4.instrumentFunction(21);
                        Text.create(value);
                        Text.fontSize(14);
                        Text.fontColor(SettingsTheme.secondaryText(this.isDark));
                        Text.width('100%');
                        Text.lineHeight(20);
                    }, Text);
                    Text.pop();
                });
            }
        }, If);
        If.pop();
        Column.pop();
    }
    private aboutInfoCard(parent = null) {
        bjccovmshb1ia4.instrumentFunction(22);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1ia4.instrumentFunction(23);
            Column.create();
            Column.width('100%');
            Column.padding(16);
            Column.backgroundColor(this.aboutCardHovered || this.aboutCardPressed ? (bjccovmshb1ia4.instrumentBranch(23, 0, true), SettingsTheme.cardHoverBackground(this.isDark)) : (bjccovmshb1ia4.instrumentBranch(23, 0, false), SettingsTheme.cardBackground(this.isDark)));
            Column.borderRadius(SettingsTheme.CARD_RADIUS);
            Column.border({
                width: 1,
                color: SettingsTheme.borderColor(this.isDark)
            });
            Column.shadow(SettingsTheme.shadow(this.isDark, this.aboutCardHovered));
            Column.translate({ y: this.aboutCardHovered ? (bjccovmshb1ia4.instrumentBranch(23, 1, true), -5) : (bjccovmshb1ia4.instrumentBranch(23, 1, false), 0) });
            Column.scale({
                x: this.aboutCardPressed ? (bjccovmshb1ia4.instrumentBranch(23, 2, true), 0.99) : (bjccovmshb1ia4.instrumentBranch(23, 2, false), this.aboutCardHovered ? (bjccovmshb1ia4.instrumentBranch(23, 3, true), 1.012) : (bjccovmshb1ia4.instrumentBranch(23, 3, false), 1)),
                y: this.aboutCardPressed ? (bjccovmshb1ia4.instrumentBranch(23, 4, true), 0.99) : (bjccovmshb1ia4.instrumentBranch(23, 4, false), this.aboutCardHovered ? (bjccovmshb1ia4.instrumentBranch(23, 5, true), 1.012) : (bjccovmshb1ia4.instrumentBranch(23, 5, false), 1))
            });
            Column.margin({ bottom: 10 });
            Column.onHover((isHover: boolean) => {
                bjccovmshb1ia4.instrumentFunction(24);
                bjccovmshb1ia4.instrumentRegion(24, 1);
                this.setAboutCardHovered(isHover);
            });
            Column.onTouch((event: TouchEvent) => {
                bjccovmshb1ia4.instrumentFunction(25);
                if (event.type === TouchType.Down) {
                    bjccovmshb1ia4.instrumentBranch(25, 0, true);
                    bjccovmshb1ia4.instrumentRegion(25, 1);
                    this.aboutCardPressed = true;
                    bjccovmshb1ia4.instrumentRegion(22, 1);
                    return;
                }
                else {
                    bjccovmshb1ia4.instrumentBranch(25, 0, false);
                }
                if (event.type === TouchType.Up || event.type === TouchType.Cancel) {
                    bjccovmshb1ia4.instrumentBranch(25, 1, true);
                    bjccovmshb1ia4.instrumentRegion(25, 2);
                    this.aboutCardPressed = false;
                }
                else {
                    bjccovmshb1ia4.instrumentBranch(25, 1, false);
                }
            });
        }, Column);
        this.aboutInfoRow.bind(this)(SettingsText.ABOUT_GITHUB_LABEL, SettingsText.ABOUT_GITHUB_VALUE);
        this.aboutInfoRow.bind(this)(SettingsText.ABOUT_FREERDP_ADAPTATION_LABEL, SettingsText.ABOUT_FREERDP_ADAPTATION_VALUE);
        this.aboutInfoRow.bind(this)(SettingsText.ABOUT_XRDP_ADAPTATION_LABEL, SettingsText.ABOUT_XRDP_ADAPTATION_VALUE);
        this.aboutInfoRow.bind(this)(SettingsText.ABOUT_LICENSE_LABEL, SettingsText.ABOUT_LICENSE_VALUE);
        Column.pop();
    }
    initialRender() {
        bjccovmshb1ia4.instrumentFunction(26);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1ia4.instrumentFunction(27);
            Column.create();
            Column.width('100%');
            Column.height('100%');
            Column.backgroundColor(SettingsTheme.pageBackground(this.isDark));
        }, Column);
        {
            this.observeComponentCreation2((elmtId, isInitialRender) => {
                bjccovmshb1ia4.instrumentFunction(28);
                if (isInitialRender) {
                    let componentCall = new SettingsPageHeader(this, {
                        title: SettingsText.PROJECT_HELP_TITLE,
                        subtitle: SettingsText.PROJECT_HELP_SUBTITLE,
                        isDark: this.isDark,
                        onBack: this.onBack
                    }, undefined, elmtId, () => { }, { page: "common/src/main/ets/components/settings/ProjectHelpPage.ets", line: 94, col: 7 });
                    ViewPU.create(componentCall);
                    let paramsLambda = () => {
                        return {
                            title: SettingsText.PROJECT_HELP_TITLE,
                            subtitle: SettingsText.PROJECT_HELP_SUBTITLE,
                            isDark: this.isDark,
                            onBack: this.onBack
                        };
                    };
                    componentCall.paramsGenerator_ = paramsLambda;
                }
                else {
                    this.updateStateVarsOfChildByElmtId(elmtId, {
                        title: SettingsText.PROJECT_HELP_TITLE,
                        subtitle: SettingsText.PROJECT_HELP_SUBTITLE,
                        isDark: this.isDark
                    });
                }
            }, { name: "SettingsPageHeader" });
        }
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1ia4.instrumentFunction(29);
            Scroll.create();
            Scroll.layoutWeight(1);
        }, Scroll);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1ia4.instrumentFunction(30);
            Column.create();
            Column.alignItems(HorizontalAlign.Start);
            Column.padding({ left: 20, right: 20, bottom: 24 });
            Column.width('100%');
        }, Column);
        {
            this.observeComponentCreation2((elmtId, isInitialRender) => {
                bjccovmshb1ia4.instrumentFunction(31);
                if (isInitialRender) {
                    let componentCall = new SettingsSectionTitle(this, {
                        title: SettingsText.USAGE_SECTION_PREP,
                        subtitle: SettingsText.USAGE_SECTION_PREP_DESC,
                        isDark: this.isDark
                    }, undefined, elmtId, () => { }, { page: "common/src/main/ets/components/settings/ProjectHelpPage.ets", line: 103, col: 11 });
                    ViewPU.create(componentCall);
                    let paramsLambda = () => {
                        return {
                            title: SettingsText.USAGE_SECTION_PREP,
                            subtitle: SettingsText.USAGE_SECTION_PREP_DESC,
                            isDark: this.isDark
                        };
                    };
                    componentCall.paramsGenerator_ = paramsLambda;
                }
                else {
                    this.updateStateVarsOfChildByElmtId(elmtId, {
                        title: SettingsText.USAGE_SECTION_PREP,
                        subtitle: SettingsText.USAGE_SECTION_PREP_DESC,
                        isDark: this.isDark
                    });
                }
            }, { name: "SettingsSectionTitle" });
        }
        {
            this.observeComponentCreation2((elmtId, isInitialRender) => {
                bjccovmshb1ia4.instrumentFunction(32);
                if (isInitialRender) {
                    let componentCall = new SettingsInfoCard(this, {
                        title: SettingsText.USAGE_PREP_TITLE,
                        body: SettingsText.USAGE_PREP_BODY,
                        isDark: this.isDark
                    }, undefined, elmtId, () => { }, { page: "common/src/main/ets/components/settings/ProjectHelpPage.ets", line: 109, col: 11 });
                    ViewPU.create(componentCall);
                    let paramsLambda = () => {
                        return {
                            title: SettingsText.USAGE_PREP_TITLE,
                            body: SettingsText.USAGE_PREP_BODY,
                            isDark: this.isDark
                        };
                    };
                    componentCall.paramsGenerator_ = paramsLambda;
                }
                else {
                    this.updateStateVarsOfChildByElmtId(elmtId, {
                        title: SettingsText.USAGE_PREP_TITLE,
                        body: SettingsText.USAGE_PREP_BODY,
                        isDark: this.isDark
                    });
                }
            }, { name: "SettingsInfoCard" });
        }
        {
            this.observeComponentCreation2((elmtId, isInitialRender) => {
                bjccovmshb1ia4.instrumentFunction(33);
                if (isInitialRender) {
                    let componentCall = new SettingsInfoCard(this, {
                        title: SettingsText.USAGE_WINDOWS_RDP_TITLE,
                        body: SettingsText.USAGE_WINDOWS_RDP_BODY,
                        isDark: this.isDark
                    }, undefined, elmtId, () => { }, { page: "common/src/main/ets/components/settings/ProjectHelpPage.ets", line: 114, col: 11 });
                    ViewPU.create(componentCall);
                    let paramsLambda = () => {
                        return {
                            title: SettingsText.USAGE_WINDOWS_RDP_TITLE,
                            body: SettingsText.USAGE_WINDOWS_RDP_BODY,
                            isDark: this.isDark
                        };
                    };
                    componentCall.paramsGenerator_ = paramsLambda;
                }
                else {
                    this.updateStateVarsOfChildByElmtId(elmtId, {
                        title: SettingsText.USAGE_WINDOWS_RDP_TITLE,
                        body: SettingsText.USAGE_WINDOWS_RDP_BODY,
                        isDark: this.isDark
                    });
                }
            }, { name: "SettingsInfoCard" });
        }
        {
            this.observeComponentCreation2((elmtId, isInitialRender) => {
                bjccovmshb1ia4.instrumentFunction(34);
                if (isInitialRender) {
                    let componentCall = new SettingsInfoCard(this, {
                        title: SettingsText.USAGE_USERNAME_TITLE,
                        body: SettingsText.USAGE_USERNAME_BODY,
                        isDark: this.isDark
                    }, undefined, elmtId, () => { }, { page: "common/src/main/ets/components/settings/ProjectHelpPage.ets", line: 119, col: 11 });
                    ViewPU.create(componentCall);
                    let paramsLambda = () => {
                        return {
                            title: SettingsText.USAGE_USERNAME_TITLE,
                            body: SettingsText.USAGE_USERNAME_BODY,
                            isDark: this.isDark
                        };
                    };
                    componentCall.paramsGenerator_ = paramsLambda;
                }
                else {
                    this.updateStateVarsOfChildByElmtId(elmtId, {
                        title: SettingsText.USAGE_USERNAME_TITLE,
                        body: SettingsText.USAGE_USERNAME_BODY,
                        isDark: this.isDark
                    });
                }
            }, { name: "SettingsInfoCard" });
        }
        {
            this.observeComponentCreation2((elmtId, isInitialRender) => {
                bjccovmshb1ia4.instrumentFunction(35);
                if (isInitialRender) {
                    let componentCall = new SettingsInfoCard(this, {
                        title: SettingsText.USAGE_HARDWARE_ACCEL_TITLE,
                        body: SettingsText.USAGE_HARDWARE_ACCEL_BODY,
                        isDark: this.isDark
                    }, undefined, elmtId, () => { }, { page: "common/src/main/ets/components/settings/ProjectHelpPage.ets", line: 124, col: 11 });
                    ViewPU.create(componentCall);
                    let paramsLambda = () => {
                        return {
                            title: SettingsText.USAGE_HARDWARE_ACCEL_TITLE,
                            body: SettingsText.USAGE_HARDWARE_ACCEL_BODY,
                            isDark: this.isDark
                        };
                    };
                    componentCall.paramsGenerator_ = paramsLambda;
                }
                else {
                    this.updateStateVarsOfChildByElmtId(elmtId, {
                        title: SettingsText.USAGE_HARDWARE_ACCEL_TITLE,
                        body: SettingsText.USAGE_HARDWARE_ACCEL_BODY,
                        isDark: this.isDark
                    });
                }
            }, { name: "SettingsInfoCard" });
        }
        {
            this.observeComponentCreation2((elmtId, isInitialRender) => {
                bjccovmshb1ia4.instrumentFunction(36);
                if (isInitialRender) {
                    let componentCall = new SettingsInfoCard(this, {
                        title: SettingsText.USAGE_FORM_TITLE,
                        body: SettingsText.USAGE_FORM_BODY,
                        isDark: this.isDark
                    }, undefined, elmtId, () => { }, { page: "common/src/main/ets/components/settings/ProjectHelpPage.ets", line: 129, col: 11 });
                    ViewPU.create(componentCall);
                    let paramsLambda = () => {
                        return {
                            title: SettingsText.USAGE_FORM_TITLE,
                            body: SettingsText.USAGE_FORM_BODY,
                            isDark: this.isDark
                        };
                    };
                    componentCall.paramsGenerator_ = paramsLambda;
                }
                else {
                    this.updateStateVarsOfChildByElmtId(elmtId, {
                        title: SettingsText.USAGE_FORM_TITLE,
                        body: SettingsText.USAGE_FORM_BODY,
                        isDark: this.isDark
                    });
                }
            }, { name: "SettingsInfoCard" });
        }
        {
            this.observeComponentCreation2((elmtId, isInitialRender) => {
                bjccovmshb1ia4.instrumentFunction(37);
                if (isInitialRender) {
                    let componentCall = new SettingsInfoCard(this, {
                        title: SettingsText.REMOTE_FILES_FEATURE_TITLE,
                        body: SettingsText.REMOTE_FILES_FEATURE_DESC,
                        isDark: this.isDark
                    }, undefined, elmtId, () => { }, { page: "common/src/main/ets/components/settings/ProjectHelpPage.ets", line: 134, col: 11 });
                    ViewPU.create(componentCall);
                    let paramsLambda = () => {
                        return {
                            title: SettingsText.REMOTE_FILES_FEATURE_TITLE,
                            body: SettingsText.REMOTE_FILES_FEATURE_DESC,
                            isDark: this.isDark
                        };
                    };
                    componentCall.paramsGenerator_ = paramsLambda;
                }
                else {
                    this.updateStateVarsOfChildByElmtId(elmtId, {
                        title: SettingsText.REMOTE_FILES_FEATURE_TITLE,
                        body: SettingsText.REMOTE_FILES_FEATURE_DESC,
                        isDark: this.isDark
                    });
                }
            }, { name: "SettingsInfoCard" });
        }
        {
            this.observeComponentCreation2((elmtId, isInitialRender) => {
                bjccovmshb1ia4.instrumentFunction(38);
                if (isInitialRender) {
                    let componentCall = new SettingsSectionTitle(this, {
                        title: SettingsText.USAGE_SECTION_SECURITY,
                        subtitle: SettingsText.USAGE_SECTION_SECURITY_DESC,
                        isDark: this.isDark
                    }, undefined, elmtId, () => { }, { page: "common/src/main/ets/components/settings/ProjectHelpPage.ets", line: 140, col: 11 });
                    ViewPU.create(componentCall);
                    let paramsLambda = () => {
                        return {
                            title: SettingsText.USAGE_SECTION_SECURITY,
                            subtitle: SettingsText.USAGE_SECTION_SECURITY_DESC,
                            isDark: this.isDark
                        };
                    };
                    componentCall.paramsGenerator_ = paramsLambda;
                }
                else {
                    this.updateStateVarsOfChildByElmtId(elmtId, {
                        title: SettingsText.USAGE_SECTION_SECURITY,
                        subtitle: SettingsText.USAGE_SECTION_SECURITY_DESC,
                        isDark: this.isDark
                    });
                }
            }, { name: "SettingsSectionTitle" });
        }
        {
            this.observeComponentCreation2((elmtId, isInitialRender) => {
                bjccovmshb1ia4.instrumentFunction(39);
                if (isInitialRender) {
                    let componentCall = new SettingsInfoCard(this, {
                        title: SettingsText.USAGE_CERT_TITLE,
                        body: SettingsText.USAGE_CERT_BODY,
                        isDark: this.isDark
                    }, undefined, elmtId, () => { }, { page: "common/src/main/ets/components/settings/ProjectHelpPage.ets", line: 146, col: 11 });
                    ViewPU.create(componentCall);
                    let paramsLambda = () => {
                        return {
                            title: SettingsText.USAGE_CERT_TITLE,
                            body: SettingsText.USAGE_CERT_BODY,
                            isDark: this.isDark
                        };
                    };
                    componentCall.paramsGenerator_ = paramsLambda;
                }
                else {
                    this.updateStateVarsOfChildByElmtId(elmtId, {
                        title: SettingsText.USAGE_CERT_TITLE,
                        body: SettingsText.USAGE_CERT_BODY,
                        isDark: this.isDark
                    });
                }
            }, { name: "SettingsInfoCard" });
        }
        {
            this.observeComponentCreation2((elmtId, isInitialRender) => {
                bjccovmshb1ia4.instrumentFunction(40);
                if (isInitialRender) {
                    let componentCall = new SettingsInfoCard(this, {
                        title: SettingsText.USAGE_TROUBLESHOOT_TITLE,
                        body: SettingsText.USAGE_TROUBLESHOOT_BODY,
                        isDark: this.isDark
                    }, undefined, elmtId, () => { }, { page: "common/src/main/ets/components/settings/ProjectHelpPage.ets", line: 151, col: 11 });
                    ViewPU.create(componentCall);
                    let paramsLambda = () => {
                        return {
                            title: SettingsText.USAGE_TROUBLESHOOT_TITLE,
                            body: SettingsText.USAGE_TROUBLESHOOT_BODY,
                            isDark: this.isDark
                        };
                    };
                    componentCall.paramsGenerator_ = paramsLambda;
                }
                else {
                    this.updateStateVarsOfChildByElmtId(elmtId, {
                        title: SettingsText.USAGE_TROUBLESHOOT_TITLE,
                        body: SettingsText.USAGE_TROUBLESHOOT_BODY,
                        isDark: this.isDark
                    });
                }
            }, { name: "SettingsInfoCard" });
        }
        {
            this.observeComponentCreation2((elmtId, isInitialRender) => {
                bjccovmshb1ia4.instrumentFunction(41);
                if (isInitialRender) {
                    let componentCall = new SettingsSectionTitle(this, {
                        title: SettingsText.PROJECT_HELP_ABOUT_SECTION,
                        subtitle: SettingsText.ABOUT_SECTION_PROJECT_DESC,
                        isDark: this.isDark
                    }, undefined, elmtId, () => { }, { page: "common/src/main/ets/components/settings/ProjectHelpPage.ets", line: 157, col: 11 });
                    ViewPU.create(componentCall);
                    let paramsLambda = () => {
                        return {
                            title: SettingsText.PROJECT_HELP_ABOUT_SECTION,
                            subtitle: SettingsText.ABOUT_SECTION_PROJECT_DESC,
                            isDark: this.isDark
                        };
                    };
                    componentCall.paramsGenerator_ = paramsLambda;
                }
                else {
                    this.updateStateVarsOfChildByElmtId(elmtId, {
                        title: SettingsText.PROJECT_HELP_ABOUT_SECTION,
                        subtitle: SettingsText.ABOUT_SECTION_PROJECT_DESC,
                        isDark: this.isDark
                    });
                }
            }, { name: "SettingsSectionTitle" });
        }
        this.aboutInfoCard.bind(this)();
        Column.pop();
        Scroll.pop();
        Column.pop();
    }
    rerender() {
        this.updateDirtyElements();
    }
}
