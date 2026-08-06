import type common from "@ohos:app.ability.common";
import { RdpLogger } from "@normalized:N&&&common/src/main/ets/rdp/RdpLogger&";
import { WindowsConnectionStore } from "@normalized:N&&&common/src/main/ets/rdp/WindowsConnectionStore&";
import type { WindowsConnectionProfile, WindowsConnectionSaveInput, WindowsConnectionSnapshot } from "@normalized:N&&&common/src/main/ets/rdp/WindowsConnectionStore&";
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
let bjccovmshb1i43 = new BjcCov({ version: "bjc v1.0.0", versionCode: 10000, path: "common/src/main/ets/rdp/WindowsConnectionProfileCoordinator.ets", hash: "b08512ea36f9f0c3af8e39e5a29dd9e16dac432ea76db869f7a467a99b90ac94", lineCnt: 63, count: 0, projectPath: "", functions: { 0: { name: "WindowsConnectionProfileCoordinator.constructor", count: 0, regions: { 0: { startLoc: { line: 13, col: 3 }, endLoc: { line: 15, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 14, col: 5 }, endLoc: { line: 15, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 0 }, 1: { name: "WindowsConnectionProfileCoordinator.load", count: 0, regions: { 0: { startLoc: { line: 17, col: 3 }, endLoc: { line: 24, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 18, col: 9 }, endLoc: { line: 20, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 20, col: 7 }, endLoc: { line: 23, col: 6 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 1 }, 2: { name: "WindowsConnectionProfileCoordinator.loadPassword", count: 0, regions: { 0: { startLoc: { line: 26, col: 3 }, endLoc: { line: 33, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 27, col: 9 }, endLoc: { line: 29, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 29, col: 7 }, endLoc: { line: 32, col: 6 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 2 }, 3: { name: "WindowsConnectionProfileCoordinator.deleteProfile", count: 0, regions: { 0: { startLoc: { line: 35, col: 3 }, endLoc: { line: 42, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 36, col: 9 }, endLoc: { line: 38, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 38, col: 7 }, endLoc: { line: 41, col: 6 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 3 }, 4: { name: "WindowsConnectionProfileCoordinator.saveProfile", count: 0, regions: { 0: { startLoc: { line: 44, col: 3 }, endLoc: { line: 52, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 45, col: 9 }, endLoc: { line: 48, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 48, col: 7 }, endLoc: { line: 51, col: 6 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 47, col: 14 }, endLoc: { line: 47, col: 71 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 4 }, 5: { name: "WindowsConnectionProfileCoordinator.clearPassword", count: 0, regions: { 0: { startLoc: { line: 54, col: 3 }, endLoc: { line: 61, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 55, col: 9 }, endLoc: { line: 57, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 57, col: 7 }, endLoc: { line: 60, col: 6 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 5 } }, exeLine: { 0: 1, 1: 2, 2: 3, 3: 4, 4: 5, 5: 6, 6: 7, 7: 8, 8: 10, 9: 11, 10: 13, 11: 14, 12: 17, 13: 18, 14: 19, 15: 20, 16: 21, 17: 22, 18: 26, 19: 27, 20: 28, 21: 29, 22: 30, 23: 31, 24: 35, 25: 36, 26: 37, 27: 38, 28: 39, 29: 40, 30: 44, 31: 45, 32: 46, 33: 47, 34: 48, 35: 49, 36: 50, 37: 54, 38: 55, 39: 56, 40: 57, 41: 58, 42: 59 } });
export class WindowsConnectionProfileCoordinator {
    private readonly store: WindowsConnectionStore;
    constructor(context: common.Context) {
        bjccovmshb1i43.instrumentFunction(0);
        bjccovmshb1i43.instrumentRegion(0, 1);
        this.store = new WindowsConnectionStore(context);
    }
    async load(): Promise<WindowsConnectionSnapshot | null> {
        bjccovmshb1i43.instrumentFunction(1);
        try {
            bjccovmshb1i43.instrumentRegion(1, 1);
            return await this.store.loadSnapshot();
        }
        catch (error) {
            bjccovmshb1i43.instrumentRegion(1, 2);
            RdpLogger.warn(`windows connection profiles load failed: ${JSON.stringify(error)}`);
            return null;
        }
    }
    async loadPassword(profile: WindowsConnectionProfile): Promise<string | null> {
        bjccovmshb1i43.instrumentFunction(2);
        try {
            bjccovmshb1i43.instrumentRegion(2, 1);
            return await this.store.loadPassword(profile);
        }
        catch (error) {
            bjccovmshb1i43.instrumentRegion(2, 2);
            RdpLogger.warn(`windows connection profile password select failed: ${JSON.stringify(error)}`);
            return null;
        }
    }
    async deleteProfile(profileId: string): Promise<WindowsConnectionSnapshot | null> {
        bjccovmshb1i43.instrumentFunction(3);
        try {
            bjccovmshb1i43.instrumentRegion(3, 1);
            return await this.store.deleteProfile(profileId);
        }
        catch (error) {
            bjccovmshb1i43.instrumentRegion(3, 2);
            RdpLogger.warn(`windows connection profile delete failed: ${JSON.stringify(error)}`);
            return null;
        }
    }
    async saveProfile(input: WindowsConnectionSaveInput): Promise<WindowsConnectionSnapshot | null> {
        bjccovmshb1i43.instrumentFunction(4);
        try {
            bjccovmshb1i43.instrumentRegion(4, 1);
            const profile = await this.store.saveProfile(input);
            return profile === null ? (bjccovmshb1i43.instrumentBranch(4, 0, true), null) : (bjccovmshb1i43.instrumentBranch(4, 0, false), await this.store.loadSnapshot());
        }
        catch (error) {
            bjccovmshb1i43.instrumentRegion(4, 2);
            RdpLogger.warn(`windows connection profile save failed: ${JSON.stringify(error)}`);
            return null;
        }
    }
    async clearPassword(profileId: string): Promise<WindowsConnectionSnapshot | null> {
        bjccovmshb1i43.instrumentFunction(5);
        try {
            bjccovmshb1i43.instrumentRegion(5, 1);
            return await this.store.clearPassword(profileId);
        }
        catch (error) {
            bjccovmshb1i43.instrumentRegion(5, 2);
            RdpLogger.warn(`windows connection password clear failed: ${JSON.stringify(error)}`);
            return null;
        }
    }
}
