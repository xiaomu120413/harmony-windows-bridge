import type { NodeContent } from "@ohos:arkui.node";
import { NativeRdpGateway } from "@normalized:N&&&common/src/main/ets/rdp/NativeRdpGateway&";
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
let bjccovmshb1i11 = new BjcCov({ version: "bjc v1.0.0", versionCode: 10000, path: "common/src/main/ets/rdp/RdpSurfaceContentHost.ets", hash: "5023e6e9dc54237a1eb4b2bb656ac69764bbef65e85fa1899c7597186b0aef86", lineCnt: 21, count: 0, projectPath: "", functions: { 0: { name: "RdpSurfaceContentHost.attach", count: 0, regions: { 0: { startLoc: { line: 6, col: 3 }, endLoc: { line: 12, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 8, col: 21 }, endLoc: { line: 10, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 11, col: 5 }, endLoc: { line: 12, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 8, col: 9 }, endLoc: { line: 8, col: 19 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 0 }, 1: { name: "RdpSurfaceContentHost.detach", count: 0, regions: { 0: { startLoc: { line: 14, col: 3 }, endLoc: { line: 19, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 16, col: 21 }, endLoc: { line: 18, col: 6 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 16, col: 9 }, endLoc: { line: 16, col: 19 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 1 } }, exeLine: { 0: 1, 1: 2, 2: 3, 3: 5, 4: 6, 5: 7, 6: 8, 7: 9, 8: 11, 9: 14, 10: 15, 11: 16, 12: 17 } });
export class RdpSurfaceContentHost {
    attach(content: NodeContent): boolean {
        bjccovmshb1i11.instrumentFunction(0);
        const result = NativeRdpGateway.attachXComponentContent(content);
        if (!result.ok) {
            bjccovmshb1i11.instrumentBranch(0, 0, true);
            bjccovmshb1i11.instrumentRegion(0, 1);
            RdpLogger.error(`XComponent content attach failed: ${result.message}`);
        }
        else {
            bjccovmshb1i11.instrumentBranch(0, 0, false);
        }
        bjccovmshb1i11.instrumentRegion(0, 2);
        return result.ok;
    }
    detach(): void {
        bjccovmshb1i11.instrumentFunction(1);
        const result = NativeRdpGateway.detachXComponentContent();
        if (!result.ok) {
            bjccovmshb1i11.instrumentBranch(1, 0, true);
            bjccovmshb1i11.instrumentRegion(1, 1);
            RdpLogger.warn(`XComponent content detach failed: ${result.message}`);
        }
        else {
            bjccovmshb1i11.instrumentBranch(1, 0, false);
        }
    }
}
