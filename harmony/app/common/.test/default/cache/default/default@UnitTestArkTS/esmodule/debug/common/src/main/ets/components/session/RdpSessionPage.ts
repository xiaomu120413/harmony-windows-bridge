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
let bjccovmshb1i8i = new BjcCov({ version: "bjc v1.0.0", versionCode: 10000, path: "common/src/main/ets/components/session/RdpSessionPage.ets", hash: "0f2cbaedaaf03e254a55fdb0553079b72a6330cb1843ad8259f4e320c362df94", lineCnt: 79, count: 0, projectPath: "", functions: { 0: { name: "anonymous_1", count: 0, regions: { 0: { startLoc: { line: 9, col: 18 }, endLoc: { line: 9, col: 28 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 0 }, 1: { name: "anonymous_2", count: 0, regions: { 0: { startLoc: { line: 11, col: 20 }, endLoc: { line: 11, col: 53 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 1 }, 2: { name: "anonymous_3", count: 0, regions: { 0: { startLoc: { line: 12, col: 20 }, endLoc: { line: 12, col: 30 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 2 }, 3: { name: "anonymous_4", count: 0, regions: { 0: { startLoc: { line: 9, col: 31 }, endLoc: { line: 10, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 3 }, 4: { name: "anonymous_5", count: 0, regions: { 0: { startLoc: { line: 11, col: 56 }, endLoc: { line: 11, col: 97 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 4 }, 5: { name: "anonymous_6", count: 0, regions: { 0: { startLoc: { line: 12, col: 33 }, endLoc: { line: 13, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 5 }, 6: { name: "setInitiallyProvidedValue", count: 0, regions: { 0: { startLoc: { line: 6, col: 31 }, endLoc: { line: 8, col: 44 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 6 }, 7: { name: "updateStateVars", count: 0, regions: { 0: { startLoc: { line: 6, col: 9 }, endLoc: { line: 8, col: 27 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 7 }, 8: { name: "noticeTitle", count: 0, regions: { 0: { startLoc: { line: 6, col: 9 }, endLoc: { line: 6, col: 20 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 8 }, 9: { name: "noticeTitle", count: 0, regions: { 0: { startLoc: { line: 6, col: 9 }, endLoc: { line: 6, col: 28 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 9 }, 10: { name: "noticeSubtitle", count: 0, regions: { 0: { startLoc: { line: 7, col: 9 }, endLoc: { line: 7, col: 23 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 10 }, 11: { name: "noticeSubtitle", count: 0, regions: { 0: { startLoc: { line: 7, col: 9 }, endLoc: { line: 7, col: 31 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 11 }, 12: { name: "remoteLoginWaiting", count: 0, regions: { 0: { startLoc: { line: 8, col: 9 }, endLoc: { line: 8, col: 27 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 12 }, 13: { name: "remoteLoginWaiting", count: 0, regions: { 0: { startLoc: { line: 8, col: 9 }, endLoc: { line: 8, col: 36 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 13 }, 14: { name: "anonymous_7", count: 0, regions: { 0: { startLoc: { line: 9, col: 18 }, endLoc: { line: 9, col: 28 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 14 }, 15: { name: "anonymous_8", count: 0, regions: { 0: { startLoc: { line: 11, col: 20 }, endLoc: { line: 11, col: 53 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 15 }, 16: { name: "anonymous_9", count: 0, regions: { 0: { startLoc: { line: 12, col: 20 }, endLoc: { line: 12, col: 30 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 16 }, 17: { name: "aboutToAppear", count: 0, regions: { 0: { startLoc: { line: 16, col: 3 }, endLoc: { line: 20, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 17, col: 5 }, endLoc: { line: 20, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 17 }, 18: { name: "aboutToDisappear", count: 0, regions: { 0: { startLoc: { line: 22, col: 3 }, endLoc: { line: 25, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 23, col: 5 }, endLoc: { line: 25, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 18 }, 19: { name: "buildSessionNotice", count: 0, regions: { 0: { startLoc: { line: 27, col: 3 }, endLoc: { line: 55, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 19 }, 20: { name: "anonymous_10", count: 0, regions: { 0: { startLoc: { line: 29, col: 5 }, endLoc: { line: 54, col: 20 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 49, col: 22 }, endLoc: { line: 49, col: 93 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 52, col: 14 }, endLoc: { line: 52, col: 90 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 20 }, 21: { name: "anonymous_11", count: 0, regions: { 0: { startLoc: { line: 30, col: 7 }, endLoc: { line: 44, col: 40 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 21 }, 22: { name: "anonymous_12", count: 0, regions: { 0: { startLoc: { line: 31, col: 9 }, endLoc: { line: 36, col: 60 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 22 }, 23: { name: "anonymous_13", count: 0, regions: { 0: { startLoc: { line: 37, col: 9 }, endLoc: { line: 42, col: 60 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 23 }, 24: { name: "initialRender", count: 0, regions: { 0: { startLoc: { line: 57, col: 3 }, endLoc: { line: 77, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 24 }, 25: { name: "anonymous_14", count: 0, regions: { 0: { startLoc: { line: 58, col: 5 }, endLoc: { line: 76, col: 33 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 25 }, 26: { name: "anonymous_15", count: 0, regions: { 0: { startLoc: { line: 59, col: 7 }, endLoc: { line: 69, col: 37 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 26 }, 27: { name: "anonymous_16", count: 0, regions: { 0: { startLoc: { line: 64, col: 19 }, endLoc: { line: 68, col: 10 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 65, col: 58 }, endLoc: { line: 67, col: 12 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 65, col: 15 }, endLoc: { line: 65, col: 56 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 27 }, 28: { name: "anonymous_17", count: 0, regions: { 0: { startLoc: { line: 60, col: 9 }, endLoc: { line: 60, col: 40 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 28 }, 29: { name: "anonymous_18", count: 0, regions: { 0: { startLoc: { line: 70, col: 7 }, endLoc: { line: 72, col: 8 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 70, col: 40 }, endLoc: { line: 72, col: 8 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 70, col: 11 }, endLoc: { line: 70, col: 38 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 29 }, 30: { name: "anonymous_19", count: 0, regions: { 0: { startLoc: { line: 71, col: 9 }, endLoc: { line: 71, col: 34 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 30 } }, exeLine: { 0: 1, 1: 4, 2: 5, 3: 6, 4: 7, 5: 8, 6: 9, 7: 11, 8: 12, 9: 14, 10: 16, 11: 17, 12: 18, 13: 19, 14: 22, 15: 23, 16: 24, 17: 28, 18: 29, 19: 30, 20: 31, 21: 32, 22: 33, 23: 34, 24: 35, 25: 36, 26: 37, 27: 38, 28: 39, 29: 40, 30: 41, 31: 42, 32: 44, 33: 46, 34: 47, 35: 48, 36: 49, 37: 50, 38: 51, 39: 52, 40: 54, 41: 57, 42: 58, 43: 59, 44: 60, 45: 62, 46: 63, 47: 64, 48: 65, 49: 66, 50: 69, 51: 70, 52: 71, 53: 74, 54: 75, 55: 76 } });
if (!("finalizeConstruction" in ViewPU.prototype)) {
    Reflect.set(ViewPU.prototype, "finalizeConstruction", () => { });
}
interface RdpSessionPage_Params {
    surfaceContent?: NodeContent;
    noticeTitle?: string;
    noticeSubtitle?: string;
    remoteLoginWaiting?: boolean;
    onSurfaceLoad?: () => void;
    onAttachContent?: (content: NodeContent) => boolean;
    onDetachContent?: () => void;
    previousKeyboardAvoidMode?: KeyboardAvoidMode;
}
import type { KeyboardAvoidMode } from "@ohos:arkui.UIContext";
import { NodeContent } from "@ohos:arkui.node";
export class RdpSessionPage extends ViewPU {
    constructor(parent, params, __localStorage, elmtId = -1, paramsLambda = undefined, extraInfo) {
        super(parent, __localStorage, elmtId, extraInfo);
        if (typeof paramsLambda === "function") {
            this.paramsGenerator_ = paramsLambda;
        }
        this.surfaceContent = new NodeContent();
        this.__noticeTitle = new SynchedPropertySimpleOneWayPU(params.noticeTitle, this, "noticeTitle");
        this.__noticeSubtitle = new SynchedPropertySimpleOneWayPU(params.noticeSubtitle, this, "noticeSubtitle");
        this.__remoteLoginWaiting = new SynchedPropertySimpleOneWayPU(params.remoteLoginWaiting, this, "remoteLoginWaiting");
        this.onSurfaceLoad = () => {
            bjccovmshb1i8i.instrumentFunction(3);
        };
        this.onAttachContent = (_content: NodeContent): boolean => { bjccovmshb1i8i.instrumentFunction(4); return false; };
        this.onDetachContent = (): void => {
            bjccovmshb1i8i.instrumentFunction(5);
        };
        this.previousKeyboardAvoidMode = 0;
        this.setInitiallyProvidedValue(params);
        this.finalizeConstruction();
    }
    setInitiallyProvidedValue(params: RdpSessionPage_Params) {
        bjccovmshb1i8i.instrumentFunction(6);
        if (params.surfaceContent !== undefined) {
            this.surfaceContent = params.surfaceContent;
        }
        else {
        }
        if (params.noticeTitle === undefined) {
            this.__noticeTitle.set('');
        }
        else {
        }
        if (params.noticeSubtitle === undefined) {
            this.__noticeSubtitle.set('');
        }
        else {
        }
        if (params.remoteLoginWaiting === undefined) {
            this.__remoteLoginWaiting.set(false);
        }
        else {
        }
        if (params.onSurfaceLoad !== undefined) {
            this.onSurfaceLoad = params.onSurfaceLoad;
        }
        else {
        }
        if (params.onAttachContent !== undefined) {
            this.onAttachContent = params.onAttachContent;
        }
        else {
        }
        if (params.onDetachContent !== undefined) {
            this.onDetachContent = params.onDetachContent;
        }
        else {
        }
        if (params.previousKeyboardAvoidMode !== undefined) {
            this.previousKeyboardAvoidMode = params.previousKeyboardAvoidMode;
        }
        else {
        }
    }
    updateStateVars(params: RdpSessionPage_Params) {
        bjccovmshb1i8i.instrumentFunction(7);
        this.__noticeTitle.reset(params.noticeTitle);
        this.__noticeSubtitle.reset(params.noticeSubtitle);
        this.__remoteLoginWaiting.reset(params.remoteLoginWaiting);
    }
    purgeVariableDependenciesOnElmtId(rmElmtId) {
        this.__noticeTitle.purgeDependencyOnElmtId(rmElmtId);
        this.__noticeSubtitle.purgeDependencyOnElmtId(rmElmtId);
        this.__remoteLoginWaiting.purgeDependencyOnElmtId(rmElmtId);
    }
    aboutToBeDeleted() {
        this.__noticeTitle.aboutToBeDeleted();
        this.__noticeSubtitle.aboutToBeDeleted();
        this.__remoteLoginWaiting.aboutToBeDeleted();
        SubscriberManager.Get().delete(this.id__());
        this.aboutToBeDeletedInternal();
    }
    private surfaceContent: NodeContent;
    private __noticeTitle: SynchedPropertySimpleOneWayPU<string>;
    get noticeTitle() {
        bjccovmshb1i8i.instrumentFunction(8);
        return this.__noticeTitle.get();
    }
    set noticeTitle(newValue: string) {
        bjccovmshb1i8i.instrumentFunction(9);
        this.__noticeTitle.set(newValue);
    }
    private __noticeSubtitle: SynchedPropertySimpleOneWayPU<string>;
    get noticeSubtitle() {
        bjccovmshb1i8i.instrumentFunction(10);
        return this.__noticeSubtitle.get();
    }
    set noticeSubtitle(newValue: string) {
        bjccovmshb1i8i.instrumentFunction(11);
        this.__noticeSubtitle.set(newValue);
    }
    private __remoteLoginWaiting: SynchedPropertySimpleOneWayPU<boolean>;
    get remoteLoginWaiting() {
        bjccovmshb1i8i.instrumentFunction(12);
        return this.__remoteLoginWaiting.get();
    }
    set remoteLoginWaiting(newValue: boolean) {
        bjccovmshb1i8i.instrumentFunction(13);
        this.__remoteLoginWaiting.set(newValue);
    }
    private onSurfaceLoad: () => void;
    private onAttachContent: (content: NodeContent) => boolean;
    private onDetachContent: () => void;
    private previousKeyboardAvoidMode: KeyboardAvoidMode;
    aboutToAppear(): void {
        bjccovmshb1i8i.instrumentFunction(17);
        bjccovmshb1i8i.instrumentRegion(17, 1);
        const uiContext = this.getUIContext();
        this.previousKeyboardAvoidMode = uiContext.getKeyboardAvoidMode();
        uiContext.setKeyboardAvoidMode(4);
    }
    aboutToDisappear(): void {
        bjccovmshb1i8i.instrumentFunction(18);
        bjccovmshb1i8i.instrumentRegion(18, 1);
        this.onDetachContent();
        this.getUIContext().setKeyboardAvoidMode(this.previousKeyboardAvoidMode);
    }
    private buildSessionNotice(parent = null) {
        bjccovmshb1i8i.instrumentFunction(19);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i8i.instrumentFunction(20);
            Row.create();
            Row.padding({ left: 14, right: 14, top: 10, bottom: 10 });
            Row.margin({ left: 18, top: 18, right: 18 });
            Row.constraintSize({ maxWidth: 520 });
            Row.backgroundColor(this.remoteLoginWaiting ? (bjccovmshb1i8i.instrumentBranch(20, 0, true), 'rgba(84,64,22,0.88)') : (bjccovmshb1i8i.instrumentBranch(20, 0, false), 'rgba(18,22,30,0.82)'));
            Row.border({
                width: 1,
                color: this.remoteLoginWaiting ? (bjccovmshb1i8i.instrumentBranch(20, 1, true), 'rgba(255,190,92,0.42)') : (bjccovmshb1i8i.instrumentBranch(20, 1, false), 'rgba(255,255,255,0.18)')
            });
            Row.borderRadius(8);
        }, Row);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i8i.instrumentFunction(21);
            Column.create();
            Column.alignItems(HorizontalAlign.Start);
        }, Column);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i8i.instrumentFunction(22);
            Text.create(this.noticeTitle);
            Text.fontSize(14);
            Text.fontWeight(FontWeight.Medium);
            Text.fontColor(Color.White);
            Text.maxLines(1);
            Text.textOverflow({ overflow: TextOverflow.Ellipsis });
        }, Text);
        Text.pop();
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i8i.instrumentFunction(23);
            Text.create(this.noticeSubtitle);
            Text.fontSize(12);
            Text.fontColor('rgba(255,255,255,0.72)');
            Text.margin({ top: 4 });
            Text.maxLines(1);
            Text.textOverflow({ overflow: TextOverflow.Ellipsis });
        }, Text);
        Text.pop();
        Column.pop();
        Row.pop();
    }
    initialRender() {
        bjccovmshb1i8i.instrumentFunction(24);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i8i.instrumentFunction(25);
            Stack.create({ alignContent: Alignment.TopStart });
            Stack.width('100%');
            Stack.height('100%');
            Stack.backgroundColor(Color.Black);
        }, Stack);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i8i.instrumentFunction(26);
            Stack.create();
            Stack.width('100%');
            Stack.height('100%');
            Stack.onAppear(() => {
                bjccovmshb1i8i.instrumentFunction(27);
                if (this.onAttachContent(this.surfaceContent)) {
                    bjccovmshb1i8i.instrumentBranch(27, 0, true);
                    bjccovmshb1i8i.instrumentRegion(27, 1);
                    this.onSurfaceLoad();
                }
                else {
                    bjccovmshb1i8i.instrumentBranch(27, 0, false);
                }
            });
            Stack.backgroundColor(Color.Black);
        }, Stack);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i8i.instrumentFunction(28);
            ContentSlot.create(this.surfaceContent);
        }, ContentSlot);
        Stack.pop();
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            bjccovmshb1i8i.instrumentFunction(29);
            If.create();
            if (this.noticeTitle.length > 0) {
                bjccovmshb1i8i.instrumentBranch(29, 0, true);
                bjccovmshb1i8i.instrumentRegion(29, 1);
                this.ifElseBranchUpdateFunction(0, () => {
                    bjccovmshb1i8i.instrumentFunction(30);
                    this.buildSessionNotice.bind(this)();
                });
            }
            else {
                bjccovmshb1i8i.instrumentBranch(29, 0, false);
                this.ifElseBranchUpdateFunction(1, () => {
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
