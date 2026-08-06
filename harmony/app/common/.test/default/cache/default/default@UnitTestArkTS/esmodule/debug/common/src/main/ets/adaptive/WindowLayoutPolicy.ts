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
let bjccovmshb1hx9 = new BjcCov({ version: "bjc v1.0.0", versionCode: 10000, path: "common/src/main/ets/adaptive/WindowLayoutPolicy.ets", hash: "fb6125b2cea08f0189e4ce17734dbaa74c3ce85e619dd24693d331aadcf9bef9", lineCnt: 18, count: 0, projectPath: "", functions: { 0: { name: "layoutModeForWidthBreakpoint", count: 0, regions: { 0: { startLoc: { line: 6, col: 1 }, endLoc: { line: 17, col: 2 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 8, col: 5 }, endLoc: { line: 8, col: 35 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 9, col: 5 }, endLoc: { line: 10, col: 33 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 11, col: 5 }, endLoc: { line: 11, col: 35 }, count: 0, ignored: 0 }, 4: { startLoc: { line: 12, col: 5 }, endLoc: { line: 12, col: 35 }, count: 0, ignored: 0 }, 5: { startLoc: { line: 13, col: 5 }, endLoc: { line: 13, col: 35 }, count: 0, ignored: 0 }, 6: { startLoc: { line: 14, col: 5 }, endLoc: { line: 15, col: 32 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 8, col: 10 }, endLoc: { line: 8, col: 34 }, trueCount: 0, falseCount: 0, group: { 0: 0, 1: 1, 2: 2, 3: 3, 4: 4, 5: 5 }, ignored: 0 }, 1: { startLoc: { line: 9, col: 10 }, endLoc: { line: 9, col: 34 }, trueCount: 0, falseCount: 0, group: { 0: 0, 1: 1, 2: 2, 3: 3, 4: 4, 5: 5 }, ignored: 0 }, 2: { startLoc: { line: 11, col: 10 }, endLoc: { line: 11, col: 34 }, trueCount: 0, falseCount: 0, group: { 0: 0, 1: 1, 2: 2, 3: 3, 4: 4, 5: 5 }, ignored: 0 }, 3: { startLoc: { line: 12, col: 10 }, endLoc: { line: 12, col: 34 }, trueCount: 0, falseCount: 0, group: { 0: 0, 1: 1, 2: 2, 3: 3, 4: 4, 5: 5 }, ignored: 0 }, 4: { startLoc: { line: 13, col: 10 }, endLoc: { line: 13, col: 34 }, trueCount: 0, falseCount: 0, group: { 0: 0, 1: 1, 2: 2, 3: 3, 4: 4, 5: 5 }, ignored: 0 }, 5: { startLoc: { line: 14, col: 5 }, endLoc: { line: 14, col: 5 }, trueCount: 0, falseCount: 0, group: { 0: 0, 1: 1, 2: 2, 3: 3, 4: 4, 5: 5 }, ignored: 0 } }, ignored: 0, index: 0 } }, exeLine: { 0: 1, 1: 2, 2: 3, 3: 6, 4: 7, 5: 8, 6: 9, 7: 10, 8: 11, 9: 12, 10: 13, 11: 14, 12: 15 } });
export enum LayoutMode {
    COMPACT = "compact",
    EXPANDED = "expanded"
}
export function layoutModeForWidthBreakpoint(widthBreakpoint: WidthBreakpoint): LayoutMode {
    bjccovmshb1hx9.instrumentFunction(0);
    switch (widthBreakpoint) {
        case WidthBreakpoint.WIDTH_LG:
            bjccovmshb1hx9.instrumentBranch(0, 0, true);
            bjccovmshb1hx9.instrumentRegion(0, 1);
        case WidthBreakpoint.WIDTH_XL:
            bjccovmshb1hx9.instrumentBranch(0, 1, true);
            bjccovmshb1hx9.instrumentRegion(0, 2);
            return LayoutMode.EXPANDED;
        case WidthBreakpoint.WIDTH_XS:
            bjccovmshb1hx9.instrumentBranch(0, 2, true);
            bjccovmshb1hx9.instrumentRegion(0, 3);
        case WidthBreakpoint.WIDTH_SM:
            bjccovmshb1hx9.instrumentBranch(0, 3, true);
            bjccovmshb1hx9.instrumentRegion(0, 4);
        case WidthBreakpoint.WIDTH_MD:
            bjccovmshb1hx9.instrumentBranch(0, 4, true);
            bjccovmshb1hx9.instrumentRegion(0, 5);
        default:
            bjccovmshb1hx9.instrumentBranch(0, 5, true);
            bjccovmshb1hx9.instrumentRegion(0, 6);
            return LayoutMode.COMPACT;
    }
}
