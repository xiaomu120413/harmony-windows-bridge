import hilog from "@ohos:hilog";
import { HILOG_DOMAIN, HILOG_TAG } from "@normalized:N&&&common/src/main/ets/rdp/RdpConstants&";
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
let bjccovmshb1i0w = new BjcCov({ version: "bjc v1.0.0", versionCode: 10000, path: "common/src/main/ets/rdp/RdpLogger.ets", hash: "7984276718b092d49d3308082c811a627c22bbed39222a81dfb2d9f8a0a93b72", lineCnt: 24, count: 0, projectPath: "", functions: { 0: { name: "RdpLogger.debug", count: 0, regions: { 0: { startLoc: { line: 8, col: 3 }, endLoc: { line: 10, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 9, col: 5 }, endLoc: { line: 10, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 0 }, 1: { name: "RdpLogger.info", count: 0, regions: { 0: { startLoc: { line: 12, col: 3 }, endLoc: { line: 14, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 13, col: 5 }, endLoc: { line: 14, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 1 }, 2: { name: "RdpLogger.warn", count: 0, regions: { 0: { startLoc: { line: 16, col: 3 }, endLoc: { line: 18, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 17, col: 5 }, endLoc: { line: 18, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 2 }, 3: { name: "RdpLogger.error", count: 0, regions: { 0: { startLoc: { line: 20, col: 3 }, endLoc: { line: 22, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 21, col: 5 }, endLoc: { line: 22, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 3 } }, exeLine: { 0: 1, 1: 2, 2: 3, 3: 4, 4: 5, 5: 7, 6: 8, 7: 9, 8: 12, 9: 13, 10: 16, 11: 17, 12: 20, 13: 21 } });
export class RdpLogger {
    static debug(line: string): void {
        bjccovmshb1i0w.instrumentFunction(0);
        bjccovmshb1i0w.instrumentRegion(0, 1);
        hilog.debug(HILOG_DOMAIN, HILOG_TAG, '%{private}s', line);
    }
    static info(line: string): void {
        bjccovmshb1i0w.instrumentFunction(1);
        bjccovmshb1i0w.instrumentRegion(1, 1);
        hilog.info(HILOG_DOMAIN, HILOG_TAG, '%{private}s', line);
    }
    static warn(line: string): void {
        bjccovmshb1i0w.instrumentFunction(2);
        bjccovmshb1i0w.instrumentRegion(2, 1);
        hilog.warn(HILOG_DOMAIN, HILOG_TAG, '%{private}s', line);
    }
    static error(line: string): void {
        bjccovmshb1i0w.instrumentFunction(3);
        bjccovmshb1i0w.instrumentRegion(3, 1);
        hilog.error(HILOG_DOMAIN, HILOG_TAG, '%{private}s', line);
    }
}
