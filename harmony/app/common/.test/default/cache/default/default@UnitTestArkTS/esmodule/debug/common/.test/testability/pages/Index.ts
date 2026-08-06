if (!("finalizeConstruction" in ViewPU.prototype)) {
    Reflect.set(ViewPU.prototype, "finalizeConstruction", () => { });
}
interface Index1_Params {
    message?: string;
}
import AbilityDelegatorRegistry from "@ohos:app.ability.abilityDelegatorRegistry";
import { Hypium } from "@normalized:N&&&@ohos/hypium/index&1.0.24";
import testsuite from "@normalized:N&&&common/src/test/List.test&";
const global: object = new Function("return this")();
const savePath = '__savePath__';
const readPath = '__readPath__';
const testMode: string = '__testMode__';
global[savePath] = "C:/Users/mu/Desktop/code/demo/harmony/app/common/.test/default/intermediates/test/coverage_data/js_coverage.json";
global[readPath] = "C:/Users/mu/Desktop/code/demo/harmony/app/common/.test/default/intermediates/test/coverage_data/js_coverage.json";
global[testMode] = 'unitTest';
class Index1 extends ViewPU {
    constructor(parent, params, __localStorage, elmtId = -1, paramsLambda = undefined, extraInfo) {
        super(parent, __localStorage, elmtId, extraInfo);
        if (typeof paramsLambda === "function") {
            this.paramsGenerator_ = paramsLambda;
        }
        this.__message = new ObservedPropertySimplePU('Hello World', this, "message");
        this.setInitiallyProvidedValue(params);
        this.finalizeConstruction();
    }
    setInitiallyProvidedValue(params: Index1_Params) {
        if (params.message !== undefined) {
            this.message = params.message;
        }
    }
    updateStateVars(params: Index1_Params) {
    }
    purgeVariableDependenciesOnElmtId(rmElmtId) {
        this.__message.purgeDependencyOnElmtId(rmElmtId);
    }
    aboutToBeDeleted() {
        this.__message.aboutToBeDeleted();
        SubscriberManager.Get().delete(this.id__());
        this.aboutToBeDeletedInternal();
    }
    aboutToAppear() {
        console.info('[LOCAL_TEST] START');
        let abilityDelegator: AbilityDelegatorRegistry.AbilityDelegator;
        abilityDelegator = AbilityDelegatorRegistry.getAbilityDelegator();
        let abilityDelegatorArguments: AbilityDelegatorRegistry.AbilityDelegatorArgs;
        abilityDelegatorArguments = AbilityDelegatorRegistry.getArguments();
        abilityDelegatorArguments.bundleName = "com.muhub.desktop";
        abilityDelegatorArguments.parameters = {
            "-b": "com.muhub.desktop",
            "-m": "common",
            "-s class": "",
            "-s timeout": "15000",
            "-s coverage": "true"
        };
        abilityDelegatorArguments.testCaseNames = "";
        console.info("[LOCAL_TEST] " + abilityDelegatorArguments);
        Hypium.hypiumTest(abilityDelegator, abilityDelegatorArguments, testsuite);
        console.info('[LOCAL_TEST] END');
    }
    private __message: ObservedPropertySimplePU<string>;
    get message() {
        return this.__message.get();
    }
    set message(newValue: string) {
        this.__message.set(newValue);
    }
    initialRender() {
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            Row.create();
            Row.height('100%');
        }, Row);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            Column.create();
            Column.width('100%');
        }, Column);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            Text.create(this.message);
            Text.fontSize(50);
            Text.fontWeight(FontWeight.Bold);
        }, Text);
        Text.pop();
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            Button.createWithChild();
            Button.type(ButtonType.Capsule);
            Button.margin({
                top: 20
            });
            Button.backgroundColor('#0D9FFB');
            Button.width('35%');
            Button.height('5%');
            Button.onClick(() => {
            });
        }, Button);
        this.observeComponentCreation2((elmtId, isInitialRender) => {
            Text.create('next page');
            Text.fontSize(20);
            Text.fontWeight(FontWeight.Bold);
        }, Text);
        Text.pop();
        Button.pop();
        Column.pop();
        Row.pop();
    }
    rerender() {
        this.updateDirtyElements();
    }
    static getEntryName(): string {
        return "Index1";
    }
}
registerNamedRoute(() => new Index1(undefined, {}), "", { bundleName: "com.muhub.desktop", moduleName: "common", pagePath: "../../../.test/testability/pages/Index", pageFullPath: "common/.test/testability/pages/Index", integratedHsp: "false", moduleType: "sharedHsp" });
