import type window from "@ohos:window";
import { NativeRdpGateway } from "@normalized:N&&&common/src/main/ets/rdp/NativeRdpGateway&";
import type { NativeCommandResult } from "@normalized:N&&&common/src/main/ets/rdp/NativeRdpGateway&";
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
let bjccovmshb1i00 = new BjcCov({ version: "bjc v1.0.0", versionCode: 10000, path: "common/src/main/ets/rdp/ImeHostWindowBinder.ets", hash: "1a6220458b7ecb5a6e94d56a82ec1059db2e21065a3b05a699ddf3d8954e4829", lineCnt: 17, count: 0, projectPath: "", functions: { 0: { name: "ImeHostWindowBinder.bind", count: 0, regions: { 0: { startLoc: { line: 6, col: 3 }, endLoc: { line: 15, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 7, col: 9 }, endLoc: { line: 12, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 12, col: 7 }, endLoc: { line: 14, col: 6 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 9, col: 23 }, endLoc: { line: 11, col: 8 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 9, col: 11 }, endLoc: { line: 9, col: 21 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 0 } }, exeLine: { 0: 1, 1: 2, 2: 3, 3: 5, 4: 6, 5: 7, 6: 8, 7: 9, 8: 10, 9: 12, 10: 13 } });
export class ImeHostWindowBinder {
    bind(mainWindow: window.Window): void {
        bjccovmshb1i00.instrumentFunction(0);
        try {
            bjccovmshb1i00.instrumentRegion(0, 1);
            const result: NativeCommandResult = NativeRdpGateway.bindImeHostWindow(mainWindow.getWindowProperties().id);
            if (!result.ok) {
                bjccovmshb1i00.instrumentBranch(0, 0, true);
                bjccovmshb1i00.instrumentRegion(0, 3);
                RdpLogger.warn(`IME host window rejected: ${result.message}`);
            }
            else {
                bjccovmshb1i00.instrumentBranch(0, 0, false);
            }
        }
        catch (error) {
            bjccovmshb1i00.instrumentRegion(0, 2);
            RdpLogger.warn(`IME host window binding failed: ${JSON.stringify(error)}`);
        }
    }
}
